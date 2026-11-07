#pragma once
// Reads ONE ICY metadata block from a stream and hands the StreamTitle text to a
// callback.
//
// Derived from the metadata block handling in ESP8266Audio's
// AudioFileSourceICYStream::readInternal (Copyright (C) 2017 Earle F.
// Philhower, III, GPLv3). The sequence of stream reads, the read sizes and the
// pauses are the same as in that code. What differs: the working buffer is always
// NUL-terminated after the data it holds, every string search runs on that
// terminated text, and the data length is tracked explicitly, so no byte outside
// the buffer is read or written for any block, and the title passed on is always
// a NUL-terminated string inside the buffer.
//
// Header-only and independent of Arduino so the host tests can compile it.
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace icy_metadata {

constexpr size_t kBufferBytes = 256 + 16 + 1;  // same size as the original
constexpr int kHistoryBytes = 16;              // carried between reads
constexpr int kKeyBytes = 12;                  // length of "StreamTitle="

// First occurrence of key in text[0..size), or nullptr.
inline const char* findKey(const char* text, int size, const char* key, int keyBytes) {
    for (int at = 0; at + keyBytes <= size; ++at)
        if (std::memcmp(text + at, key, size_t(keyBytes)) == 0) return text + at;
    return nullptr;
}

// Consumes exactly `size` bytes of a metadata block from `stream` (read(uint8_t*, n)
// returns the count, 0 for "nothing yet", < 0 for an error) and calls
// onTitle(const char*) at most once. pause() is called where the original code
// waited one millisecond. Returns false when the stream reported an error; the
// remaining bytes of the block are then not consumed (same as the original).
template <class Stream, class OnTitle, class Pause>
bool consume(Stream& stream, int size, OnTitle&& onTitle, Pause&& pause) {
    char buf[kBufferBytes];
    char* const readInto = buf + kHistoryBytes;
    std::memset(buf, 0, kHistoryBytes);  // no residual match across blocks
    while (size > 0) {
        const int chunk = size > 256 ? 256 : size;
        int got = stream.read(reinterpret_cast<uint8_t*>(readInto), size_t(chunk));
        if (got < 0) return false;
        if (got == 0) { pause(); continue; }
        if (got > chunk) got = chunk;  // a stream never returns more than asked
        size -= got;
        // buf[0..15] = carried history, buf[16..16+got) = new data
        const int end = kHistoryBytes + got;
        const char* header = findKey(buf, end, "StreamTitle=", kKeyBytes);
        if (!header) {
            // No match: keep the last 16 bytes as history and read on.
            std::memmove(buf, buf + end - kHistoryBytes, kHistoryBytes);
            pause();
            continue;
        }
        // Move the header to the front. `sized` is the value the original code
        // used to size its next reads; `have` is the number of real data bytes.
        int sized = end - int(header - buf) + 1;
        int have = sized - 1;
        std::memmove(buf, header, size_t(have));
        // Fill the buffer to the end with more data from the same block.
        while (size > 0 && sized < 255) {
            const int want = size > (256 - sized) ? (256 - sized) : size;
            got = stream.read(reinterpret_cast<uint8_t*>(buf + have), size_t(want));
            if (got < 0) return false;
            if (got == 0) { pause(); continue; }
            if (got > want) got = want;
            size -= got;
            sized += got;
            have += got;
        }
        buf[have] = '\0';  // have <= 272 < kBufferBytes: all searches below stop here
        char* title = buf + kKeyBytes;
        if (*title == '\'' || *title == '"') {
            const char closing[3] = { *title, ';', '\0' };
            char* stop = std::strstr(title + 1, closing);
            if (!stop) stop = std::strchr(buf + kKeyBytes + 1, ';');
            if (stop) *stop = '\0';
            ++title;
        } else {
            char* stop = std::strchr(title, ';');
            if (stop) *stop = '\0';
        }
        onTitle(static_cast<const char*>(title));
        // Skip the rest of the block so the audio stays aligned.
        while (size > 0) {
            const int skip = size > 256 ? 256 : size;
            got = stream.read(reinterpret_cast<uint8_t*>(buf), size_t(skip));
            if (got < 0) return false;
            if (got == 0) { pause(); continue; }
            if (got > skip) got = skip;
            size -= got;
        }
    }
    return true;
}

}  // namespace icy_metadata

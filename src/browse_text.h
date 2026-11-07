#pragma once
// Sanitizes a station / country / genre name received from Radio Browser so it
// can ALWAYS be encoded into an RTCP browser page.
//
// rtcp_browser::state() refuses a whole page (and the RTCP server then closes
// the link with reason application_invalid) when any name on it is empty,
// contains a control character (< 0x20, 0x7F) or is not valid UTF-8 (including
// a multi-byte character cut by a fixed-size copy). A single bad name from the
// public database therefore disconnected the REMOTE. This function is applied
// once, at parse time, to every name that later reaches state().
//
// Rules (output always valid for rtcp_browser validText/copyText):
//  - a control character (< 0x20, 0x7F, including NUL inside a counted string)
//    is treated as a space;
//  - leading and trailing spaces are removed and runs of spaces collapse to one;
//  - an invalid UTF-8 byte (lone continuation, bad lead, truncated or overlong
//    sequence, surrogate, > U+10FFFF) is replaced by one '?' per bad byte. '?' is
//    kept instead of dropping the byte so the user can see the name was damaged,
//    and it is plain ASCII printable, accepted by the REMOTE font and validator;
//  - the output is cut at a UTF-8 character boundary so that it fits capacity
//    INCLUDING the terminating NUL; a character is never split;
//  - an empty result is replaced by the placeholder (ASCII, cut to fit).
// Names that are already clean (valid UTF-8, no control character, no leading,
// trailing or doubled space, fitting the capacity) are copied byte-identical.
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace browse_text {

// Length in bytes of the valid UTF-8 character starting at in[at], or 0 when the
// bytes there are not a valid character (same rules as the RTCP validators).
inline size_t validWidth(const uint8_t* in, size_t size, size_t at) {
    const uint8_t first = in[at];
    size_t width;
    uint32_t cp;
    if (first < 0x80) return 1;
    if (first >= 0xc2 && first <= 0xdf) { width = 2; cp = first & 0x1f; }
    else if (first >= 0xe0 && first <= 0xef) { width = 3; cp = first & 0x0f; }
    else if (first >= 0xf0 && first <= 0xf4) { width = 4; cp = first & 0x07; }
    else return 0;
    if (at + width > size) return 0;
    for (size_t i = 1; i < width; ++i) {
        const uint8_t next = in[at + i];
        if ((next & 0xc0) != 0x80) return 0;
        cp = (cp << 6) | (next & 0x3f);
    }
    if ((width == 2 && cp < 0x80) || (width == 3 && cp < 0x800) ||
        (width == 4 && cp < 0x10000) || (cp >= 0xd800 && cp <= 0xdfff) ||
        cp > 0x10ffff) return 0;
    return width;
}

// Writes at most capacity-1 bytes plus the NUL. Returns the length written.
// capacity 0: nothing is written.
inline size_t sanitize(const char* text, size_t size, char* out, size_t capacity,
                       const char* placeholder) {
    if (!out || !capacity) return 0;
    const uint8_t* in = reinterpret_cast<const uint8_t*>(text);
    size_t used = 0;
    bool pending_space = false;
    for (size_t at = 0; in && at < size;) {
        const uint8_t byte = in[at];
        if (byte < 0x20 || byte == 0x7f || byte == ' ') {
            pending_space = used != 0;
            ++at;
            continue;
        }
        size_t width = validWidth(in, size, at);
        const char replacement = '?';
        const uint8_t* source = in + at;
        if (!width) { source = reinterpret_cast<const uint8_t*>(&replacement); width = 1; }
        const size_t need = width + (pending_space ? 1 : 0);
        if (used + need > capacity - 1) break;  // never split a character
        if (pending_space) out[used++] = ' ';
        pending_space = false;
        std::memcpy(out + used, source, width);
        used += width;
        at += width;
    }
    if (!used && placeholder) {
        while (placeholder[used] && used < capacity - 1) { out[used] = placeholder[used]; ++used; }
        while (used > 1 && out[used - 1] == ' ') --used;  // a cut placeholder never ends in a space
    }
    out[used] = 0;
    return used;
}

inline size_t sanitize(const char* text, char* out, size_t capacity,
                       const char* placeholder) {
    return sanitize(text, text ? std::strlen(text) : 0, out, capacity, placeholder);
}

}  // namespace browse_text

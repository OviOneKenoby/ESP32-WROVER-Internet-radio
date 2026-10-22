#include "rtcp_btx.h"

#include <cstring>

namespace rtcp_btx {
namespace {

uint64_t get64(const uint8_t* p) {
    uint64_t value = 0;
    for (unsigned i = 0; i < 8; ++i) value = (value << 8) | p[i];
    return value;
}

void put16(uint8_t* p, uint16_t value) {
    p[0] = uint8_t(value >> 8);
    p[1] = uint8_t(value);
}

void put64(uint8_t* p, uint64_t value) {
    for (int i = 7; i >= 0; --i) {
        p[i] = uint8_t(value);
        value >>= 8;
    }
}

bool decode(const unsigned char* p, size_t available, size_t& width, uint32_t& cp) {
    if (!available) return false;
    if (p[0] < 0x80) {
        width = 1;
        cp = p[0];
        return cp >= 0x20 && cp != 0x7f;
    }
    if (p[0] >= 0xc2 && p[0] <= 0xdf) {
        width = 2;
        cp = p[0] & 0x1f;
    } else if (p[0] >= 0xe0 && p[0] <= 0xef) {
        width = 3;
        cp = p[0] & 0x0f;
    } else if (p[0] >= 0xf0 && p[0] <= 0xf4) {
        width = 4;
        cp = p[0] & 0x07;
    } else {
        return false;
    }
    if (width > available) return false;
    for (size_t i = 1; i < width; ++i) {
        if ((p[i] & 0xc0) != 0x80) return false;
        cp = (cp << 6) | (p[i] & 0x3f);
    }
    if ((width == 2 && cp < 0x80) || (width == 3 && cp < 0x800) ||
        (width == 4 && cp < 0x10000) ||
        (cp >= 0xd800 && cp <= 0xdfff) || cp > 0x10ffff) return false;
    return true;
}

uint8_t flagsFor(const Snapshot& snapshot, uint8_t artist_size, uint8_t title_size) {
    uint8_t flags = 0;
    if (artist_size) flags |= artist_valid;
    if (title_size) flags |= title_valid;
    if (snapshot.state.playback == Playback::stopped ||
        snapshot.state.playback == Playback::paused) flags |= play_available;
    if (snapshot.state.playback == Playback::playing) flags |= pause_available;
    flags |= previous_available | next_available;
    return flags;
}

}  // namespace

CapabilityReply Session::capabilityRequest() {
    if (negotiation_ == Negotiation::before_first) {
        negotiation_ = Negotiation::probe_window;
        return CapabilityReply::legacy;
    }
    if (negotiation_ == Negotiation::probe_window) {
        negotiation_ = Negotiation::btx;
        return CapabilityReply::btx;
    }
    return CapabilityReply::invalid;
}

void Session::observeNonCapability() {
    if (negotiation_ == Negotiation::probe_window) negotiation_ = Negotiation::legacy;
}

void Session::clear() {
    negotiation_ = Negotiation::before_first;
    command_started_us_ = 0;
}

bool Mailbox::put(const Command& command) {
    if (occupied_ || !command.request || !command.source_generation) return false;
    request_ = command.request;
    source_generation_ = command.source_generation;
    operation_ = command.operation;
    expected_playback_ = command.expected_playback;
    occupied_ = true;
    return true;
}

bool Mailbox::take(Command& command) {
    if (!occupied_) return false;
    command.request = request_;
    command.source_generation = source_generation_;
    command.operation = operation_;
    command.expected_playback = expected_playback_;
    clear();
    return true;
}

void Mailbox::clear() {
    request_ = source_generation_ = 0;
    operation_ = Operation::play;
    expected_playback_ = Playback::unknown;
    occupied_ = false;
}

bool validUtf8Text(const char* text, size_t size) {
    if (!text && size) return false;
    size_t at = 0;
    while (at < size) {
        size_t width = 0;
        uint32_t cp = 0;
        if (!decode(reinterpret_cast<const unsigned char*>(text) + at,
                    size - at, width, cp)) return false;
        at += width;
    }
    return true;
}

bool copyLocalText(const char* source, size_t maximum, char* destination,
                   size_t capacity, uint8_t& size) {
    size = 0;
    if (!destination || !capacity || maximum + 1 > capacity) return false;
    destination[0] = 0;
    if (!source || !source[0]) return true;
    constexpr size_t kMaximumLocalBytes = 511;
    const size_t source_size = strnlen(source, kMaximumLocalBytes + 1);
    if (source_size > kMaximumLocalBytes || !validUtf8Text(source, source_size)) return false;
    size_t at = 0;
    while (at < source_size) {
        size_t width = 0;
        uint32_t cp = 0;
        if (!decode(reinterpret_cast<const unsigned char*>(source) + at,
                    source_size - at, width, cp)) return false;
        if (at + width > maximum) break;
        at += width;
    }
    memcpy(destination, source, at);
    destination[at] = 0;
    size = uint8_t(at);
    return true;
}

bool advance(uint64_t& value) {
    if (!value || value == UINT64_MAX) {
        value = 0;
        return false;
    }
    ++value;
    return true;
}

size_t capabilities(uint8_t* output, size_t capacity) {
    if (!output || capacity < 16) return 0;
    output[0] = kCapabilityResponse;
    output[1] = kMajor;
    output[2] = kMinor;
    output[3] = kFeatures;
    output[4] = kMaxArtistBytes;
    output[5] = kMaxTitleBytes;
    put16(output + 6, kFreshnessMs);
    put64(output + 8, kCapabilityRevision);
    return 16;
}

size_t stateResponse(const Snapshot& snapshot, uint64_t sequence,
                     uint8_t* output, size_t capacity) {
    if (!output || !sequence || !snapshot.state.source_generation ||
        !snapshot.state.metadata_revision ||
        uint8_t(snapshot.state.source) > uint8_t(Source::other) ||
        uint8_t(snapshot.state.connection) > uint8_t(Connection::connected) ||
        uint8_t(snapshot.state.playback) > uint8_t(Playback::paused)) return 0;

    Connection connection = snapshot.state.connection;
    Playback playback = snapshot.state.playback;
    uint8_t artist_size = snapshot.artist_size;
    uint8_t title_size = snapshot.title_size;
    if (snapshot.state.source != Source::bluetooth) {
        connection = Connection::not_applicable;
        playback = Playback::unknown;
        artist_size = title_size = 0;
    } else if (connection != Connection::connected) {
        if (connection == Connection::not_applicable) return 0;
        playback = Playback::unknown;
        artist_size = title_size = 0;
    }
    if (artist_size > kMaxArtistBytes || title_size > kMaxTitleBytes ||
        (artist_size && !validUtf8Text(snapshot.artist, artist_size)) ||
        (title_size && !validUtf8Text(snapshot.title, title_size))) return 0;

    const size_t total = kFixedStateBytes + artist_size + title_size;
    if (total > kMaxStateBytes || capacity < total) return 0;
    const uint8_t flags = snapshot.state.source == Source::bluetooth &&
                                  connection == Connection::connected
                              ? flagsFor(snapshot, artist_size, title_size)
                              : 0;
    output[0] = kStateResponse;
    output[1] = kMajor;
    output[2] = uint8_t(snapshot.state.source);
    output[3] = uint8_t(connection);
    output[4] = uint8_t(playback);
    output[5] = flags;
    output[6] = artist_size;
    output[7] = title_size;
    put64(output + 8, sequence);
    put64(output + 16, snapshot.state.source_generation);
    put64(output + 24, snapshot.state.metadata_revision);
    size_t at = kFixedStateBytes;
    if (artist_size) {
        memcpy(output + at, snapshot.artist, artist_size);
        at += artist_size;
    }
    if (title_size) {
        memcpy(output + at, snapshot.title, title_size);
        at += title_size;
    }
    return at;
}

bool parseCommand(const uint8_t* input, size_t size, Command& command) {
    if (!input || size != 19 || input[0] != kCommandRequest) return false;
    command.request = get64(input + 1);
    command.operation = Operation(input[9]);
    command.expected_playback = Playback(input[10]);
    command.source_generation = get64(input + 11);
    return command.request && command.source_generation && input[9] >= 1 && input[9] <= 4 &&
           input[10] <= uint8_t(Playback::paused);
}

Decision admit(const Snapshot& snapshot, const Command& command) {
    Decision decision{};
    if (!command.request || !command.source_generation ||
        uint8_t(command.operation) < uint8_t(Operation::play) ||
        uint8_t(command.operation) > uint8_t(Operation::next) ||
        uint8_t(command.expected_playback) > uint8_t(Playback::paused)) return decision;
    if (command.source_generation != snapshot.state.source_generation ||
        command.expected_playback != snapshot.state.playback) {
        decision.error = stale;
        return decision;
    }
    if (snapshot.state.source != Source::bluetooth ||
        snapshot.state.connection != Connection::connected) {
        decision.error = unavailable;
        return decision;
    }
    bool available = false;
    switch (command.operation) {
        case Operation::play:
            available = snapshot.state.playback == Playback::stopped ||
                        snapshot.state.playback == Playback::paused;
            break;
        case Operation::pause:
            available = snapshot.state.playback == Playback::playing;
            break;
        case Operation::previous:
        case Operation::next:
            available = true;
            break;
    }
    decision.error = available ? success : unavailable;
    return decision;
}

}  // namespace rtcp_btx

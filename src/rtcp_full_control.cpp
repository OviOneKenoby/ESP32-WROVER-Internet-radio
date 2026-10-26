#include "rtcp_full_control.h"

#include <cstring>

namespace rtcp_full_control {
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

void put32(uint8_t* p, uint32_t value) {
    for (int i = 3; i >= 0; --i) {
        p[i] = uint8_t(value);
        value >>= 8;
    }
}

void put64(uint8_t* p, uint64_t value) {
    for (int i = 7; i >= 0; --i) {
        p[i] = uint8_t(value);
        value >>= 8;
    }
}

bool validSource(Source source) {
    return source == Source::radio || source == Source::bluetooth;
}

bool validMenuMode(MenuMode mode) {
    return mode >= MenuMode::station_select && mode <= MenuMode::other;
}

bool validMenuText(const char* text, size_t size) {
    if (!text && size) return false;
    for (size_t at = 0; at < size;) {
        const uint8_t first = static_cast<uint8_t>(text[at]);
        size_t width = 0;
        uint32_t codepoint = 0;
        if (first < 0x80) {
            width = 1;
            codepoint = first;
            if (codepoint < 0x20 || codepoint == 0x7f) return false;
        } else if (first >= 0xc2 && first <= 0xdf) {
            width = 2;
            codepoint = first & 0x1f;
        } else if (first >= 0xe0 && first <= 0xef) {
            width = 3;
            codepoint = first & 0x0f;
        } else if (first >= 0xf0 && first <= 0xf4) {
            width = 4;
            codepoint = first & 0x07;
        } else {
            return false;
        }
        if (at + width > size) return false;
        for (size_t i = 1; i < width; ++i) {
            const uint8_t continuation = static_cast<uint8_t>(text[at + i]);
            if ((continuation & 0xc0) != 0x80) return false;
            codepoint = (codepoint << 6) | (continuation & 0x3f);
        }
        if ((width == 2 && codepoint < 0x80) ||
            (width == 3 && codepoint < 0x800) ||
            (width == 4 && codepoint < 0x10000) ||
            (codepoint >= 0xd800 && codepoint <= 0xdfff) ||
            codepoint > 0x10ffff)
            return false;
        at += width;
    }
    return true;
}

void hashByte(uint64_t& hash, uint8_t value) {
    hash ^= value;
    hash *= 1099511628211ULL;
}

void hash16(uint64_t& hash, uint16_t value) {
    hashByte(hash, uint8_t(value >> 8));
    hashByte(hash, uint8_t(value));
}

bool copyMenuText(const char* source, uint8_t* output, size_t capacity,
                  uint8_t& written) {
    written = 0;
    if (!source || !source[0]) return false;
    const size_t source_size = std::strlen(source);
    if (!validMenuText(source, source_size)) return false;
    size_t size = source_size < kMenuNameBytes ? source_size : kMenuNameBytes;
    while (size && (static_cast<uint8_t>(source[size]) & 0xc0) == 0x80) --size;
    if (!size || size > capacity) return false;
    std::memcpy(output, source, size);
    written = uint8_t(size);
    return true;
}

}  // namespace

bool SourceMailbox::put(const SourceCommand& command) {
    if (occupied_ || !command.request || !command.expected_generation ||
        !validSource(command.target)) return false;
    request_ = command.request;
    expected_generation_ = command.expected_generation;
    target_ = command.target;
    occupied_ = true;
    return true;
}

bool SourceMailbox::take(SourceCommand& command) {
    if (!occupied_) return false;
    command.request = request_;
    command.expected_generation = expected_generation_;
    command.target = target_;
    clear();
    return true;
}

void SourceMailbox::clear() {
    request_ = 0;
    expected_generation_ = 0;
    target_ = Source::none;
    occupied_ = false;
}

size_t capabilities(uint8_t* output, size_t capacity) {
    if (!output || capacity < 20) return 0;
    output[0] = kCapabilityResponse;
    output[1] = kMajor;
    output[2] = kMinor;
    output[3] = 0;
    put32(output + 4, kFeatureSourceSwitch | kFeatureMenuProjection |
                          kFeatureCatalogQuery | kFeatureCatalogCommand |
                          kFeatureRadioBrowser);
    put16(output + 8, kFreshnessMs);
    output[10] = output[11] = 0;
    put64(output + 12, kCapabilityRevision);
    return 20;
}

uint64_t MenuRevision::observe(const MenuView& view) {
    uint64_t signature = 1469598103934665603ULL;
    hashByte(signature, uint8_t(view.mode));
    hashByte(signature, view.flags);
    hashByte(signature, view.error);
    hash16(signature, view.selected);
    hash16(signature, view.count);
    hash16(signature, view.window_start);
    hashByte(signature, view.window_count);
    for (uint8_t i = 0; i < view.window_count && i < kMenuWindowItems; ++i) {
        const MenuItemView& item = view.items[i];
        hash16(signature, item.token);
        hashByte(signature, item.active ? 1 : 0);
        const char* name = item.name ? item.name : "";
        const size_t size = std::strlen(name);
        for (size_t at = 0; at < size; ++at)
            hashByte(signature, static_cast<uint8_t>(name[at]));
        hashByte(signature, 0);
    }
    if (!signature) signature = 1;
    if (!signature_) {
        signature_ = signature;
    } else if (signature_ != signature) {
        signature_ = signature;
        if (revision_ != UINT64_MAX) ++revision_;
    }
    return revision_;
}

size_t menuState(const MenuView& view, uint64_t sequence, uint64_t revision,
                 uint8_t* output, size_t capacity) {
    const bool invalid_selection = view.count ? view.selected >= view.count
                                               : view.selected != 0;
    const bool invalid_error = (view.flags & menu_error) ? view.error == 0
                                                         : view.error != 0;
    if (!output || capacity < 28 || !sequence || !revision ||
        !validMenuMode(view.mode) || (view.flags & ~(menu_loading | menu_error)) ||
        view.window_count > kMenuWindowItems || invalid_selection ||
        view.window_start > view.count ||
        size_t(view.window_start) + view.window_count > view.count ||
        invalid_error)
        return 0;
    output[0] = kMenuStateResponse;
    output[1] = kMajor;
    output[2] = uint8_t(view.mode);
    output[3] = view.flags;
    put16(output + 4, view.selected);
    put16(output + 6, view.count);
    put16(output + 8, view.window_start);
    output[10] = view.window_count;
    output[11] = view.error;
    put64(output + 12, sequence);
    put64(output + 20, revision);
    size_t at = 28;
    for (uint8_t i = 0; i < view.window_count; ++i) {
        const MenuItemView& item = view.items[i];
        if (!item.name || item.token >= view.count || capacity - at < 4)
            return 0;
        put16(output + at, item.token);
        output[at + 2] = item.active ? 1 : 0;
        uint8_t text_size = 0;
        if (!copyMenuText(item.name, output + at + 4, capacity - at - 4,
                          text_size))
            return 0;
        output[at + 3] = text_size;
        at += 4 + text_size;
    }
    return at;
}

size_t sourceState(const SourceState& state, uint64_t sequence,
                   uint8_t* output, size_t capacity) {
    if (!output || capacity < 24 || !sequence || !state.generation ||
        !validSource(state.source) ||
        uint8_t(state.transition) > uint8_t(Transition::failed) ||
        state.last_error > internal) return 0;
    output[0] = kSourceStateResponse;
    output[1] = kMajor;
    output[2] = uint8_t(state.source);
    output[3] = uint8_t(state.transition);
    output[4] = state.bluetooth_connected ? 1 : 0;
    output[5] = state.last_error;
    output[6] = output[7] = 0;
    put64(output + 8, sequence);
    put64(output + 16, state.generation);
    return 24;
}

bool parseSourceCommand(const uint8_t* input, size_t size,
                        SourceCommand& command) {
    if (!input || size != 18 || input[0] != kSourceSelectRequest) return false;
    command.request = get64(input + 1);
    command.target = Source(input[9]);
    command.expected_generation = get64(input + 10);
    return command.request && command.expected_generation &&
           validSource(command.target);
}

Decision admitSource(const SourceState& state, const SourceCommand& command,
                     bool command_pending) {
    Decision decision{};
    if (!command.request || !command.expected_generation ||
        !validSource(command.target)) return decision;
    if (command.expected_generation != state.generation) {
        decision.error = stale;
        return decision;
    }
    if (state.transition != Transition::stable || command_pending) {
        decision.error = busy;
        return decision;
    }
    if (command.target == state.source) {
        decision.error = success;
        decision.idempotent = true;
        return decision;
    }
    decision.error = success;
    return decision;
}

}  // namespace rtcp_full_control

#include "rtcp_catalog.h"

#include <cstring>

namespace rtcp_catalog {
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

bool validCollection(Collection collection) {
    return collection >= Collection::saved && collection <= Collection::recent;
}

bool validOperation(Operation operation) {
    return operation >= Operation::play && operation <= Operation::add_favorite;
}

bool validText(const char* text, size_t size) {
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
            width = 2; codepoint = first & 0x1f;
        } else if (first >= 0xe0 && first <= 0xef) {
            width = 3; codepoint = first & 0x0f;
        } else if (first >= 0xf0 && first <= 0xf4) {
            width = 4; codepoint = first & 0x07;
        } else return false;
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
            codepoint > 0x10ffff) return false;
        at += width;
    }
    return true;
}

bool copyText(const char* source, uint8_t* output, size_t capacity,
              uint8_t& written) {
    written = 0;
    if (!source || !source[0]) return false;
    const size_t source_size = std::strlen(source);
    if (!validText(source, source_size)) return false;
    size_t size = source_size < kNameBytes ? source_size : kNameBytes;
    while (size && (static_cast<uint8_t>(source[size]) & 0xc0) == 0x80) --size;
    if (!size || size > capacity) return false;
    std::memcpy(output, source, size);
    written = uint8_t(size);
    return true;
}

}  // namespace

bool Mailbox::put(const Command& command) {
    if (occupied_ || !command.request || !command.expected_revision ||
        !validOperation(command.operation) || !validCollection(command.collection))
        return false;
    command_ = command;
    occupied_ = true;
    return true;
}

bool Mailbox::take(Command& command) {
    if (!occupied_) return false;
    command = command_;
    clear();
    return true;
}

void Mailbox::clear() {
    command_ = {};
    occupied_ = false;
}

bool parseQuery(const uint8_t* input, size_t size, Query& query) {
    if (!input || size != 5 || input[0] != kQueryRequest) return false;
    query.collection = Collection(input[1]);
    query.offset = uint16_t(input[2]) << 8 | input[3];
    query.limit = input[4];
    return validCollection(query.collection) && query.limit &&
           query.limit <= kWindowItems;
}

size_t page(const PageView& view, uint64_t sequence,
            uint8_t* output, size_t capacity) {
    if (!output || capacity < 28 || !sequence || !view.revision ||
        !validCollection(view.collection) || (view.flags & ~full) ||
        !view.capacity || view.count > view.capacity || view.start > view.count ||
        view.item_count > kWindowItems ||
        size_t(view.start) + view.item_count > view.count)
        return 0;
    output[0] = kPageResponse;
    output[1] = kMajor;
    output[2] = uint8_t(view.collection);
    output[3] = view.flags;
    put16(output + 4, view.count);
    put16(output + 6, view.start);
    output[8] = view.item_count;
    output[9] = view.capacity;
    output[10] = output[11] = 0;
    put64(output + 12, sequence);
    put64(output + 20, view.revision);
    size_t at = 28;
    for (uint8_t i = 0; i < view.item_count; ++i) {
        const ItemView& item = view.items[i];
        if (!item.name || item.token >= view.count || item.token != view.start + i ||
            capacity - at < 4) return 0;
        put16(output + at, item.token);
        output[at + 2] = item.active ? 1 : 0;
        uint8_t text_size = 0;
        if (!copyText(item.name, output + at + 4, capacity - at - 4,
                      text_size)) return 0;
        output[at + 3] = text_size;
        at += 4 + text_size;
    }
    return at;
}

bool parseCommand(const uint8_t* input, size_t size, Command& command) {
    if (!input || size != 21 || input[0] != kCommandRequest) return false;
    command.request = get64(input + 1);
    command.operation = Operation(input[9]);
    command.collection = Collection(input[10]);
    command.token = uint16_t(input[11]) << 8 | input[12];
    command.expected_revision = get64(input + 13);
    return command.request && command.expected_revision &&
           validOperation(command.operation) && validCollection(command.collection);
}

Decision admit(const Command& command, uint64_t current_revision,
               uint16_t current_count, bool pending) {
    Decision decision{};
    if (!command.request || !command.expected_revision ||
        !validOperation(command.operation) || !validCollection(command.collection))
        return decision;
    if (command.expected_revision != current_revision) {
        decision.error = stale;
        return decision;
    }
    if (pending) {
        decision.error = busy;
        return decision;
    }
    if (command.token >= current_count) {
        decision.error = unavailable;
        return decision;
    }
    if (command.operation == Operation::remove &&
        command.collection == Collection::recent) {
        decision.error = unsupported;
        return decision;
    }
    if (command.operation == Operation::add_favorite &&
        command.collection != Collection::recent) {
        decision.error = unsupported;
        return decision;
    }
    decision.error = success;
    return decision;
}

}  // namespace rtcp_catalog

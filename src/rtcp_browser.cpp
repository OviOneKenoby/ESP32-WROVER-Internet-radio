#include "rtcp_browser.h"

#include <cstring>

namespace rtcp_browser {
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

bool validStage(Stage stage) {
    return stage >= Stage::countries && stage <= Stage::results;
}

bool validStatus(Status status) {
    return status >= Status::idle && status <= Status::failed;
}

bool validOperation(Operation operation) {
    return operation >= Operation::start && operation <= Operation::cancel;
}

bool validText(const char* text, size_t size) {
    if (!text || !size) return false;
    for (size_t at = 0; at < size;) {
        const uint8_t first = static_cast<uint8_t>(text[at]);
        size_t width = 0;
        uint32_t codepoint = 0;
        if (first < 0x80) {
            width = 1; codepoint = first;
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
    if (occupied_ || !command.request || !validOperation(command.operation))
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
    if (!input || size != 14 || input[0] != kStateRequest) return false;
    query.stage = Stage(input[1]);
    query.offset = uint16_t(input[2]) << 8 | input[3];
    query.limit = input[4];
    query.filter = input[5];
    query.generation = get64(input + 6);
    return validStage(query.stage) && query.limit && query.limit <= kWindowItems &&
           (query.generation || query.stage == Stage::countries) &&
           ((query.stage == Stage::countries &&
             (!query.filter || (query.filter >= 'A' && query.filter <= 'Z'))) ||
            (query.stage != Stage::countries && !query.filter));
}

size_t state(const StateView& view, uint64_t sequence,
             uint8_t* output, size_t capacity) {
    const bool failed = view.status == Status::failed;
    if (!output || capacity < 36 || !sequence || !view.generation ||
        !view.revision || !validStage(view.stage) || !validStatus(view.status) ||
        view.error > parse_failed || failed != (view.error != success) ||
        view.start > view.count || view.item_count > kWindowItems ||
        size_t(view.start) + view.item_count > view.count ||
        (view.status != Status::ready && view.item_count)) return 0;
    output[0] = kStateResponse;
    output[1] = kMajor;
    output[2] = uint8_t(view.stage);
    output[3] = uint8_t(view.status);
    output[4] = view.error;
    output[5] = view.item_count;
    put16(output + 6, view.count);
    put16(output + 8, view.start);
    output[10] = output[11] = 0;
    put64(output + 12, sequence);
    put64(output + 20, view.generation);
    put64(output + 28, view.revision);
    size_t at = 36;
    for (uint8_t i = 0; i < view.item_count; ++i) {
        const ItemView& item = view.items[i];
        if (!item.name || capacity - at < 4)
            return 0;
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
    if (!input || size != 29 || input[0] != kCommandRequest) return false;
    command.request = get64(input + 1);
    command.operation = Operation(input[9]);
    command.token = uint16_t(input[10]) << 8 | input[11];
    command.value = input[12];
    command.expected_generation = get64(input + 13);
    command.expected_revision = get64(input + 21);
    if (!command.request || !validOperation(command.operation)) return false;
    if (command.operation == Operation::start)
        return !command.token && !command.value && !command.expected_generation &&
               !command.expected_revision;
    if (command.operation == Operation::cancel)
        return !command.token && !command.value && command.expected_generation &&
               !command.expected_revision;
    if (!command.expected_generation || !command.expected_revision) return false;
    if (command.operation == Operation::select_name)
        return command.token <= 26 && !command.value;
    return !command.value;
}

Decision admit(const Command& command, uint64_t generation, uint64_t revision,
               Stage stage, Status status, uint16_t count, bool pending) {
    Decision decision{};
    if (!command.request || !validOperation(command.operation)) return decision;
    if (pending) { decision.error = busy; return decision; }
    if (command.operation == Operation::start) {
        decision.error = success;
        return decision;
    }
    if (!generation || command.expected_generation != generation) {
        decision.error = stale;
        return decision;
    }
    if (command.operation == Operation::cancel) {
        decision.error = success;
        return decision;
    }
    if (!revision || command.expected_revision != revision) {
        decision.error = stale;
        return decision;
    }
    if (status == Status::loading) { decision.error = busy; return decision; }
    if (status == Status::failed || status == Status::idle) {
        decision.error = unavailable;
        return decision;
    }
    const bool any = command.token == kAnyToken;
    switch (command.operation) {
    case Operation::select_country:
        // Country tokens are opaque indices into the complete bounded
        // TUNER catalog; a filtered page's visible count can be smaller.
        if (stage != Stage::countries)
            return decision;
        break;
    case Operation::select_name:
        if (stage != Stage::countries || command.token > 26) return decision;
        break;
    case Operation::select_genre:
        if (stage != Stage::genres || (!any && command.token >= count))
            return decision;
        break;
    case Operation::play:
    case Operation::add_favorite:
        if (stage != Stage::results || command.token >= count) return decision;
        break;
    default:
        return decision;
    }
    decision.error = success;
    return decision;
}

}  // namespace rtcp_browser

#include "rtcp_full_control.h"

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
    put32(output + 4, kFeatureSourceSwitch);
    put16(output + 8, kFreshnessMs);
    output[10] = output[11] = 0;
    put64(output + 12, kCapabilityRevision);
    return 20;
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

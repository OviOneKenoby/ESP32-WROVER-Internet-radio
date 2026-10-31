#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <array>

namespace rtcp {

enum class DiagnosticRequest : uint8_t {
    none = 0,
    identity = 1,
    capabilities = 3,
    snapshot = 5,
    volume_set = 7,
    playback_command = 9,
    now_playing = 10,
    btx_state = 13,
    btx_command = 15,
    full_control_capabilities = 16,
    source_state = 18,
    source_select = 20,
    menu_state = 21,
    catalog_query = 23,
    catalog_command = 25,
    browser_state = 26,
    browser_command = 28,
    unknown = 255,
};

enum class DiagnosticStage : uint8_t {
    idle = 0,
    accepted,
    client_finish,
    record_received,
    response_built,
    response_written,
    closed,
};

enum class CloseReason : uint8_t {
    none = 0,
    peer_eof,
    timeout,
    handshake_invalid,
    framing_invalid,
    authentication_failed,
    application_invalid,
    response_write_failed,
    transport_error,
};

struct Diagnostics {
    struct CloseEvent {
        uint32_t uptime_ms{0};
        uint32_t session{0};
        int32_t socket_error{0};
        uint64_t request_counter{0};
        uint64_t response_counter{0};
        uint32_t request_ms{0};
        uint32_t response_ms{0};
        uint8_t state{0};
        CloseReason reason{CloseReason::none};
        DiagnosticRequest request{DiagnosticRequest::none};
        DiagnosticRequest response_request{DiagnosticRequest::none};
        DiagnosticStage stage{DiagnosticStage::idle};
    };
    uint32_t connections{0};
    uint32_t client_finishes{0};
    uint32_t records{0};
    uint32_t snapshot_requests{0};
    uint32_t snapshot_completions{0};
    uint32_t partial_writes{0};
    uint32_t failed_writes{0};
    uint32_t timeouts{0};
    uint32_t eofs{0};
    uint32_t closes{0};
    uint32_t allocation_failure_delta{0};
    uint32_t free_internal{0};
    uint32_t largest_internal{0};
    uint64_t request_counter{0};
    uint64_t response_counter{0};
    uint32_t request_ms{0};
    uint32_t response_ms{0};
    uint16_t response_size{0};
    uint16_t response_written{0};
    DiagnosticRequest request{DiagnosticRequest::none};
    DiagnosticRequest response_request{DiagnosticRequest::none};
    CloseReason close_reason{CloseReason::none};
    DiagnosticStage stage{DiagnosticStage::idle};
    uint8_t reserved{0};
    int32_t transport_error{0};
    std::array<CloseEvent,4> close_history{};
    uint8_t close_history_next{0};
    uint8_t close_history_count{0};
    uint16_t close_history_reserved{0};

    static void increment(uint32_t& value, uint32_t by = 1) {
        const uint32_t remaining = std::numeric_limits<uint32_t>::max() - value;
        value += by > remaining ? remaining : by;
    }

    void accepted(uint32_t free_bytes, uint32_t largest_block) {
        increment(connections);
        free_internal = free_bytes;
        largest_internal = largest_block;
        stage = DiagnosticStage::accepted;
        transport_error = 0;
    }

    void clientFinish() {
        increment(client_finishes);
        stage = DiagnosticStage::client_finish;
    }

    void received(DiagnosticRequest kind, uint64_t counter, uint32_t now_ms,
                  uint32_t free_bytes, uint32_t largest_block) {
        increment(records);
        request = kind;
        request_counter = counter;
        request_ms = now_ms;
        if (kind == DiagnosticRequest::snapshot) increment(snapshot_requests);
        free_internal = free_bytes;
        largest_internal = largest_block;
        stage = DiagnosticStage::record_received;
    }

    void responseBuilt(size_t bytes) {
        response_size = bytes > UINT16_MAX ? UINT16_MAX : static_cast<uint16_t>(bytes);
        response_written = 0;
        stage = DiagnosticStage::response_built;
    }

    void responseWrite(size_t written, uint32_t allocation_delta,
                       uint64_t counter, uint32_t now_ms) {
        response_written = written > UINT16_MAX ? UINT16_MAX : static_cast<uint16_t>(written);
        increment(allocation_failure_delta, allocation_delta);
        if (written < response_size) {
            increment(written ? partial_writes : failed_writes);
            close_reason = CloseReason::response_write_failed;
            return;
        }
        if (request == DiagnosticRequest::snapshot) increment(snapshot_completions);
        response_request = request;
        response_counter = counter;
        response_ms = now_ms;
        stage = DiagnosticStage::response_written;
    }

    void closed(CloseReason reason, uint32_t free_bytes, uint32_t largest_block,
                uint32_t uptime_ms, uint8_t connection_state,
                int32_t socket_error = 0) {
        increment(closes);
        if (reason == CloseReason::timeout) increment(timeouts);
        if (reason == CloseReason::peer_eof) increment(eofs);
        close_reason = reason;
        free_internal = free_bytes;
        largest_internal = largest_block;
        const DiagnosticStage failure_stage = stage;
        stage = DiagnosticStage::closed;
        transport_error = socket_error;
        CloseEvent& event = close_history[close_history_next];
        event.uptime_ms = uptime_ms;
        event.session = connections;
        event.socket_error = socket_error;
        event.request_counter = request_counter;
        event.response_counter = response_counter;
        event.request_ms = request_ms;
        event.response_ms = response_ms;
        event.state = connection_state;
        event.reason = reason;
        event.request = request;
        event.response_request = response_request;
        event.stage = failure_stage;
        close_history_next = uint8_t((close_history_next + 1) % close_history.size());
        if (close_history_count < close_history.size()) ++close_history_count;
    }
};

static_assert(sizeof(Diagnostics) == 296, "RTCP diagnostics must remain fixed and bounded");

inline DiagnosticRequest diagnosticRequest(uint8_t value) {
    switch (value) {
        case 1: return DiagnosticRequest::identity;
        case 3: return DiagnosticRequest::capabilities;
        case 5: return DiagnosticRequest::snapshot;
        case 7: return DiagnosticRequest::volume_set;
        case 9: return DiagnosticRequest::playback_command;
        case 10: return DiagnosticRequest::now_playing;
        case 13: return DiagnosticRequest::btx_state;
        case 15: return DiagnosticRequest::btx_command;
        case 16: return DiagnosticRequest::full_control_capabilities;
        case 18: return DiagnosticRequest::source_state;
        case 20: return DiagnosticRequest::source_select;
        case 21: return DiagnosticRequest::menu_state;
        case 23: return DiagnosticRequest::catalog_query;
        case 25: return DiagnosticRequest::catalog_command;
        case 26: return DiagnosticRequest::browser_state;
        case 28: return DiagnosticRequest::browser_command;
        default: return DiagnosticRequest::unknown;
    }
}

}  // namespace rtcp

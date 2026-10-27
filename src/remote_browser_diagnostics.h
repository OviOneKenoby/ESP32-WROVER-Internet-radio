#pragma once

#include <cstdint>
#include <limits>

enum class RemoteBrowserCancelReason : uint8_t {
    none = 0,
    restart = 1,
    remote_command = 2,
    rtcp_closed = 3,
    playback_started = 4,
    physical_navigation = 5,
};

enum class RemoteBrowserTransportStage : uint8_t {
    idle = 0,
    client_initialized = 1,
    connecting = 2,
    connected = 3,
    headers_sent = 4,
    header_received = 5,
    body_received = 6,
    finished = 7,
    disconnected = 8,
    event_error = 9,
    deadline = 10,
};

struct RemoteBrowserDiagnostics {
    uint32_t start_requests{0};
    uint32_t start_results{0};
    uint32_t page_queries{0};
    uint32_t pages_produced{0};
    uint32_t jobs_completed{0};
    uint32_t jobs_failed{0};
    uint32_t cancellations{0};
    uint64_t generation{0};
    uint64_t revision{0};
    uint32_t elapsed_ms{0};
    int32_t transport_error{0};
    int32_t first_transport_error{0};
    int16_t http_status{0};
    int16_t first_http_status{0};
    int16_t tls_error{0};
    uint16_t tls_flags{0};
    uint8_t active{0};
    uint8_t job{0};
    uint8_t status{0};
    uint8_t error{0};
    RemoteBrowserCancelReason cancel_reason{RemoteBrowserCancelReason::none};
    uint8_t mirror{0};
    RemoteBrowserTransportStage transport_stage{RemoteBrowserTransportStage::idle};
    RemoteBrowserTransportStage first_failure_stage{RemoteBrowserTransportStage::idle};
    uint8_t first_failure_mirror{0};
    uint8_t deadline_expired{0};

    static void increment(uint32_t& value) {
        if (value != std::numeric_limits<uint32_t>::max()) ++value;
    }
};

static_assert(sizeof(RemoteBrowserDiagnostics) == 80,
              "browser diagnostics must remain fixed and allocation-free");

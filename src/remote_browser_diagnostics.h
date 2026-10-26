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
    uint8_t active{0};
    uint8_t job{0};
    uint8_t status{0};
    uint8_t error{0};
    RemoteBrowserCancelReason cancel_reason{RemoteBrowserCancelReason::none};
    uint8_t reserved[3]{};

    static void increment(uint32_t& value) {
        if (value != std::numeric_limits<uint32_t>::max()) ++value;
    }
};

static_assert(sizeof(RemoteBrowserDiagnostics) == 56,
              "browser diagnostics must remain fixed and allocation-free");

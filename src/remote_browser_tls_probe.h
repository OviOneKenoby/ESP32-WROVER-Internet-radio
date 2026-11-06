#pragma once

#include <cstdint>

namespace RemoteBrowserTlsProbe {

struct Snapshot {
    uint32_t calls{0};
    uint32_t failures{0};
    int32_t system_error{0};
    int32_t mbedtls_error{0};
    int32_t esp_error{0};
    uint32_t cert_flags{0};
    uint32_t start_internal_free{0};
    uint32_t start_internal_largest{0};
    uint32_t start_psram_free{0};
    uint32_t failure_internal_free{0};
    uint32_t failure_internal_largest{0};
    uint32_t failure_psram_free{0};
    uint32_t allocation_failures_start{0};
    uint32_t allocation_failures_at_failure{0};
    uint32_t allocation_failure_last_size{0};
    uint32_t allocation_failure_last_caps{0};
    uint32_t allocation_failure_largest_size{0};
    uint8_t available{0};
    uint8_t connection_state{0};
};

void resetJob();
void beginAttempt();
Snapshot snapshot();

}  // namespace RemoteBrowserTlsProbe

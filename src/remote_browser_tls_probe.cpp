#include "remote_browser_tls_probe.h"

#include "system_info.h"
#include <esp_heap_caps.h>
#include <esp_tls.h>
#include <esp_tls_errors.h>
#include <limits>

namespace {
RemoteBrowserTlsProbe::Snapshot probe;
bool attemptActive = false;

void increment(uint32_t& value) {
    if (value != std::numeric_limits<uint32_t>::max()) ++value;
}
}

namespace RemoteBrowserTlsProbe {

void resetJob() {
    probe = {};
    attemptActive = false;
}

void beginAttempt() {
    if (!probe.calls) {
        probe.start_internal_free =
            heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        probe.start_internal_largest =
            heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        probe.start_psram_free =
            heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        probe.allocation_failures_start = SystemInfo::allocationFailureCount();
    }
    increment(probe.calls);
    attemptActive = true;
}

Snapshot snapshot() { return probe; }

}  // namespace RemoteBrowserTlsProbe

extern "C" int __real_esp_tls_conn_new_async(const char* hostname,
                                               int hostlen, int port,
                                               const esp_tls_cfg_t* cfg,
                                               esp_tls_t* tls);

extern "C" int __wrap_esp_tls_conn_new_async(const char* hostname,
                                               int hostlen, int port,
                                               const esp_tls_cfg_t* cfg,
                                               esp_tls_t* tls) {
    const int result = __real_esp_tls_conn_new_async(
        hostname, hostlen, port, cfg, tls);
    if (result >= 0) {
        if (result > 0) attemptActive = false;
        return result;
    }
    if (!attemptActive) return result;
    attemptActive = false;
    increment(probe.failures);
    if (probe.failures != 1) return result;

    probe.failure_internal_free =
        heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    probe.failure_internal_largest =
        heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    probe.failure_psram_free =
        heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    probe.allocation_failures_at_failure = SystemInfo::allocationFailureCount();

    // The pinned Arduino-ESP32 2.0.17 exposes the connection error handle on
    // esp_tls_t but does not provide the newer esp_tls_get_error_handle API.
    // Read it synchronously while the failed async connection still exists.
    esp_tls_error_handle_t handle = tls ? tls->error_handle : nullptr;
    if (!handle) return result;
    if (esp_tls_get_and_clear_error_type(handle, ESP_TLS_ERR_TYPE_SYSTEM,
                                         &probe.system_error) == ESP_OK)
        probe.available |= 1u;
    if (esp_tls_get_and_clear_error_type(handle, ESP_TLS_ERR_TYPE_MBEDTLS,
                                         &probe.mbedtls_error) == ESP_OK)
        probe.available |= 2u;
    int cert_flags = 0;
    if (esp_tls_get_and_clear_error_type(handle,
            ESP_TLS_ERR_TYPE_MBEDTLS_CERT_FLAGS, &cert_flags) == ESP_OK) {
        probe.cert_flags = uint32_t(cert_flags);
        probe.available |= 4u;
    }
    if (esp_tls_get_and_clear_error_type(handle, ESP_TLS_ERR_TYPE_ESP,
                                         &probe.esp_error) == ESP_OK)
        probe.available |= 8u;
    return result;
}

#include "tls_record_buffer_psram.h"

#include <esp_heap_caps.h>
#include <mbedtls/ssl_internal.h>

static_assert(MBEDTLS_SSL_IN_BUFFER_LEN ==
                  TlsRecordBufferPsram::kResolvedRecordBufferBytes,
              "pinned mbedTLS input record-buffer size changed");
static_assert(MBEDTLS_SSL_OUT_BUFFER_LEN ==
                  TlsRecordBufferPsram::kResolvedRecordBufferBytes,
              "pinned mbedTLS output record-buffer size changed");

extern "C" void* __real_esp_mbedtls_mem_calloc(std::size_t count,
                                                std::size_t size);

extern "C" void* __wrap_esp_mbedtls_mem_calloc(std::size_t count,
                                                std::size_t size) {
    if (TlsRecordBufferPsram::isRecordBufferRequest(count, size)) {
        void* external = heap_caps_calloc(
            count, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (external) return external;
    }

    // Preserve the pinned framework's allocator for every other mbedTLS
    // object and as the fail-safe path when PSRAM is unavailable/exhausted.
    return __real_esp_mbedtls_mem_calloc(count, size);
}

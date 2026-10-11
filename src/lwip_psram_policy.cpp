#ifdef TUNER_LWIP_PSRAM_TX
#include "lwip_psram_policy.h"
#include <Arduino.h>
#include <lwip/mem.h>
#include <lwip/opt.h>
#include <esp_heap_caps.h>
#include <esp_spi_flash.h>
#include <soc/soc_memory_layout.h>

// This fixed SDK's mem_malloc is plain malloc(size), without a stats prefix.
// Its mem_free is free(ptr); ESP-IDF free accepts internal and PSRAM pointers.
// Do not enable this replacement with a different lwIP allocator ABI.
static_assert(MEM_LIBC_MALLOC == 1 && MEM_STATS == 0, "review lwIP allocator ABI");
static_assert(MEM_ALIGNMENT <= 4, "review PSRAM allocation alignment");
extern "C" void* __real_mem_malloc(mem_size_t size);
namespace {
struct Counters { uint32_t calls, external, fallback, original, externalBytes; };
DRAM_ATTR Counters counters;
DRAM_ATTR portMUX_TYPE policyMux = portMUX_INITIALIZER_UNLOCKED;
void IRAM_ATTR add(uint32_t& value, size_t amount = 1) {
    value = amount > UINT32_MAX - value ? UINT32_MAX : value + uint32_t(amount);
}
}
extern "C" void* __wrap_mem_malloc(mem_size_t size) {
    // Only lwIP heap allocations, not global malloc or Wi-Fi DMA allocation.
    // Preserve the original path for small blocks and unsuitable contexts.
    bool attempted = false;
    void* result = nullptr;
    if (size >= 512 && spi_flash_cache_enabled() && !xPortInIsrContext()) {
        attempted = true;
        result = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    const bool external = result != nullptr;
    if (!result) result = __real_mem_malloc(size);
    portENTER_CRITICAL(&policyMux);
    add(counters.calls);
    if (external) { add(counters.external); add(counters.externalBytes, size); }
    else { add(counters.original); if (attempted) add(counters.fallback); }
    portEXIT_CRITICAL(&policyMux);
    return result;
}
void LwipPsramPolicy::printSnapshot() {
    Counters copy;
    portENTER_CRITICAL(&policyMux); copy = counters; portEXIT_CRITICAL(&policyMux);
    Serial.printf("[LWIP_MEMORY] threshold=512 calls=%u\n", copy.calls);
    Serial.printf("[LWIP_MEMORY] psram=%u bytes=%u\n", copy.external, copy.externalBytes);
    Serial.printf("[LWIP_MEMORY] original=%u fallback=%u\n", copy.original, copy.fallback);
}
#endif

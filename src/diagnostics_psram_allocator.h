#pragma once

#include <ArduinoJson.h>
#include <esp_heap_caps.h>

// Dedicated to the full /api/diagnostics document. There is deliberately no
// INTERNAL fallback: a failed PSRAM allocation is surfaced through
// JsonDocument::overflowed() and the endpoint's existing bounded 503 path.
class DiagnosticsPsramAllocator final : public ArduinoJson::Allocator {
public:
    void* allocate(size_t size) override {
        return size ? heap_caps_malloc(size, capabilities()) : nullptr;
    }

    void deallocate(void* pointer) override {
        heap_caps_free(pointer);
    }

    void* reallocate(void* pointer, size_t size) override {
        if (!size) {
            heap_caps_free(pointer);
            return nullptr;
        }
        return heap_caps_realloc(pointer, size, capabilities());
    }

    static constexpr uint32_t capabilities() {
        return MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;
    }
};

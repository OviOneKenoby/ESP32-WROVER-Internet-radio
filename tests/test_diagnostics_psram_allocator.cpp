#include "diagnostics_psram_allocator.h"

#include <cstdlib>
#include <iostream>

namespace {
uint32_t lastCapabilities = 0;
size_t allocationCalls = 0;
size_t reallocationCalls = 0;
size_t freeCalls = 0;
bool failAllocations = false;
int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}
}  // namespace

extern "C" void* heap_caps_malloc(size_t size, uint32_t caps) {
    ++allocationCalls;
    lastCapabilities = caps;
    return failAllocations ? nullptr : std::malloc(size);
}

extern "C" void* heap_caps_realloc(void* pointer, size_t size, uint32_t caps) {
    ++reallocationCalls;
    lastCapabilities = caps;
    return failAllocations ? nullptr : std::realloc(pointer, size);
}

extern "C" void heap_caps_free(void* pointer) {
    ++freeCalls;
    std::free(pointer);
}

int main() {
    DiagnosticsPsramAllocator allocator;
    const uint32_t required = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;

    void* pointer = allocator.allocate(64);
    expect(pointer != nullptr, "allocation succeeds");
    expect(allocationCalls == 1, "allocation forwarded once");
    expect(lastCapabilities == required, "allocation uses PSRAM and 8BIT only");
    expect((lastCapabilities & MALLOC_CAP_INTERNAL) == 0,
           "allocation has no INTERNAL fallback");

    pointer = allocator.reallocate(pointer, 128);
    expect(pointer != nullptr, "reallocation succeeds");
    expect(reallocationCalls == 1, "reallocation forwarded once");
    expect(lastCapabilities == required, "reallocation preserves capabilities");
    allocator.deallocate(pointer);
    expect(freeCalls == 1, "deallocation forwarded once");

    expect(allocator.allocate(0) == nullptr, "zero allocation is bounded");
    expect(allocationCalls == 1, "zero allocation does not call heap API");

    pointer = allocator.allocate(16);
    expect(pointer != nullptr, "second allocation succeeds");
    expect(allocator.reallocate(pointer, 0) == nullptr,
           "zero reallocation releases and returns null");
    expect(freeCalls == 2, "zero reallocation frees storage");

    failAllocations = true;
    expect(allocator.allocate(64) == nullptr, "allocation failure propagates");
    ArduinoJson::JsonDocument document(&allocator);
    document["field"] = 1;
    expect(document.overflowed(), "ArduinoJson reports PSRAM allocation failure");

    if (failures) return 1;
    std::cout << "PASS diagnostics PSRAM allocator lifecycle\n";
    return 0;
}

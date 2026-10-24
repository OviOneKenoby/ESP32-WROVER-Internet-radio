#include "mp3_memory_diagnostics.h"

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <soc/soc_memory_types.h>

#include "system_info.h"

namespace Mp3MemoryDiagnostics {
namespace {

DRAM_ATTR Model model;
DRAM_ATTR portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
DRAM_ATTR bool active = false;
DRAM_ATTR std::uint8_t current_source = 0;
DRAM_ATTR std::uint8_t current_bluetooth_lifecycle = 0;

static_assert(kInternalCaps == (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
              "internal heap capability mask changed");
static_assert(kDmaCaps == (MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA),
              "DMA heap capability mask changed");

std::uint16_t boundedBlocks(std::size_t value) {
    return value > UINT16_MAX ? UINT16_MAX : static_cast<std::uint16_t>(value);
}

HeapState readHeap(std::uint32_t caps) {
    multi_heap_info_t info{};
    heap_caps_get_info(&info, caps);
    HeapState value;
    value.free_bytes = static_cast<std::uint32_t>(info.total_free_bytes);
    value.largest_block = static_cast<std::uint32_t>(info.largest_free_block);
    value.allocated_blocks = boundedBlocks(info.allocated_blocks);
    value.free_blocks = boundedBlocks(info.free_blocks);
    return value;
}

const char* milestoneName(std::uint8_t milestone) {
    static const char* const names[kMilestoneCount] = {
        "before_construction", "after_generator_allocation",
        "after_helix_initialization", "after_i2s_initialization",
        "stable_playback", "after_teardown",
    };
    return milestone < kMilestoneCount ? names[milestone] : "invalid";
}

const char* allocationName(std::uint8_t sequence) {
    static const char* const names[kAllocationCount] = {
        "MP3DecInfo", "FrameHeader", "SideInfo", "ScaleFactorInfo",
        "HuffmanInfo", "DequantInfo", "IMDCTInfo", "SubbandInfo",
    };
    return sequence > 0 && sequence <= kAllocationCount
        ? names[sequence - 1] : "invalid";
}

const char* placementName(Placement placement) {
    switch (placement) {
        case Placement::internal: return "internal";
        case Placement::external: return "psram";
        case Placement::other: return "other";
        default: return "unavailable";
    }
}

}  // namespace

void beginSession(std::uint8_t source, std::uint8_t bluetooth_lifecycle) {
    std::uint32_t next = 1;
    portENTER_CRITICAL(&mux);
    const std::uint32_t current = model.session();
    next = current == UINT32_MAX ? UINT32_MAX : current + 1;
    model.begin(next);
    current_source = source;
    current_bluetooth_lifecycle = bluetooth_lifecycle;
    active = true;
    portEXIT_CRITICAL(&mux);
    capture(TUNER_MP3_MEMORY_BEFORE_CONSTRUCTION);
}

void setContext(std::uint8_t source, std::uint8_t bluetooth_lifecycle) {
    portENTER_CRITICAL(&mux);
    current_source = source;
    current_bluetooth_lifecycle = bluetooth_lifecycle;
    portEXIT_CRITICAL(&mux);
}

void capture(std::uint8_t milestone) {
    if (milestone >= kMilestoneCount) return;
    portENTER_CRITICAL(&mux);
    const bool enabled = active;
    const std::uint8_t source = current_source;
    const std::uint8_t bluetooth = current_bluetooth_lifecycle;
    portEXIT_CRITICAL(&mux);
    if (!enabled) return;

    Snapshot value;
    value.internal = readHeap(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    value.dma = readHeap(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    value.psram_free = static_cast<std::uint32_t>(
        heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    value.allocation_failures = SystemInfo::allocationFailureCount();
    value.uptime_ms = millis();
    value.source = source;
    value.bluetooth_lifecycle = bluetooth;
    value.milestone = milestone;
    value.valid = true;

    portENTER_CRITICAL(&mux);
    model.recordSnapshot(value);
    if (milestone == TUNER_MP3_MEMORY_AFTER_TEARDOWN) active = false;
    portEXIT_CRITICAL(&mux);
}

void printSnapshot() {
    std::uint32_t session;
    portENTER_CRITICAL(&mux);
    session = model.session();
    portEXIT_CRITICAL(&mux);
    Serial.printf("[MP3_MEMORY] session=%lu storage=%u internal_caps=0x%08lx dma_caps=0x%08lx\n",
                  static_cast<unsigned long>(session), unsigned(sizeof(Model)),
                  static_cast<unsigned long>(kInternalCaps),
                  static_cast<unsigned long>(kDmaCaps));

    for (std::size_t index = 0; index < kMilestoneCount; ++index) {
        Snapshot value;
        portENTER_CRITICAL(&mux);
        value = model.snapshot(index);
        portEXIT_CRITICAL(&mux);
        if (!value.valid) continue;
        Serial.printf(
            "[MP3_MEMORY_PHASE] id=%u name=%s uptime_ms=%lu source=%u(%s) bluetooth=%u(%s) internal=%lu/%lu blocks=%u/%u dma=%lu/%lu blocks=%u/%u psram=%lu alloc_failures=%lu\n",
            unsigned(value.milestone), milestoneName(value.milestone),
            static_cast<unsigned long>(value.uptime_ms), unsigned(value.source),
            SystemInfo::audioSourceName(static_cast<AudioSource>(value.source)),
            unsigned(value.bluetooth_lifecycle),
            SystemInfo::bluetoothLifecycleName(
                static_cast<BluetoothLifecycleState>(value.bluetooth_lifecycle)),
            static_cast<unsigned long>(value.internal.free_bytes),
            static_cast<unsigned long>(value.internal.largest_block),
            unsigned(value.internal.allocated_blocks),
            unsigned(value.internal.free_blocks),
            static_cast<unsigned long>(value.dma.free_bytes),
            static_cast<unsigned long>(value.dma.largest_block),
            unsigned(value.dma.allocated_blocks), unsigned(value.dma.free_blocks),
            static_cast<unsigned long>(value.psram_free),
            static_cast<unsigned long>(value.allocation_failures));
    }

    for (std::size_t index = 0; index < kAllocationCount; ++index) {
        Allocation value;
        portENTER_CRITICAL(&mux);
        value = model.allocation(index);
        portEXIT_CRITICAL(&mux);
        if (!value.valid) continue;
        Serial.printf(
            "[MP3_MEMORY_ALLOC] sequence=%u name=%s requested=%lu success=%u placement=%s allocated=%lu\n",
            unsigned(value.sequence), allocationName(value.sequence),
            static_cast<unsigned long>(value.requested), value.success ? 1u : 0u,
            placementName(value.placement),
            static_cast<unsigned long>(value.allocated));
    }
}

void recordAllocation(std::uint8_t sequence, std::size_t requested,
                      const void* pointer) {
    portENTER_CRITICAL(&mux);
    const bool enabled = active;
    portEXIT_CRITICAL(&mux);
    if (!enabled) return;

    Allocation value;
    value.sequence = sequence;
    value.requested = static_cast<std::uint32_t>(requested);
    value.success = pointer != nullptr;
    value.valid = true;
    if (pointer) {
        value.allocated = static_cast<std::uint32_t>(
            heap_caps_get_allocated_size(const_cast<void*>(pointer)));
        if (esp_ptr_internal(pointer)) {
            value.placement = Placement::internal;
        } else if (esp_ptr_external_ram(pointer)) {
            value.placement = Placement::external;
        } else {
            value.placement = Placement::other;
        }
    }

    portENTER_CRITICAL(&mux);
    model.recordAllocation(value);
    portEXIT_CRITICAL(&mux);
}

}  // namespace Mp3MemoryDiagnostics

extern "C" void tuner_mp3_memory_milestone(std::uint8_t milestone) {
    Mp3MemoryDiagnostics::capture(milestone);
}

extern "C" void tuner_mp3_memory_allocation(std::uint8_t sequence,
                                             std::size_t requested,
                                             const void* pointer) {
    Mp3MemoryDiagnostics::recordAllocation(sequence, requested, pointer);
}

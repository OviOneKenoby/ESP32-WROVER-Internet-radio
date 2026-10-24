#pragma once

#include <cstddef>
#include <cstdint>

#include "mp3_memory_diagnostics_hooks.h"

namespace Mp3MemoryDiagnostics {

constexpr std::size_t kMilestoneCount = 6;
constexpr std::size_t kAllocationCount = 8;
constexpr std::uint32_t kInternalCaps = (1u << 11) | (1u << 2);
constexpr std::uint32_t kDmaCaps = (1u << 11) | (1u << 3);

enum class Placement : std::uint8_t {
    unavailable = 0,
    internal = 1,
    external = 2,
    other = 3,
};

struct HeapState {
    std::uint32_t free_bytes{0};
    std::uint32_t largest_block{0};
    std::uint16_t allocated_blocks{0};
    std::uint16_t free_blocks{0};
};

struct Snapshot {
    HeapState internal{};
    HeapState dma{};
    std::uint32_t psram_free{0};
    std::uint32_t allocation_failures{0};
    std::uint32_t uptime_ms{0};
    std::uint8_t source{0};
    std::uint8_t bluetooth_lifecycle{0};
    std::uint8_t milestone{0};
    bool valid{false};
};

struct Allocation {
    std::uint32_t requested{0};
    std::uint32_t allocated{0};
    std::uint8_t sequence{0};
    Placement placement{Placement::unavailable};
    bool success{false};
    bool valid{false};
};

class Model {
public:
    void begin(std::uint32_t next_session) {
        session_ = next_session;
        for (auto& item : snapshots_) item = Snapshot{};
        for (auto& item : allocations_) item = Allocation{};
    }

    void recordSnapshot(const Snapshot& value) {
        if (value.milestone >= kMilestoneCount) return;
        snapshots_[value.milestone] = value;
        snapshots_[value.milestone].valid = true;
    }

    void recordAllocation(const Allocation& value) {
        if (value.sequence == 0 || value.sequence > kAllocationCount) return;
        allocations_[value.sequence - 1] = value;
        allocations_[value.sequence - 1].valid = true;
    }

    std::uint32_t session() const { return session_; }
    const Snapshot& snapshot(std::size_t index) const { return snapshots_[index]; }
    const Allocation& allocation(std::size_t index) const { return allocations_[index]; }

private:
    Snapshot snapshots_[kMilestoneCount]{};
    Allocation allocations_[kAllocationCount]{};
    std::uint32_t session_{0};
};

static_assert(sizeof(HeapState) == 12, "MP3 heap state layout changed");
static_assert(sizeof(Snapshot) == 40, "MP3 milestone snapshot layout changed");
static_assert(sizeof(Allocation) == 12, "MP3 allocation record layout changed");
static_assert(sizeof(Model) == 340, "MP3 diagnostic storage layout changed");

void beginSession(std::uint8_t source, std::uint8_t bluetooth_lifecycle);
void setContext(std::uint8_t source, std::uint8_t bluetooth_lifecycle);
void capture(std::uint8_t milestone);
void printSnapshot();

}  // namespace Mp3MemoryDiagnostics

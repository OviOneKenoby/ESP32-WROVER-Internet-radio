#include "../src/mp3_memory_diagnostics.h"

#include <cstdlib>
#include <iostream>

namespace {
unsigned checks = 0;
void expect(bool value, const char* message) {
    ++checks;
    if (!value) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}
}

int main() {
    using namespace Mp3MemoryDiagnostics;

    expect(sizeof(HeapState) == 12, "fixed heap-state layout");
    expect(sizeof(Snapshot) == 40, "fixed phase layout");
    expect(sizeof(Allocation) == 12, "fixed allocation layout");
    expect(sizeof(Model) == 340, "bounded diagnostic storage");

    Model model;
    model.begin(7);
    expect(model.session() == 7, "session identity retained");

    Snapshot before;
    before.milestone = TUNER_MP3_MEMORY_BEFORE_CONSTRUCTION;
    before.internal.free_bytes = 26000;
    before.internal.largest_block = 9000;
    before.dma.free_bytes = 12000;
    before.dma.largest_block = 4000;
    before.valid = true;
    model.recordSnapshot(before);
    expect(model.snapshot(0).valid &&
               model.snapshot(0).internal.free_bytes == 26000 &&
               model.snapshot(0).dma.largest_block == 4000,
           "phase snapshot retained");

    Snapshot invalid;
    invalid.milestone = 99;
    model.recordSnapshot(invalid);
    expect(model.snapshot(0).internal.free_bytes == 26000,
           "invalid phase cannot overwrite storage");

    for (std::uint8_t sequence = 1; sequence <= kAllocationCount; ++sequence) {
        Allocation allocation;
        allocation.sequence = sequence;
        allocation.requested = 100u * sequence;
        allocation.allocated = allocation.requested + 8;
        allocation.success = true;
        allocation.placement = sequence & 1 ? Placement::internal
                                             : Placement::external;
        model.recordAllocation(allocation);
    }
    expect(model.allocation(0).valid &&
               model.allocation(0).placement == Placement::internal,
           "first allocation retained");
    expect(model.allocation(7).valid &&
               model.allocation(7).placement == Placement::external &&
               model.allocation(7).requested == 800,
           "eighth allocation retained");

    Allocation invalidAllocation;
    invalidAllocation.sequence = 9;
    invalidAllocation.requested = 1234;
    model.recordAllocation(invalidAllocation);
    expect(model.allocation(7).requested == 800,
           "out-of-range allocation cannot overwrite storage");

    model.begin(8);
    expect(model.session() == 8 && !model.snapshot(0).valid &&
               !model.allocation(0).valid,
           "new session clears bounded evidence");

    expect(kInternalCaps == ((1u << 11) | (1u << 2)),
           "internal capability mask exact");
    expect(kDmaCaps == ((1u << 11) | (1u << 3)),
           "DMA capability mask exact");

    std::cout << "PASS " << checks << "/" << checks
              << " MP3 memory diagnostic checks\n";
}

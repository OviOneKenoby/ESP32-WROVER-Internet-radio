#pragma once
#include <cstdint>

// Fixed storage preserves the first four failures and the most recent one.
// Caller supplies synchronization. Never stores borrowed task-name pointers.
struct AllocationFailureRecord {
    uint32_t sequence = 0, size = 0, caps = 0, tick = 0;
    uint32_t taskHandle = 0, pcs[6] = {};
    char allocator[24] = {}, task[16] = {};
    uint8_t core = 0, frameCount = 0;
    bool isr = false, cacheEnabled = false;
};

struct AllocationFailureRecords {
    AllocationFailureRecord first[4], last;
    uint32_t total = 0;
    inline __attribute__((always_inline)) void append(AllocationFailureRecord record) {
        if (total != UINT32_MAX) ++total;
        record.sequence = total;
        if (total <= 4) first[total - 1] = record;
        last = record;
    }
};

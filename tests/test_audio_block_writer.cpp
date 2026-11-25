#include "audio_block_writer.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

// An output with irregular back-pressure: takes 0..n bytes, in frame units.
struct Sink {
    std::vector<uint32_t> got;
    unsigned calls = 0;
    bool blocked = false;
    size_t operator()(const uint8_t* data, size_t bytes) {
        ++calls;
        // Refuses about a third of the calls, in no fixed rhythm.
        if (blocked || (calls * 2654435761u >> 28) < 5) return 0;
        size_t take = 4 * (1 + (calls * 37) % 90);
        if (take > bytes) take = bytes;
        for (size_t i = 0; i < take; i += 4) {
            uint32_t f;
            std::memcpy(&f, data + i, 4);
            got.push_back(f);
        }
        return take;
    }
};

int main() {
    AudioBlockWriter<128> w;
    Sink sink;
    auto write = [&](const uint8_t* d, size_t n) { return sink(d, n); };
    // Every accepted frame arrives once, in order, whatever the output takes.
    uint32_t next = 0;
    unsigned refused = 0;
    while (next < 100000) {
        if (w.push(next, write)) ++next;
        else ++refused;
    }
    while (!w.drain(write)) {}
    assert(refused > 0 && sink.got.size() == 100000);
    for (uint32_t i = 0; i < 100000; ++i) assert(sink.got[i] == i);

    // A blocked output: exactly one block is accepted, then refusal, no loss.
    Sink stuck;
    stuck.blocked = true;
    AudioBlockWriter<128> b;
    auto none = [&](const uint8_t* d, size_t n) { return stuck(d, n); };
    for (uint32_t i = 0; i < 128; ++i) assert(b.push(i, none));
    assert(!b.push(128, none) && !b.push(128, none) && b.stored() == 128);
    stuck.blocked = false;
    while (!b.push(128, none)) {}
    while (!b.drain(none)) {}
    assert(stuck.got.size() == 129);
    for (uint32_t i = 0; i < 129; ++i) assert(stuck.got[i] == i);

    // One driver call per block when the output accepts everything.
    unsigned calls = 0;
    auto all = [&](const uint8_t*, size_t n) { ++calls; return n; };
    AudioBlockWriter<128> c;
    for (uint32_t i = 0; i < 1280; ++i) assert(c.push(i, all));
    assert(calls == 10 && c.stored() == 0);
    c.push(1, all);
    c.clear();
    assert(c.stored() == 0);
    std::puts("Audio block writer: PASS");
}

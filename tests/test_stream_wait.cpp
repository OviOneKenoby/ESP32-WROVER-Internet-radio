#include "stream_wait.h"
#include <cassert>
#include <cstdio>

// Simulated socket: `rate` bytes arrive per millisecond up to a window.
struct Sim {
    uint32_t clock = 0, sleeps = 0;
    long have = 0, rate = 0, window = 5760;
};

static Sim run(size_t wanted, uint32_t timeout, long rate, long initial = 0,
               uint32_t startClock = 0) {
    Sim s;
    s.rate = rate;
    s.have = initial;
    s.clock = startClock;
    stream_wait::forData(
        wanted, timeout, [&] { return s.have; }, [&] { return s.clock; },
        [&] {
            ++s.clock;
            ++s.sleeps;
            s.have += s.rate;
            if (s.have > s.window) s.have = s.window;
        });
    return s;
}

int main() {
    static_assert(stream_wait::PARTIAL_TARGET < 5760, "must fit the window");
    // A refill larger than the window returns at the partial target.
    Sim s = run(16384, 3000, 8);
    assert(s.have >= 4096 && s.clock == 512);
    // A small read waits only for what it wants.
    s = run(100, 3000, 8);
    assert(s.clock == 13);
    // Data already there: no sleep.
    s = run(1600, 3000, 8, 5000);
    assert(s.sleeps == 0);
    // No data: bounded by the timeout, one sleep per step.
    s = run(8192, 500, 0);
    assert(s.clock == 500 && s.sleeps == 500);
    // Millisecond counter wrap.
    s = run(8192, 500, 0, 0, 0xFFFFFF00u);
    assert(static_cast<uint32_t>(s.clock - 0xFFFFFF00u) == 500);
    // An error value from available() ends at the timeout, not immediately.
    s = run(16, 20, 0, -1);
    assert(s.sleeps == 20);
    std::puts("Stream wait: PASS");
}

#include "audio_output_policy.h"
#include <cassert>
#include <cstdio>

using namespace audio_output;

int main() {
    static_assert(DMA_BUFFERS >= 2 && DMA_BUFFERS <= 128, "I2S driver limits");
    // The queue must outlast the worst measured decode (41 ms) plus the sleep,
    // and hold more than one 2048-frame AAC+ output block.
    assert(DMA_FRAMES * 1000u / 44100u >= 41 + FULL_OUTPUT_SLEEP_MS + 15);
    assert(DMA_FRAMES * 1000u / 48000u >= 41 + FULL_OUTPUT_SLEEP_MS + 10);
    assert(DMA_FRAMES > 2048);
    assert(audioTaskSleepMs(true, 0) == FULL_OUTPUT_SLEEP_MS);
    assert(audioTaskSleepMs(true, 500) == FULL_OUTPUT_SLEEP_MS);
    assert(audioTaskSleepMs(false, 0) == 0);
    assert(audioTaskSleepMs(false, 9) == 0);
    assert(audioTaskSleepMs(false, 10) == BUSY_YIELD_MS);
    std::puts("Audio output policy: PASS");
}

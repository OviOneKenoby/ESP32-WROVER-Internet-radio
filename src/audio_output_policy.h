#pragma once
// Internet-radio I2S output sizing and audio-task pacing. No Arduino
// dependency, so the decisions can be tested on the host.
#include <stdint.h>

namespace audio_output {

// ESP8266Audio fixes the DMA buffer length at 128 stereo frames.
constexpr uint32_t DMA_FRAMES_PER_BUFFER = 128;
// 24 buffers hold 69.7 ms at 44.1 kHz (the library default of 8 holds
// 23.2 ms, less than one measured AAC+ frame decode of up to 41 ms).
constexpr int DMA_BUFFERS = 24;
constexpr uint32_t DMA_FRAMES = DMA_BUFFERS * DMA_FRAMES_PER_BUFFER;

constexpr uint32_t FULL_OUTPUT_SLEEP_MS = 5;
constexpr uint32_t BUSY_YIELD_INTERVAL_MS = 10;
constexpr uint32_t BUSY_YIELD_MS = 1;

// How long the audio task sleeps after one decoder step.
constexpr uint32_t audioTaskSleepMs(bool outputFull,
                                    uint32_t sinceLastSleepMs) {
    return outputFull ? FULL_OUTPUT_SLEEP_MS
         : sinceLastSleepMs >= BUSY_YIELD_INTERVAL_MS ? BUSY_YIELD_MS
         : 0;
}

} // namespace audio_output

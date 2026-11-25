#pragma once
#include <AudioOutputI2S.h>

#include "audio_output_policy.h"

// The library output with a larger DMA queue that also remembers whether a
// sample was refused because the queue was full.
class PacedAudioOutput : public AudioOutputI2S {
public:
    PacedAudioOutput()
        : AudioOutputI2S(0, AudioOutputI2S::EXTERNAL_I2S,
                         audio_output::DMA_BUFFERS) {}

    bool ConsumeSample(int16_t sample[2]) override {
        const bool accepted = AudioOutputI2S::ConsumeSample(sample);
        if (!accepted) full_ = true;
        return accepted;
    }

    // Reads and clears the flag. Called only by the owner of the decoder.
    bool takeFull() {
        const bool full = full_;
        full_ = false;
        return full;
    }

private:
    bool full_ = false;
};

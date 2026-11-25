#pragma once
// The sample conversion in ConsumeSample() is adapted from
// AudioOutputI2S::ConsumeSample() of ESP8266Audio, Copyright (C) 2017
// Earle F. Philhower, III, GNU General Public License version 3 or (at your
// option) any later version. See THIRD_PARTY_NOTICES.md.
#include <AudioOutputI2S.h>
#include <driver/i2s.h>

#include "audio_block_writer.h"
#include "audio_output_policy.h"

// The library output with a larger DMA queue, written in blocks, that also
// remembers whether a sample was refused because the queue was full.
class PacedAudioOutput : public AudioOutputI2S {
public:
    PacedAudioOutput()
        : AudioOutputI2S(0, AudioOutputI2S::EXTERNAL_I2S,
                         audio_output::DMA_BUFFERS) {}

    // Samples left over from an earlier stream are never played.
    bool begin() override {
        block_.clear();
        full_ = false;
        return AudioOutputI2S::begin();
    }
    bool stop() override {
        block_.clear();
        return AudioOutputI2S::stop();
    }
    bool SetRate(int hz) override {
        if (i2sOn && hz != static_cast<int>(hertz)) block_.drain(writer());
        return AudioOutputI2S::SetRate(hz);
    }

    // Same sample conversion as AudioOutputI2S::ConsumeSample for the
    // external I2S output; only the driver call is batched.
    bool ConsumeSample(int16_t sample[2]) override {
        if (!i2sOn) return false;
        int16_t ms[2] = {sample[0], sample[1]};
        MakeSampleStereo16(ms);
        if (mono) {
            const int32_t sum = ms[LEFTCHANNEL] + ms[RIGHTCHANNEL];
            ms[LEFTCHANNEL] = ms[RIGHTCHANNEL] = (sum >> 1) & 0xffff;
        }
        const uint32_t frame =
            (static_cast<uint32_t>(Amplify(ms[RIGHTCHANNEL])) << 16) |
            (Amplify(ms[LEFTCHANNEL]) & 0xffff);
        const bool accepted = block_.push(frame, writer());
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
    // Non-blocking driver write used by the block writer.
    struct DriverWrite {
        i2s_port_t port;
        size_t operator()(const uint8_t* data, size_t bytes) const {
            size_t written = 0;
            i2s_write(port, data, bytes, &written, 0);
            return written;
        }
    };
    DriverWrite writer() const {
        return DriverWrite{static_cast<i2s_port_t>(portNo)};
    }

    AudioBlockWriter<audio_output::WRITE_BLOCK_FRAMES> block_;
    bool full_ = false;
};

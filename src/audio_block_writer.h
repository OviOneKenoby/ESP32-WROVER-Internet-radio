#pragma once
// Collects stereo frames and hands them to the output in blocks. No Arduino
// dependency, so ordering and back-pressure can be tested on the host.
//
// Write is callable as size_t(const uint8_t* data, size_t bytes) and returns
// how many bytes it accepted without blocking.
#include <stddef.h>
#include <stdint.h>

template <unsigned FRAMES>
class AudioBlockWriter {
public:
    // Returns false, without taking the frame, while a full block cannot be
    // handed on. The caller offers the same frame again later.
    template <class Write>
    bool push(uint32_t frame, Write write) {
        if (count_ == FRAMES && !drain(write)) return false;
        block_[count_++] = frame;
        if (count_ == FRAMES) drain(write);
        return true;
    }

    // Hands on what is stored; false if the output did not take all of it.
    template <class Write>
    bool drain(Write write) {
        const size_t total = count_ * sizeof(block_[0]);
        while (sent_ < total) {
            const size_t taken = write(
                reinterpret_cast<const uint8_t*>(block_) + sent_,
                total - sent_);
            if (!taken) return false;
            sent_ += taken;
        }
        clear();
        return true;
    }

    void clear() {
        count_ = 0;
        sent_ = 0;
    }
    unsigned stored() const { return count_; }

private:
    uint32_t block_[FRAMES];
    unsigned count_ = 0;
    size_t sent_ = 0;
};

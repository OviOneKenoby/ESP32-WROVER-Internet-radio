#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum TunerMp3MemoryMilestone {
    TUNER_MP3_MEMORY_BEFORE_CONSTRUCTION = 0,
    TUNER_MP3_MEMORY_AFTER_GENERATOR_ALLOCATION = 1,
    TUNER_MP3_MEMORY_AFTER_HELIX_INITIALIZATION = 2,
    TUNER_MP3_MEMORY_AFTER_I2S_INITIALIZATION = 3,
    TUNER_MP3_MEMORY_STABLE_PLAYBACK = 4,
    TUNER_MP3_MEMORY_AFTER_TEARDOWN = 5,
};

void tuner_mp3_memory_milestone(uint8_t milestone);
void tuner_mp3_memory_allocation(uint8_t sequence, size_t requested,
                                 const void* pointer);

#ifdef __cplusplus
}
#endif

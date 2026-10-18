#ifndef VOLUME_TARGET_H
#define VOLUME_TARGET_H

#include <stdint.h>

namespace VolumeTarget {
inline uint8_t afterDetents(uint8_t current, int32_t detents) {
    int64_t target = static_cast<int64_t>(current) + static_cast<int64_t>(detents) * 5;
    if (target < 0) target = 0;
    if (target > 100) target = 100;
    return static_cast<uint8_t>(target);
}
}
#endif

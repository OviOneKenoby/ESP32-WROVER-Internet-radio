#ifndef CORRECTNESS_GUARDS_H
#define CORRECTNESS_GUARDS_H

#include <stddef.h>
#include <stdint.h>

namespace CorrectnessGuards {

inline bool parseListIndex(const char* text, size_t length, size_t itemCount,
                           uint8_t& index) {
    if (!text || length == 0 || length > 3) return false;

    uint16_t value = 0;
    for (size_t i = 0; i < length; ++i) {
        const char c = text[i];
        if (c < '0' || c > '9') return false;

        const uint8_t digit = (uint8_t)(c - '0');
        if (value > (uint16_t)((UINT8_MAX - digit) / 10U)) return false;
        value = (uint16_t)(value * 10U + digit);
    }

    if ((size_t)value >= itemCount) return false;
    index = (uint8_t)value;
    return true;
}

inline bool nextStationIndex(uint8_t current, uint8_t count, uint8_t& next) {
    if (count == 0 || current >= count) return false;
    next = (uint8_t)((current + 1U) % count);
    return true;
}

inline bool previousStationIndex(uint8_t current, uint8_t count,
                                 uint8_t& previous) {
    if (count == 0 || current >= count) return false;
    previous = current == 0 ? (uint8_t)(count - 1U) : (uint8_t)(current - 1U);
    return true;
}

inline uint8_t volumeDown(uint8_t current, uint8_t step = 5) {
    return current > step ? (uint8_t)(current - step) : 0;
}

} // namespace CorrectnessGuards

#endif // CORRECTNESS_GUARDS_H

#pragma once
// A station URL may only be copied into a fixed buffer when the whole string
// AND its terminating NUL fit. A longer URL must be rejected: truncating it
// (strlcpy/strncpy) silently turns it into a DIFFERENT address that would then
// be opened. Pure and header-only so the host tests can call it.
#include <cstddef>
#include <cstring>

inline bool urlFitsBuffer(size_t length, size_t capacity) {
    return capacity > 0 && length < capacity;
}

inline bool urlFitsBuffer(const char* url, size_t capacity) {
    return url && urlFitsBuffer(std::strlen(url), capacity);
}

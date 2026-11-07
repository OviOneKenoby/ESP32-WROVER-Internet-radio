#pragma once
// Text rules for station records that come from outside the firmware (web
// portal, Browse results, stored lists).
//
//  - Names are cleaned with browse_text::sanitize (the same rule as Browse).
//  - URLs are never cleaned: a URL that contains a control character, a space or
//    a byte >= 0x7F is rejected, because "fixing" an address would open a
//    different one.
// Pure and header-only so the host tests can call it.
#include "browse_text.h"
#include <cstddef>
#include <cstdint>

namespace station_text {

constexpr const char* kNamePlaceholder = "(no name)";

// True when every byte of url[0..length) is printable ASCII (0x21..0x7E).
// A NUL inside the counted range is rejected.
inline bool urlAcceptable(const char* url, size_t length) {
    if (!url || !length) return false;
    for (size_t i = 0; i < length; ++i) {
        const uint8_t c = static_cast<uint8_t>(url[i]);
        if (c < 0x21 || c > 0x7e) return false;
    }
    return true;
}

inline bool urlAcceptable(const char* url) {
    return url && urlAcceptable(url, std::strlen(url));
}

// Copies a station name into a fixed record field (capacity includes the NUL).
inline void copyName(char* out, size_t capacity, const char* name, size_t length) {
    browse_text::sanitize(name, length, out, capacity, kNamePlaceholder);
}

inline void copyName(char* out, size_t capacity, const char* name) {
    copyName(out, capacity, name, name ? std::strlen(name) : 0);
}

// Cleans a name that is already stored in a record field, in memory only.
template <size_t N>
inline void cleanStoredName(char (&field)[N]) {
    char copy[N];
    field[N - 1] = '\0';
    std::memcpy(copy, field, N);
    copyName(field, N, copy);
}

}  // namespace station_text

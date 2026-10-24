#ifndef TLS_RECORD_BUFFER_PSRAM_H
#define TLS_RECORD_BUFFER_PSRAM_H

#include <cstddef>
#include <cstdint>
#include <limits>

namespace TlsRecordBufferPsram {

// Resolved from the pinned Arduino-ESP32 2.0.17 / ESP-IDF 4.4.7 mbedTLS
// configuration. The target translation unit statically verifies this value
// against both mbedTLS internal record-buffer macros.
constexpr std::size_t kResolvedRecordBufferBytes = 16717;

inline bool requestSize(std::size_t count, std::size_t size,
                        std::size_t& bytes) {
    if (count != 0 && size > std::numeric_limits<std::size_t>::max() / count) {
        return false;
    }
    bytes = count * size;
    return true;
}

inline bool isRecordBufferRequest(std::size_t count, std::size_t size) {
    std::size_t bytes = 0;
    return requestSize(count, size, bytes) &&
           bytes == kResolvedRecordBufferBytes;
}

}  // namespace TlsRecordBufferPsram

#endif  // TLS_RECORD_BUFFER_PSRAM_H

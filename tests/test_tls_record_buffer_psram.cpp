#include "../src/tls_record_buffer_psram.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
unsigned checks = 0;

void expect(bool value, const char* message) {
    ++checks;
    if (!value) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}
}  // namespace

int main() {
    using namespace TlsRecordBufferPsram;

    expect(kResolvedRecordBufferBytes == 16717,
           "resolved pinned record-buffer size");
    expect(isRecordBufferRequest(1, 16717),
           "mbedTLS one-element record buffer selected");
    expect(isRecordBufferRequest(16717, 1),
           "equivalent calloc product selected");
    expect(!isRecordBufferRequest(1, 16384),
           "TLS content length alone is not selected");
    expect(!isRecordBufferRequest(1, 16716),
           "neighboring smaller allocation remains internal");
    expect(!isRecordBufferRequest(1, 16718),
           "neighboring larger allocation remains internal");
    expect(!isRecordBufferRequest(0, 16717),
           "zero-count allocation remains unchanged");
    expect(!isRecordBufferRequest(
               std::numeric_limits<std::size_t>::max(), 2),
           "overflow cannot alias the record-buffer size");

    std::size_t bytes = 0;
    expect(requestSize(1, 16717, bytes) && bytes == 16717,
           "checked size calculation exact");
    expect(!requestSize(std::numeric_limits<std::size_t>::max(), 2, bytes),
           "checked size calculation rejects overflow");

    std::cout << "PASS " << checks << "/" << checks
              << " TLS record-buffer allocation policy checks\n";
}

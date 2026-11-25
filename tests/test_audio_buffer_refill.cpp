// Exercises the library's AudioFileSourceBuffer.cpp itself. Copy
// AudioFileSourceBuffer.{h,cpp} from the ESP8266Audio library into an empty
// folder and compile them with this file:
//   g++ -std=c++17 -Itests/stubs/audio_buffer -I<folder> \
//       tests/test_audio_buffer_refill.cpp <folder>/AudioFileSourceBuffer.cpp
// The unpatched library fails the sequence check; the patched one passes.
#include "AudioFileSourceBuffer.h"
#include <cstdio>
#include <cstdlib>

// Numbered byte stream delivered in irregular, sometimes empty, pieces.
class Source : public AudioFileSource {
public:
    uint32_t next = 0, calls = 0;
    uint32_t read(void* data, uint32_t len) override {
        return give(data, len, 1 + (calls * 37) % 900);
    }
    uint32_t readNonBlock(void* data, uint32_t len) override {
        const uint32_t n = (calls % 3 == 0) ? 0 : (calls * 53) % 400;
        return give(data, len, n);
    }
private:
    uint32_t give(void* data, uint32_t len, uint32_t n) {
        ++calls;
        if (n > len) n = len;
        for (uint32_t i = 0; i < n; ++i)
            static_cast<uint8_t*>(data)[i] = static_cast<uint8_t>(next++ % 251);
        return n;
    }
};

static uint32_t underflows = 0;
static void status(void*, int code, const char*) {
    if (code == AudioFileSourceBuffer::STATUS_UNDERFLOW) ++underflows;
}

int main() {
    Source source;
    static uint8_t ring[4096];
    AudioFileSourceBuffer buffer(&source, ring, sizeof(ring));
    buffer.RegisterStatusCB(status, nullptr);
    uint8_t out[1600];
    uint32_t expected = 0;
    for (uint32_t step = 0; step < 20000; ++step) {
        const uint32_t want = 1 + (step * 131) % sizeof(out);
        const uint32_t got = buffer.read(out, want);
        for (uint32_t i = 0; i < got; ++i, ++expected) {
            if (out[i] != static_cast<uint8_t>(expected % 251)) {
                std::printf("Audio buffer refill: FAIL at byte %u after %u underflows\n",
                            expected, underflows);
                return 1;
            }
        }
        if (step % 5 == 0) buffer.loop();
    }
    if (!underflows) { std::puts("no underflow exercised"); return 2; }
    std::printf("Audio buffer refill: PASS (%u bytes, %u underflows)\n",
                expected, underflows);
}

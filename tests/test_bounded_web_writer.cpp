#include "bounded_web_writer.h"
#include <ArduinoJson.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

static int checks = 0;
static void expect(bool ok, const char* message) {
    ++checks;
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct Transport {
    uint32_t clock = 0, perSendMs = 0;
    size_t limit = 100000;
    int failure = 0;
    bool retry = false;
    std::string bytes;
    uint32_t now() { return clock; }
    void pause() { ++clock; }
    int send(const uint8_t* data, size_t size) {
        clock += perSendMs;
        if (failure) return failure;
        const size_t count = std::min(limit, size);
        bytes.append(reinterpret_cast<const char*>(data), count);
        return int(count);
    }
    bool retryable() { return retry; }
    int error() { return 12; }
};

int main() {
    const std::string payload(2001, 'x');
    const auto* data = reinterpret_cast<const uint8_t*>(payload.data());
    Transport transport;
    BoundedWebWriter<Transport> writer(transport);
    transport.limit = 13;
    expect(writer.write(data, payload.size()) == payload.size(), "partial sends complete");
    expect(transport.bytes == payload && writer.sent == payload.size(), "partial bytes exact");
    expect(writer.calls > 1 && !writer.failed(), "partial sends counted");

    writer.reset(); transport.bytes.clear(); transport.failure = -1; transport.retry = true;
    expect(writer.write(data, 1) == 0, "backpressure writes zero");
    expect(writer.failed() && writer.error == -1, "deadline is recorded");
    expect(writer.waits == 2000 && transport.clock == 2000, "bounded retries");
    const auto calls = writer.calls;
    expect(writer.write(data, 1) == 0 && writer.calls == calls, "no writes after failure");

    writer.reset(); transport.failure = 0; transport.retry = false;
    transport.limit = 1; transport.clock = 0; transport.perSendMs = 1;
    expect(writer.write(data, payload.size()) == 2000, "progress does not renew deadline");
    expect(writer.failed() && writer.sent == 2000, "partial timeout counted");

    writer.reset(); transport.bytes.clear(); transport.limit = 100000;
    transport.perSendMs = 0; transport.clock = 100;
    expect(writer.write(data, 100) == 100, "header succeeds");
    transport.clock += 2000;
    expect(writer.write(data, 100) == 0 && writer.failed(), "header and body share deadline");

    writer.reset(); transport.clock = UINT32_MAX - 999; transport.failure = -1; transport.retry = true;
    expect(writer.write(data, 1) == 0 && writer.waits == 2000, "deadline survives millis wrap");
    writer.reset(); transport.failure = -1; transport.retry = false;
    expect(writer.write(data, 1) == 0 && writer.error == 12 && writer.calls == 1, "terminal errno fails immediately");
    writer.reset(); transport.failure = 0; transport.limit = 0;
    expect(writer.write(data, 1) == 0 && writer.error == -2, "zero send fails immediately");

    writer.reset(); transport.bytes.clear(); transport.limit = 100000;
    BufferedWebWriter<BoundedWebWriter<Transport>> buffered(writer);
    for (uint8_t value : payload) expect(buffered.write(value) == 1, "byte accepted");
    expect(buffered.flush(), "final buffer flush");
    expect(transport.bytes == payload, "buffered bytes exact");
    expect(writer.calls == 4, "byte writes batch into four socket calls");

    writer.reset(); transport.bytes.clear();
    JsonDocument doc;
    doc["station"] = "Ține ritmul — SBC";
    doc["long"] = std::string(2200, 'z');
    BufferedWebWriter<BoundedWebWriter<Transport>> json(writer);
    const size_t accepted = serializeJson(doc, json);
    expect(json.flush(), "ArduinoJson final flush");
    std::string reference;
    serializeJson(doc, reference);
    expect(transport.bytes == reference, "ArduinoJson exact UTF-8 wire format");
    expect(writer.sent == measureJson(doc) && accepted == writer.sent, "Content-Length exact");
    expect(writer.calls == (reference.size() + 511) / 512, "ArduinoJson batched writes");

    writer.reset(); transport.failure = -1; transport.retry = false;
    BufferedWebWriter<BoundedWebWriter<Transport>> broken(writer);
    expect(broken.write(data, payload.size()) == 512, "stop serialization after failed buffer");
    expect(!broken.flush() && broken.write(data, 1) == 0 && writer.sent == 0, "accepted bytes never masquerade as sent bytes");
    std::cout << "PASS: " << checks << " bounded web writer checks\n";
}

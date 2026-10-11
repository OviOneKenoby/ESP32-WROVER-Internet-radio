#pragma once
#include <WebServer.h>
#include <lwip/sockets.h>
#include <cerrno>
#include "bounded_web_writer.h"
#include "system_info.h"
#include <esp_heap_caps.h>

struct WebResponseDiagnostics {
    const char* route = "none";
    uint32_t responses = 0, buildMs = 0, headerMs = 0, bodyMs = 0;
    size_t expectedBody = 0, sentBody = 0, sentHeader = 0;
    uint32_t writeCalls = 0, retryWaits = 0;
    int socketError = 0;
    bool failed = false, jsonOverflow = false;
    size_t freeStart = 0, largestStart = 0, freeBuilt = 0, largestBuilt = 0;
    size_t freeEnd = 0, largestEnd = 0;
    uint32_t allocStart = 0, allocBuilt = 0, allocEnd = 0;
};

class ObservedWebServer : public WebServer {
    struct Transport {
        ObservedWebServer& owner;
        explicit Transport(ObservedWebServer& server) : owner(server) {}
        int lastError = 0;
        uint32_t now() { return millis(); }
        void pause() { delay(1); }
        int send(const uint8_t* data, size_t size) {
            int result = ::send(owner.clientFd(), data, size, MSG_DONTWAIT);
            lastError = result < 0 ? errno : 0;
            return result;
        }
        bool retryable() { return lastError == EAGAIN || lastError == EWOULDBLOCK || lastError == EINTR; }
        int error() { return lastError; }
    };
public:
    explicit ObservedWebServer(int port) : WebServer(port), transport_{*this}, writer_(transport_) {}
    void beginResponse(const char* route) {
        const uint32_t count = diagnostics.responses + 1;
        diagnostics = WebResponseDiagnostics{};
        diagnostics.route = route;
        diagnostics.responses = count;
        diagnostics.freeStart = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        diagnostics.largestStart = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        diagnostics.allocStart = SystemInfo::allocationFailures().count;
        writer_.reset(); firstWrite_ = true; startedMs_ = millis();
    }
    void finishResponse() {
        diagnostics.freeEnd = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        diagnostics.largestEnd = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        diagnostics.allocEnd = SystemInfo::allocationFailures().count;
    }
    size_t write(const uint8_t* data, size_t size) { return writeObserved(data, size, false); }
    WebResponseDiagnostics diagnostics;
protected:
    size_t _currentClientWrite(const char* data, size_t size) override {
        const bool header = firstWrite_;
        return writeObserved(reinterpret_cast<const uint8_t*>(data), size, header);
    }
    size_t _currentClientWrite_P(PGM_P data, size_t size) override {
        // Classic ESP32 flash is memory mapped; WiFiClient::write_P also calls write.
        return writeObserved(reinterpret_cast<const uint8_t*>(data), size, false);
    }
private:
    int clientFd() { return _currentClient.fd(); }
    size_t writeObserved(const uint8_t* data, size_t size, bool header) {
        if (firstWrite_) {
            diagnostics.buildMs = millis() - startedMs_; firstWrite_ = false;
            diagnostics.freeBuilt = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
            diagnostics.largestBuilt = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
            diagnostics.allocBuilt = SystemInfo::allocationFailures().count;
        }
        const uint32_t start = millis();
        const size_t sent = writer_.write(data, size);
        if (header) { diagnostics.sentHeader += sent; diagnostics.headerMs += millis() - start; }
        else { diagnostics.sentBody += sent; diagnostics.bodyMs += millis() - start; }
        diagnostics.writeCalls = writer_.calls;
        diagnostics.retryWaits = writer_.waits;
        diagnostics.socketError = writer_.error;
        diagnostics.failed = writer_.failed();
        if (sent != size) _currentClient.stop();
        return sent;
    }
    Transport transport_;
    BoundedWebWriter<Transport> writer_;
    bool firstWrite_ = true;
    uint32_t startedMs_ = 0;
};

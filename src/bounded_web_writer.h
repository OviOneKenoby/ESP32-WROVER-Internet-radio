#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

// Transport must supply now(), pause(), send(data,size), retryable(), error().
// A single deadline covers every write, including headers. Never renew it on
// progress. Used only by the application task that owns WebServer's client.
template<class Transport>
class BoundedWebWriter {
public:
    explicit BoundedWebWriter(Transport& transport) : transport_(transport) {}
    void reset() {
        started_ = failed_ = false;
        sent = calls = waits = 0;
        error = 0;
    }
    size_t write(const uint8_t* data, size_t size) {
        if (failed_ || !size) return 0;
        if (!started_) { started_ = true; start_ = transport_.now(); }
        size_t done = 0;
        while (done < size) {
            if (uint32_t(transport_.now() - start_) >= budgetMs) {
                failed_ = true; error = timeoutError; break;
            }
            ++calls;
            const int result = transport_.send(data + done, size - done);
            if (result > 0) { done += size_t(result); sent += size_t(result); }
            else if (result < 0 && transport_.retryable()) {
                ++waits; transport_.pause();
            } else {
                failed_ = true; error = result < 0 ? transport_.error() : closedError;
                break;
            }
        }
        return done;
    }
    bool failed() const { return failed_; }
    static constexpr uint32_t budgetMs = 2000;
    static constexpr int timeoutError = -1;
    static constexpr int closedError = -2;
    size_t sent = 0;
    uint32_t calls = 0, waits = 0;
    int error = 0;
private:
    Transport& transport_;
    bool started_ = false, failed_ = false;
    uint32_t start_ = 0;
};

// ArduinoJson's byte-oriented serializer must not create a TCP send per byte.
// Fixed storage; serialization accepted bytes are not socket-delivered bytes.
template<class Sink>
class BufferedWebWriter {
public:
    explicit BufferedWebWriter(Sink& sink) : sink_(sink) {}
    size_t write(uint8_t value) { return write(&value, 1); }
    size_t write(const uint8_t* data, size_t size) {
        size_t accepted = 0;
        while (accepted < size && !failed_) {
            size_t count = size - accepted;
            if (count > sizeof(buffer_) - used_) count = sizeof(buffer_) - used_;
            memcpy(buffer_ + used_, data + accepted, count);
            used_ += count; accepted += count;
            if (used_ == sizeof(buffer_) && !flush()) break;
        }
        return accepted;
    }
    bool flush() {
        if (failed_) return false;
        if (used_ && sink_.write(buffer_, used_) != used_) failed_ = true;
        used_ = 0;
        return !failed_;
    }
private:
    Sink& sink_;
    uint8_t buffer_[512];
    size_t used_ = 0;
    bool failed_ = false;
};

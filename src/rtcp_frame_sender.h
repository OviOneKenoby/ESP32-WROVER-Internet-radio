#pragma once

#include <cerrno>
#include <cstddef>
#include <cstdint>

namespace rtcp {

enum class FrameSendStatus : uint8_t {
    complete,
    deadline,
    peer_closed,
    transport_error,
};

struct FrameSendResult {
    FrameSendStatus status{FrameSendStatus::transport_error};
    size_t written{0};
    int error{0};

    bool complete() const { return status == FrameSendStatus::complete; }
};

inline bool frameSendRetryable(int error) {
    return error == EAGAIN || error == EWOULDBLOCK || error == EINTR;
}

inline bool frameSendDeadlineReached(uint32_t now, uint32_t deadline) {
    return int32_t(now - deadline) >= 0;
}

// Transport supplies nowMs(), waitWritable(timeout_ms), send(data, size), and
// error(). The one absolute deadline is never renewed after partial progress.
// A successfully written prefix is advanced exactly once and is never replayed.
template <typename Transport>
FrameSendResult sendFrameBounded(Transport& transport, const uint8_t* frame,
                                 size_t size, uint32_t deadline_ms) {
    FrameSendResult result{};
    if (!frame || !size) {
        result.error = EINVAL;
        return result;
    }

    while (result.written < size) {
        const uint32_t now = transport.nowMs();
        if (frameSendDeadlineReached(now, deadline_ms)) {
            result.status = FrameSendStatus::deadline;
            result.error = ETIMEDOUT;
            return result;
        }

        const uint32_t remaining = deadline_ms - now;
        const int ready = transport.waitWritable(remaining);
        if (ready < 0) {
            const int error = transport.error();
            if (frameSendRetryable(error)) continue;
            result.status = FrameSendStatus::transport_error;
            result.error = error;
            return result;
        }
        if (!ready) continue;

        const int sent = transport.send(frame + result.written,
                                        size - result.written);
        if (sent > 0) {
            const size_t progress = static_cast<size_t>(sent);
            if (progress > size - result.written) {
                result.status = FrameSendStatus::transport_error;
                result.error = EIO;
                return result;
            }
            result.written += progress;
            continue;
        }
        if (!sent) {
            result.status = FrameSendStatus::peer_closed;
            return result;
        }

        const int error = transport.error();
        if (frameSendRetryable(error)) continue;
        result.status = FrameSendStatus::transport_error;
        result.error = error;
        return result;
    }

    result.status = FrameSendStatus::complete;
    return result;
}

}  // namespace rtcp

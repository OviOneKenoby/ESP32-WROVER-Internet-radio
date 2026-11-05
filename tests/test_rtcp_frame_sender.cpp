#include "../src/rtcp_frame_sender.h"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace {

struct Step {
    int ready{1};
    int sent{0};
    int error{0};
    uint32_t elapsed_ms{0};
};

class FakeTransport {
public:
    explicit FakeTransport(std::vector<Step> steps) : steps_(std::move(steps)) {}

    uint32_t nowMs() const { return now_; }
    int error() const { return error_; }

    int waitWritable(uint32_t timeout_ms) {
        if (at_ >= steps_.size()) {
            now_ += timeout_ms;
            return 0;
        }
        const Step& step = steps_[at_];
        now_ += std::min(step.elapsed_ms, timeout_ms);
        error_ = step.ready < 0 ? step.error : 0;
        if (step.ready <= 0) ++at_;
        return step.ready;
    }

    int send(const uint8_t* data, size_t size) {
        assert(at_ < steps_.size());
        const Step step = steps_[at_++];
        error_ = step.sent < 0 ? step.error : 0;
        if (step.sent > 0) {
            const size_t count = std::min<size_t>(size, size_t(step.sent));
            bytes_.insert(bytes_.end(), data, data + count);
            offsets_.push_back(size_t(data - frame_));
            return int(count);
        }
        return step.sent;
    }

    void frame(const uint8_t* value) { frame_ = value; }
    const std::vector<uint8_t>& bytes() const { return bytes_; }
    const std::vector<size_t>& offsets() const { return offsets_; }

private:
    std::vector<Step> steps_;
    size_t at_{0};
    uint32_t now_{0};
    int error_{0};
    const uint8_t* frame_{nullptr};
    std::vector<uint8_t> bytes_;
    std::vector<size_t> offsets_;
};

rtcp::FrameSendResult run(FakeTransport& transport,
                          const std::vector<uint8_t>& frame,
                          uint32_t deadline = 500) {
    transport.frame(frame.data());
    return rtcp::sendFrameBounded(transport, frame.data(), frame.size(), deadline);
}

}  // namespace

int main() {
    const std::vector<uint8_t> frame{0, 1, 2, 3, 4, 5, 6, 7};

    FakeTransport complete({{1, 8, 0, 1}});
    auto result = run(complete, frame);
    assert(result.complete() && result.written == frame.size());
    assert(complete.bytes() == frame);

    FakeTransport partial({{1, 3, 0, 1}, {1, 2, 0, 1}, {1, 3, 0, 1}});
    result = run(partial, frame);
    assert(result.complete() && partial.bytes() == frame);
    assert((partial.offsets() == std::vector<size_t>{0, 3, 5}));

    FakeTransport retryable({{-1, 0, EINTR, 5},
                             {1, -1, EAGAIN, 5},
                             {1, -1, EWOULDBLOCK, 5},
                             {1, -1, EINTR, 5},
                             {1, 8, 0, 1}});
    result = run(retryable, frame);
    assert(result.complete() && retryable.bytes() == frame);

    FakeTransport no_progress({{0, 0, 0, 200}, {0, 0, 0, 200},
                               {0, 0, 0, 100}});
    result = run(no_progress, frame);
    assert(result.status == rtcp::FrameSendStatus::deadline &&
           result.error == ETIMEDOUT && result.written == 0);

    FakeTransport partial_deadline({{1, 3, 0, 1}, {0, 0, 0, 499}});
    result = run(partial_deadline, frame);
    assert(result.status == rtcp::FrameSendStatus::deadline &&
           result.written == 3 && partial_deadline.bytes().size() == 3);

    FakeTransport terminal({{1, -1, EIO, 1}});
    result = run(terminal, frame);
    assert(result.status == rtcp::FrameSendStatus::transport_error &&
           result.error == EIO && result.written == 0);

    FakeTransport peer_closed({{1, 0, 0, 1}});
    result = run(peer_closed, frame);
    assert(result.status == rtcp::FrameSendStatus::peer_closed &&
           result.written == 0);

    FakeTransport partial_peer_closed({{1, 4, 0, 1}, {1, 0, 0, 1}});
    result = run(partial_peer_closed, frame);
    assert(result.status == rtcp::FrameSendStatus::peer_closed &&
           result.written == 4 && partial_peer_closed.bytes().size() == 4);

    std::puts("RTCP bounded frame sender: PASS");
}

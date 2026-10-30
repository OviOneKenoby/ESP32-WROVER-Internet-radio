#pragma once

#include <cstdint>

namespace remote_browser_transport {

constexpr uint32_t kTransportTimeoutMs = 8000;
constexpr uint32_t kJobDeadlineMs = 40000;
constexpr uint32_t kRetryDelayMs = 2000;
constexpr uint8_t kMaximumAttempts = 4;
constexpr uint8_t kMaxRedirects = 3;

enum class FailureAction : uint8_t {
    stop,
    next_mirror,
    delayed_canonical_retry,
};

constexpr bool isTransientHttpStatus(int status) {
    return status == 429 || status == 500 || status == 502 ||
           status == 503 || status == 504;
}

constexpr FailureAction failureAction(bool transport_failed, int http_status,
                                      uint8_t mirror, uint8_t mirror_count,
                                      bool delayed_retry_used) {
    if (!transport_failed && !isTransientHttpStatus(http_status))
        return FailureAction::stop;
    if (delayed_retry_used)
        return FailureAction::stop;
    if (uint8_t(mirror + 1) < mirror_count)
        return FailureAction::next_mirror;
    return FailureAction::delayed_canonical_retry;
}

static_assert(3 * kTransportTimeoutMs + kRetryDelayMs + kTransportTimeoutMs <=
              kJobDeadlineMs, "browser retry policy exceeds its job deadline");

}  // namespace remote_browser_transport

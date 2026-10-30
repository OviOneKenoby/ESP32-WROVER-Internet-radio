#include "remote_browser_transport_policy.h"

#include <cassert>
#include <cstdio>

int main() {
    using namespace remote_browser_transport;

    static_assert(kMaximumAttempts == 4);
    static_assert(3 * kTransportTimeoutMs + kRetryDelayMs +
                  kTransportTimeoutMs <= kJobDeadlineMs);

    assert(isTransientHttpStatus(429));
    assert(isTransientHttpStatus(500));
    assert(isTransientHttpStatus(502));
    assert(isTransientHttpStatus(503));
    assert(isTransientHttpStatus(504));
    assert(!isTransientHttpStatus(400));
    assert(!isTransientHttpStatus(404));

    assert(failureAction(true, 0, 0, 3, false) ==
           FailureAction::next_mirror);
    assert(failureAction(false, 503, 1, 3, false) ==
           FailureAction::next_mirror);
    assert(failureAction(false, 503, 2, 3, false) ==
           FailureAction::delayed_canonical_retry);
    assert(failureAction(true, 0, 0, 3, true) == FailureAction::stop);
    assert(failureAction(false, 503, 0, 3, true) == FailureAction::stop);
    assert(failureAction(false, 404, 0, 3, false) == FailureAction::stop);
    assert(failureAction(false, 200, 0, 3, false) == FailureAction::stop);

    std::puts("REMOTE browser reliability policy: PASS");
}

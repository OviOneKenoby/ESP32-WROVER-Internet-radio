#pragma once
// Bounded wait for data in the blocking read of the network audio sources.
// No Arduino dependency, so it can be tested on the host.
#include <stddef.h>
#include <stdint.h>

namespace stream_wait {

// Enough to continue with. It must stay below what one socket can hold
// (TCP window 5760 bytes), otherwise the target can never be reached.
constexpr size_t PARTIAL_TARGET = 4096;

// Sleeps until min(wanted, PARTIAL_TARGET) bytes are available or the timeout
// passes. The caller reads whatever is available afterwards.
template <class Available, class Now, class Sleep>
void forData(size_t wanted, uint32_t timeoutMs, Available available, Now now,
             Sleep sleep) {
    const long target = static_cast<long>(
        wanted < PARTIAL_TARGET ? wanted : PARTIAL_TARGET);
    const uint32_t started = now();
    while (static_cast<long>(available()) < target &&
           static_cast<uint32_t>(now() - started) < timeoutMs) {
        sleep();
    }
}

} // namespace stream_wait

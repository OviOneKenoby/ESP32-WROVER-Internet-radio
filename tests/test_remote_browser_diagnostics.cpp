#include "remote_browser_diagnostics.h"

#include <cassert>
#include <cstdio>

int main() {
    RemoteBrowserDiagnostics value{};
    value.start_requests = UINT32_MAX - 1;
    RemoteBrowserDiagnostics::increment(value.start_requests);
    RemoteBrowserDiagnostics::increment(value.start_requests);
    assert(value.start_requests == UINT32_MAX);
    assert(sizeof(RemoteBrowserDiagnostics) == 56);
    assert(uint8_t(RemoteBrowserCancelReason::rtcp_closed) == 3);
    std::puts("REMOTE browser diagnostics: PASS");
}

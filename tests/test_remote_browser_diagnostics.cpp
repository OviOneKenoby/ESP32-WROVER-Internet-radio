#include "remote_browser_diagnostics.h"

#include <cassert>
#include <cstdio>

int main() {
    RemoteBrowserDiagnostics value{};
    value.start_requests = UINT32_MAX - 1;
    RemoteBrowserDiagnostics::increment(value.start_requests);
    RemoteBrowserDiagnostics::increment(value.start_requests);
    assert(value.start_requests == UINT32_MAX);
    assert(sizeof(RemoteBrowserDiagnostics) == 112);
    assert(uint8_t(RemoteBrowserCancelReason::rtcp_closed) == 3);

    // Successful START followed by a page response.
    value.start_requests = 1;
    value.start_results = 1;
    value.page_queries = 1;
    value.pages_produced = 1;
    value.jobs_completed = 1;
    value.generation = 7;
    value.revision = 4;
    value.active = 1;
    value.job = 0;
    value.status = 2;
    value.error = 0;
    assert(value.start_requests == value.start_results);
    assert(value.page_queries == value.pages_produced);
    assert(value.jobs_completed == 1 && value.jobs_failed == 0);

    // Loading, empty completion, HTTPS failure, timeout and cancellation are
    // retained as bounded state/counters without packet or credential data.
    value.job = 1;
    value.status = 1;
    assert(value.job == 1 && value.status == 1);
    value.job = 0;
    value.status = 2;
    value.error = 0;
    assert(value.pages_produced == 1 && value.error == 0);
    RemoteBrowserDiagnostics::increment(value.jobs_failed);
    value.status = 3;
    value.error = 7;
    assert(value.jobs_failed == 1 && value.status == 3 && value.error == 7);
    RemoteBrowserDiagnostics::increment(value.jobs_failed);
    value.error = 8;
    assert(value.jobs_failed == 2 && value.error == 8);
    RemoteBrowserDiagnostics::increment(value.cancellations);
    value.cancel_reason = RemoteBrowserCancelReason::rtcp_closed;
    value.active = 0;
    value.job = 0;
    assert(value.cancellations == 1 &&
           value.cancel_reason == RemoteBrowserCancelReason::rtcp_closed);
    assert(value.active == 0 && value.job == 0);
    value.mirror = 1;
    value.transport_stage = RemoteBrowserTransportStage::connected;
    value.transport_error = -1;
    value.http_status = 503;
    value.elapsed_ms = 30000;
    value.deadline_expired = 1;
    value.first_failure_stage = RemoteBrowserTransportStage::connecting;
    value.first_failure_mirror = 0;
    value.first_transport_error = -2;
    assert(value.mirror == 1 && value.http_status == 503 &&
           value.transport_stage == RemoteBrowserTransportStage::connected);
    assert(value.transport_error == -1 && value.elapsed_ms == 30000 &&
           value.deadline_expired == 1);
    assert(value.first_failure_stage == RemoteBrowserTransportStage::connecting &&
           value.first_failure_mirror == 0 && value.first_transport_error == -2);
    value.recordTls(104, -0x7280, 0x1234, 0x8001, 15);
    value.recordTls(110, -0x6800, 0, 0x8002, 11);
    assert(value.tls_system_error == 110 && value.tls_mbedtls_error == -0x6800 &&
           value.first_tls_system_error == 104 &&
           value.first_tls_mbedtls_error == -0x7280 &&
           value.first_tls_cert_flags == 0x1234 &&
           value.first_tls_available == 15);
    std::puts("REMOTE browser diagnostics: PASS");
}

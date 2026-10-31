#include "../src/rtcp_diagnostics.h"
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
unsigned checks = 0;
void expect(bool value, const char* message) {
    ++checks;
    if (!value) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
}

int main() {
    using namespace rtcp;
    Diagnostics d;
    expect(sizeof d == 296, "fixed 296-byte storage");

    d.accepted(2304, 948);
    d.clientFinish();
    d.received(DiagnosticRequest::identity, 0, 10, 2200, 900);
    d.responseBuilt(80);
    d.responseWrite(80, 0, 0, 11);
    d.received(DiagnosticRequest::capabilities, 1, 20, 2100, 880);
    d.responseBuilt(60);
    d.responseWrite(60, 0, 1, 21);
    d.received(DiagnosticRequest::snapshot, 2, 30, 2000, 840);
    d.responseBuilt(63);
    d.responseWrite(63, 0, 2, 31);
    expect(d.snapshot_requests == 1 && d.snapshot_completions == 1,
           "normal snapshot progression");
    expect(d.response_size == 63 && d.response_written == 63,
           "complete response byte counts");

    d.responseBuilt(63); d.responseWrite(17, 1, 3, 40);
    expect(d.partial_writes == 1 && d.failed_writes == 0,
           "partial write classified");
    expect(d.close_reason == CloseReason::response_write_failed &&
           d.allocation_failure_delta == 1, "partial write evidence");

    d.responseBuilt(63); d.responseWrite(0, 2, 3, 41);
    expect(d.partial_writes == 1 && d.failed_writes == 1,
           "failed write classified");
    expect(d.allocation_failure_delta == 3, "allocation delta accumulated");

    d.closed(CloseReason::timeout, 1900, 700, 100, 4, 110);
    d.accepted(1850, 675);
    d.closed(CloseReason::peer_eof, 1800, 650, 200, 4, 104);
    expect(d.timeouts == 1 && d.eofs == 1 && d.closes == 2,
           "timeout and EOF classified");
    expect(d.close_history_count == 2 &&
           d.close_history[(d.close_history_next + 3) % 4].socket_error == 104 &&
           d.close_history[(d.close_history_next + 2) % 4].socket_error == 110,
           "close evidence retained across reconnection");
    expect(d.close_history[(d.close_history_next + 3) % 4].stage ==
               DiagnosticStage::accepted &&
           d.close_history[(d.close_history_next + 2) % 4].stage ==
               DiagnosticStage::response_built,
           "failure stage retained before closed state");
    const auto& retained = d.close_history[(d.close_history_next + 2) % 4];
    expect(retained.request == DiagnosticRequest::snapshot &&
           retained.request_counter == 2 && retained.request_ms == 30 &&
           retained.response_request == DiagnosticRequest::snapshot &&
           retained.response_counter == 2 && retained.response_ms == 31,
           "request and response correlation retained across reconnection");

    d.snapshot_requests = std::numeric_limits<uint32_t>::max();
    d.received(DiagnosticRequest::snapshot, 4, 50, 1700, 600);
    expect(d.snapshot_requests == std::numeric_limits<uint32_t>::max(),
           "counter saturation");

    expect(diagnosticRequest(1) == DiagnosticRequest::identity &&
           diagnosticRequest(3) == DiagnosticRequest::capabilities &&
           diagnosticRequest(5) == DiagnosticRequest::snapshot &&
           diagnosticRequest(7) == DiagnosticRequest::volume_set &&
           diagnosticRequest(9) == DiagnosticRequest::playback_command &&
           diagnosticRequest(10) == DiagnosticRequest::now_playing &&
           diagnosticRequest(13) == DiagnosticRequest::btx_state &&
           diagnosticRequest(15) == DiagnosticRequest::btx_command &&
           diagnosticRequest(16) == DiagnosticRequest::full_control_capabilities &&
           diagnosticRequest(18) == DiagnosticRequest::source_state &&
           diagnosticRequest(20) == DiagnosticRequest::source_select &&
           diagnosticRequest(21) == DiagnosticRequest::menu_state &&
           diagnosticRequest(23) == DiagnosticRequest::catalog_query &&
           diagnosticRequest(25) == DiagnosticRequest::catalog_command &&
           diagnosticRequest(26) == DiagnosticRequest::browser_state &&
           diagnosticRequest(28) == DiagnosticRequest::browser_command &&
           diagnosticRequest(0x42) == DiagnosticRequest::unknown,
           "request type is bounded and payload-free");
    expect(sizeof(Diagnostics) == 296, "diagnostic budget");

    std::cout << "PASS: " << checks << " RTCP diagnostic checks\n";
}

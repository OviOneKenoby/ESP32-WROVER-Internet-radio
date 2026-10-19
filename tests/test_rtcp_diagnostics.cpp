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
    expect(sizeof d == 60, "fixed 60-byte storage");

    d.accepted(2304, 948);
    d.clientFinish();
    d.received(DiagnosticRequest::identity, 2200, 900);
    d.responseBuilt(80);
    d.responseWrite(80, 0);
    d.received(DiagnosticRequest::capabilities, 2100, 880);
    d.responseBuilt(60);
    d.responseWrite(60, 0);
    d.received(DiagnosticRequest::snapshot, 2000, 840);
    d.responseBuilt(63);
    d.responseWrite(63, 0);
    expect(d.snapshot_requests == 1 && d.snapshot_completions == 1,
           "normal snapshot progression");
    expect(d.response_size == 63 && d.response_written == 63,
           "complete response byte counts");

    d.responseBuilt(63); d.responseWrite(17, 1);
    expect(d.partial_writes == 1 && d.failed_writes == 0,
           "partial write classified");
    expect(d.close_reason == CloseReason::response_write_failed &&
           d.allocation_failure_delta == 1, "partial write evidence");

    d.responseBuilt(63); d.responseWrite(0, 2);
    expect(d.partial_writes == 1 && d.failed_writes == 1,
           "failed write classified");
    expect(d.allocation_failure_delta == 3, "allocation delta accumulated");

    d.closed(CloseReason::timeout, 1900, 700);
    d.closed(CloseReason::peer_eof, 1800, 650);
    expect(d.timeouts == 1 && d.eofs == 1 && d.closes == 2,
           "timeout and EOF classified");

    d.snapshot_requests = std::numeric_limits<uint32_t>::max();
    d.received(DiagnosticRequest::snapshot, 1700, 600);
    expect(d.snapshot_requests == std::numeric_limits<uint32_t>::max(),
           "counter saturation");

    expect(diagnosticRequest(1) == DiagnosticRequest::identity &&
           diagnosticRequest(3) == DiagnosticRequest::capabilities &&
           diagnosticRequest(5) == DiagnosticRequest::snapshot &&
           diagnosticRequest(7) == DiagnosticRequest::volume_set &&
           diagnosticRequest(9) == DiagnosticRequest::playback_command &&
           diagnosticRequest(10) == DiagnosticRequest::now_playing &&
           diagnosticRequest(0x42) == DiagnosticRequest::unknown,
           "request type is bounded and payload-free");
    expect(sizeof(Diagnostics) <= 64, "diagnostic budget");

    std::cout << "PASS: " << checks << " RTCP diagnostic checks\n";
}

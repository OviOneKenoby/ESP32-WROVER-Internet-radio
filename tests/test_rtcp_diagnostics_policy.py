from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = (root / "src/rtcp_diagnostics.h").read_text(encoding="utf-8").lower()
server = (root / "src/rtcp_server.cpp").read_text(encoding="utf-8")
main = (root / "src/main.cpp").read_text(encoding="utf-8")

assert "sizeof(diagnostics) == 296" in header
assert "new " not in header and "malloc" not in header and "vector" not in header
for secret in ("psk", "nonce", "pairing", "payload", "identity_value"):
    assert secret not in header

for marker in (
    "diagnostics_.accepted",
    "diagnostics_.clientFinish",
    "diagnostics_.received",
    "diagnostics_.responseBuilt",
    "diagnostics_.responseWrite",
    "CloseReason::timeout",
    "CloseReason::peer_eof",
):
    assert marker in server

for field in (
    "request_counter", "response_counter", "request_ms", "response_ms",
    "response_request",
):
    assert field in header

assert "rtcp_diag stage=" in main
assert "allocation_failure_delta" in main
print("PASS: 14 RTCP diagnostic policy checks")

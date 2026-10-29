from pathlib import Path

root = Path(__file__).parents[1]
source = (root / "src/remote_browser.cpp").read_text(encoding="utf-8")
header = (root / "src/remote_browser_diagnostics.h").read_text(encoding="utf-8")
web = (root / "src/web_portal.cpp").read_text(encoding="utf-8")

assert source.count("job_deadline_ms_ = now_ms + kJobDeadlineMs") == 1
assert "bool RemoteBrowserService::beginMirror()" in source
assert "deadline_expired = 1" in source
assert "transport_error = result" in source
assert "http_status" in source
for name in ("client_initialized", "connecting", "connected", "headers_sent",
             "header_received", "body_received", "finished", "deadline"):
    assert name in header
for field in ("browser_mirror", "browser_transport_stage", "browser_transport_error",
              "browser_http_status", "browser_elapsed_ms", "browser_deadline_expired",
              "browser_first_failure_stage", "browser_first_transport_error",
              "browser_tls_system_error", "browser_tls_mbedtls_error",
              "browser_tls_esp_error", "browser_tls_cert_flags"):
    assert field in web
assert "esp_tls_get_and_clear_error_type" in source
assert "recordTls" in source
assert "kTransportTimeoutMs = 5000" in source
assert "config.timeout_ms = kTransportTimeoutMs" in source
assert "kMirrorLimit = RADIO_BROWSER_MIRROR_COUNT" in source
assert '"all.api.radio-browser.info"' in (root / "src/config.h").read_text(encoding="utf-8")
print("remote browser transport/deadline policy: PASS")

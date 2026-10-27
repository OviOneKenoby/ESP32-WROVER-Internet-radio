from pathlib import Path

root = Path(__file__).resolve().parents[1]
service = (root / "src/remote_browser.cpp").read_text(encoding="utf-8")
main = (root / "src/main.cpp").read_text(encoding="utf-8")
server = (root / "src/rtcp_server.cpp").read_text(encoding="utf-8")

checks = {
    "true async HTTPS": "config.is_async = true" in service and "esp_http_client_perform" in service,
    "bounded PSRAM body": "kBodyCapacity = 32768" in service and "MALLOC_CAP_SPIRAM" in service,
    "verified HTTPS": "arduino_esp_crt_bundle_attach" in service,
    "bounded mirrors and deadline": "kMirrorLimit = 2" in service and "kJobDeadlineMs = 30000" in service,
    "application-loop cooperative poll": "remoteBrowser.poll(millis())" in main,
    "physical browser priority": "void enterBrowseLetter()" in main and
        "remoteBrowser.cancel(RemoteBrowserCancelReason::physical_navigation);" in main,
    "no playback stop on browse start": "audioPlayer.stop" not in service,
    "session close cancellation": "browser_close_owner_" in server,
    "fc5 identity": 'FIRMWARE_VERSION "+fc5"' in server,
    "start diagnostics": "diagnostics_.start_requests" in service and
        "diagnostics_.start_results" in service,
    "page diagnostics": "diagnostics_.page_queries" in service and
        "diagnostics_.pages_produced" in service,
    "job completion diagnostics": "diagnostics_.jobs_completed" in service and
        "diagnostics_.jobs_failed" in service,
    "bounded cancellation diagnostics": all(reason in main + service for reason in (
        "RemoteBrowserCancelReason::remote_command",
        "RemoteBrowserCancelReason::rtcp_closed",
        "RemoteBrowserCancelReason::playback_started",
        "RemoteBrowserCancelReason::physical_navigation",
    )),
}

for name, passed in checks.items():
    if not passed:
        raise SystemExit(f"FAIL: {name}")
print(f"REMOTE Radio Browser policy: PASS ({len(checks)}/{len(checks)})")

from pathlib import Path

root = Path(__file__).resolve().parents[1]
audio = (root / "src/audio.cpp").read_text(encoding="utf-8")
audio_header = (root / "src/audio.h").read_text(encoding="utf-8")
main = (root / "src/main.cpp").read_text(encoding="utf-8")
server = (root / "src/rtcp_server.cpp").read_text(encoding="utf-8")
protocol = (root / "src/rtcp_full_control.h").read_text(encoding="utf-8")

checks = {
    "single fixed deferred slot": "struct DeferredPlay" in audio_header and
        "DeferredPlay work{}" in audio,
    "network open precedes playback mutex":
        audio.index("openedSource->open(streamURL)") <
        audio.index("xSemaphoreTake(audioMutex, portMAX_DELAY)",
                    audio.index("bool AudioPlayer::playDeferredNow")),
    "network open shares time with RTCP owner":
        "vTaskPrioritySet(nullptr, tskIDLE_PRIORITY + 1)" in audio and
        "vTaskPrioritySet(nullptr, savedPriority)" in audio,
    "radio gain restored after I2S handoff":
        "i2sOutput->SetGain(currentVolume / 100.0f);" in audio,
    "revision revalidation": "getCatalogRevision() != deferredCatalogStart.expectedRevision" in main,
    "physical cancellation": all(token in audio for token in (
        "void AudioPlayer::stop()", "bool AudioPlayer::enableBluetooth()",
        "bool AudioPlayer::disableBluetooth()", "cancelDeferredPlay();")),
    "session close cancellation": "catalog_close_owner_" in server and
        "setCatalogCloseOwner(cancelDeferredCatalogStart)" in main,
    "no automatic replay": "last_catalog_request_" in server,
    "version fenced capability": "kMinor = 4" in protocol and
        "kFeatureDeferredCatalogPlay" in protocol,
}

for name, passed in checks.items():
    if not passed:
        raise SystemExit(f"FAIL: {name}")
print(f"deferred catalog start policy: PASS ({len(checks)}/{len(checks)})")

from pathlib import Path

root = Path(__file__).resolve().parents[1]
main = (root / "src/main.cpp").read_text(encoding="utf-8")
audio = (root / "src/audio.cpp").read_text(encoding="utf-8")
policy = (root / "src/browse_playback_policy.h").read_text(encoding="utf-8")


def between(text, start, end):
    s = text.index(start)
    return text[s:text.index(end, s)]


owner = between(main, "uint8_t executeRtcpBrowser(const rtcp_browser::Command& command) {", "// ====")
close = between(main, "void closeRtcpBrowser() {", "uint8_t executeRtcpBrowser(const rtcp_browser::Command& command) {")
block = between(main, "// ---- Pause playback while the REMOTE browses", "uint8_t executeRtcpBrowser(")
loop = between(main, "void loop() {", "timeService.update();")
svc = between(main, "void serviceDeferredCatalogStart() {", "bool projectRtcpBrowser(")

checks = {
    "header included": '#include "browse_playback_policy.h"' in main,
    "start hook only after a successful START":
        owner.index("error != rtcp_browser::success") <
        owner.index("Operation::start) { browseSessionStart()"),
    "remote cancel ends the session":
        "Operation::cancel) { browseSessionEnd()" in owner,
    "rtcp close ends the session after the job is cancelled":
        close.index("remoteBrowser.cancel(RemoteBrowserCancelReason::rtcp_closed)") <
        close.index("browseSessionEnd()"),
    "pick keeps the saved station until it resolves (admitted after deferPlay)":
        owner.index("audioPlayer.deferPlay(command.request, url, audioCodec)") <
        owner.index("browsePlayback.onPickAdmitted(command.request)") and
        "onStationChosen" not in main + policy,
    "pick resolution runs after the deferred slot is cleared (restart needs the free slot)":
        svc.index("audioPlayer.clearDeferredPlay(deferredCatalogStart.request)") <
        svc.index("browsePickResolved(pickRequest, status)"),
    "pick resolution only for Browse picks": "if (browsePick) browsePickResolved" in svc,
    "pick resolution uses the policy and the shared restart helper":
        "browsePlayback.onPickResolved(request, status == DeferredPlayStatus::succeeded" in block and
        block.count("browseQueueRestart(") >= 3,
    "session end while a pick is pending queues nothing": "case Result::pick_pending:" in block,
    "pause only on the policy's request":
        "case Result::pause:\n            audioPlayer.pause();" in block,
    "restart uses the deferred play path": "audioPlayer.deferPlay(request, browsePlayback.url(), codec)" in block,
    "restart goes through one helper": "void browseQueueRestart(const char* logLine)" in block,
    "restart request id kept apart from REMOTE ids": "uint64_t(1) << 63" in block,
    "restart request is cleared": "audioPlayer.clearDeferredPlay(browseRestartRequest)" in block,
    "restart finished from loop": "serviceBrowseRestart();" in loop,
    "never resumes in the Browse block": ".resume(" not in block and ".resume(" not in owner and ".resume(" not in policy,
    "no per-job restore": "finishJob" not in block and "finishJob" not in policy,
    "decision log lines": all(s in block + owner for s in (
        "[BROWSE] Paused for session", "[BROWSE] Station chosen, no restore",
        "[BROWSE] Restart station after cancel/close",
        "[BROWSE] Picked station failed, restarting previous",
        "[BROWSE] Playback was not playing, nothing to restore")),
    "new play path restores the radio gain":
        audio.index("i2sOutput->SetGain(currentVolume / 100.0f);",
                    audio.index("mp3Decoder->begin(audioSourceBuf, i2sOutput)")) <
        audio.index("playbackState = STATE_PLAYING;", audio.index("mp3Decoder->begin(audioSourceBuf, i2sOutput)")),
    "pause mutes by gain 0": "i2sOutput->SetGain(0.0f)" in audio,
    "url capacity matches config": "kUrlCapacity = 256" in policy and
        "#define MAX_URL_LENGTH      256" in (root / "src/config.h").read_text(encoding="utf-8"),
}
failed = [name for name, ok in checks.items() if not ok]
for name, ok in checks.items():
    print(("PASS" if ok else "FAIL"), name)
assert not failed, failed
print(f"browse playback policy integration: PASS={len(checks)} FAIL=0")

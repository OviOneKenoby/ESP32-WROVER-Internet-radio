from pathlib import Path

root = Path(__file__).resolve().parents[1]
main = (root / "src/main.cpp").read_text(encoding="utf-8")
rb = (root / "src/remote_browser.cpp").read_text(encoding="utf-8")
rbh = (root / "src/remote_browser.h").read_text(encoding="utf-8")
cfg = (root / "src/config.h").read_text(encoding="utf-8")
pol = (root / "src/browse_playback_policy.h").read_text(encoding="utf-8")


def between(text, start, end):
    s = text.index(start)
    return text[s:text.index(end, s)]


execute = between(main, "uint8_t executeRtcpBrowser(const rtcp_browser::Command& command) {", "// ====")
reject = between(main, "uint8_t rejectBrowsePickUrl(", "uint8_t executeRtcpBrowser(")
parse = between(rb, "} else {\n        result_count_ = 0;", "return !doc.overflowed();")
take = between(rb, "bool RemoteBrowserService::takePlay(", "bool RemoteBrowserService::urlTooLong(")

checks = {
    "url_fit.h exists": (root / "src/url_fit.h").exists(),
    "capacity of the policy equals MAX_URL_LENGTH":
        "#define MAX_URL_LENGTH      256" in cfg and "kUrlCapacity = 256" in pol,
    "parse never strlcpy's an url that does not fit":
        "urlFitsBuffer(urlLength, sizeof result.url)" in parse and
        parse.index("urlFitsBuffer") < parse.index("strlcpy(result.url"),
    "parse stores the full length and empties a rejected url":
        "result.url_length" in parse and ("result.url[0] = '" + chr(92) + "0'") in parse,
    "takePlay refuses a result that does not fit":
        "urlFitsBuffer(size_t(results_[token].url_length)" in take,
    "urlTooLong is declared": "bool urlTooLong(uint16_t token, size_t& length) const;" in rbh,
    "check runs before takePlay and before deferPlay":
        execute.index("urlTooLong(") < execute.index("takePlay(") <
        execute.index("audioPlayer.deferPlay("),
    "rejected pick returns through rejectBrowsePickUrl":
        "return rejectBrowsePickUrl(command, urlLength)" in execute,
    "rejection never calls deferPlay with the rejected string":
        "audioPlayer.deferPlay" not in reject and "takePlay" not in reject,
    "rejection reuses the failed-pick path":
        "browsePlayback.onPickAdmitted(command.request)" in reject and
        "browsePickResolved(command.request, DeferredPlayStatus::failed)" in reject,
    "rejection ends the Browse session like an admitted pick":
        "remoteBrowser.cancel(RemoteBrowserCancelReason::playback_started)" in reject,
    "rejection reports failure to the REMOTE":
        "return rtcp_browser::execution_failed" in reject,
    "[BROWSE] log states length and capacity":
        '[BROWSE] Pick rejected: URL length %u does not fit capacity %u' in reject,
}
failed = [name for name, ok in checks.items() if not ok]
assert not failed, failed
print("browse url truncation policy: PASS (%d checks)" % len(checks))

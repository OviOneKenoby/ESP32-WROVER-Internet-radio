from pathlib import Path

root = Path(__file__).resolve().parents[1]
main = (root / "src/main.cpp").read_text(encoding="utf-8")
server = (root / "src/rtcp_server.cpp").read_text(encoding="utf-8")
header = (root / "src/rtcp_server.h").read_text(encoding="utf-8")

loop = main.split("void loop()", 1)[1].split("void setupWiFi()", 1)[0]
first_poll = loop.index("rtcpServer.poll(audioPlayer)")
web = loop.index("webPortal.handle()")
second_poll = loop.index("rtcpServer.poll(audioPlayer)", first_poll + 1)

assert first_poll < web < second_poll
assert "delay(2);" in loop
assert "audio.setVolume(percent);" in server
assert "timing_.lastApplyUs" in server
assert "timing_.lastResponseUs" in server
assert "uint32_t commands" in header
assert "uint32_t maxApplyUs" in header
assert "uint32_t maxResponseUs" in header

print("RTCP LATENCY POLICY SUMMARY PASS=1 FAIL=0 assertions=9")

from pathlib import Path

root = Path(__file__).resolve().parents[1]
main = (root / "src/main.cpp").read_text(encoding="utf-8")
display_h = (root / "src/display.h").read_text(encoding="utf-8")
display_cpp = (root / "src/display.cpp").read_text(encoding="utf-8")
server = (root / "src/rtcp_server.cpp").read_text(encoding="utf-8")

# The existing GxEPD2 busy-wait hook keeps RTCP on the application owner. It
# must be registered only after network setup and must yield on every callback.
assert "void setBusyService(void (*callback)(const void*)" in display_h
assert "epd.epd2.setBusyCallback(callback, context);" in display_cpp
assert main.index("setupWiFi();") < main.index(
    "display.setBusyService(serviceRtcpWhileDisplayBusy);"
)
callback = main.split("void serviceRtcpWhileDisplayBusy(const void*) {", 1)[1].split("}", 1)[0]
assert callback.index("rtcpServer.poll(audioPlayer);") < callback.index("delay(1);")

# Buffered complete application input wins over the idle deadline check.
# Partial input retains the original bounded timeout, while the handshake's
# original total deadline remains unchanged.
assert "state_ == State::records && rx_used_ != rx_needed_ &&" in server
assert "int32_t(millis() - deadline_ms_) >= 0" in server
handshake = server.split("bool Server::processHandshake()", 1)[1].split(
    "bool Server::sendApplication", 1
)[0]
assert "if (int32_t(millis() - deadline_ms_) >= 0)" in handshake
record_poll = server.split("void Server::poll(AudioPlayer& audio)", 1)[1]
assert record_poll.index("if (!readBounded())") < record_poll.index(
    "state_ == State::records && rx_used_ != rx_needed_"
)
assert record_poll.index("rx_used_ != rx_needed_") < record_poll.index("processRecord(audio)")

print("RTCP display-service policy: 9/9 PASS")

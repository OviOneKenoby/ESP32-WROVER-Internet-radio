from pathlib import Path

root = Path(__file__).resolve().parents[1]
server = (root / "src/rtcp_server.cpp").read_text(encoding="utf-8")
header = (root / "src/rtcp_server.h").read_text(encoding="utf-8")
main = (root / "src/main.cpp").read_text(encoding="utf-8")

assert "kPlaybackCommand = 9, kNowPlayingRequest = 10" in server
assert "playback_mailbox_.put(command,decision.target_station)" in server
assert "bool Server::servicePlayback()" in server
assert "handlePlayingInput" not in server
assert "std::array<uint8_t, kMaxWire> rx_" in header
assert "std::array<uint8_t, kMaxWire> tx_" in header
assert "kMaxPlaintext = 512" in header
assert "D-009 persistent projection and mailbox exceed the approved budget" in header
assert "rtcpServer.setPlaybackOwner(executeRtcpPlayback)" in main
assert "playStation(targetStation, false)" in main
fixed = main.split("bool playStation(uint8_t stationIdx, bool updateDisplay)", 1)[1]
fixed = fixed.split("void playDiscoveredStation", 1)[0]
assert fixed.index("rtcpServer.invalidateMediaContext()") < fixed.index("audioPlayer.play")
assert "stationManager.addToFavorites" not in fixed

print("D009 INTEGRATION POLICY 12/12 PASS")

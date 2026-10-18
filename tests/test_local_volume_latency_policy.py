from pathlib import Path

root = Path(__file__).resolve().parents[1]
main = (root / "src/main.cpp").read_text(encoding="utf-8")
audio = (root / "src/audio.cpp").read_text(encoding="utf-8")
input_cpp = (root / "src/input.cpp").read_text(encoding="utf-8")
target = (root / "src/volume_target.h").read_text(encoding="utf-8")

loop = main.split("void loop()", 1)[1].split("void setupWiFi()", 1)[0]
assert loop.index("inputControl.update()") < loop.index("wifiManager.update()")
assert "VOLUME_OVERLAY_QUIET_MS = 250" in main
assert "volumeOverlayPending && !inputControl.hasPendingEncoderSteps()" in loop
assert "inputControl.takePendingEncoderSteps()" in main
assert "display.updateVolume" not in main.split("void showVolumeOverlay()", 1)[1].split("// ============================================", 1)[0]
assert "encoderPendingSteps = 0" in input_cpp
assert "VolumeTarget::afterDetents" in audio
assert "static_cast<int64_t>(detents) * 5" in target
assert "if (target < 0) target = 0" in target
assert "if (target > 100) target = 100" in target
assert "localVolumeLastApplyUs" in main
print("TOTAL 10/10 PASS")

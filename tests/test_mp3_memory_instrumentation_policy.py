from pathlib import Path

root = Path(__file__).resolve().parents[1]
patch = (root / "tools/patch_esp8266audio_aac.py").read_text(encoding="utf-8")
audio = (root / "src/audio.cpp").read_text(encoding="utf-8")
main = (root / "src/main.cpp").read_text(encoding="utf-8")
runtime = (root / "src/mp3_memory_diagnostics.cpp").read_text(encoding="utf-8")
header = (root / "src/mp3_memory_diagnostics.h").read_text(encoding="utf-8")

checks = 0


def check(value, message):
    global checks
    checks += 1
    assert value, message


for sequence, name in enumerate([
    "MP3DecInfo", "FrameHeader", "SideInfo", "ScaleFactorInfo",
    "HuffmanInfo", "DequantInfo", "IMDCTInfo", "SubbandInfo",
], start=1):
    check(
        f"tuner_mp3_memory_allocation({sequence}, sizeof({name})" in patch,
        f"Helix allocation {sequence} ({name}) instrumented",
    )

check("TUNER_MP3_MEMORY_AFTER_GENERATOR_ALLOCATION" in patch,
      "generator allocation milestone patched")
check("TUNER_MP3_MEMORY_AFTER_HELIX_INITIALIZATION" in patch,
      "Helix initialization milestone patched")
check("TUNER_MP3_MEMORY_BEFORE_CONSTRUCTION" in runtime,
      "pre-construction phase captured")
check("TUNER_MP3_MEMORY_AFTER_I2S_INITIALIZATION" in audio,
      "I2S phase captured")
check("TUNER_MP3_MEMORY_STABLE_PLAYBACK" in audio,
      "stable playback phase captured")
check("TUNER_MP3_MEMORY_AFTER_TEARDOWN" in audio,
      "teardown phase captured")
check("heap_caps_get_info" in runtime and
      "readHeap(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA)" in runtime,
      "separate internal and DMA heap inspection")
check("heap_caps_get_allocated_size" in runtime and
      "esp_ptr_internal" in runtime and "esp_ptr_external_ram" in runtime,
      "allocation size and placement recorded")
check("Mp3MemoryDiagnostics::printSnapshot();" in main,
      "existing serial diagnostic interface used")
check("handleDiagnostics" not in runtime and "WiFiServer" not in runtime,
      "no diagnostic network interface")
check("new " not in runtime and "malloc(" not in runtime,
      "runtime diagnostic path has no explicit dynamic allocation")
check("sizeof(Model) == 340" in header,
      "fixed diagnostic storage guarded")
check('AppendUnique(CPPPATH=[env.subst("$PROJECT_SRC_DIR")])' in patch,
      "isolated dependency receives the project hook include path")

print(f"PASS {checks}/{checks} MP3 memory instrumentation policy checks")

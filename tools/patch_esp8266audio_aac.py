Import("env")

from pathlib import Path


def patch_aac_sbr_output_buffer(build_env):
    library_dir = Path(build_env["PROJECT_LIBDEPS_DIR"]) / build_env["PIOENV"] / "ESP8266Audio" / "src"
    header = library_dir / "AudioGeneratorAAC.h"
    implementation = library_dir / "AudioGeneratorAAC.cpp"
    mp3_implementation = library_dir / "AudioGeneratorMP3a.cpp"
    http_icy_implementation = library_dir / "AudioFileSourceICYStream.cpp"

    if not header.is_file() or not implementation.is_file() or not mp3_implementation.is_file() or not http_icy_implementation.is_file():
        raise RuntimeError("ESP8266Audio dependency was not installed before the AAC patch step")

    header_text = header.read_text(encoding="utf-8")
    old_header = """    int16_t *outSample; //[1024 * 2]; // Interleaved L/R\n"""
    new_header = """#ifdef ESP8266\n    const int outSampleLen = 1024 * 2;  // SBR disabled\n#else\n    const int outSampleLen = 2048 * 2;  // SBR enabled\n#endif\n    int16_t *outSample; //[1024 * 2] or [2048 * 2]; // Interleaved L/R\n"""
    if old_header in header_text:
        header.write_text(header_text.replace(old_header, new_header), encoding="utf-8")
    elif new_header not in header_text:
        raise RuntimeError("Unexpected AudioGeneratorAAC.h; refusing to apply an unverified patch")

    implementation_text = implementation.read_text(encoding="utf-8")
    replacements = {
        "1024 * 2 * sizeof(uint16_t)": "outSampleLen * sizeof(uint16_t)",
        "1024 * 2 * sizeof(int16_t)": "outSampleLen * sizeof(int16_t)",
    }
    for old, new in replacements.items():
        implementation_text = implementation_text.replace(old, new)
    if "1024 * 2 * sizeof" in implementation_text:
        raise RuntimeError("AudioGeneratorAAC.cpp was not fully patched")
    implementation.write_text(implementation_text, encoding="utf-8")

    # A stream mislabelled as MP3 can produce a nominally successful Helix
    # decode with a zero sample rate/channel count.  The legacy ESP32 I2S
    # driver divides by that rate in i2s_set_clk(), so reject the invalid
    # frame before AudioOutputI2S::SetRate() is called.
    mp3_text = mp3_implementation.read_text(encoding="utf-8")
    old_mp3_block = """            MP3GetLastFrameInfo(hMP3Decoder, &fi);
            if ((int)fi.samprate != (int)lastRate) {
                output->SetRate(fi.samprate);
                lastRate = fi.samprate;
            }
"""
    new_mp3_block = """            MP3GetLastFrameInfo(hMP3Decoder, &fi);
            if (fi.samprate <= 0 || fi.nChans <= 0) {
                cb.st(-1, \"MP3 frame has invalid format\");
                running = false;
                goto done;
            }
            if ((int)fi.samprate != (int)lastRate) {
                output->SetRate(fi.samprate);
                lastRate = fi.samprate;
            }
"""
    if old_mp3_block in mp3_text:
        mp3_implementation.write_text(mp3_text.replace(old_mp3_block, new_mp3_block), encoding="utf-8")
    elif new_mp3_block not in mp3_text:
        raise RuntimeError("Unexpected AudioGeneratorMP3a.cpp; refusing to apply an unverified patch")

    # The upstream ICY source rejects a stream when no bytes arrive within
    # 500ms. DIGI FM is a healthy Icecast stream, but its initial response
    # can exceed that interval. Match this project's verified 3s connection
    # timeout without changing the source's later read behavior.
    http_icy_text = http_icy_implementation.read_text(encoding="utf-8")
    old_http_icy_wait = "while ((stream->available() < (int)len) && (millis() - start < 500))"
    new_http_icy_wait = "while ((stream->available() < (int)len) && (millis() - start < 3000))"
    if old_http_icy_wait in http_icy_text:
        http_icy_implementation.write_text(
            http_icy_text.replace(old_http_icy_wait, new_http_icy_wait), encoding="utf-8")
    elif new_http_icy_wait not in http_icy_text:
        raise RuntimeError("Unexpected AudioFileSourceICYStream.cpp; refusing to apply an unverified patch")


def patch_before_aac_compilation(build_env, node):
    # Build middleware runs when PlatformIO is about to compile each source
    # node, after dependencies have been installed but before this library
    # source is compiled. This also works on a completely clean first build.
    source = str(node).replace("\\", "/")
    if (source.endswith("/ESP8266Audio/src/AudioGeneratorAAC.cpp") or
            source.endswith("/ESP8266Audio/src/AudioGeneratorMP3a.cpp") or
            source.endswith("/ESP8266Audio/src/AudioFileSourceICYStream.cpp")):
        patch_aac_sbr_output_buffer(build_env)
    return node


env.AddBuildMiddleware(patch_before_aac_compilation)

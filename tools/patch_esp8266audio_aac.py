Import("env")

from pathlib import Path


# The instrumented ESP8266Audio sources are compiled in PlatformIO's isolated
# library context. Export only this project's source directory so the bounded
# C hook declaration remains visible without modifying the pinned dependency.
env.AppendUnique(CPPPATH=[env.subst("$PROJECT_SRC_DIR")])


def patch_aac_sbr_output_buffer(build_env):
    library_dir = Path(build_env["PROJECT_LIBDEPS_DIR"]) / build_env["PIOENV"] / "ESP8266Audio" / "src"
    header = library_dir / "AudioGeneratorAAC.h"
    implementation = library_dir / "AudioGeneratorAAC.cpp"
    mp3_implementation = library_dir / "AudioGeneratorMP3a.cpp"
    mp3_buffers = library_dir / "libhelix-mp3" / "buffers.c"
    http_icy_implementation = library_dir / "AudioFileSourceICYStream.cpp"

    if (not header.is_file() or not implementation.is_file() or
            not mp3_implementation.is_file() or not mp3_buffers.is_file() or
            not http_icy_implementation.is_file()):
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
        mp3_text = mp3_text.replace(old_mp3_block, new_mp3_block)
    elif new_mp3_block not in mp3_text:
        raise RuntimeError("Unexpected AudioGeneratorMP3a.cpp; refusing to apply an unverified patch")

    hook_include = '#include "mp3_memory_diagnostics_hooks.h"\n'
    if hook_include not in mp3_text:
        include_anchor = '#include "AudioGeneratorMP3a.h"\n'
        if include_anchor not in mp3_text:
            raise RuntimeError("Unexpected AudioGeneratorMP3a include layout")
        mp3_text = mp3_text.replace(
            include_anchor, include_anchor + hook_include, 1)

    constructor_anchor = """AudioGeneratorMP3a::AudioGeneratorMP3a() {
    running = false;
"""
    constructor_instrumented = """AudioGeneratorMP3a::AudioGeneratorMP3a() {
    tuner_mp3_memory_milestone(TUNER_MP3_MEMORY_AFTER_GENERATOR_ALLOCATION);
    running = false;
"""
    if constructor_anchor in mp3_text:
        mp3_text = mp3_text.replace(constructor_anchor, constructor_instrumented, 1)
    elif constructor_instrumented not in mp3_text:
        raise RuntimeError("Unexpected AudioGeneratorMP3a constructor layout")

    helix_anchor = """    hMP3Decoder = MP3InitDecoder();
    if (!hMP3Decoder) {
"""
    helix_instrumented = """    hMP3Decoder = MP3InitDecoder();
    tuner_mp3_memory_milestone(TUNER_MP3_MEMORY_AFTER_HELIX_INITIALIZATION);
    if (!hMP3Decoder) {
"""
    if helix_anchor in mp3_text:
        mp3_text = mp3_text.replace(helix_anchor, helix_instrumented, 1)
    elif helix_instrumented not in mp3_text:
        raise RuntimeError("Unexpected MP3InitDecoder call layout")
    mp3_implementation.write_text(mp3_text, encoding="utf-8")

    buffers_text = mp3_buffers.read_text(encoding="utf-8")
    buffers_include = '#include "mp3_memory_diagnostics_hooks.h"\n'
    if buffers_include not in buffers_text:
        include_anchor = '#include "coder.h"\n'
        if include_anchor not in buffers_text:
            raise RuntimeError("Unexpected Helix buffers include layout")
        buffers_text = buffers_text.replace(
            include_anchor, include_anchor + buffers_include, 1)

    allocation_anchor = """    mp3DecInfo = (MP3DecInfo *)malloc(sizeof(MP3DecInfo));
    if (!mp3DecInfo) {
        return 0;
    }
    ClearBuffer(mp3DecInfo, sizeof(MP3DecInfo));

    fh = (FrameHeader *)     malloc(sizeof(FrameHeader));
    si = (SideInfo *)        malloc(sizeof(SideInfo));
    sfi = (ScaleFactorInfo *) malloc(sizeof(ScaleFactorInfo));
    hi = (HuffmanInfo *)     malloc(sizeof(HuffmanInfo));
    di = (DequantInfo *)     malloc(sizeof(DequantInfo));
    mi = (IMDCTInfo *)       malloc(sizeof(IMDCTInfo));
    sbi = (SubbandInfo *)     malloc(sizeof(SubbandInfo));
"""
    allocation_instrumented = """    mp3DecInfo = (MP3DecInfo *)malloc(sizeof(MP3DecInfo));
    tuner_mp3_memory_allocation(1, sizeof(MP3DecInfo), mp3DecInfo);
    if (!mp3DecInfo) {
        return 0;
    }
    ClearBuffer(mp3DecInfo, sizeof(MP3DecInfo));

    fh = (FrameHeader *)     malloc(sizeof(FrameHeader));
    tuner_mp3_memory_allocation(2, sizeof(FrameHeader), fh);
    si = (SideInfo *)        malloc(sizeof(SideInfo));
    tuner_mp3_memory_allocation(3, sizeof(SideInfo), si);
    sfi = (ScaleFactorInfo *) malloc(sizeof(ScaleFactorInfo));
    tuner_mp3_memory_allocation(4, sizeof(ScaleFactorInfo), sfi);
    hi = (HuffmanInfo *)     malloc(sizeof(HuffmanInfo));
    tuner_mp3_memory_allocation(5, sizeof(HuffmanInfo), hi);
    di = (DequantInfo *)     malloc(sizeof(DequantInfo));
    tuner_mp3_memory_allocation(6, sizeof(DequantInfo), di);
    mi = (IMDCTInfo *)       malloc(sizeof(IMDCTInfo));
    tuner_mp3_memory_allocation(7, sizeof(IMDCTInfo), mi);
    sbi = (SubbandInfo *)     malloc(sizeof(SubbandInfo));
    tuner_mp3_memory_allocation(8, sizeof(SubbandInfo), sbi);
"""
    if allocation_anchor in buffers_text:
        buffers_text = buffers_text.replace(
            allocation_anchor, allocation_instrumented, 1)
    elif allocation_instrumented not in buffers_text:
        raise RuntimeError("Unexpected Helix allocation layout")
    mp3_buffers.write_text(buffers_text, encoding="utf-8")

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
            source.endswith("/ESP8266Audio/src/AudioFileSourceICYStream.cpp") or
            source.endswith("/ESP8266Audio/src/libhelix-mp3/buffers.c")):
        patch_aac_sbr_output_buffer(build_env)
    return node


env.AddBuildMiddleware(patch_before_aac_compilation)

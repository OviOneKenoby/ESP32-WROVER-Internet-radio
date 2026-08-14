Import("env")

from pathlib import Path


def patch_aac_sbr_output_buffer(build_env):
    library_dir = Path(build_env["PROJECT_LIBDEPS_DIR"]) / build_env["PIOENV"] / "ESP8266Audio" / "src"
    header = library_dir / "AudioGeneratorAAC.h"
    implementation = library_dir / "AudioGeneratorAAC.cpp"

    if not header.is_file() or not implementation.is_file():
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


def patch_before_aac_compilation(build_env, node):
    # Build middleware runs when PlatformIO is about to compile each source
    # node, after dependencies have been installed but before this library
    # source is compiled. This also works on a completely clean first build.
    if str(node).replace("\\", "/").endswith("/ESP8266Audio/src/AudioGeneratorAAC.cpp"):
        patch_aac_sbr_output_buffer(build_env)
    return node


env.AddBuildMiddleware(patch_before_aac_compilation)

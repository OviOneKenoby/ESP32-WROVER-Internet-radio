from pathlib import Path


root = Path(__file__).resolve().parents[1]
platformio = (root / "platformio.ini").read_text(encoding="utf-8")
source = (root / "src" / "tls_record_buffer_psram.cpp").read_text(
    encoding="utf-8"
)
header = (root / "src" / "tls_record_buffer_psram.h").read_text(
    encoding="utf-8"
)

checks = 0


def check(value, message):
    global checks
    checks += 1
    assert value, message


check(platformio.count("-Wl,--wrap=esp_mbedtls_mem_calloc") == 1,
      "single linker wrapper")
check("MBEDTLS_SSL_IN_BUFFER_LEN" in source and
      "MBEDTLS_SSL_OUT_BUFFER_LEN" in source,
      "both resolved record-buffer sizes guarded")
check("kResolvedRecordBufferBytes = 16717" in header,
      "hardware-observed allocation size exact")
check("MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT" in source,
      "record buffers request byte-addressable PSRAM")
check(source.index("heap_caps_calloc") <
      source.index("__real_esp_mbedtls_mem_calloc(count, size)"),
      "external allocation precedes original fallback")
check("isRecordBufferRequest(count, size)" in source,
      "external policy restricted by exact request size")
check("mbedtls_platform_set_calloc_free" not in source,
      "no runtime global allocator replacement")
check("setInsecure" not in source and "setCACert" not in source,
      "wrapper does not alter TLS verification policy")
check("malloc(" not in source and "new " not in source,
      "no unrelated allocator introduced")
check("std::numeric_limits<std::size_t>::max() / count" in header,
      "calloc multiplication overflow guarded")

print(f"PASS {checks}/{checks} TLS record-buffer source-policy checks")

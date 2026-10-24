from pathlib import Path


root = Path(__file__).resolve().parents[1]
platformio = (root / "platformio.ini").read_text(encoding="utf-8")
network = (root / "src" / "network_progress.cpp").read_text(encoding="utf-8")
system_info = (root / "src" / "system_info.cpp").read_text(encoding="utf-8")


def section(name):
    marker = f"[env:{name}]"
    start = platformio.index(marker)
    end = platformio.find("\n[env:", start + len(marker))
    return platformio[start:] if end < 0 else platformio[start:end]


checks = 0


def check(value, message):
    global checks
    checks += 1
    assert value, message


production = section("esp32-lwip-psram")
probe = section("esp32-uart-probe")
base = section("esp32-dev")

check("extends = env:esp32-dev" in production,
      "production inherits the uninstrumented base environment")
check("${env:esp32-dev.build_flags}" in production,
      "production imports only base build flags")
check("TUNER_UART_TX_PROBE" not in production and "esp32-uart-probe.build_flags" not in production,
      "production does not enable or inherit the Wi-Fi OS-adapter probe")
check("-DTUNER_UART_TX_PROBE=1" in probe,
      "probe remains isolated in the explicit diagnostic environment")
check("SoftwareTxProbe::prepare(cfg)" in network and
      "#ifdef TUNER_UART_TX_PROBE" in network,
      "diagnostic interception remains compile-time gated")
check("__real_esp_wifi_init(cfg)" in network,
      "production path forwards the original Wi-Fi configuration unchanged")
check("heap_caps_register_failed_alloc_callback(allocationFailed)" in system_info,
      "bounded allocation-failure detection remains registered")
check("-Wl,--wrap=esp_mbedtls_mem_calloc" in base and
      "extends = env:esp32-dev" in production,
      "TLS record-buffer PSRAM wrapper remains inherited from the base")
check("-Wl,--wrap=mem_malloc" in production,
      "lwIP PSRAM policy remains linked")

print(f"PASS {checks}/{checks} production Wi-Fi probe policy checks")

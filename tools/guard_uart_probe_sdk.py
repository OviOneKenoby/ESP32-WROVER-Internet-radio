"""Fail closed before building a probe tied to these private instruction paths."""
Import("env")
from hashlib import sha256
from pathlib import Path
if env.subst("$PIOENV") in ("esp32-uart-probe", "esp32-lwip-psram"):
    sdk = Path(env.PioPlatform().get_package_dir("framework-arduinoespressif32")) / "tools/sdk/esp32/lib"
    expected = {
        "libpp.a": "a4356534d066da8c1ce08b52e3e614ee4da1a3e9b3b4bfc4285d083f46b75937",
        "libnet80211.a": "6db8c03e58385cae462871647c680489ba84b5c29e86abca9c89dca9d8bb7ba1",
    }
    for name, digest in expected.items():
        if sha256((sdk / name).read_bytes()).hexdigest() != digest:
            raise RuntimeError("STOP: UART probe requires reviewed fixed SDK: " + name)
    print("UART probe: fixed private driver archive hashes PASS")

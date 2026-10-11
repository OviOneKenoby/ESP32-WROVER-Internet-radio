"""Verify fixed SDK allocator ABI and large PBUF_RAM routing in the exact ELF."""
import re
import runpy
from pathlib import Path

# Also require the existing public TX/ref/free/config routing checks.
v = runpy.run_path(str(Path(__file__).with_name("verify_network_progress_elf.py")))
s, functions, calls, literals = (v[k] for k in ("symbols", "functions", "calls", "literals"))
wrapper = "__wrap_mem_malloc"
assert s[wrapper] in calls("pbuf_alloc"), "PBUF_RAM allocation bypasses PSRAM policy"
assert calls(wrapper).count(s["mem_malloc"]) == 1, "original fallback call missing/duplicated"
assert calls(wrapper).count(s["heap_caps_malloc"]) == 1, "PSRAM allocator call missing/duplicated"
assert s["spi_flash_cache_enabled"] in calls(wrapper)
assert s["xPortInIsrContext"] in calls(wrapper)
assert calls("mem_malloc").count(s["malloc"]) == 1, "original ABI is not ordinary malloc"
assert calls("mem_free").count(s["free"]) == 1, "free ABI changed"
for name in ("mem_malloc", "mem_free"):
    body = "\n".join(functions[name])
    assert re.search(r"\bmov(?:\.n)?\s+a10,\s*a2", body), "allocator argument no longer forwarded unchanged"
    assert not re.search(r"\baddi(?:\.n)?\s+a(?:2|10),", body), "stats prefix or pointer adjustment requires review"
body = "\n".join(functions[wrapper])
assert re.search(r"\bmovi(?:\.n)?\s+a11,\s*0x404", body), "wrong effective PSRAM caps"
assert re.search(r"\bor\s+a3,\s*a2,\s*a2", body), "size operand changed"
assert re.search(r"\bmovi\s+a2,\s*0x1ff", body), "threshold operand changed"
assert re.search(r"\bbltu\s+a2,\s*a3,", body), "effective unsigned size >511 predicate changed"
for name in ("printf", "esp_log_write", "heap_caps_get_free_size", "heap_caps_get_largest_free_block"):
    if name in s:
        assert s[name] not in calls(wrapper), "unsafe diagnostic call in allocation path"
print("PASS PBUF_RAM -> wrapper -> PSRAM caps0x404 / original fallback once")
print("PASS original malloc/free ABI unchanged, no stats prefix, argument forwarding unchanged")
print("PASS cache/ISR guards and threshold512 present; host tests verify branch behavior")
print("Hardware PSRAM/DMA pressure, latency and delivery still NOT RUN")

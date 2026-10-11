"""Static final-ELF checks; not a substitute for the UART positive control."""
import hashlib
import re
import struct
import subprocess
import sys
from pathlib import Path

elf, reference, tool = map(Path, sys.argv[1:4])
subprocess.run([sys.executable, str(Path(__file__).with_name("verify_tx_classification_elf.py")),
                str(elf), str(tool)], check=True)
assert hashlib.sha256(reference.read_bytes()).hexdigest() == "22d30785e7c25f3587a04ecfbe73610a438d7d78ab843d1773de167ebc75d999"

def load(path):
    data = path.read_bytes()
    sections = [struct.unpack_from("<10I", data, struct.unpack_from("<I", data, 32)[0] + i * struct.unpack_from("<H", data, 46)[0])
                for i in range(struct.unpack_from("<H", data, 48)[0])]
    def word(address):
        for s in sections:
            if s[1] != 8 and s[3] <= address <= s[3] + s[5] - 4:
                return struct.unpack_from("<I", data, s[4] + address - s[3])[0]
        raise AssertionError(hex(address))
    nm = subprocess.check_output([str(tool / "xtensa-esp32-elf-nm.exe"), "-n", str(path)], text=True)
    symbols = {m[2]: int(m[1], 16) for line in nm.splitlines() if (m := re.match(r"^([0-9a-f]+)\s+\S\s+(.+)$", line))}
    text = subprocess.check_output([str(tool / "xtensa-esp32-elf-objdump.exe"), "-d", str(path)], text=True)
    def body(name):
        m = re.search(r"^[0-9a-f]+ <" + re.escape(name) + r">:\n(.*?)(?=^[0-9a-f]+ <|\Z)", text, re.M | re.S)
        assert m, name
        return m[1]
    return word, symbols, body

rw, rs, rb = load(reference)
word, symbols, body = load(elf)
sites = [("esf_buf_alloc", 0x7a), ("ieee80211_output_process", 0x4f),
         ("ieee80211_output_process", 0x108), ("ieee80211_output_process", 0x226),
         ("esf_buf_alloc_dynamic", 0x78), ("esf_buf_alloc_dynamic", 0x1a8)]

def normalized(name, s, b):
    lines = []
    for line in b(name).splitlines():
        m = re.match(r"\s*([0-9a-f]+):\s+[0-9a-f ]+\t(.*)", line)
        if not m:
            continue
        asm = re.sub(r"\b[0-9a-f]{8}\b", "ADDR", m[2])
        # Relink changes absolute references but instruction/register/control
        # patterns and symbol-relative branch offsets must stay identical.
        lines.append((int(m[1], 16) - s[name], asm))
    return lines

for name in {n for n, _ in sites} | {"ieee80211_alloc_tx_buf", "ieee80211_output_do", "wifi_osi_funcs_register"}:
    assert normalized(name, symbols, body) == normalized(name, rs, rb), "private path changed: " + name
    print("PASS unchanged normalized private instructions:", name)
for name, offset in sites:
    call = symbols[name] + offset - 3
    lines = body(name).splitlines()
    i = next(i for i, line in enumerate(lines) if re.match(rf"\s*{call:x}:", line))
    assert "callx8" in lines[i]
    assert any(re.search(r"\bl32i\s+a\d+, a\d+, 88\b", line) for line in lines[max(0, i-5):i]), "not declared mutex-unlock offset"
    print(f"PASS OSI _mutex_unlock site {name}+{offset:#x} return={call+3:#x}")

# Same registered OSI pointer is used by the private allocation code.
reg_literal = int(re.search(r"l32r\s+a9,\s*([0-9a-f]+)", body("wifi_osi_funcs_register"))[1], 16)
esf_literal = int(re.search(r"l32r\s+a11,\s*([0-9a-f]+)", body("esf_buf_alloc"))[1], 16)
assert word(reg_literal) == word(esf_literal), "allocation bypasses registered adapter"
print("PASS allocation uses the registered declared OSI table")

unlock = next(n for n in symbols if n.endswith("unlockEPv") and "GLOBAL" in n)
assert 0x40080000 <= symbols[unlock] < 0x400c0000
def original_is_first_call(name, original_fragment):
    original = next(a for n, a in symbols.items() if original_fragment in n and "GLOBAL" in n)
    assert 0x3ffb0000 <= original < 0x40000000, "original pointer not DRAM"
    registers = {}
    for line in body(name).splitlines():
        m = re.search(r"\bl32r\s+(a\d+),\s*([0-9a-f]+)", line)
        if m:
            registers[m[1]] = word(int(m[2], 16))
        m = re.search(r"\bl32i(?:\.n)?\s+(a\d+), (a\d+), 0\s*$", line)
        if m:
            registers[m[1]] = ("pointer", registers.get(m[2]))
        m = re.search(r"\bcallx8\s+(a\d+)", line)
        if m:
            assert registers.get(m[1]) == ("pointer", original), "helper before original or wrong forwarding pointer"
            print("PASS first call forwards original through DRAM:", name)
            return
        assert not re.search(r"\bcall(?:0|4|8|12)\b", line), "added call before original"
    raise AssertionError("original call missing: " + name)
original_is_first_call(unlock, "originalUnlock")
for fragment in ("beginTx", "endTx"):
    name = next(n for n in symbols if "SoftwareTxProbe" in n and fragment in n)
    assert 0x40080000 <= symbols[name] < 0x400c0000
for name in (unlock,):
    assert "esp_cpu_process_stack_pc" not in body(name), "flash-outlined PC helper"
    assert not re.search(r"<(?:malloc|calloc|realloc|printf|esp_log_write|heap_caps_get[^>]*)>", body(name))
    print("PASS IRAM callback without allocator/logger/heap queries:", name)
for fragment in ("internalMalloc", "wifiMalloc", "observeMalloc"):
    name = next(n for n in symbols if fragment in n and "GLOBAL" in n)
    assert 0x40080000 <= symbols[name] < 0x400c0000
    assert "esp_cpu_process_stack_pc" not in body(name), "flash-outlined PC helper"
    assert not re.search(r"<(?:printf|esp_log_write|heap_caps_get[^>]*)>", body(name))
    print("PASS IRAM typed allocation observation:", name)
    if fragment != "observeMalloc":
        original_is_first_call(name, "originalInternalMalloc" if fragment == "internalMalloc" else "originalWifiMalloc")
for offset in (348, 364):
    assert 0x40080000 <= word(symbols["g_wifi_osi_funcs"] + offset) < 0x400c0000
malloc_return = symbols["esf_buf_alloc_dynamic"] + 0xa8
assert re.search(rf"^\s*{malloc_return-3:x}:.*callx8", body("esf_buf_alloc_dynamic"), re.M)
print("PASS declared allocation callback return site", hex(malloc_return))

# printf's 64-byte Arduino stack-buffer fallback must not allocate on x.
source = Path("src/software_tx_probe.cpp").read_text()
for fmt in re.findall(r'Serial\.printf\("((?:[^"\\]|\\.)*)"', source):
    text = bytes(fmt, "utf8").decode("unicode_escape")
    worst = re.sub(r"%08x", "f" * 8, text)
    worst = re.sub(r"%u", "4294967295", worst)
    assert len(worst) < 64, (len(worst), text)
print("PASS all serial printf fragments fit the stack buffer at UINT32_MAX")
print("STATIC ONLY: runtime positive control, scope and unwind coverage still required")

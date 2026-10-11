"""Verify actual final-ELF wrapper routing; no additional Python dependencies."""
import re
import struct
import subprocess
import sys
from pathlib import Path

elf = Path(sys.argv[1])
tool_dir = Path(sys.argv[2])
data = elf.read_bytes()
assert data[:6] == b"\x7fELF\x01\x01", "expected ELF32 little endian"
section_offset = struct.unpack_from("<I", data, 32)[0]
section_size, section_count = struct.unpack_from("<HH", data, 46)
sections = [struct.unpack_from("<10I", data, section_offset + i * section_size)
            for i in range(section_count)]

def word(address):
    for section in sections:
        _, kind, _, start, offset, size, *_ = section
        if kind != 8 and start <= address and address + 4 <= start + size:
            return struct.unpack_from("<I", data, offset + address - start)[0]
    raise AssertionError(f"literal not file-backed: {address:#x}")

nm = subprocess.check_output([str(tool_dir / "xtensa-esp32-elf-nm.exe"), "-n", str(elf)], text=True)
symbols = {}
for line in nm.splitlines():
    match = re.match(r"^([0-9a-f]+)\s+\S\s+(.+)$", line)
    if match:
        symbols[match[2]] = int(match[1], 16)
disasm = subprocess.check_output([str(tool_dir / "xtensa-esp32-elf-objdump.exe"), "-d", str(elf)], text=True)
functions = {}
current = None
for line in disasm.splitlines():
    match = re.match(r"^([0-9a-f]+) <(.+)>:$", line)
    if match:
        current = match[2]
        functions[current] = []
    elif current:
        functions[current].append(line)

def literals(name):
    result = []
    for line in functions[name]:
        match = re.search(r"\bl32r\s+(a\d+),\s*([0-9a-f]+)", line)
        if match:
            result.append(word(int(match[2], 16)))
    return result

def calls(name):
    # Compiler's direct calls and literal-load/callx pairs; reject unresolved
    # indirect calls rather than silently accepting a routing assumption.
    registers = {}
    result = []
    for line in functions[name]:
        match = re.search(r"\bl32r\s+(a\d+),\s*([0-9a-f]+)", line)
        if match:
            registers[match[1]] = word(int(match[2], 16))
        match = re.search(r"\bcall(?:0|4|8|12)\s+([0-9a-f]+)", line)
        if match:
            result.append(int(match[1], 16))
        match = re.search(r"\bcallx(?:0|4|8|12)\s+(a\d+)", line)
        if match:
            assert match[1] in registers, f"unresolved indirect call in {name}: {line}"
            result.append(registers[match[1]])
    return result

names = ("esp_wifi_internal_tx_by_ref", "esp_netif_netstack_buf_ref",
         "esp_netif_netstack_buf_free", "esp_netif_receive")
for original in names:
    wrapper = "__wrap_" + original
    assert 0x40080000 <= symbols[wrapper] < 0x400c0000, f"{wrapper} not IRAM"
    actual = calls(wrapper)
    assert actual.count(symbols[original]) == 1, f"original call count: {wrapper}"
    forbidden = ("malloc", "calloc", "realloc", "printf", "esp_log_write")
    for symbol, address in symbols.items():
        if any(part in symbol for part in forbidden):
            assert address not in actual, f"unsafe added call: {symbol}"
    print(f"PASS {wrapper}: IRAM, exactly one original call")

assert symbols["__wrap_esp_wifi_internal_tx_by_ref"] in calls("wifi_transmit_wrap"), "Wi-Fi TX bypass"
start_literals = literals("wifi_start")
for original in names[1:]:
    assert symbols["__wrap_" + original] in start_literals, f"callback registration bypass: {original}"
print("PASS wifi_transmit_wrap routes to wrapper")
print("PASS wifi_start registers wrapped ref/free and RX callbacks")
print("PASS final ELF boundary routing verified (not hardware TX delivery)")

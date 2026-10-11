"""End-to-end ELF verifier tests: real candidate, historic failure, mutations."""
import re
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

candidate, historical, tool = map(Path, sys.argv[1:4])
verifier = Path(__file__).with_name("verify_tx_classification_elf.py")

def verify(path, success, diagnostic=None):
    result = subprocess.run([sys.executable, str(verifier), str(path), str(tool)],
                            capture_output=True, text=True)
    assert (result.returncode == 0) == success, result.stdout + result.stderr
    if diagnostic:
        assert diagnostic in result.stderr, result.stdout + result.stderr

verify(candidate, True)
verify(historical, False, "REJECT fe228ca737b9 unlock normal comparison")
data = candidate.read_bytes()
sections = [struct.unpack_from("<10I", data, struct.unpack_from("<I", data, 32)[0] +
            i * struct.unpack_from("<H", data, 46)[0])
            for i in range(struct.unpack_from("<H", data, 48)[0])]
nm = subprocess.check_output([str(tool / "xtensa-esp32-elf-nm.exe"), "-n", str(candidate)], text=True)
table = int(next(line.split()[0] for line in nm.splitlines() if "classificationSites" in line), 16)

def file_offset(address):
    for s in sections:
        if s[1] != 8 and s[3] <= address <= s[3] + s[5] - 4:
            return s[4] + address - s[3]
    raise AssertionError("not file backed")

with tempfile.TemporaryDirectory(prefix="tuner-classifier-test-") as temp:
    path = Path(temp) / "mutated.elf"
    # Every base and offset operand must independently be checked. Mutation of
    # context 6 recreates the exact erroneous +0xa2 classification offset.
    for index in range(9):
        for field in (0, 4):
            changed = bytearray(data)
            position = file_offset(table + index * 8 + field)
            previous = struct.unpack_from("<I", changed, position)[0]
            replacement = 0xa2 if index == 6 and field == 4 else previous + 1
            struct.pack_into("<I", changed, position, replacement)
            path.write_bytes(changed)
            verify(path, False, "wrong final")
print("PASS 20 ELF verifier checks: candidate accepted; historic ELF and all 18 operand mutations rejected")

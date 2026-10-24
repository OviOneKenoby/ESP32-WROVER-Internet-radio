import struct
import sys
from pathlib import Path

from elftools.elf.elffile import ELFFile


def symbol(table, name):
    matches = table.get_symbol_by_name(name)
    if not matches:
        raise RuntimeError(f"missing ELF symbol: {name}")
    return matches[0]


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: verify_tls_record_buffer_wrapper_elf.py firmware.elf")
    path = Path(sys.argv[1])
    with path.open("rb") as stream:
        elf = ELFFile(stream)
        table = elf.get_section_by_name(".symtab")
        if table is None:
            raise RuntimeError("ELF has no symbol table")

        wrapper = symbol(table, "__wrap_esp_mbedtls_mem_calloc")
        original = symbol(table, "esp_mbedtls_mem_calloc")
        allocator_pointer = symbol(table, "mbedtls_calloc_func")

        pointer_address = allocator_pointer["st_value"]
        containing = None
        for section in elf.iter_sections():
            start = section["sh_addr"]
            end = start + section["sh_size"]
            if start <= pointer_address and pointer_address + 4 <= end:
                containing = section
                break
        if containing is None:
            raise RuntimeError("mbedtls_calloc_func is outside file-backed sections")

        offset = pointer_address - containing["sh_addr"]
        target = struct.unpack_from("<I", containing.data(), offset)[0]
        wrapper_address = wrapper["st_value"]
        original_address = original["st_value"]

        if target != wrapper_address:
            raise RuntimeError(
                "mbedtls_calloc_func does not target the wrapper: "
                f"pointer=0x{target:08x}, wrapper=0x{wrapper_address:08x}, "
                f"original=0x{original_address:08x}"
            )

        print(
            "PASS TLS allocator ELF binding: "
            f"mbedtls_calloc_func=0x{pointer_address:08x} -> "
            f"wrapper=0x{wrapper_address:08x}; "
            f"original=0x{original_address:08x} retained for fallback"
        )


if __name__ == "__main__":
    main()

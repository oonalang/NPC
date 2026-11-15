#!/usr/bin/env python3
import argparse
import mmap
from pathlib import Path
import re
import runpy
import struct
import sys


def main():
    parser = argparse.ArgumentParser(description="Verify CODM session-control addresses against the supplied ARM64 game ELF")
    parser.add_argument("library", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    trigger = runpy.run_path(str(root / "tests/verify-trigger-profile.py"))
    source = (root / "src/main/jni/System/Hooks/SessionProtectionProfile.h").read_text()
    records = re.findall(r'\{"([^"]+)",\s*(0x[0-9A-Fa-f]+),\s*\{([^}]*)\}\}', source)
    if len(records) != 13 or len({int(rva, 16) for _, rva, _ in records}) != 13:
        raise ValueError("Expected 13 distinct session-control fingerprints")
    with args.library.open("rb") as file, mmap.mmap(file.fileno(), 0, access=mmap.ACCESS_READ) as library:
        trigger["verify"](library, root / "src/main/jni/Trigger")
        offset = struct.unpack_from("<Q", library, 32)[0]
        size, count = struct.unpack_from("<HH", library, 54)
        segments = [struct.unpack_from("<IIQQQQQQ", library, offset + i * size) for i in range(count)]
        for name, rva_text, literals in records:
            rva = int(rva_text, 16)
            expected = trigger["byte_literals"](literals)
            if len(expected) != 16:
                raise ValueError(f"Invalid record at {rva_text}")
            for kind, flags, start, address, _, length, _, _ in segments:
                if kind == 1 and flags & 5 == 5 and address <= rva and rva - address + 16 <= length:
                    at = start + rva - address
                    if library[at:at + 16] != expected:
                        raise ValueError(f"{name}: {rva_text}")
                    break
            else:
                raise ValueError(f"Entry {rva_text} is outside readable executable file segments")
    print("Session protection: all 13 plaintext method/ABI/layout records match.")
    print("This does not verify server acceptance or whether a second device can replace the login.")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, struct.error) as error:
        print(f"Session profile verification failed: {error}", file=sys.stderr)
        sys.exit(1)

#!/usr/bin/env python3
import argparse
import mmap
from pathlib import Path
import re
import runpy
import struct
import sys


def main():
    parser = argparse.ArgumentParser(description="Verify hitbox profile records against the supplied ARM64 game ELF")
    parser.add_argument("library", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    trigger = runpy.run_path(str(root / "tests/verify-trigger-profile.py"))
    source = (root / "src/main/jni/System/Hooks/HitboxProfile.h").read_text()
    records = re.findall(r"\{(0x[0-9A-Fa-f]+),\s*\{([^}]*)\}\}", source)
    if len(records) != 17:
        raise ValueError("Expected fourteen plaintext anchors and three encoded method records")
    with args.library.open("rb") as file, mmap.mmap(file.fileno(), 0, access=mmap.ACCESS_READ) as library:
        trigger["verify"](library, root / "src/main/jni/Trigger")
        offset = struct.unpack_from("<Q", library, 32)[0]
        size, count = struct.unpack_from("<HH", library, 54)
        segments = [struct.unpack_from("<IIQQQQQQ", library, offset + i * size) for i in range(count)]
        for rva_text, literals in records:
            rva = int(rva_text, 16)
            expected = trigger["byte_literals"](literals)
            if len(expected) != 16:
                raise ValueError(f"Invalid record at {rva_text}")
            for kind, flags, start, address, _, length, _, _ in segments:
                if kind == 1 and flags & 5 == 5 and address <= rva and rva - address + 16 <= length:
                    at = start + rva - address
                    if library[at:at + 16] != expected:
                        raise ValueError(f"Hitbox disk fingerprint differs at {rva_text}")
                    break
            else:
                raise ValueError(f"Hitbox entry {rva_text} is outside readable executable file segments")
    print("Hitbox: all 17 disk records match (14 plaintext anchors, 3 encoded methods).")
    print("Decoded runtime instructions, hook ABI and in-game damage still require Android device verification.")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, struct.error) as error:
        print(f"Hitbox profile verification failed: {error}", file=sys.stderr)
        sys.exit(1)

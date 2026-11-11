#!/usr/bin/env python3
"""Check the trigger's pinned profile against a supplied ARM64 game ELF."""

import argparse
import mmap
from pathlib import Path
import re
import struct
import sys


def byte_literals(value):
    return bytes(int(number, 16) for number in re.findall(r"0x([0-9a-fA-F]{2})\b", value))


def verify(library, source):
    if library[:7] != b"\x7fELF\x02\x01\x01" or len(library) < 64:
        raise ValueError("expected a little-endian ELF64 library")
    kind, machine = struct.unpack_from("<HH", library, 16)
    if kind != 3 or machine != 183:
        raise ValueError("expected an AArch64 shared library")
    ph_offset = struct.unpack_from("<Q", library, 32)[0]
    ph_size, ph_count = struct.unpack_from("<HH", library, 54)
    if ph_size != 56 or not 0 < ph_count <= 512:
        raise ValueError("invalid ELF program header table")
    if ph_offset + ph_size * ph_count > len(library):
        raise ValueError("truncated ELF program header table")
    segments = [struct.unpack_from("<IIQQQQQQ", library, ph_offset + i * ph_size)
                for i in range(ph_count)]

    def mapped_bytes(rva, size):
        for segment_kind, flags, offset, address, _, file_size, _, _ in segments:
            if (segment_kind == 1 and flags & 5 == 5 and address <= rva and
                    rva - address + size <= file_size):
                start = offset + rva - address
                if start + size <= len(library):
                    return library[start:start + size]
        raise ValueError(f"0x{rva:X} is outside readable executable file segments")

    notes = []
    for segment_kind, _, offset, _, _, file_size, _, _ in segments:
        if segment_kind != 4:
            continue
        end = offset + file_size
        if end > len(library):
            raise ValueError("truncated ELF note segment")
        while offset + 12 <= end:
            name_size, value_size, note_kind = struct.unpack_from("<III", library, offset)
            name_at = offset + 12
            value_at = name_at + ((name_size + 3) & ~3)
            next_note = value_at + ((value_size + 3) & ~3)
            if next_note > end:
                raise ValueError("truncated ELF note")
            if note_kind == 3 and library[name_at:name_at + name_size] == b"GNU\0":
                notes.append(library[value_at:value_at + value_size])
            offset = next_note

    profile = source / "NativeProfile.cpp"
    expected_id_match = re.search(r"constexpr unsigned char expected\[\]\{([^}]+)\}",
                                  profile.read_text())
    if not expected_id_match:
        raise ValueError("could not read the pinned build ID")
    expected_id = byte_literals(expected_id_match.group(1))
    if not expected_id or expected_id not in notes:
        raise ValueError(f"build ID differs; expected {expected_id.hex()}")

    checks = re.findall(r'\{"([^"]+)",\s*(0x[0-9A-Fa-f]+),\s*\{([^}]*)\}\}',
                        (source / "NativeProfileChecks.inc").read_text())
    # The live comparison flag does not change offline disk verification.
    probes = re.findall(r'\{"([^"]+)",\s*(0x[0-9A-Fa-f]+),\s*\{([^}]*)\},\s*(?:true|false)\}',
                        (source / "TriggerCompatibility.cpp").read_text())
    if len(checks) != 44 or len(probes) != 7:
        raise ValueError("expected all 44 profile fingerprints and all 7 compatibility probes")
    if len({int(rva, 16) for _, rva, _ in checks}) != 44:
        raise ValueError("duplicate profile fingerprint addresses")
    failures = []
    for entries, width in ((checks, 16), (probes, 8)):
        for name, rva_text, literals in entries:
            expected = byte_literals(literals)
            if len(expected) != width:
                raise ValueError(f"invalid fingerprint: {name}")
            actual = mapped_bytes(int(rva_text, 16), len(expected))
            if actual != expected:
                failures.append(f"{name}: expected {expected.hex()}, found {actual.hex()}")
    if failures:
        raise ValueError("\n".join(failures))
    print(f"Build ID {expected_id.hex()}: {len(checks)} fingerprints and "
          f"{len(probes)} compatibility probes match")
    print("This validates the disk image; loaded runtime code still requires device testing.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("library", type=Path, help="supplied libunity.so")
    args = parser.parse_args()
    source = Path(__file__).resolve().parents[1] / "src/main/jni/Trigger"
    try:
        with args.library.open("rb") as binary:
            with mmap.mmap(binary.fileno(), 0, access=mmap.ACCESS_READ) as library:
                verify(library, source)
    except (OSError, ValueError, struct.error) as error:
        print(f"Profile verification failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

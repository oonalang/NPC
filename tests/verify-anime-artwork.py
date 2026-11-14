#!/usr/bin/env python3
"""Check the embedded ESP preview artwork against the texture loader contract."""
import struct
import sys
import zlib
from pathlib import Path

EXPECTED_WIDTH = 1024
EXPECTED_HEIGHT = 1536
SIGNATURE = b"\x89PNG\r\n\x1a\n"


def fail(message):
    print(f"FAIL: {message}", file=sys.stderr)
    sys.exit(1)


def main():
    source = Path(__file__).resolve().parent.parent / "src/main/jni/Assets/adult-preview.png"
    data = source.read_bytes()
    if data[:8] != SIGNATURE:
        fail(f"{source.name} has no PNG signature")
    offset = 8
    header = None
    compressed = bytearray()
    ended = False
    while offset + 12 <= len(data):
        count = struct.unpack(">I", data[offset:offset + 4])[0]
        if count > len(data) - offset - 12:
            fail(f"{source.name} chunk length runs past the file")
        kind = data[offset + 4:offset + 8]
        body = data[offset + 8:offset + 8 + count]
        stored = struct.unpack(">I", data[offset + 8 + count:offset + 12 + count])[0]
        if zlib.crc32(kind + body) & 0xFFFFFFFF != stored:
            fail(f"{source.name} {kind.decode('ascii', 'replace')} chunk CRC mismatch")
        if kind == b"IHDR":
            if header is not None:
                fail("duplicate IHDR")
            header = body
        elif kind == b"IDAT":
            if header is None:
                fail("IDAT before IHDR")
            compressed += body
        elif kind == b"IEND":
            ended = True
            break
        offset += count + 12
    if header is None or not ended or len(header) != 13:
        fail("missing IHDR, IDAT or IEND")
    width, height, depth, color, compression, filtering, interlace = struct.unpack(">IIBBBBB", header)
    if (width, height) != (EXPECTED_WIDTH, EXPECTED_HEIGHT):
        fail(f"artwork is {width}x{height}, loader expects {EXPECTED_WIDTH}x{EXPECTED_HEIGHT}")
    if (depth, color, compression, filtering, interlace) != (8, 6, 0, 0, 0):
        fail(f"artwork is not non-interlaced 8-bit RGBA (depth {depth}, color {color})")
    raw = zlib.decompress(bytes(compressed))
    stride = 1 + width * 4
    if len(raw) != stride * height:
        fail(f"decoded scanlines are {len(raw)} bytes, expected {stride * height}")
    for row in range(height):
        if raw[row * stride] > 4:
            fail(f"scanline {row} uses unknown filter {raw[row * stride]}")
    print(f"{source.name}: {width}x{height} RGBA8, {len(compressed)} compressed bytes, "
          f"{len(raw)} decoded bytes, {height} filtered scanlines verified")


if __name__ == "__main__":
    main()

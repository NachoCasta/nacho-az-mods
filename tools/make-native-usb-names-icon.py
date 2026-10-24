#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Render the original Native USB Names storefront icon; no firmware assets."""
import struct
import zlib
from pathlib import Path

SIZE = 256
DARK = (22, 25, 30)
BLUE = (75, 172, 239)
WHITE = (235, 241, 246)
ORANGE = (255, 154, 30)
pixels = bytearray(bytes(DARK) * SIZE * SIZE)

def rect(x, y, width, height, color):
    for row in range(y, y + height):
        for column in range(x, x + width):
            offset = (row * SIZE + column) * 3
            pixels[offset:offset + 3] = bytes(color)

rect(98, 25, 60, 55, WHITE)
rect(108, 36, 12, 21, DARK)
rect(136, 36, 12, 21, DARK)
rect(72, 76, 112, 154, BLUE)
rect(87, 108, 82, 70, WHITE)
rect(99, 121, 58, 8, DARK)
rect(99, 138, 42, 8, DARK)
rect(99, 155, 51, 8, DARK)
rect(112, 197, 32, 12, ORANGE)

def chunk(kind, data):
    return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data) & 0xffffffff)

raw = b''.join(b'\0' + pixels[y * SIZE * 3:(y + 1) * SIZE * 3] for y in range(SIZE))
png = (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', SIZE, SIZE, 8, 2, 0, 0, 0))
       + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))
path = Path(__file__).resolve().parents[1] / 'src/native-usb-names/icon.png'
path.write_bytes(png)
print(path)

#!/usr/bin/env python3
"""Print the NUL-terminated string at each given virtual address of EP147.

usage: vstr.py <elf> <hexaddr> [<hexaddr> ...]
"""
import struct
import sys

d = open(sys.argv[1], 'rb').read()
phoff = struct.unpack_from('<Q', d, 0x20)[0]
es, n = struct.unpack_from('<HH', d, 0x36)
segs = [struct.unpack_from('<IIQQQQ', d, phoff + i * es) for i in range(n)]


def off(va):
    for t, _, o, v, _, sz in segs:
        if t == 1 and v <= va < v + sz:
            return o + va - v
    return None


for a in sys.argv[2:]:
    va = int(a, 16)
    o = off(va)
    print(hex(va), repr(d[o:d.index(b'\0', o)].decode(errors='replace')) if o is not None else '?')

#!/usr/bin/env python3
"""Dump EP147's NFS slot table at 0x2bb6958 (28-byte records): for each record,
the pointer at +0x10 and +0x18 resolved to strings, and the int at +0x20.

usage: nfstab.py <elf> [count]
"""
import struct
import sys

d = open(sys.argv[1], 'rb').read()
count = int(sys.argv[2]) if len(sys.argv) > 2 else 8
phoff = struct.unpack_from('<Q', d, 0x20)[0]
es, n = struct.unpack_from('<HH', d, 0x36)
segs = [struct.unpack_from('<IIQQQQ', d, phoff + i * es) for i in range(n)]


def off(va):
    for t, _, o, v, _, sz in segs:
        if t == 1 and v <= va < v + sz:
            return o + va - v
    return None


def s(va):
    o = off(va)
    if o is None:
        return f'?{va:#x}'
    return repr(d[o:d.index(b'\0', o)].decode(errors='replace'))


base = 0x2bb6958
for i in range(count):
    r = base + 28 * i
    o = off(r)
    raw = d[o:o + 0x24]
    p1 = struct.unpack_from('<Q', d, off(r + 0x10))[0]
    p2 = struct.unpack_from('<Q', d, off(r + 0x18))[0]
    v = struct.unpack_from('<i', d, off(r + 0x20))[0]
    print(f'[{i}] +10={s(p1)} +18={s(p2)} +20={v}   raw={raw.hex()}')

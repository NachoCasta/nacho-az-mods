#!/usr/bin/env python3
"""vmem.py <elf> ARG...: read EP147 by virtual address.
  <hex>[:len]   hex bytes
  s<hex>        C string
  q<hex>[:n]    n qwords (default 8), each followed by the string it points to, if any"""
import struct
import sys

d = open(sys.argv[1], 'rb').read()
phoff = struct.unpack_from('<Q', d, 0x20)[0]
es, n = struct.unpack_from('<HH', d, 0x36)
segs = [struct.unpack_from('<IIQQQQ', d, phoff + i * es) for i in range(n)]


def off(va):
    for t, _, o, v, _p, sz in segs:
        if t == 1 and v <= va < v + sz:
            return o + va - v
    return None


def cstr(va):
    o = off(va)
    if o is None or not 0x28fa0c0 <= va < 0x384b430:  # .rodata only
        return None
    s = d[o:d.index(b'\0', o)]
    return s if s and all(32 <= c < 127 for c in s) else None


for a in sys.argv[2:]:
    if a[0] == 's':
        print(a, repr(cstr(int(a[1:], 16))))
    elif a[0] == 'q':
        va, _, k = a[1:].partition(':')
        va = int(va, 16)
        for i in range(int(k or '8', 0)):
            v = struct.unpack_from('<Q', d, off(va + 8 * i))[0]
            s = cstr(v)
            print(hex(va + 8 * i), hex(v), repr(s.decode()) if s else '')
    else:
        va, _, ln = a.partition(':')
        va, ln = int(va, 16), int(ln or '16', 0)
        o = off(va)
        print(hex(va), d[o:o + ln].hex(' ') if o is not None else '?')

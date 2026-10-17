#!/usr/bin/env python3
"""Find aarch64 code that builds the address of given strings (adrp + add) in EP147.

usage: xref.py <elf> <objdump -d output> <string> [<string> ...]
Prints, for each string: its virtual address and every instruction pair that
materializes it, with the enclosing function start (nearest preceding label or
stp x29, x30 prologue).
"""
import re
import struct
import sys

elf, dis, *needles = sys.argv[1:]
data = open(elf, 'rb').read()

# program headers -> file offset to vaddr
e_phoff = struct.unpack_from('<Q', data, 0x20)[0]
e_phentsize, e_phnum = struct.unpack_from('<HH', data, 0x36)
segs = []
for i in range(e_phnum):
    o = e_phoff + i * e_phentsize
    p_type, _, p_offset, p_vaddr, _, p_filesz = struct.unpack_from('<IIQQQQ', data, o)
    if p_type == 1:
        segs.append((p_offset, p_vaddr, p_filesz))


def vaddr(off):
    for so, sv, sz in segs:
        if so <= off < so + sz:
            return sv + off - so
    return None


targets = {}
for n in needles:
    b = n.encode() + b'\0'
    start = 0
    while True:
        i = data.find(b, start)
        if i < 0:
            break
        # must be the start of a string
        if i == 0 or data[i - 1] == 0:
            va = vaddr(i)
            if va:
                targets[va] = n
                print(f'string {n!r} at {va:#x}')
        start = i + 1

adrp_re = re.compile(r'^\s*([0-9a-f]+):\s+adrp\s+(x\d+), ([0-9a-f]+)')
add_re = re.compile(r'^\s*([0-9a-f]+):\s+add\s+(x\d+), (x\d+), #0x([0-9a-f]+)')
page_of = {va & ~0xfff: va for va in targets}
pending = {}  # reg -> (page, addr)
last_func = None
prologue_re = re.compile(r'^\s*([0-9a-f]+):\s+stp\s+x29, x30, \[sp, #-')
with open(dis) as f:
    for line in f:
        m = prologue_re.match(line)
        if m:
            last_func = int(m.group(1), 16)
        m = adrp_re.match(line)
        if m:
            page = int(m.group(3), 16)
            if any(page == pg for pg in page_of):
                pending[m.group(2)] = page
            else:
                pending.pop(m.group(2), None)
            continue
        m = add_re.match(line)
        if m and m.group(3) in pending:
            va = pending[m.group(3)] + int(m.group(4), 16)
            if va in targets:
                fn = f'{last_func:#x}' if last_func else '?'
                print(f'  {targets[va]!r}: code at {m.group(1)} (func ~{fn})')

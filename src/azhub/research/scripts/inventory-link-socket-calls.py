#!/usr/bin/env python3
"""Inventory direct AArch64 socket PLT calls without exporting disassembly.

This is a conservative local instruction-window analysis, not a control-flow
proof. Unknown arguments stay unknown. Firmware and objdump text stay local.
"""
import argparse
from collections import deque
import hashlib
import json
from pathlib import Path
import re
import subprocess

INSTRUCTION = re.compile(r'^\s*([0-9a-f]+):\s+[0-9a-f]{8}\s+([\w.]+)\s*(.*)$')
REGISTER = re.compile(r'^[wx](\d+)$')
TARGETS = {'setsockopt': {'level': 1, 'option': 2, 'length': 4},
           'getsockopt': {'level': 1, 'option': 2},
           'recvfrom': {'buffer_size': 2, 'flags': 3},
           'recvmsg': {'flags': 2},
           'recvmmsg': {'flags': 3},
           'recv': {'buffer_size': 2, 'flags': 3},
           'read': {'buffer_size': 2}, 'readv': {'iov_count': 2},
           'syscall': {'number': 0}}

def register(value):
    match = REGISTER.fullmatch(value.strip())
    return int(match[1]) if match else None

def argument(window, number):
    """Resolve only straight-line MOV chains after the last control transfer."""
    bits = 64
    for _, operation, operands in reversed(window):
        if operation in {'b', 'bl', 'br', 'blr', 'ret', 'cbz', 'cbnz', 'tbz', 'tbnz'} or operation.startswith('b.'):
            return None
        parts = [part.strip() for part in operands.split('//')[0].split(',')]
        if operation != 'mov':
            # Multi-destination loads can write a non-first operand. Reject
            # any unmodeled mention instead of guessing destination semantics.
            if re.search(r'\b[wx]' + str(number) + r'\b', operands):
                return None
            continue
        if not parts or register(parts[0]) != number:
            continue
        if operation != 'mov' or len(parts) != 2:
            # Includes stores using the register: conservative unknown is safer
            # than assuming every unmodeled instruction preserves its value.
            return None
        source = parts[1]
        if parts[0].startswith('w'):
            bits = 32
        if source in {'wzr', 'xzr'}:
            return 0
        if source.startswith('#'):
            try:
                return int(source[1:], 0) & ((1 << bits) - 1)
            except ValueError:
                return None
        number = register(source)
        if number is None:
            return None
    return None

def scan(lines):
    calls, imports, window = [], {}, deque(maxlen=32)
    for line in lines:
        label = re.match(r'^([0-9a-f]+) <(\w+)@plt>:', line)
        if label and label[2] in TARGETS:
            imports[label[2]] = '0x' + label[1].lstrip('0')
        match = INSTRUCTION.match(line)
        if not match:
            continue
        address, operation, operands = match.groups()
        target = re.search(r'<(\w+)@plt>', operands)
        if operation in {'bl', 'b'} and target and target[1] in TARGETS:
            name = target[1]
            values = {key: argument(window, reg) for key, reg in TARGETS[name].items()}
            calls.append({'address': '0x' + address, 'function': name, 'transfer': operation, **values})
        window.append((address, operation, operands))
    return calls, imports

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('elf', type=Path)
    parser.add_argument('--objdump', default='aarch64-linux-gnu-objdump')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    digest = hashlib.sha256()
    with args.elf.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    process = subprocess.Popen([args.objdump, '-d', str(args.elf)], stdout=subprocess.PIPE,
                               stderr=subprocess.DEVNULL, text=True)
    calls, imports = scan(process.stdout)
    if process.wait():
        raise SystemExit('objdump failed; inventory not written')
    args.output.write_text(json.dumps({'elf_sha256': digest.hexdigest(), 'imports': imports,
        'calls': calls, 'limits': ['Direct BL/B-to-PLT transfers only; indirect calls, inline syscalls, libc and other modules are not covered',
        'Argument values are conservative local MOV-chain findings, not whole-program control-flow proof',
        'Unknown level/option/flags must not be treated as absence of an operation']}, indent=2)+'\n', encoding='utf-8')
    print(f'Inventoried {len(calls)} direct socket calls; no raw disassembly exported')

if __name__ == '__main__':
    main()

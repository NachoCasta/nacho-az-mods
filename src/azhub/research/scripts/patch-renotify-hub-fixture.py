#!/usr/bin/env python3
"""Reconstruct an emulator-only renotifier fixture, preserving the original.

Usage: patch-renotify-hub-fixture.py ORIGINAL OUTPUT
The emulator's box_of ancestor match otherwise announces hub storage as native
USB2, opening an extra native library server and invalidating isolation tests.
"""
from pathlib import Path
import sys

source, target = map(Path, sys.argv[1:])
text = source.read_text(encoding="utf-8")
marker = "box_of() {\n"
assert text.count(marker) == 1
assert "Emulator hub fixture" not in text
insert = '''    # Emulator hub fixture: nested storage belongs to USB Hub Support.
    udevadm info --query=path --name="$1" 2>/dev/null | grep -qE '/[0-9]+-[0-9]+[.]' && return 0
'''
target.write_text(text.replace(marker, marker + insert), encoding="utf-8", newline="\n")

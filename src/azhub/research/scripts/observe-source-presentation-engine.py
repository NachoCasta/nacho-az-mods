#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Read bounded published state from the exact existing GPL emulator core.

No injection, debugger attachment, process pause, memory write, deployment or
controller access. Offsets are verified against this core's exported snapshot
and play-observer implementations; every other core binary is refused.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import time

CORE_SHA256 = '3e0205320a0dda69a72e1de5070be82c89fb9e73ec21ce3f561c1dbb23d60ea6'
STATE_OFFSET = 0x363c8
PLAYHEAD_OFFSET = 0x3d758
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--samples', type=int, default=8)
p.add_argument('--advancing-deck', type=int, choices=(1, 2, 3, 4))
p.add_argument('--mod-binary', type=Path, help='Also observe bounded own-mod cache tickets from this exact ELF')
args = p.parse_args()
assert 2 <= args.samples <= 32
root = Path(__file__).resolve().parents[4]
assert args.output.resolve().is_relative_to(root / 'local')
spec = importlib.util.spec_from_file_location('guarded_emulator', Path(__file__).with_name('eject-ui-emu.py'))
emu = importlib.util.module_from_spec(spec)
spec.loader.exec_module(emu)
def guest(command):
    rc, text = emu.azemu.ssh(command, timeout=30)
    assert rc == 0, ('Emulator read failed', rc)
    return text
assert guest('uname -r').strip() == '6.6.138-dirty'
pid = guest('pidof EP147').strip()
assert pid.isdigit()
assert guest('readlink /proc/' + pid + '/exe').strip().endswith('/EP147')
rows = guest('cat /proc/' + pid + '/maps').splitlines()
row = next(row.split() for row in rows if '/azstem.so' in row and row.split()[2] == '00000000')
assert guest('sha256sum /proc/' + pid + '/map_files/' + row[0]).split()[0] == CORE_SHA256
base = int(row[0].split('-')[0], 16)
def read(offset, size):
    data = guest('dd if=/proc/' + pid + '/mem bs=1 skip=' + str(base + offset) +
                 ' count=' + str(size) + ' 2>/dev/null | od -v -b')
    result = bytes(int(value, 8) for line in data.splitlines() for value in line.split()[1:])
    assert len(result) == size
    return result
samples = []
for index in range(args.samples):
    assert guest('pidof EP147').strip() == pid
    states = read(STATE_OFFSET, 4).decode('ascii')
    assert all(value in 'ELU' for value in states)
    positions = struct.unpack('<4q', read(PLAYHEAD_OFFSET, 32))
    samples.append({'states': states, 'playhead_samples': positions,
                    'position_seconds': [value/44100 if value >= 0 else -1 for value in positions]})
    if index + 1 < args.samples: time.sleep(.35)
result = {'pid': int(pid), 'core_sha256': CORE_SHA256, 'sample_rate': 44100,
          'samples': samples, 'audio_signal_captured': False, 'read_only': True}
if args.mod_binary:
    binary = args.mod_binary.resolve()
    assert binary.is_relative_to(root / 'local')
    raw = binary.read_bytes()
    assert raw[:6] == b'\x7fELF\x02\x01'
    digest = hashlib.sha256(raw).hexdigest()
    mapping = next(row.split() for row in rows if '/usb-hub-support.so' in row and row.split()[2] == '00000000')
    assert guest('sha256sum /proc/' + pid + '/map_files/' + mapping[0]).split()[0] == digest
    section_offset = struct.unpack_from('<Q', raw, 40)[0]
    section_size, section_count = struct.unpack_from('<HH', raw, 58)
    assert section_size == 64 and section_count < 256
    sections = [struct.unpack_from('<IIQQQQIIQQ', raw, section_offset + i*section_size) for i in range(section_count)]
    symbols = {}
    for section in sections:
        if section[1] != 2: continue
        assert section[9] == 24
        strings = sections[section[6]]
        for offset in range(section[4], section[4]+section[5], section[9]):
            name, _, _, _, value, length = struct.unpack_from('<IBBHQQ', raw, offset)
            start = strings[4]+name
            name = raw[start:raw.index(b'\0', start)].decode()
            if name in ('safe_cached_decks', 'safe_detach_pool'): symbols[name] = (value, length)
    # Current AArch64 source layouts: cached deck32 bytes, detach ticket48.
    assert symbols['safe_cached_decks'][1] == 4*32
    assert symbols['safe_detach_pool'][1] == 16*48
    base = int(mapping[0].split('-')[0], 16)
    decks = read(*symbols['safe_cached_decks'])
    tickets = read(*symbols['safe_detach_pool'])
    proofs = []
    for deck in range(4):
        controller, epoch, content, valid, ticket = struct.unpack_from('<QQIii', decks, deck*32)
        if not valid: continue
        assert 0 <= ticket < 16
        owner, media, connection, fence, references, phase, submitted = struct.unpack_from('<QQQQIii', tickets, ticket*48)
        proofs.append({'deck': deck+1, 'epoch': epoch, 'content_id': content,
                       'media': hex(media), 'controller_present': bool(controller),
                       'owner_present': bool(owner), 'connection_present': bool(connection),
                       'references': references, 'phase': phase, 'submitted': submitted})
    assert guest('pidof EP147').strip() == pid
    result['own_mod_cache'] = {'sha256': digest, 'proofs': proofs,
                              'scope': 'Own retained cache-ticket metadata; no proprietary memory traversal'}
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(json.dumps(result, indent=2), encoding='utf-8')
if args.advancing_deck:
    deck = args.advancing_deck - 1
    assert all(value['states'][deck] == 'L' for value in samples), 'Expected loaded deck'
    positions = [value['position_seconds'][deck] for value in samples]
    assert min(positions) >= 0 and max(positions)-min(positions) > .05, 'Playhead did not advance'
print('PASS: bounded exact-core published-state observation, unchanged PID=' + pid)

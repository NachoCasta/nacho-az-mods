#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Owned native USB1 withdrawal helper for the isolated loaded-status runner.

No standalone entry point, installation or physical-device endpoint. Callers
must keep the synthetic status and real NFS handle alive through withdrawal.
"""
import json
from pathlib import Path
import subprocess
import sys
import time


class NativeWithdrawal:
    def __init__(self, emulator, guest, output, application_pid):
        assert emulator.azemu.SSH_PORT == 2222
        assert emulator.azemu.QMP == ('127.0.0.1', 4446)
        self.emulator, self.guest, self.output = emulator, guest, output
        self.pid = application_pid.strip()
        assert self.pid.isdigit()
        self.requested = False
        self.detached = False
        self.readded = False
        self.events = []
        assert guest('uname -r').strip() == '6.6.138-dirty'
        assert guest('readlink -f /C').strip() == '/media/usb/sdb1'
        assert '/usb1/1-1/' in guest('readlink -f /sys/class/block/sdb/device')
        assert '/usb2/2-1/' in guest('readlink -f /sys/class/block/sda/device')
        guest('test -d /media/usb/sda1/MODS/community-mods/usb-hub-support')
        assert emulator.qmp('qom-get', {'path': '/machine/peripheral/nativecontrol2', 'property': 'parent_bus'})['return'].endswith('/ehci.0')
        assert emulator.qmp('qom-get', {'path': '/machine/peripheral/nativecontrol2', 'property': 'port'})['return'] == '1'
        blocks = emulator.qmp('query-block', {})['return']
        assert any(row.get('qdev', '').startswith('/machine/peripheral/nativecontrol2/') and
                   row.get('inserted', {}).get('node-name') == 'sharing-native-music-v1' for row in blocks)
        self.original = self.mounts()
        assert all('/media/usb/sd'+letter+'1' in self.original for letter in 'abcdefg')
        self.pdb_hash = guest('sha256sum /media/usb/sdb1/PIONEER/rekordbox/export.pdb').split()[0]
        self.note('guards_passed', fixture='sdb1', mods='sda1', native_port=1)

    def note(self, event, **values):
        self.events.append(dict(event=event, monotonic_ns=time.monotonic_ns(), **values))
        (self.output/'native-withdrawal.json').write_text(json.dumps(self.events, indent=2)+'\n')

    def mounts(self):
        return {line.split()[1]: line for line in self.guest('cat /proc/mounts').splitlines()
                if len(line.split()) >= 2 and line.split()[1].startswith('/media/usb/')}

    def neighbors(self):
        current = self.mounts()
        assert all(current.get(path) == row for path, row in self.original.items()
                   if path != '/media/usb/sdb1'), 'Native neighbor/hub mount changed'

    def withdraw(self):
        logs = self.guest('cat /tmp/azmod-community/usb-hub-support.log.1 /tmp/azmod-community/usb-hub-support.log 2>/dev/null || true')
        marker = 'native-status-observer installed pid='+self.pid+';'
        assert marker in logs
        current = logs.rsplit(marker, 1)[1]
        assert 'native-status-observer key=3:1:4:3 accepted=1 b0=3 b1=0 b4=1 b5=3 b6=1 word8_le=1884 ' in current
        (self.output/'native-accepted-before-stop.log').write_text(current)
        self.requested = True
        self.note('STOP1_requested_with_native_accepted_loaded_status')
        subprocess.run([sys.executable, str(Path(__file__).with_name('eject-ui-emu.py')), 'stop', '1', '1.7'], check=True, timeout=40)
        deadline = time.monotonic()+15
        while '/media/usb/sdb1' in self.mounts():
            self.neighbors()
            assert time.monotonic() < deadline, 'Native STOP1 did not unmount fixture'
            time.sleep(.2)
        self.neighbors()
        self.note('fixture_unmounted_neighbors_preserved')

    def restore(self):
        if not self.requested:
            return
        if not self.detached and '/media/usb/sdb1' in self.mounts():
            self.note('fixture_still_mounted_no_replug_needed')
            self.requested = False
            return
        if not self.detached:
            assert 'return' in self.emulator.qmp('device_del', {'id': 'nativecontrol2'})
            self.detached = True
        if not self.readded:
            deadline = time.monotonic()+15
            while self.guest('if [ -e /sys/bus/usb/devices/1-1 ]; then echo present; fi').strip():
                assert time.monotonic() < deadline
                time.sleep(.2)
            assert 'return' in self.emulator.qmp('device_add', {'driver': 'usb-storage', 'id': 'nativecontrol2',
                'drive': 'sharing-native-music-v1', 'bus': 'ehci.0', 'port': '1'})
            self.readded = True
        deadline = time.monotonic()+30
        while '/media/usb/sdb1' not in self.mounts():
            assert time.monotonic() < deadline, 'Owned native fixture did not remount'
            time.sleep(.2)
        assert self.guest('sha256sum /media/usb/sdb1/PIONEER/rekordbox/export.pdb').split()[0] == self.pdb_hash
        self.neighbors()
        self.note('owned_native_fixture_restored_hash_and_neighbors_verified')
        self.requested = self.detached = self.readded = False

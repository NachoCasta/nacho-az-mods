#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Check an already activated emulator runtime with isolated independent clients.

Guest operations are read-only. This does not install, approve, reload or edit
staging/settings. Host TAP/namespace and temporary QEMU transport are owned and
cleaned by this bounded check. Physical controllers are never contacted.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import sys
import time

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--classes', type=Path, required=True)
p.add_argument('--expected-runtime', required=True)
p.add_argument('--evidence-directory', type=Path, required=True)
p.add_argument('--indices', default='0,4')
p.add_argument('--files', action='store_true', help='Also fetch exact PDB/audio from selected hub sources and native USB1')
args = p.parse_args()
assert re.fullmatch('[0-9a-f]{64}', args.expected_runtime)
indices = [int(i) for i in args.indices.split(',')]
assert 2 <= len(indices) <= 8 and len(set(indices)) == len(indices) and all(0 <= i < 8 for i in indices)
root = Path(__file__).resolve().parents[4]
out = args.evidence_directory.resolve()
assert out.is_relative_to(root / 'local')
out.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location('guarded_emulator', Path(__file__).with_name('eject-ui-emu.py'))
emu = importlib.util.module_from_spec(spec)
spec.loader.exec_module(emu)

def guest(command):
    for attempt in range(3):
        rc, result = emu.azemu.ssh(command, timeout=30)
        if rc == 0: return result
        if rc != 255: break
        time.sleep(.5)
    raise AssertionError((command, rc, result))

def host(*command):
    return subprocess.run(['wsl.exe', '-u', 'root', '--exec', *command],
                          capture_output=True, text=True, check=True, timeout=30).stdout

assert guest('uname -r').strip() == '6.6.138-dirty'
pid = guest('pidof EP147').strip()
assert pid.isdigit()
maps = guest('cat /proc/' + pid + '/maps')
entry = next(row.split()[0] for row in maps.splitlines() if '/usb-hub-support.so' in row)
assert guest('sha256sum /proc/' + pid + '/map_files/' + entry).split()[0] == args.expected_runtime
logs = guest('cat /tmp/azmod-community/usb-hub-support.log')
launch = re.findall(r'public-sharing managed launch pid=(\d+)', logs)[-1]
command = guest('cat /proc/' + launch + '/cmdline').split('\0')
assert command[1] == '--broker-candidate-emulator' and command[4] == pid
assert re.fullmatch('[0-9a-f]{32}', command[3])
helper_log = command[0].rsplit('/assets/', 1)[0] + '/data/public-sharing-' + command[3] + '.log'
helper_text = guest('cat ' + helper_log)
addresses = {int(slot): address for slot, address in re.findall(
    r'public source sockets ready interface=azpub(\d+) address=(192\.0\.2\.\d+)', helper_text)}
assert all(i in addresses for i in indices), addresses
assert all('public source library ready player=' + str(41+i) in helper_text for i in indices)
assert len({addresses[i] for i in indices}) == len(indices)
assert not host('ip', 'netns', 'list').strip(), 'Existing host namespaces must remain untouched'
base = '/mnt/' + root.drive[0].lower() + root.as_posix()[2:]
classpath = base + '/' + args.classes.resolve().relative_to(root).as_posix() + ':' + base + '/local/link-peer-build/dependencies/*'
oracle = root / 'local/diagnostics/sharing-hub0-export.pdb'
oracle_hash = hashlib.sha256(oracle.read_bytes()).hexdigest()
namespace = False
bridge = None
stop_file = out / 'transport.stop'
assert not stop_file.exists(), 'Use a fresh evidence directory'
assert not any(v.get('label') == 'azhubtapnat' for v in emu.qmp('query-chardev', {})['return'])
assert not any(v.get('name') == 'azhubtapguard' for v in emu.qmp('qom-list', {'path':'/objects'})['return'])
children = []
streams = []
result = {'runtime_sha256': args.expected_runtime, 'pid': int(pid), 'indices': indices,
          'addresses': addresses, 'oracle_sha256': oracle_hash, 'physical_AZ_touched': False}
try:
    host('ip', 'netns', 'add', 'azhub-link'); namespace = True
    host('ip', 'netns', 'exec', 'azhub-link', 'ip', 'tuntap', 'add', 'dev', 'azlink0', 'mode', 'tap')
    host('ip', '-n', 'azhub-link', 'link', 'set', 'lo', 'up')
    host('ip', '-n', 'azhub-link', 'addr', 'add', '192.0.2.2/24', 'brd', '192.0.2.255', 'dev', 'azlink0')
    host('ip', '-n', 'azhub-link', 'link', 'set', 'azlink0', 'address', '02:00:00:00:02:02')
    host('ip', '-n', 'azhub-link', 'link', 'set', 'azlink0', 'up')
    def launch_child(command, name):
        stdout = (out / (name + '.stdout')).open('w', encoding='utf-8')
        stderr = (out / (name + '.stderr')).open('w', encoding='utf-8')
        streams.extend([stdout, stderr])
        child = subprocess.Popen(command, stdout=stdout, stderr=stderr)
        children.append(child)
        return child
    bridge = launch_child([sys.executable, str(Path(__file__).with_name('link-primary-tap-transport.py')),
                           '--seconds', '240', '--stop-file', str(stop_file)], 'transport')
    deadline = time.monotonic() + 20
    while '"ready": true' not in (out / 'transport.stdout').read_text(encoding='utf-8'):
        assert bridge.poll() is None and time.monotonic() < deadline, 'Transport readiness failed'
        time.sleep(.2)
    print('ISOLATED_TRANSPORT_READY', flush=True)
    prefix = ['wsl.exe', '-u', 'root', '--exec', 'ip', 'netns', 'exec', 'azhub-link', 'java',
        '-Xmx192m', '-XX:ActiveProcessorCount=2',
        '-Dazhub.testDhcpFixture=true',
        *['-Dazhub.testSourceAddress' + str(i) + '=' + addresses[i] for i in indices],
        '-cp', classpath]
    client = launch_child(prefix + ['LiveProvidersControl', base + '/local/diagnostics/sharing-hub0-export.pdb',
        base + '/local/diagnostics/sharing-pdb-tables-v1/java-entities.tsv', '--indices=' + args.indices], 'clients')
    checks = [client]
    if args.files:
        files = [('PIONEER/rekordbox/export.pdb', oracle),
                 ('Contents/Aaliyah/Are You That Somebody/Aaliyah - Are You That Somebody-1.mp3',
                  root / 'local/diagnostics/sharing-hub0-audio.mp3')]
        for label, flag in [('hub' + str(i+1), '--emulator-link-' + str(i+1)) for i in indices] + [('native1', '--emulator-native-usb1')]:
            for number, (relative, artifact) in enumerate(files):
                checks.append(launch_child(prefix + ['LocalFileServiceClient', relative,
                    str(artifact.stat().st_size), hashlib.sha256(artifact.read_bytes()).hexdigest(), flag],
                    label + '-file' + str(number)))
    deadline = time.monotonic() + 190
    for check in checks:
        assert check.wait(timeout=max(1, deadline-time.monotonic())) == 0, 'Independent client failed; inspect client logs'
    assert guest('pidof EP147').strip() == pid
    result['menus'] = 'pass'
    if args.files: result['exact_file_fetches'] = 2 * (len(indices) + 1)
    print('PASS: attached exact runtime, independent complete menus, unchanged EP147 PID', flush=True)
finally:
    if bridge and bridge.poll() is None:
        stop_file.write_text('stop\n', encoding='utf-8')
        try: bridge.wait(timeout=20)
        except subprocess.TimeoutExpired:
            # Restore NAT before any forced process termination. Preflight
            # refused pre-existing objects, so these belong to this trial.
            objects = emu.qmp('qom-list', {'path':'/objects'})['return']
            if any(v.get('name') == 'azhubtapguard' for v in objects):
                emu.qmp('object-del', {'id':'azhubtapguard'})
    for child in reversed(children):
        if child.poll() is None:
            child.terminate()
            try: child.wait(timeout=10)
            except subprocess.TimeoutExpired: child.kill(); child.wait(timeout=10)
    for stream in streams: stream.close()
    objects = emu.qmp('qom-list', {'path':'/objects'})['return']
    if any(v.get('name') == 'azhubtapguard' for v in objects):
        emu.qmp('object-del', {'id':'azhubtapguard'})
    if any(v.get('label') == 'azhubtapnat' for v in emu.qmp('query-chardev', {})['return']):
        emu.qmp('chardev-remove', {'id':'azhubtapnat'})
    if namespace:
        # The Windows WSL launcher may exit before its terminated Linux child
        # has completed teardown. Observe those owned children before deciding
        # that namespace cleanup failed; never kill an unverified Linux PID.
        until = time.monotonic() + 10
        while True:
            remaining = host('ip', 'netns', 'pids', 'azhub-link').strip()
            if not remaining or time.monotonic() >= until:
                break
            time.sleep(.2)
        assert not remaining, ('Owned namespace still has processes', remaining)
        host('ip', 'netns', 'del', 'azhub-link')
        result['host_namespace_removed'] = True
    (out / 'result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')

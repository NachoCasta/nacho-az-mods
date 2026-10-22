#!/usr/bin/env python3
"""Explicit localhost-only owned-network component control and scoped module cleanup."""
import argparse
import hashlib
import json
import sys
import time
import uuid
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe', type=Path, required=True)
    parser.add_argument('--module', type=Path, required=True)
    parser.add_argument('--evidence-directory', type=Path, required=True)
    args = parser.parse_args()
    sys.path.insert(0, 'C:/Users/icast/xdj-az-emu/tools')
    import azemu as az
    assert az.SSH_PORT == 2222 and az.QMP == ('127.0.0.1', 4446)
    out = args.evidence_directory
    out.mkdir(parents=True, exist_ok=False)
    commands = []

    def run(text, check=True):
        code, output = az.ssh(text + ' 2>&1', timeout=30)
        commands.append({'command': text, 'exit': code, 'output': output})
        if check:
            assert code == 0, (text, code, output)
        return code, output.strip()

    def state():
        return {name: run(command)[1] for name, command in {
            'pid': 'pidof EP147', 'address': 'ip -o -4 addr show dev eth0',
            'routes': 'ip -4 route show', 'dns': 'sha256sum /etc/resolv.conf',
            'namespace': 'ls -Lid /proc/self/ns/net',
        }.items()}

    remote = '/tmp/azhub-owned-network-' + uuid.uuid4().hex
    module = False
    try:
        assert run('uname -r')[1] == '6.6.138-dirty'
        assert not run('ip netns list')[1]
        assert 'azhub_test_macvlan' not in run('cat /proc/modules')[1]
        assert hashlib.sha256(args.module.read_bytes()).hexdigest() == 'd125c73ce850c91a8485f32a4ef52ea318f3fd5b1d7eb712f18c510654e268ad'
        before = state()
        for local, target in ((args.probe, remote), (args.module, remote + '.ko')):
            assert az.push_file(str(local.resolve()), target)
            assert run('sha256sum ' + target)[1].split()[0] == hashlib.sha256(local.read_bytes()).hexdigest()
        run('insmod ' + remote + '.ko')
        module = True
        run('chmod 700 ' + remote)
        output = run(remote + ' --emulator')[1]
        assert 'PASS:' in output
        after = state()
        assert before == after
        assert not run('ip netns list')[1]
        (out / 'result.json').write_text(json.dumps({
            'output': output, 'probe_sha256': hashlib.sha256(args.probe.read_bytes()).hexdigest(),
            'native_before': before, 'native_after': after, 'physical_AZ_touched': False,
        }, indent=2) + '\n', encoding='utf-8')
    finally:
        try:
            if module:
                deadline = time.monotonic() + 5
                while run('rmmod azhub_test_macvlan', False)[0]:
                    assert time.monotonic() < deadline
                    time.sleep(.1)
            run('rm -f ' + remote + ' ' + remote + '.ko')
        finally:
            (out / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n', encoding='utf-8')
    print('PASS: owned network control, native state preserved and module/temp cleanup complete')


if __name__ == '__main__':
    main()

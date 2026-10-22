#!/usr/bin/env python3
"""Explicit localhost-emulator-only exact netlink address removal control."""
import argparse
import hashlib
import json
import sys
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
    evidence = args.evidence_directory
    evidence.mkdir(parents=True, exist_ok=False)
    history = []

    def command(text):
        code, output = az.ssh(text + ' 2>&1', timeout=30)
        history.append({'command': text, 'code': code, 'output': output})
        assert code == 0, (text, code, output)
        return output.strip()

    def snapshot():
        return {key: command(value) for key, value in {
            'pid': 'pidof EP147', 'routes': 'ip -4 route show',
            'addresses': 'ip -o -4 addr show dev eth0',
            'dns': 'sha256sum /etc/resolv.conf',
            'namespace': 'ls -Lid /proc/self/ns/net',
        }.items()}

    token = uuid.uuid4().hex[:12]
    ns = 'azhub-rm-' + token
    probe = '/tmp/azhub-rm-probe-' + token
    module_path = '/tmp/azhub-rm-module-' + token + '.ko'
    module = scope = link = False
    try:
        assert command('uname -r') == '6.6.138-dirty'
        assert not command('ip netns list')
        assert 'azhub_test_macvlan' not in command('cat /proc/modules')
        assert hashlib.sha256(args.module.read_bytes()).hexdigest() == 'd125c73ce850c91a8485f32a4ef52ea318f3fd5b1d7eb712f18c510654e268ad'
        before = snapshot()
        assert az.push_file(str(args.probe.resolve()), probe)
        assert az.push_file(str(args.module.resolve()), module_path)
        for local, remote in ((args.probe, probe), (args.module, module_path)):
            assert command('sha256sum ' + remote).split()[0] == hashlib.sha256(local.read_bytes()).hexdigest()
        command('chmod 700 ' + probe)
        command('insmod ' + module_path)
        module = True
        command('ip netns add ' + ns)
        scope = True
        command('ip link add link eth0 name azpub7 address 02:00:00:00:12:73 type macvlan mode bridge')
        link = True
        command('ip link set azpub7 netns ' + ns)
        link = False
        command('ip netns exec ' + ns + ' ip link set azpub7 up')
        for host in (3, 4):
            command('ip netns exec ' + ns + ' ip addr add 192.0.2.' + str(host) + '/24 broadcast 192.0.2.255 dev azpub7')
        addresses_before = command('ip netns exec ' + ns + ' ip -o -4 addr show dev azpub7')
        assert '192.0.2.3/24' in addresses_before and '192.0.2.4/24' in addresses_before
        output = command(probe + ' --live /var/run/netns/' + ns)
        assert 'PASS: kernel-acknowledged' in output
        addresses_after = command('ip netns exec ' + ns + ' ip -o -4 addr show dev azpub7')
        assert '192.0.2.3/24' not in addresses_after and '192.0.2.4/24' in addresses_after
        after = snapshot()
        assert before == after, (before, after)
        (evidence / 'result.json').write_text(json.dumps({
            'output': output, 'addresses_before': addresses_before, 'addresses_after': addresses_after,
            'native_before': before, 'native_after': after,
            'probe_sha256': hashlib.sha256(args.probe.read_bytes()).hexdigest(),
            'physical_AZ_touched': False,
        }, indent=2) + '\n', encoding='utf-8')
    finally:
        try:
            if link:
                command('ip link del azpub7')
            if scope:
                command('ip netns del ' + ns)
            if module:
                command('rmmod azhub_test_macvlan')
            command('rm -f ' + probe + ' ' + module_path)
            assert not command('ip netns list')
            assert 'azhub_test_macvlan' not in command('cat /proc/modules')
        finally:
            (evidence / 'commands.json').write_text(json.dumps(history, indent=2) + '\n', encoding='utf-8')
    print('PASS: exact address removed, neighbor retained, native state unchanged, owned scope cleaned')


if __name__ == '__main__':
    main()

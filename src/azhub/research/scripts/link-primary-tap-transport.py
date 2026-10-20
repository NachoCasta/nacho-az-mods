#!/usr/bin/env python3
"""Bounded localhost QEMU/TAP bridge preserving NAT management and peer TCP.

Requires the prepared isolated azhub-link namespace and QEMU mirror/injection
channels. A temporary QEMU ingress redirect suppresses NAT packets claiming the
TAP peer's address; otherwise slirp resets valid peer TCP sessions. Other NAT
traffic, including SSH, returns through the existing guest injection channel.
No guest files, NICs, addresses, firmware or application processes are changed.
"""
import argparse
import ipaddress
import json
from pathlib import Path
import socket
import struct
import subprocess
import threading
import time


class Frames:
    def __init__(self):
        self.pending = bytearray()

    def feed(self, data):
        self.pending.extend(data)
        frames = []
        while len(self.pending) >= 4:
            size = struct.unpack('!I', self.pending[:4])[0]
            if not 14 <= size <= 65536:
                raise ValueError('Invalid Ethernet frame size')
            if len(self.pending) < size + 4:
                break
            frames.append(bytes(self.pending[4:size + 4]))
            del self.pending[:size + 4]
        return frames

    def finish(self):
        if self.pending:
            raise ValueError('Truncated framed stream')


def framed(frame):
    if not 14 <= len(frame) <= 65536:
        raise ValueError('Invalid Ethernet frame size')
    return struct.pack('!I', len(frame)) + frame


def owned_peer(frame):
    """Only the TAP owns192.0.2.2; NAT must not impersonate it."""
    if len(frame) >= 34 and frame[12:14] == b'\x08\x00':
        ihl = (frame[14] & 15) * 4
        total = int.from_bytes(frame[16:18], 'big')
        if frame[14] >> 4 == 4 and ihl >= 20 and ihl <= total <= len(frame) - 14:
            return frame[26:30] == bytes([192, 0, 2, 2])
    if len(frame) >= 42 and frame[12:14] == b'\x08\x06':
        if frame[14:20] == b'\x00\x01\x08\x00\x06\x04':
            return frame[28:32] == bytes([192, 0, 2, 2])
    return False


def verify_namespace(rows):
    peers = [row for row in rows if row.get('ifname') != 'lo']
    if len(peers) != 1 or peers[0].get('ifname') != 'azlink0':
        raise ValueError('Expected only lo and isolated azlink0')
    peer = peers[0]
    if 'UP' not in peer.get('flags', []):
        raise ValueError('TAP must already be up')
    addresses = [(a.get('local'), a.get('prefixlen'), a.get('broadcast'))
                 for a in peer.get('addr_info', []) if a.get('family') == 'inet']
    if addresses != [('192.0.2.2', 24, '192.0.2.255')]:
        raise ValueError('Unexpected isolated peer address')
    for address in peer.get('addr_info', []):
        if address.get('family') == 'inet6' and not ipaddress.ip_address(address['local']).is_link_local:
            raise ValueError('Unexpected global IPv6 address')


def qmp(command, arguments=None):
    # Fixed automation endpoint; never connect to a physical controller.
    with socket.create_connection(('127.0.0.1', 4446), timeout=5) as connection:
        connection.settimeout(5)
        stream = connection.makefile('rwb')
        stream.readline()
        for request in ({'execute': 'qmp_capabilities'},
                        {'execute': command, 'arguments': arguments or {}}):
            stream.write(json.dumps(request).encode() + b'\n')
            stream.flush()
            while True:
                line = stream.readline()
                if not line:
                    raise EOFError('QMP closed before reply')
                result = json.loads(line)
                if 'error' in result:
                    raise RuntimeError((command, result['error']))
                if 'return' in result:
                    break
        return result['return']


def write_all(stream, data):
    remaining = memoryview(data)
    while remaining:
        count = stream.write(remaining)
        if not count:
            raise BrokenPipeError('TAP input closed')
        remaining = remaining[count:]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seconds', type=int, required=True)
    parser.add_argument('--capture-port', type=int, default=15101)
    parser.add_argument('--inject-port', type=int, default=15102)
    parser.add_argument('--nat-port', type=int, default=15103)
    args = parser.parse_args()
    ports = [args.capture_port, args.inject_port, args.nat_port]
    if not 5 <= args.seconds <= 3600 or len(set(ports)) != 3 or any(not 1024 <= p <= 65535 for p in ports):
        parser.error('Use5..3600 seconds and three distinct unprivileged loopback ports')
    script = Path(__file__).with_name('link-tap-stdio.py').resolve()
    if script.drive.upper() != 'C:':
        parser.error('Review explicit WSL path mapping before using another drive')
    prefix = ['wsl.exe', '-u', 'root', '-e', 'ip', 'netns', 'exec', 'azhub-link']
    check = subprocess.run(prefix + ['ip', '-j', 'addr', 'show'], capture_output=True, check=True)
    verify_namespace(json.loads(check.stdout))
    channel, guard = 'azhubtapnat', 'azhubtapguard'
    if any(row.get('label') == channel for row in qmp('query-chardev')):
        raise RuntimeError('Existing NAT channel requires owner inspection; never replace it')
    sockets, threads = [], []
    created = installed = False
    child = timer = None
    stopped = threading.Event()
    inject_lock = threading.Lock()
    errors = []
    counts = dict(guest_to_tap=0, tap_to_guest=0, nat_to_guest=0, nat_peer_suppressed=0)
    deadline = time.monotonic() + args.seconds
    try:
        capture = socket.create_connection(('127.0.0.1', args.capture_port), timeout=5)
        sockets.append(capture)
        inject = socket.create_connection(('127.0.0.1', args.inject_port), timeout=5)
        sockets.append(inject)
        qmp('chardev-add', {'id': channel, 'backend': {'type': 'socket', 'data': {
            'addr': {'type': 'inet', 'data': {'host': '127.0.0.1', 'port': str(args.nat_port)}},
            'server': True, 'wait': False}}})
        created = True
        nat = socket.create_connection(('127.0.0.1', args.nat_port), timeout=5)
        sockets.append(nat)
        capture.settimeout(1)
        nat.settimeout(1)
        child = subprocess.Popen(prefix + ['python3', '/mnt/c/' + script.as_posix()[3:]],
                                 stdin=subprocess.PIPE, stdout=subprocess.PIPE, bufsize=0)
        timer = threading.Timer(args.seconds + 10, lambda: child.terminate() if child.poll() is None else None)
        timer.daemon = True
        timer.start()

        def socket_reader(connection, management):
            records = Frames()
            try:
                while not stopped.is_set() and time.monotonic() < deadline:
                    try:
                        data = connection.recv(65536)
                    except socket.timeout:
                        continue
                    if not data:
                        records.finish()
                        break
                    for frame in records.feed(data):
                        if management:
                            if owned_peer(frame):
                                counts['nat_peer_suppressed'] += 1
                                continue
                            with inject_lock:
                                inject.sendall(framed(frame))
                            counts['nat_to_guest'] += 1
                        else:
                            write_all(child.stdin, framed(frame))
                            counts['guest_to_tap'] += 1
            except Exception as error:
                if not stopped.is_set():
                    errors.append(str(error))
            finally:
                stopped.set()
                if not management:
                    child.stdin.close()

        for connection, management in ((capture, False), (nat, True)):
            thread = threading.Thread(target=socket_reader, args=(connection, management), daemon=True)
            threads.append(thread)
            thread.start()
        qmp('object-add', {'qom-type': 'filter-redirector', 'id': guard, 'netdev': 'net0',
                          'queue': 'tx', 'position': 'head', 'outdev': channel})
        installed = True
        print(json.dumps({'ready': True, 'seconds': args.seconds, 'physical_NIC': False}), flush=True)
        records = Frames()
        while not stopped.is_set():
            block = child.stdout.read(65536)
            if not block:
                records.finish()
                break
            for frame in records.feed(block):
                with inject_lock:
                    inject.sendall(framed(frame))
                counts['tap_to_guest'] += 1
    finally:
        # Restore ordinary NAT before closing the stream that carries management.
        if installed:
            try:
                qmp('object-del', {'id': guard})
            except Exception as error:
                errors.append('Restore NAT: ' + str(error))
        stopped.set()
        for connection in sockets:
            connection.close()
        for thread in threads:
            thread.join(timeout=3)
        if child:
            try:
                child.wait(timeout=5)
            except subprocess.TimeoutExpired:
                child.terminate()
                child.wait(timeout=5)
        if timer:
            timer.cancel()
        if created:
            try:
                qmp('chardev-remove', {'id': channel})
            except Exception as error:
                errors.append('Remove capture channel: ' + str(error))
        print(json.dumps({'frames': counts, 'errors': errors}), flush=True)
    if errors:
        raise SystemExit(1)


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Bounded library admission probe on the isolated QEMU network only.

Requests media information for a private slot, without fetching track content.
This is an adversarial test client, not a CDJ implementation.
"""
import argparse
import json
import os
import socket
import struct
import subprocess


def isolated():
    if os.stat('/proc/self/ns/net').st_ino == os.stat('/proc/1/ns/net').st_ino:
        raise RuntimeError('Refusing the host network namespace')
    links = json.loads(subprocess.check_output(['ip', '-j', 'address', 'show']))
    active = [x for x in links if x['ifname'] != 'lo' and 'UP' in x['flags']]
    if len(active) != 1 or active[0]['ifname'] != 'azlink0':
        raise RuntimeError('Expected only the isolated azlink0 interface')
    addresses = [x['local'] for x in active[0]['addr_info'] if x['family'] == 'inet']
    if addresses != ['192.0.2.2']:
        raise RuntimeError('Unexpected isolated IPv4 address')


def exact(sock, size):
    result = bytearray()
    while len(result) < size:
        chunk = sock.recv(size - len(result))
        if not chunk:
            raise EOFError('Connection closed')
        result.extend(chunk)
    return bytes(result)


def number(value, size=4):
    return bytes([{1: 0x0f, 2: 0x10, 4: 0x11}[size]]) + value.to_bytes(size, 'big')


def field(sock):
    tag = exact(sock, 1)[0]
    if tag in (0x0f, 0x10, 0x11):
        return int.from_bytes(exact(sock, {0x0f: 1, 0x10: 2, 0x11: 4}[tag]), 'big')
    if tag == 0x14:
        size = int.from_bytes(exact(sock, 4), 'big')
        if size > 65536:
            raise ValueError('Oversized binary field')
        return exact(sock, size)
    raise ValueError(f'Unexpected field tag {tag:#x}')


def message(sock):
    if field(sock) != 0x872349ae:
        raise ValueError('Invalid message magic')
    transaction, kind, count = field(sock), field(sock), field(sock)
    if not isinstance(count, int) or count > 12:
        raise ValueError('Invalid argument count')
    tags = field(sock)
    if not isinstance(tags, bytes) or len(tags) != count:
        raise ValueError('Invalid argument types')
    values = [field(sock) for _ in range(count)]
    # Never emit returned binary payloads.
    return {'transaction': transaction, 'type': kind,
            'arguments': [x if isinstance(x, int) else {'bytes': len(x)} for x in values]}


def packet(transaction, kind, arguments):
    tags = b'\x06' * len(arguments)
    return (number(0x872349ae) + number(transaction) + number(kind, 2)
            + number(len(arguments), 1) + b'\x14' + struct.pack('>I', len(tags))
            + tags + b''.join(number(x) for x in arguments))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--target', choices=('192.0.2.1', '192.0.2.15'), default='192.0.2.1',
                        help='Explicit isolated emulator address; .15 is the normal-loader fixture')
    parser.add_argument('--slot', type=lambda x: int(x, 0), default=0xe0)
    parser.add_argument('--requester', type=int, choices=(3, 5, 6), default=3,
                        help='5/6 deliberately collide with private requesters; emulator only')
    parser.add_argument('--expect-rejection', action='store_true',
                        help='Require EOF/reset after setup for a reserved requester collision')
    args = parser.parse_args()
    if args.expect_rejection and args.requester not in (5, 6):
        parser.error('Expected rejection is only supported for reserved requester trials')
    if args.slot != 3 and not 0xe0 <= args.slot <= 0xe7:
        parser.error('Only native USB slot 3 or private test slots 0xe0..0xe7 are supported')
    isolated()
    with socket.create_connection((args.target, 12523), timeout=5) as query:
        query.sendall(b'\x00\x00\x00\x0fRemoteDBServer\x00')
        port = int.from_bytes(exact(query, 2), 'big')
    if not 1024 <= port <= 65535:
        raise ValueError('Invalid native library port')
    with socket.create_connection((args.target, port), timeout=5) as client:
        client.sendall(number(1))
        greeting = field(client)
        client.sendall(packet(0xfffffffe, 0, [args.requester, 0x14]))
        setup = message(client)
        closed = False
        try:
            client.sendall(packet(2, 0x3007, [(args.requester << 24) | (8 << 16) | (args.slot << 8) | 1, 0]))
            reply = message(client)
        except (EOFError, ConnectionResetError, BrokenPipeError):
            if not args.expect_rejection:
                raise
            closed, reply = True, None
        if args.expect_rejection and not closed:
            raise AssertionError('Reserved requester was not closed after setup')
    print(json.dumps({'target': 'isolated-emulator', 'address': args.target, 'port': port, 'requester': args.requester,
                      'private_slot': args.slot, 'greeting': greeting,
                      'setup': setup, 'closed_after_setup': closed, 'media_reply': reply}))


if __name__ == '__main__':
    main()

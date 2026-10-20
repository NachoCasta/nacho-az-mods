#!/usr/bin/env python3
"""Bounded receive-only PRO DJ LINK observation on an explicitly selected LAN.

No announcements, requests, firewall changes or interface changes are made.
Bind wildcard ports to receive broadcasts; record packets only from the explicit
target. Keep output local: it includes network addresses and packet prefixes.
"""
import argparse
import ipaddress
import json
import selectors
import socket
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--local-ip', required=True)
    parser.add_argument('--target', required=True)
    parser.add_argument('--prefix', type=int, required=True)
    parser.add_argument('--seconds', type=int, required=True)
    args = parser.parse_args()
    local = ipaddress.IPv4Address(args.local_ip)
    target = ipaddress.IPv4Address(args.target)
    network = ipaddress.IPv4Network((str(local), args.prefix), strict=False)
    if not (local.is_private and target.is_private and target in network
            and local != target and 1 <= args.seconds <= 60):
        parser.error('Expected distinct private addresses on the same subnet and duration1..60')
    # Verify the selected address is assigned to this host, without transmitting.
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as probe:
        probe.bind((str(local), 0))
    sockets = []
    selector = selectors.DefaultSelector()
    counts = {}
    try:
        for port in (50000, 50001, 50002):
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sockets.append(sock)
            # Exclusive ownership: a competing DJ application must cause refusal.
            if hasattr(socket, 'SO_EXCLUSIVEADDRUSE'):
                sock.setsockopt(socket.SOL_SOCKET, socket.SO_EXCLUSIVEADDRUSE, 1)
            sock.bind(('0.0.0.0', port))
            sock.setblocking(False)
            selector.register(sock, selectors.EVENT_READ, port)
        print(json.dumps({'event': 'listening', 'local_ip': str(local),
                          'target': str(target), 'seconds': args.seconds,
                          'receive_only': True}), flush=True)
        deadline = time.monotonic() + args.seconds
        while time.monotonic() < deadline:
            for key, _ in selector.select(max(0, min(0.5, deadline - time.monotonic()))):
                packet, sender = key.fileobj.recvfrom(65535)
                if sender[0] != str(target) or not packet.startswith(b'Qspt1WmJOL'):
                    continue
                port = key.data
                counts[port] = counts.get(port, 0) + 1
                print(json.dumps({'event': 'packet', 'monotonic_ns': time.monotonic_ns(),
                                  'source': sender[0], 'source_port': sender[1],
                                  'local_port': port, 'bytes': len(packet),
                                  'prefix_hex': packet[:64].hex()}), flush=True)
    finally:
        selector.close()
        for sock in sockets:
            sock.close()
    print(json.dumps({'event': 'stopped', 'counts': counts, 'receive_only': True}), flush=True)


if __name__ == '__main__':
    main()

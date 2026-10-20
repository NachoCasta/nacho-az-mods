#!/usr/bin/env python3
"""Send six synthetic private-identity claims to the isolated emulator only.

These are adversarial markers, not a complete virtual player. No audio or
library content is requested. Requires the existing isolated namespace guard.
"""
import importlib.util
import argparse
import json
from pathlib import Path
import socket


def packets():
    for member in (7, 8):
        for port, kind, size in ((50000, 6, 54), (50002, 10, 1152), (50002, 6, 192)):
            data = bytearray(size)
            data[:10] = b'Qspt1WmJOL'
            data[10] = kind
            if port == 50000:
                data[0x20:0x24] = bytes([1, 3, 0, 54])
                data[0x24] = member
                data[0x2c:0x30] = bytes([127, 77, 0, member])
            else:
                data[0x21] = member
                data[0x24 if kind == 10 else 0x27] = member
            yield member, port, kind, data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--target', choices=('192.0.2.1', '192.0.2.15'), default='192.0.2.1',
                        help='Explicit isolated emulator address; .15 is the normal-loader fixture')
    args = parser.parse_args()
    spec = importlib.util.spec_from_file_location('library_peer', Path(__file__).with_name('link-library-peer.py'))
    peer = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(peer)
    peer.isolated()
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sender:
        sender.bind(('192.0.2.2', 0))
        for member, port, kind, data in packets():
            assert sender.sendto(data, (args.target, port)) == len(data)
            print(json.dumps({'target': args.target, 'member': member, 'port': port, 'type': kind, 'bytes': len(data)}), flush=True)


if __name__ == '__main__':
    main()

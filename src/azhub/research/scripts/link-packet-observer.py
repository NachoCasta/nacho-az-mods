#!/usr/bin/env python3
"""Observe Link metadata on the isolated TAP; optionally retain full AZ status."""
import argparse
import json
import os
import socket
import struct
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seconds', type=float, default=30)
    parser.add_argument('--status-payloads', action='store_true',
                        help='Include exact 1152-byte AZ status payloads for local privacy audits')
    parser.add_argument('--rpc-summary', action='store_true',
                        help='Include bounded RPC headers on fixture file-service ports, without file payloads')
    args = parser.parse_args()
    if not 0 < args.seconds <= 3600:
        parser.error('Expected duration (0, 3600]')
    if os.stat('/proc/self/ns/net').st_ino == os.stat('/proc/1/ns/net').st_ino:
        raise SystemExit('Refusing host network namespace')
    # ETH_P_ALL includes outbound TAP frames as well as inbound IPv4 frames.
    with socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.htons(0x0003)) as capture:
        capture.bind(('azlink0', 0))
        capture.settimeout(1)
        deadline = time.monotonic() + args.seconds
        while time.monotonic() < deadline:
            try:
                frame, address = capture.recvfrom(65536)
            except socket.timeout:
                continue
            if len(frame) < 42 or frame[12:14] != b'\x08\0':
                continue
            ip = frame[14:]
            ihl = (ip[0] & 15) * 4
            total = struct.unpack_from('!H', ip, 2)[0]
            if ip[0] >> 4 != 4 or ihl < 20 or total > len(ip) or ip[9] != 17:
                continue
            if struct.unpack_from('!H', ip, 6)[0] & 0x3fff or total < ihl + 8:
                continue
            sport, dport, length, _ = struct.unpack_from('!HHHH', ip, ihl)
            if length < 19 or ihl + length > total:
                continue
            data = ip[ihl + 8:ihl + length]
            if not data.startswith(b'Qspt1WmJOL'):
                if args.rpc_summary and len(data) >= 24 and ({sport, dport} & {111, 50111, 40000, 2049}):
                    words = struct.unpack_from('!6I', data)
                    if words[1] in (0, 1):
                        print(json.dumps(dict(event='rpc_header', monotonic_ns=time.monotonic_ns(),
                            source=socket.inet_ntoa(ip[12:16]), destination=socket.inet_ntoa(ip[16:20]),
                            source_port=sport, destination_port=dport, payload_bytes=len(data),
                            header_words=words)), flush=True)
                continue
            event = dict(monotonic_ns=time.monotonic_ns(), source=socket.inet_ntoa(ip[12:16]),
                         source_mac=frame[6:12].hex(':'),destination_mac=frame[:6].hex(':'),
                         destination=socket.inet_ntoa(ip[16:20]), source_port=sport,
                         destination_port=dport, link_type=data[10], payload_bytes=len(data),
                         packet_direction=address[2])
            # Document raw protocol offsets instead of assuming every packet
            # variant has standard CDJ status semantics.
            for offset in (0x21, 0x24, 0x28, 0x29, 0x2a, 0x89, 0x9f):
                if len(data) > offset:
                    event[f'byte_{offset:02x}'] = data[offset]
            if dport == 50002 and data[10] == 10 and len(data) >= 0xa0:
                event['word_2c'] = int.from_bytes(data[0x2c:0x30], 'big')
                event['pitch_8d'] = int.from_bytes(data[0x8d:0x90], 'big')
                event['bpm_92'] = int.from_bytes(data[0x92:0x94], 'big')
                if args.status_payloads and len(data) == 1152:
                    event['payload_hex'] = data.hex()
            print(json.dumps(event), flush=True)


if __name__ == '__main__':
    main()

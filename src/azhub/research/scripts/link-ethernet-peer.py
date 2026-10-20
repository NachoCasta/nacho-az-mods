#!/usr/bin/env python3
"""Isolated QEMU socket-network peer. Never binds a physical interface.

QEMU socket backend TCP frames are a four-byte network-order length followed by
an Ethernet frame. This tool answers ARP and ICMP echo for a documentation-only
IPv4 address and records bounded packet metadata. It is infrastructure, not a
PRO DJ LINK interoperability test. Connect a dedicated emulator NIC using
netdev_add type=socket,connect=127.0.0.1:PORT; do not bridge it to a real LAN.
"""
import argparse
import ipaddress
import json
import socket
import struct
import time


def checksum(data):
    data += b'\0' * (len(data) % 2)
    total = sum(struct.unpack('!' + 'H' * (len(data) // 2), data))
    while total >> 16:
        total = (total & 0xffff) + (total >> 16)
    return (~total) & 0xffff


class EthernetPeer:
    def __init__(self, address='192.0.2.2', mac='02:00:00:00:02:02'):
        ip = ipaddress.IPv4Address(address)
        if ip not in ipaddress.IPv4Network('192.0.2.0/24'):
            raise ValueError('Emulator peer must use the isolated 192.0.2.0/24 segment')
        self.ip = ip.packed
        self.mac = bytes.fromhex(mac.replace(':', ''))
        if len(self.mac) != 6 or self.mac[0] & 1:
            raise ValueError('Expected a unicast MAC address')

    def receive(self, frame):
        """Return (bounded metadata, optional Ethernet reply)."""
        event = {'bytes': len(frame)}
        if len(frame) < 14:
            return dict(event, rejected='short-ethernet'), None
        destination, source, kind = struct.unpack('!6s6sH', frame[:14])
        event['ethertype'] = hex(kind)
        if destination not in (self.mac, b'\xff' * 6):
            return event, None
        payload = frame[14:]
        if kind == 0x806:
            if len(payload) < 28:
                return dict(event, rejected='short-arp'), None
            hw, proto, hlen, plen, op, smac, sip, _, tip = struct.unpack('!HHBBH6s4s6s4s', payload[:28])
            event['arp_operation'] = op
            if (hw, proto, hlen, plen, op, tip) == (1, 0x800, 6, 4, 1, self.ip):
                reply = struct.pack('!HHBBH6s4s6s4s', 1, 0x800, 6, 4, 2, self.mac, self.ip, smac, sip)
                return event, smac + self.mac + b'\x08\x06' + reply
        if kind != 0x800:
            return event, None
        if len(payload) < 20 or payload[0] >> 4 != 4:
            return dict(event, rejected='invalid-ipv4'), None
        ihl = (payload[0] & 15) * 4
        total = struct.unpack_from('!H', payload, 2)[0]
        fragment = struct.unpack_from('!H', payload, 6)[0]
        if ihl < 20 or total < ihl or total > len(payload) or checksum(payload[:ihl]):
            return dict(event, rejected='invalid-ipv4-header'), None
        event.update(source=str(ipaddress.IPv4Address(payload[12:16])),
                     destination=str(ipaddress.IPv4Address(payload[16:20])), protocol=payload[9])
        if fragment & 0x3fff:
            return dict(event, rejected='fragmented-ipv4'), None
        body = payload[ihl:total]
        if payload[9] == 17 and len(body) >= 8:
            sport, dport, length, _ = struct.unpack('!HHHH', body[:8])
            if length < 8 or length > len(body):
                return dict(event, rejected='invalid-udp-length'), None
            data = body[8:length]
            event.update(source_port=sport, destination_port=dport, udp_bytes=len(data))
            if data.startswith(b'Qspt1WmJOL') and len(data) > 10:
                event['link_type'] = data[10]
        if payload[9] != 1 or payload[16:20] != self.ip or len(body) < 8:
            return event, None
        if body[0:2] != b'\x08\0' or checksum(body):
            return event, None
        reply = b'\0\0\0\0' + body[4:]
        reply = reply[:2] + struct.pack('!H', checksum(reply)) + reply[4:]
        header = struct.pack('!BBHHHBBH4s4s', 0x45, 0, 20 + len(reply), 0, 0, 64, 1, 0, self.ip, payload[12:16])
        header = header[:10] + struct.pack('!H', checksum(header)) + header[12:]
        return dict(event, icmp_echo_reply=True), source + self.mac + b'\x08\0' + header + reply


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=15100)
    parser.add_argument('--seconds', type=float, default=60)
    parser.add_argument('--output', required=True)
    parser.add_argument('--connect', action='store_true',
                        help='Connect to a listening QEMU socket backend instead of accepting one')
    args = parser.parse_args()
    if not 1024 <= args.port <= 65535 or not 0 < args.seconds <= 3600:
        parser.error('Expected unprivileged port and bounded duration (0, 3600]')
    peer = EthernetPeer()
    with open(args.output, 'w', encoding='utf-8') as log, socket.socket() as server:
        server.settimeout(args.seconds)
        if args.connect:
            server.connect(('127.0.0.1', args.port))
            connection = server
            print(json.dumps({'connected': ['127.0.0.1', args.port]}), flush=True)
        else:
            server.bind(('127.0.0.1', args.port))
            server.listen(1)
            print(json.dumps({'listening': ['127.0.0.1', args.port]}), flush=True)
            connection, _ = server.accept()
        deadline = time.monotonic() + args.seconds
        with connection:
            connection.settimeout(1)
            buffer = b''
            while time.monotonic() < deadline:
                try:
                    block = connection.recv(65536)
                except socket.timeout:
                    continue
                except ConnectionResetError:
                    break
                if not block:
                    break
                buffer += block
                while len(buffer) >= 4:
                    length = struct.unpack('!I', buffer[:4])[0]
                    if not 14 <= length <= 65536:
                        raise ValueError(f'Invalid QEMU frame length: {length}')
                    if len(buffer) < 4 + length:
                        break
                    frame, buffer = buffer[4:4 + length], buffer[4 + length:]
                    event, reply = peer.receive(frame)
                    log.write(json.dumps(dict(event, monotonic_ns=time.monotonic_ns())) + '\n')
                    log.flush()
                    if reply:
                        connection.sendall(struct.pack('!I', len(reply)) + reply)


if __name__ == '__main__':
    main()

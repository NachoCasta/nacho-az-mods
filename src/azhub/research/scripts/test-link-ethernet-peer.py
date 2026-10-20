#!/usr/bin/env python3
"""Packet-boundary checks for the isolated emulator Ethernet peer."""
import importlib.util
from pathlib import Path
import struct
import unittest

spec = importlib.util.spec_from_file_location('ethernet_peer', Path(__file__).with_name('link-ethernet-peer.py'))
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class PeerTests(unittest.TestCase):
    def setUp(self):
        self.peer = module.EthernetPeer()
        self.mac = bytes.fromhex('020000000201')
        self.ip = bytes([192, 0, 2, 1])

    def ipv4(self, body, protocol=1, fragment=0):
        header = struct.pack('!BBHHHBBH4s4s', 0x45, 0, 20 + len(body), 1,
                             fragment, 64, protocol, 0, self.ip, self.peer.ip)
        header = header[:10] + struct.pack('!H', module.checksum(header)) + header[12:]
        return self.peer.mac + self.mac + b'\x08\0' + header + body

    def test_arp_reply_has_correct_ownership(self):
        arp = struct.pack('!HHBBH6s4s6s4s', 1, 0x800, 6, 4, 1, self.mac,
                          self.ip, b'\0' * 6, self.peer.ip)
        _, reply = self.peer.receive(b'\xff' * 6 + self.mac + b'\x08\x06' + arp)
        self.assertEqual(reply[:12], self.mac + self.peer.mac)
        self.assertEqual(struct.unpack('!HHBBH6s4s6s4s', reply[14:]),
                         (1, 0x800, 6, 4, 2, self.peer.mac, self.peer.ip, self.mac, self.ip))

    def test_echo_preserves_payload_and_valid_checksums(self):
        body = b'\x08\0\0\0\x12\x34\0\x01isolated-peer'
        body = body[:2] + struct.pack('!H', module.checksum(body)) + body[4:]
        event, reply = self.peer.receive(self.ipv4(body))
        self.assertTrue(event['icmp_echo_reply'])
        self.assertEqual(reply[26:34], self.peer.ip + self.ip)
        self.assertEqual(reply[34:36], b'\0\0')
        self.assertEqual(reply[38:], body[4:])
        self.assertEqual(module.checksum(reply[14:34]), 0)
        self.assertEqual(module.checksum(reply[34:]), 0)

    def test_truncation_never_generates_reply(self):
        frame = self.ipv4(b'\x08\0\0\0\x12\x34\0\x01')
        for length in range(len(frame)):
            with self.subTest(length=length):
                self.assertIsNone(self.peer.receive(frame[:length])[1])

    def test_fragment_and_udp_length_rejected(self):
        event, reply = self.peer.receive(self.ipv4(b'\0' * 8, fragment=0x2000))
        self.assertEqual(event['rejected'], 'fragmented-ipv4')
        self.assertIsNone(reply)
        event, _ = self.peer.receive(self.ipv4(struct.pack('!HHHH', 50000, 50000, 100, 0), 17))
        self.assertEqual(event['rejected'], 'invalid-udp-length')

    def test_link_metadata_does_not_log_payload(self):
        body = b'Qspt1WmJOL\x06private-test-data'
        event, reply = self.peer.receive(self.ipv4(struct.pack('!HHHH', 50000, 50000, len(body) + 8, 0) + body, 17))
        self.assertEqual(event['link_type'], 6)
        self.assertNotIn('private-test-data', str(event))
        self.assertIsNone(reply)

    def test_refuses_nonisolated_address(self):
        with self.assertRaises(ValueError):
            module.EthernetPeer('192.168.1.2')


if __name__ == '__main__':
    unittest.main()

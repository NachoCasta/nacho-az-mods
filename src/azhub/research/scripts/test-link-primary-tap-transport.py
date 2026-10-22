#!/usr/bin/env python3
"""Packet framing, NAT peer ownership and namespace guards; no live networking."""
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location('transport', Path(__file__).with_name('link-primary-tap-transport.py'))
transport = importlib.util.module_from_spec(spec)
spec.loader.exec_module(transport)


class TransportTests(unittest.TestCase):
    def dhcp(self):
        frame=bytearray(14+20+8+240)
        frame[12:14]=b'\x08\x00';frame[14]=0x45
        frame[16:18]=(len(frame)-14).to_bytes(2,'big');frame[23]=17
        frame[34:38]=b'\x00\x43\x00\x44';frame[38:40]=(248).to_bytes(2,'big')
        frame[42:45]=b'\x02\x01\x06'
        frame[70:76]=bytes.fromhex('020000001203');frame[278:282]=b'\x63\x82\x53\x63'
        return frame

    def test_only_fixture_nat_dhcp_is_suppressed(self):
        packet=self.dhcp();self.assertTrue(transport.fixture_dhcp_reply(packet))
        packet[70:76]=bytes.fromhex('525400123456')
        self.assertFalse(transport.fixture_dhcp_reply(packet))
        self.assertFalse(transport.fixture_dhcp_reply(self.ipv4((10,0,2,2))))

    def test_fixture_dhcp_bounds_and_protocol(self):
        packet=self.dhcp()
        for length in range(len(packet)):
            self.assertFalse(transport.fixture_dhcp_reply(packet[:length]))
        for offset in (12,14,16,21,23,34,36,38,42,43,44,70,278):
            mutated=bytearray(packet);mutated[offset]^=0x40
            self.assertFalse(transport.fixture_dhcp_reply(mutated),offset)

    def ipv4(self, source):
        frame = bytearray(54)
        frame[12:14] = b'\x08\x00'
        frame[14] = 0x45
        frame[16:18] = (40).to_bytes(2, 'big')
        frame[23] = 6
        frame[26:30] = bytes(source)
        frame[47] = 4  # TCP RST, as observed from slirp.
        return frame

    def namespace(self):
        return [{'ifname': 'lo'}, {'ifname': 'azlink0', 'flags': ['UP'], 'addr_info': [
            {'family': 'inet', 'local': '192.0.2.2', 'prefixlen': 24, 'broadcast': '192.0.2.255'}]}]

    def test_framing_every_split_and_coalesced_frames(self):
        first, second = bytes(range(14)), b'x' * 100
        stream = transport.framed(first) + transport.framed(second)
        for split in range(len(stream) + 1):
            parser = transport.Frames()
            self.assertEqual(parser.feed(stream[:split]) + parser.feed(stream[split:]), [first, second])
            parser.finish()

    def test_invalid_lengths_and_truncation(self):
        for size in (0, 13, 65537):
            with self.assertRaises(ValueError):
                transport.Frames().feed(size.to_bytes(4, 'big'))
        parser = transport.Frames()
        parser.feed(transport.framed(b'x' * 14)[:-1])
        with self.assertRaises(ValueError):
            parser.finish()

    def test_nat_peer_tcp_suppressed_management_preserved(self):
        self.assertTrue(transport.owned_peer(self.ipv4((192, 0, 2, 2))))
        self.assertFalse(transport.owned_peer(self.ipv4((192, 0, 2, 254))))
        self.assertFalse(transport.owned_peer(self.ipv4((192, 0, 2, 253))))

    def test_short_or_malformed_headers_are_not_peer_claims(self):
        packet = self.ipv4((192, 0, 2, 2))
        for length in range(len(packet)):
            self.assertFalse(transport.owned_peer(packet[:length]))
        packet[14] = 0x65
        self.assertFalse(transport.owned_peer(packet))
        packet[14] = 0x4f
        self.assertFalse(transport.owned_peer(packet))

    def test_nat_arp_peer_ownership(self):
        packet = bytearray(42)
        packet[12:14] = b'\x08\x06'
        packet[14:20] = b'\x00\x01\x08\x00\x06\x04'
        packet[28:32] = bytes((192, 0, 2, 2))
        self.assertTrue(transport.owned_peer(packet))
        packet[31] = 254
        self.assertFalse(transport.owned_peer(packet))
        packet[31], packet[18] = 2, 4
        self.assertFalse(transport.owned_peer(packet))

    def test_selected_dhcp_clients_only(self):
        packet=bytearray(282);packet[12:14]=b'\x08\x00';packet[14]=0x45
        packet[16:18]=(268).to_bytes(2,'big');packet[23]=17
        packet[34:38]=b'\x00\x43\x00\x44';packet[38:40]=(248).to_bytes(2,'big')
        packet[42:45]=b'\x02\x01\x06';packet[278:282]=b'\x63\x82\x53\x63'
        packet[70:76]=bytes.fromhex('020000001203')
        self.assertTrue(transport.fixture_dhcp_reply(packet))
        packet[75]=7
        self.assertFalse(transport.fixture_dhcp_reply(packet))
        clients=transport.fixture_dhcp_clients('0,4')
        self.assertTrue(transport.fixture_dhcp_reply(packet,clients))
        for length in range(len(packet)):
            self.assertFalse(transport.fixture_dhcp_reply(packet[:length],clients))
        packet[70:76]=bytes.fromhex('525400123456')
        self.assertFalse(transport.fixture_dhcp_reply(packet,clients))
        for invalid in ('0,0','8','-1','0,4,9',''):
            with self.assertRaises(ValueError):transport.fixture_dhcp_clients(invalid)

    def test_only_prepared_isolated_namespace(self):
        transport.verify_namespace(self.namespace())
        rows = self.namespace()
        rows.append({'ifname': 'eth0', 'flags': []})
        with self.assertRaises(ValueError):
            transport.verify_namespace(rows)
        rows = self.namespace()
        rows[1]['addr_info'][0]['local'] = '192.168.1.2'
        with self.assertRaises(ValueError):
            transport.verify_namespace(rows)


if __name__ == '__main__':
    unittest.main()

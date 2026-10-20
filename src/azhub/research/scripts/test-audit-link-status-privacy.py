#!/usr/bin/env python3
"""Synthetic capture fixtures for known-field assertions and bounded reporting."""
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location('status_audit', Path(__file__).with_name('audit-link-status-privacy.py'))
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


def row(deck=1, media=(0, 0, 0, 0), timestamp=100, **changes):
    packet = bytearray(1152)
    packet[:11] = b'Qspt1WmJOL\x0a'
    packet[0x21] = packet[0x24] = deck
    packet[0x28:0x2b] = bytes(media[:3])
    packet[0x2c:0x30] = media[3].to_bytes(4, 'big')
    # Preference values 7/8 are not private source identities by themselves.
    packet[0x468:0x470] = bytes([7, 8, 0, 0, 0, 0, 0, 0])
    result = dict(source='192.0.2.1', destination='192.0.2.255', destination_port=50002,
                  link_type=10, payload_bytes=1152, monotonic_ns=timestamp, payload_hex=packet.hex())
    result.update(changes)
    return result


class CaptureAuditTests(unittest.TestCase):
    def test_zero_tuple_and_preferences(self):
        report = audit.analyze([row()], private_decks=[1])
        self.assertEqual(report['result'], 'passed')
        self.assertEqual(report['observations'][0]['preference_rows']['0x468'], ['0708000000000000'])

    def test_known_source_and_content_leaks(self):
        for media in ((7, 3, 1, 495), (0, 0, 0, 495), (8, 8, 1, 0)):
            with self.subTest(media=media):
                self.assertEqual(audit.analyze([row(media=media)], private_decks=[1])['result'], 'failed')

    def test_visible_same_content_native_and_external(self):
        for media in ((1, 3, 1, 495), (3, 3, 1, 495)):
            self.assertEqual(audit.analyze([row(media=media)], expected={1: media})['result'], 'passed')
        self.assertEqual(audit.analyze([row()], expected={1: (1, 3, 1, 495)})['result'], 'failed')

    def test_full_tail_markers_are_review_candidates(self):
        sample = row()
        packet = bytearray.fromhex(sample['payload_hex'])
        packet[0x300:0x304] = bytes([127, 77, 0, 8])
        packet[0x350:0x354] = (495).to_bytes(4, 'big')
        packet[0x400:0x405] = b'LABEL'
        sample['payload_hex'] = packet.hex()
        report = audit.analyze([sample], private_decks=[1], content_ids=[495], texts=['LABEL'])
        self.assertEqual(report['result'], 'passed')
        hits = report['observations'][0]['unclassified_marker_matches']
        self.assertEqual(hits['private_ip_8'], {'0x300': 1})
        self.assertEqual(hits['content_495_be32'], {'0x350': 1})
        self.assertEqual(hits['text:LABEL'], {'0x400': 1})

    def test_auxiliary_origin_independent_of_current_sender_and_content(self):
        sample = row(media=(1, 3, 1, 495))
        packet = bytearray.fromhex(sample['payload_hex'])
        for i, offset in enumerate(audit.LOCATION_ROWS):
            packet[offset:offset+6] = bytes([1, 2, 6 if i % 2 else 7, 2, 0, 1])
            packet[offset+8:offset+12] = (800 if i % 2 else 495).to_bytes(4, 'big')
        sample['payload_hex'] = packet.hex()
        report = audit.analyze([sample], expected={1: (1, 3, 1, 495)}, content_ids=[495])
        self.assertEqual(report['result'], 'failed')
        self.assertEqual(len(report['failures']), 11)
        self.assertEqual(len(report['observations'][0]['known_auxiliary_locations']), 11)
        self.assertIn('0x194', report['observations'][0]['known_content_marker_matches']['content_495_be32'])
        self.assertFalse(report['observations'][0]['unclassified_marker_matches']['content_495_be32'])
        # Same content and arbitrary preference bytes do not imply privacy.
        for offset in audit.LOCATION_ROWS:
            packet[offset+2] = 2
        sample['payload_hex'] = packet.hex()
        self.assertEqual(audit.analyze([sample], expected={1: (1, 3, 1, 495)})['result'], 'passed')

    def test_peer_loopback_and_time_windows_excluded(self):
        samples = [row(timestamp=50, media=(7, 3, 1, 495)), row(timestamp=100),
                   row(source='192.0.2.2', media=(7, 3, 1, 495)),
                   row(destination='127.0.0.1', media=(7, 3, 1, 495))]
        report = audit.analyze(samples, private_decks=[1], from_ns=100, until_ns=100)
        self.assertEqual(report['result'], 'passed')
        self.assertEqual(report['complete_outgoing_statuses'], 1)

    def test_missing_and_invalid_payload_never_pass(self):
        for samples in ([], [row(payload_hex='')], [row(payload_hex='00' * 1152)],
                        [row(payload_hex='xyz')], [row(payload_hex='00' * 1151)],
                        [row(payload_bytes=512, payload_hex='00' * 512)]):
            self.assertEqual(audit.analyze(samples, private_decks=[1])['result'], 'insufficient')
        self.assertEqual(audit.analyze([row()])['result'], 'insufficient')

    def test_header_claims_distinct_from_media_source(self):
        sample = row()
        packet = bytearray.fromhex(sample['payload_hex'])
        packet[0x24] = 7
        sample['payload_hex'] = packet.hex()
        self.assertEqual(audit.analyze([sample], private_decks=[1])['result'], 'failed')


if __name__ == '__main__':
    unittest.main()

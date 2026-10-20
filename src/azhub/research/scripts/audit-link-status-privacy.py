#!/usr/bin/env python3
"""Check a local full-status capture against explicitly supplied deck states.

This checks known media fields across complete EP147 packets. Marker matches in
unclassified fields require review; they are not proof of those fields' meaning.
No packet payloads are copied into the report.
"""
import argparse
import collections
import hashlib
import ipaddress
import json
from pathlib import Path

SETTINGS_ROWS = (0xd9, 0xf9, 0x3fd, 0x41d, 0x468, 0x474)
LOCATION_ROWS = tuple(0x18c + 0x40 * i for i in range(10)) + (0x428,)


def media_tuple(packet):
    return (*packet[0x28:0x2b], int.from_bytes(packet[0x2c:0x30], 'big'))


def analyze(rows, source='192.0.2.1', private_decks=(), expected=None,
            content_ids=(), texts=(), from_ns=0, until_ns=None):
    expected = expected or {}
    packets, malformed = [], 0
    for row in rows:
        if (row.get('source') != source or row.get('destination_port') != 50002 or
                row.get('link_type') != 10 or row.get('payload_bytes') != 1152):
            continue
        timestamp = row.get('monotonic_ns', 0)
        if timestamp < from_ns or (until_ns is not None and timestamp > until_ns):
            continue
        if ipaddress.ip_address(row['destination']).is_loopback:
            continue
        try:
            packet = bytes.fromhex(row.get('payload_hex', ''))
        except ValueError:
            packet = b''
        if len(packet) != 1152 or packet[:11] != b'Qspt1WmJOL\x0a':
            malformed += 1
            continue
        packets.append((timestamp, packet))
    markers = {'private_ip_7': bytes([127, 77, 0, 7]),
               'private_ip_8': bytes([127, 77, 0, 8])}
    for content in content_ids:
        markers[f'content_{content}_be32'] = content.to_bytes(4, 'big')
        markers[f'content_{content}_le32'] = content.to_bytes(4, 'little')
    for text in texts:
        if not text:
            raise ValueError('Text markers must be nonempty')
        markers[f'text:{text}'] = text.encode('utf-8')
    observations, failures, missing = [], [], []
    decks = sorted({p[0x21] for _, p in packets} | set(private_decks) | set(expected))
    for deck in decks:
        selected = [(t, p) for t, p in packets if p[0x21] == deck]
        tuples = collections.Counter(media_tuple(p) for _, p in selected)
        if deck in private_decks or deck in expected:
            if not selected:
                missing.append(f'No full outgoing status for expected deck {deck}')
        if deck in private_decks and any(value != (0, 0, 0, 0) for value in tuples):
            failures.append(f'Deck {deck} exposes a nonzero known media tuple')
        if deck in expected and any(value != tuple(expected[deck]) for value in tuples):
            failures.append(f'Deck {deck} differs from its expected visible media tuple')
        # Device-number header fields are distinct from the media-source tuple.
        if any(p[0x21] in (7, 8) or p[0x24] in (7, 8) for _, p in selected):
            failures.append(f'Private member number in outgoing deck {deck} header')
        locations = {}
        for offset in LOCATION_ROWS:
            counts = collections.Counter((p[offset+1], p[offset+2], p[offset+3], p[offset+4],
                                          p[offset+5], int.from_bytes(p[offset+8:offset+12], 'big'))
                                         for _, p in selected if p[offset] == 1)
            if counts:
                locations[hex(offset)] = [dict(kind=k[0], member_index=k[1], media_kind=k[2],
                                               media_subkind=k[3], track_type=k[4], content_id=k[5], packets=n)
                                          for k, n in sorted(counts.items())]
            if any(k[0] == 2 and k[1] in (6, 7) for k in counts):
                failures.append(f'Deck {deck} exposes a private auxiliary location at {hex(offset)}')
        hits, known_hits = {}, {}
        content_offsets = {0x2c} | {offset+8 for offset in LOCATION_ROWS}
        for name, marker in markers.items():
            offsets = collections.Counter()
            for _, packet in selected:
                offsets.update(i for i in range(len(packet) - len(marker) + 1)
                               if packet[i:i + len(marker)] == marker)
            known_content = name.startswith('content_') and name.endswith('_be32')
            known_hits[name] = {hex(offset): count for offset, count in sorted(offsets.items())
                                if known_content and offset in content_offsets}
            hits[name] = {hex(offset): count for offset, count in sorted(offsets.items())
                          if not known_content or offset not in content_offsets}
        observations.append({
            'deck': deck, 'packets': len(selected),
            'first_ns': selected[0][0] if selected else None,
            'last_ns': selected[-1][0] if selected else None,
            'media_tuples': [{'player': key[0], 'slot': key[1], 'track_type': key[2],
                              'content_id': key[3], 'packets': count}
                             for key, count in sorted(tuples.items())],
            'unclassified_marker_matches': hits,
            'known_content_marker_matches': known_hits,
            'known_auxiliary_locations': locations,
            'content_marker_offset_classification': {
                '0x2c': 'Current source content ID',
                **{hex(offset+8): 'Auxiliary TrackLocation content ID' for offset in LOCATION_ROWS}},
            'preference_rows': {hex(offset): sorted({p[offset:offset + 8].hex() for _, p in selected})
                                for offset in SETTINGS_ROWS},
        })
    if malformed:
        missing.append(f'{malformed} status records lack a valid complete payload')
    if not private_decks and not expected:
        missing.append('No expected deck state supplied')
    return {'result': 'failed' if failures else 'insufficient' if missing else 'passed',
            'scope': 'Known media fields and exact marker search across complete 1152-byte statuses',
            'complete_outgoing_statuses': len(packets), 'failures': failures,
            'missing_evidence': missing, 'observations': observations,
            'limits': ['Marker matches outside known media fields require semantic review.',
                       'Absence of selected markers does not identify every extended field.',
                       'Only supplied state windows are covered; this does not infer cached-eject or replacement acceptance.']}


def expected_tuple(value):
    parts = [int(part, 0) for part in value.split(':')]
    if len(parts) != 5 or not 1 <= parts[0] <= 4 or any(not 0 <= x <= 255 for x in parts[1:4]) or not 0 <= parts[4] <= 0xffffffff:
        raise argparse.ArgumentTypeError('Expected deck:player:slot:type:content with deck 1..4')
    return parts[0], tuple(parts[1:])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    parser.add_argument('--source', default='192.0.2.1')
    parser.add_argument('--private-deck', type=int, choices=range(1, 5), action='append', default=[])
    parser.add_argument('--expected', type=expected_tuple, action='append', default=[])
    parser.add_argument('--content-id', type=lambda s: int(s, 0), action='append', default=[])
    parser.add_argument('--text', action='append', default=[])
    parser.add_argument('--from-ns', type=int, default=0)
    parser.add_argument('--until-ns', type=int)
    args = parser.parse_args()
    if set(args.private_deck) & {deck for deck, _ in args.expected}:
        parser.error('A deck cannot be both private and expected visible')
    if any(not 0 < content <= 0xffffffff for content in args.content_id):
        parser.error('Content IDs must be nonzero uint32 values')
    raw = args.capture.read_bytes()
    report = analyze([json.loads(line) for line in raw.decode().splitlines()], args.source,
                     args.private_deck, dict(args.expected), args.content_id, args.text,
                     args.from_ns, args.until_ns)
    report['capture_sha256'] = hashlib.sha256(raw).hexdigest()
    print(json.dumps(report, indent=2))
    return {'passed': 0, 'failed': 1, 'insufficient': 2}[report['result']]


if __name__ == '__main__':
    raise SystemExit(main())

#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Read-only file transport, EOF and withdrawal controls using synthetic data."""
import tempfile
from pathlib import Path
import struct
import unittest
from link_fixture_files import FixtureFiles, words, opaque


def status(reply):return struct.unpack_from('!I', reply)[0]


class Files(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(); self.addCleanup(self.temp.cleanup)
        path = Path(self.temp.name) / 'fixture.bin'
        self.data = bytes(range(256)) * 41; path.write_bytes(self.data)
        self.export = FixtureFiles({'/Contents/Fixture.bin': path})
        self.addCleanup(self.export.close)

    def lookup(self, handle, name):
        reply = self.export.request(4, handle + opaque(name.encode('utf-16le')))
        self.assertEqual(status(reply), 0)
        return reply[4:36]

    def test_lookup_full_transfer_partial_and_eof(self):
        directory = self.lookup(self.export.root, 'Contents')
        handle = self.lookup(directory, 'Fixture.bin')
        result = b''
        for offset in range(0, len(self.data) + 8192, 8192):
            reply = self.export.request(6, handle + words(offset, 8192, 0))
            self.assertEqual(status(reply), 0)
            length = struct.unpack_from('!I', reply, 72)[0]
            self.assertEqual(length, len(reply[76:76+length]))
            result += reply[76:76+length]
        self.assertEqual(result, self.data)
        self.assertEqual(status(self.export.request(6, handle + words(0, 8193, 0))), 5)

    def test_traversal_unknown_handles_and_write_refusal(self):
        for name in ('..', '.', '../fixture.bin', 'a/b', 'a\\b'):
            self.assertEqual(status(self.export.request(4, self.export.root + opaque(name.encode()))), 2)
        self.assertEqual(status(self.export.request(1, b'x'*32)), 70)
        self.assertEqual(status(self.export.request(8, self.export.root)), 30)
        self.assertEqual(status(self.export.request(6, self.export.root + words(0,1,0))), 21)

    def test_eight_source_handle_ownership_and_replacement(self):
        path=Path(self.temp.name)/'fixture.bin'
        exports=[FixtureFiles({'/Track.bin':path}) for _ in range(8)]
        for export in exports:self.addCleanup(export.close)
        handles=[export.request(4,export.root+opaque(b'Track.bin'))[4:36] for export in exports]
        self.assertEqual(len(set(handles)),8)
        for i,export in enumerate(exports):
            self.assertEqual(status(export.request(6,handles[i]+words(0,16,0))),0)
            self.assertEqual(status(export.request(6,handles[(i+1)%8]+words(0,16,0))),70)
        exports[3].close()
        replacement=Path(self.temp.name)/'replacement.bin';replacement.write_bytes(b'new source')
        new=FixtureFiles({'/Track.bin':replacement});self.addCleanup(new.close)
        self.assertEqual(status(new.request(6,handles[3]+words(0,16,0))),70)
        for i in (2,4):self.assertEqual(status(exports[i].request(6,handles[i]+words(0,16,0))),0)

    def test_directory_listing_and_generation_withdrawal(self):
        self.assertEqual(self.export.mount(opaque('/C/'.encode('utf-16le'))), words(0)+self.export.root)
        self.assertEqual(status(self.export.mount(opaque(b'/unknown'))), 2)
        reply = self.export.request(16, self.export.root + words(0, 8192))
        self.assertEqual(status(reply), 0)
        self.assertIn('Contents'.encode('utf-16le'), reply)
        self.export.close()
        self.assertEqual(status(self.export.mount(opaque(b'/C/'))), 2)
        self.assertEqual(status(self.export.request(1, self.export.root)), 70)
        self.assertEqual(status(self.export.request(16, self.export.root + words(0,8192))), 70)
        self.assertTrue(all(n['file'] is None or n['file'].closed for n in self.export.nodes.values()))


if __name__ == '__main__':unittest.main()

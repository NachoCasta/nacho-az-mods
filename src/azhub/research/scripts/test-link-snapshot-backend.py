#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Synthetic playlist/session controls; no private fixture data required."""
import json
from pathlib import Path
import tempfile
import unittest
from link_snapshot_backend import SnapshotBackend


class Sessions(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        path = Path(self.temp.name) / 'library.json'
        path.write_text(json.dumps({'format': 1, 'tracks': [
            {'id': 10, 'title': 'Alpha'}, {'id': 11, 'title': 'Beta'}],
            'playlists': [
                {'id': 1, 'parent': 0, 'name': 'Folder', 'folder': True, 'tracks': []},
                {'id': 2, 'parent': 1, 'name': 'Set', 'folder': False, 'tracks': [11, 10, 999]}]}))
        self.backend = SnapshotBackend(path)

    def request(self, kind, args, menus):
        return self.backend.request({'transaction': 1, 'type': kind, 'arguments': args}, menus)

    def test_nested_playlist_preserves_order_and_skips_missing_ids(self):
        menus = {}
        self.assertEqual(self.request(0x1105, [0x01010401, 0, 0, 1], menus), [(0x4000, [0x1105, 1])])
        self.assertEqual(self.request(0x3000, [0x01010401, 0, 64], menus)[1][1][6], 1)
        self.request(0x1105, [0x01010401, 0, 1, 1], menus)
        self.assertEqual(self.request(0x3000, [0x01010401, 0, 64], menus)[1][1][6], 8)
        self.request(0x1105, [0x01010401, 0, 2, 0], menus)
        rows = self.request(0x3000, [0x01010401, 0, 64], menus)[1:-1]
        self.assertEqual([row[1][1] for row in rows], [11, 10])
        self.assertEqual([row[1][9] for row in rows], [1, 2])

    def test_empty_analysis_responses_roundtrip_wire_without_desync(self):
        import link_db_wire as wire
        from io import BytesIO
        class Stream(BytesIO):
            def recv(self, size):return self.read(size)
        for kind,response in ((0x2104,0x4702),(0x2b04,0x4e02),(0x2c04,0x4f02),(0x2004,0x4402)):
            replies=self.request(kind,[0x01080401,10],{})
            self.assertEqual(replies[0][0],response)
            frame=wire.packet(1,*replies[0])+wire.packet(2,0x4000,[0,17])
            stream=Stream(frame)
            self.assertEqual(wire.message(stream)['arguments'],replies[0][1])
            self.assertEqual(wire.message(stream)['transaction'],2)
            self.assertEqual(stream.read(),b'')

    def test_mp3_track_info_has_decoder_path_size_and_duration(self):
        menus = {}
        self.backend.tracks[10].update(path='/Contents/Fixture.mp3',file_size=12345,duration=213,bpm100=12850)
        self.assertEqual(self.request(0x2102,[0x01030401,10],menus),[(0x4000,[0x2102,7])])
        rows=self.request(0x3000,[0x01030401,0,64],menus)[1:-1]
        self.assertEqual(rows[0][1][1],1)
        self.assertEqual(rows[4][1][0],12345)
        self.assertEqual(rows[4][1][3],'/Contents/Fixture.mp3')
        self.assertEqual(rows[1][1][1],213)
        self.backend.tracks[10]['path']='/Contents/Other.m4a'
        self.assertEqual(self.request(0x2102,[0x01040401,10],menus),[(0x4003,[0x2102,0])])

    def test_metadata_does_not_replace_browser_menu(self):
        menus = {}
        self.request(0x1004, [0x01010401, 0], menus)
        self.backend.tracks[11].update(artist='Artist', artist_id=5, duration=213, bpm100=12850)
        reply = self.request(0x2002, [0x01030401, 11], menus)
        self.assertEqual(reply, [(0x4000, [0x2002, 11])])
        rows = self.request(0x3000, [0x01030401, 0, 64], menus)[1:-1]
        values = {row[1][6]: row[1] for row in rows}
        self.assertEqual(values[7][3], 'Artist')
        self.assertEqual(values[11][1], 213)
        self.assertEqual(values[13][1], 12850)
        self.assertEqual(self.request(0x3000, [0x01010401, 0, 1], menus)[1][1][3], 'Alpha')
        self.assertEqual(self.request(0x2002, [0x01040401, 999], menus), [(0x4003, [0x2002, 0])])

    def test_connection_and_preview_contexts_do_not_replace_each_other(self):
        first, second = {}, {}
        self.request(0x1004, [0x01010401, 0], first)
        self.request(0x1105, [0x01020401, 0, 0, 1], first)
        self.request(0x1105, [0x01010401, 0, 2, 0], second)
        self.assertEqual(self.request(0x3000, [0x01010401, 0, 1], first)[1][1][3], 'Alpha')
        self.assertEqual(self.request(0x3000, [0x01020401, 0, 1], first)[1][1][3], 'Folder')
        self.assertEqual(self.request(0x3000, [0x01010401, 0, 1], second)[1][1][3], 'Beta')
        self.assertEqual(self.request(0x3000, [0x01020401, 0, 1], second), [(0x4003, [0x3000, 0])])


if __name__ == '__main__':
    unittest.main()

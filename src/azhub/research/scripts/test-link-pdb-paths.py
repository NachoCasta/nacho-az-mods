#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Synthetic structural controls; no firmware or music fixtures required."""
import struct
import unittest
from link_pdb_paths import PdbFormatError, track_paths


def fixture():
    data = bytearray(1024)
    struct.pack_into("<2I", data, 4, 512, 1)
    struct.pack_into("<4I", data, 28, 0, 0, 1, 1)
    struct.pack_into("<3I", data, 516, 1, 0, 2)
    data[536] = 1
    row = 552
    struct.pack_into("<I", data, row + 72, 17)
    offset = 136
    for index, text in [(14, ""), (17, "Title"), (20, "/Contents/track.mp3")]:
        raw = text.encode("ascii")
        struct.pack_into("<H", data, row + 94 + index * 2, offset)
        data[row + offset] = (len(raw) + 1) * 2 + 1
        data[row + offset + 1:row + offset + 1 + len(raw)] = raw
        offset += len(raw) + 1
    struct.pack_into("<H", data, 1020, 1)
    return data


class ReaderTests(unittest.TestCase):
    def test_valid_and_deleted_row(self):
        data = fixture()
        self.assertEqual(track_paths(data), {17: {"id": 17, "path": "/Contents/track.mp3", "analysis_path": "", "title": "Title"}})
        data[1020] = 0
        self.assertEqual(track_paths(data), {})

    def test_header_page_and_row_bounds(self):
        for offset, value in [(4, 0), (8, 65), (36, 100), (516, 9), (520, 1)]:
            data = fixture()
            struct.pack_into("<I", data, offset, value)
            with self.assertRaises(PdbFormatError):
                track_paths(data)
        data = fixture()
        struct.pack_into("<H", data, 1018, 500)
        with self.assertRaises(PdbFormatError):
            track_paths(data)
        for length in (0, 27, 512, 1023):
            with self.assertRaises(PdbFormatError):
                track_paths(fixture()[:length])

    def test_cycle_and_duplicate_identity(self):
        data = fixture() + fixture()[512:]
        struct.pack_into("<I", data, 40, 2)
        struct.pack_into("<I", data, 524, 1)
        with self.assertRaises(PdbFormatError):
            track_paths(data)
        struct.pack_into("<I", data, 524, 2)
        struct.pack_into("<I", data, 1028, 2)
        with self.assertRaises(PdbFormatError):
            track_paths(data)

    def test_string_bounds_and_encoding(self):
        for mutation in (0, 1, 2, 0x90):
            data = fixture()
            data[688] = mutation
            with self.assertRaises(PdbFormatError):
                track_paths(data)
        data = fixture()
        struct.pack_into("<H", data, 552 + 94 + 20 * 2, 500)
        with self.assertRaises(PdbFormatError):
            track_paths(data)


if __name__ == "__main__":
    unittest.main()

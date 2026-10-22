# SPDX-License-Identifier: GPL-3.0-only
"""Bounded research reader for classic export.pdb track paths, not exportExt.

Format reference: Deep Symmetry crate-digger v0.2.1 rekordbox_pdb.ksy.
Independent implementation; no firmware code or generated parser is included.
"""
import struct


class PdbFormatError(ValueError):
    pass


def track_paths(data):
    """Return ID/path/analysis/title records; reject malformed active track chains."""
    def require(condition):
        if not condition:
            raise PdbFormatError("Invalid or unsupported PDB structure")

    def u16(buffer, offset):
        require(0 <= offset <= len(buffer) - 2)
        return struct.unpack_from("<H", buffer, offset)[0]

    def u32(buffer, offset):
        require(0 <= offset <= len(buffer) - 4)
        return struct.unpack_from("<I", buffer, offset)[0]

    require(28 <= len(data) <= 256 * 1024 * 1024)
    page_size, tables = u32(data, 4), u32(data, 8)
    require(512 <= page_size <= 65536 and page_size & (page_size - 1) == 0)
    require(len(data) % page_size == 0 and 1 <= tables <= 64 and 28 + tables * 16 <= page_size)
    tracks = [28 + index * 16 for index in range(tables) if u32(data, 28 + index * 16) == 0]
    require(len(tracks) == 1)
    table = tracks[0]
    current, last = u32(data, table + 8), u32(data, table + 12)
    pages = len(data) // page_size
    require(0 < current < pages and 0 < last < pages)
    seen, rows = set(), {}
    while True:
        require(0 < current < pages and current not in seen)
        seen.add(current)
        page = memoryview(data)[current * page_size:(current + 1) * page_size]
        require(u32(page, 4) == current and u32(page, 8) == 0)
        if not page[27] & 0x40:
            small, large = page[24], u16(page, 34)
            count = large if large > small and large != 0x1fff else small
            groups = (count + 15) // 16
            heap_end = page_size - groups * 36
            require(heap_end >= 40)
            for group in range(groups):
                base = page_size - group * 36
                present = u16(page, base - 4)
                for index in range(16):
                    if not present & (1 << index):
                        continue
                    require(group * 16 + index < count)
                    row = 40 + u16(page, base - 6 - 2 * index)
                    require(40 <= row <= heap_end - 136)

                    def text(string_index):
                        offset = row + u16(page, row + 94 + string_index * 2)
                        require(row + 136 <= offset < heap_end)
                        kind = page[offset]
                        if kind in (0x40, 0x90):
                            require(offset + 4 <= heap_end)
                            length = u16(page, offset + 1)
                            require(length >= 4 and offset + length <= heap_end)
                            raw = bytes(page[offset + 4:offset + length])
                            encoding = "utf-16le" if kind == 0x90 else "ascii"
                        else:
                            require(kind & 1 and kind >= 3)
                            length = (kind >> 1) - 1
                            require(offset + 1 + length <= heap_end)
                            raw = bytes(page[offset + 1:offset + 1 + length])
                            encoding = "ascii"
                        try:
                            value = raw.decode(encoding)
                        except UnicodeError as error:
                            raise PdbFormatError("Invalid PDB string encoding") from error
                        require("\0" not in value and len(value.encode("utf-8")) < 4096)
                        return value

                    identity = u32(page, row + 72)
                    require(identity and identity not in rows and len(rows) < 100000)
                    rows[identity] = {"id": identity, "path": text(20), "analysis_path": text(14), "title": text(17)}
        if current == last:
            return rows
        current = u32(page, 12)

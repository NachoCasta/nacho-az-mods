# SPDX-License-Identifier: GPL-3.0-only
"""Historical rejected custom badge prototype; not used by the native adapter.

Rebuild original USB hub badge PNGs and their C header (Python standard library).

All glyphs and geometry below are original project artwork, distributed under
GPL-3.0-only. No font files, native assets, screenshots or external images are used.
Pixels are supersampled 3x with deterministic box filtering. PNG encoding uses
RGBA8, filter 0 and zlib level 9; the header embeds exactly these PNG bytes.
Usage: python generate-source-badges.py [--preview-dir local/badge-previews]
"""
import argparse
import pathlib
import struct
import zlib

# Original monoline sans glyph centerlines, in a 5 by 7 design grid.
PATHS = {
    'H': (((0,0),(0,7)), ((5,0),(5,7)), ((0,3.5),(5,3.5))),
    'U': (((0,0),(0,5.5),(.5,6.5),(1.5,7),(3.5,7),(4.5,6.5),(5,5.5),(5,0)),),
    'B': (((0,7),(0,0),(3.2,0),(4.6,.6),(5,1.6),(4.6,2.8),(3.3,3.5),(0,3.5)),
          ((3.3,3.5),(4.6,4.1),(5,5.3),(4.6,6.4),(3.2,7),(0,7))),
    'S': (((5,.5),(3.8,0),(1.2,0),(0,1),(0,2.4),(1,3.3),(4,3.8),(5,4.7),(5,6),(3.8,7),(1.2,7),(0,6.5)),),
    '1': (((1,1.3),(2.7,0),(2.7,7)), ((.8,7),(4.6,7))),
    '2': (((0,1),(1,0),(3.8,0),(5,1),(5,2.4),(0,7),(5,7)),),
    '3': (((0,.5),(1,0),(3.8,0),(5,1),(5,2.4),(3.4,3.5),(1.8,3.5)),
          ((3.4,3.5),(5,4.6),(5,6),(3.8,7),(1,7),(0,6.5))),
    '4': (((4,7),(4,0),(0,4.8),(5,4.8)),),
    '5': (((5,0),(0,0),(0,3.3),(3.7,3.3),(5,4.3),(5,6),(3.8,7),(1,7),(0,6.4)),),
    '6': (((4.8,.5),(3.8,0),(1.3,0),(0,1.4),(0,5.7),(1.2,7),(3.8,7),(5,5.8),(5,4.4),(3.8,3.3),(1.2,3.3),(0,4.4)),),
    '7': (((0,0),(5,0),(1.5,7)),),
    '8': (((1.2,3.5),(0,2.4),(0,1.2),(1.2,0),(3.8,0),(5,1.2),(5,2.4),(3.8,3.5),(1.2,3.5),(0,4.6),(0,5.8),(1.2,7),(3.8,7),(5,5.8),(5,4.6),(3.8,3.5)),),
}
WHITE = (246, 247, 248, 255)
DARK = (25, 28, 31, 255)
GRAY = (202, 207, 211, 255)

class Canvas:
    def __init__(self, width, height):
        self.width, self.height = width, height
        self.pixels = bytearray(width * height * 9 * 4)

    def rect(self, x, y, width, height, color):
        for py in range(round(y * 3), round((y + height) * 3)):
            for px in range(round(x * 3), round((x + width) * 3)):
                if 0 <= px < self.width * 3 and 0 <= py < self.height * 3:
                    start = (py * self.width * 3 + px) * 4
                    self.pixels[start:start + 4] = bytes(color)

    def line(self, x1, y1, x2, y2, thickness, color):
        radius = thickness / 2
        dx, dy = x2 - x1, y2 - y1
        distance = dx * dx + dy * dy
        for py in range(max(0, int((min(y1, y2) - radius) * 3)), min(self.height * 3, int((max(y1, y2) + radius) * 3) + 1)):
            for px in range(max(0, int((min(x1, x2) - radius) * 3)), min(self.width * 3, int((max(x1, x2) + radius) * 3) + 1)):
                x, y = (px + .5) / 3, (py + .5) / 3
                t = max(0, min(1, ((x - x1) * dx + (y - y1) * dy) / distance))
                if (x - x1 - t * dx) ** 2 + (y - y1 - t * dy) ** 2 <= radius ** 2:
                    start = (py * self.width * 3 + px) * 4
                    self.pixels[start:start + 4] = bytes(color)

    def text(self, text, x, y, scale, color=WHITE):
        for letter in text:
            if letter != ' ':
                for path in PATHS[letter]:
                    for (x1, y1), (x2, y2) in zip(path, path[1:]):
                        self.line(x + x1 * scale, y + y1 * scale,
                                  x + x2 * scale, y + y2 * scale, .7 * scale, color)
            x += (4 if letter == ' ' else 6.5) * scale

    def png(self):
        raw = bytearray()
        for y in range(self.height):
            raw.append(0)
            for x in range(self.width):
                samples = [(sy * self.width * 3 + sx) * 4
                           for sy in range(y * 3, y * 3 + 3)
                           for sx in range(x * 3, x * 3 + 3)]
                alpha = sum(self.pixels[i + 3] for i in samples)
                for channel in range(3):
                    raw.append(round(sum(self.pixels[i + channel] * self.pixels[i + 3]
                                         for i in samples) / alpha) if alpha else 0)
                raw.append(round(alpha / 9))
        def chunk(kind, data):
            return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
        return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', self.width, self.height, 8, 6, 0, 0, 0))
                + chunk(b'IDAT', zlib.compress(bytes(raw), 9)) + chunk(b'IEND', b''))

def stick(canvas, center, top, scale=1):
    # A plain USB stick: exposed connector, inset contacts and solid body.
    canvas.rect(center - 7 * scale, top, 14 * scale, 10 * scale, WHITE)
    canvas.rect(center - 4 * scale, top + 2 * scale, 2 * scale, 4 * scale, DARK)
    canvas.rect(center + 2 * scale, top + 2 * scale, 2 * scale, 4 * scale, DARK)
    canvas.rect(center - 11 * scale, top + 9 * scale, 22 * scale, 24 * scale, WHITE)
    canvas.rect(center - 3 * scale, top + 28 * scale, 6 * scale, 2 * scale, DARK)

def badge(family, number):
    if family == 'source':
        canvas = Canvas(90, 90)
        canvas.rect(13, 4, 64, 18, GRAY)
        canvas.text('HUB', 28, 6, 2, DARK)
        stick(canvas, 45, 28)
        canvas.text('USB ' + str(number), 17, 70, 2)
    elif family == 'browse':
        canvas = Canvas(95, 70)
        canvas.rect(21, 1, 53, 14, GRAY)
        canvas.text('HUB', 35, 3, 1.5, DARK)
        stick(canvas, 47.5, 19, .8)
        canvas.text('USB ' + str(number), 26.5, 53, 1.5)
    else:
        canvas = Canvas(170, 41)
        canvas.rect(7, 9.5, 22, 22, WHITE)
        canvas.rect(8.5, 11, 19, 19, (0, 0, 0, 0))
        canvas.text('H', 12.5, 12.8, 2.2)
        canvas.text('USB ' + str(number), 38, 12, 2.3)
    return canvas.png()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--preview-dir', type=pathlib.Path)
    args = parser.parse_args()
    root = pathlib.Path(__file__).resolve().parents[2]
    lines = ['/* SPDX-License-Identifier: GPL-3.0-only */',
             '/* Generated by research/scripts/generate-source-badges.py; original project artwork. */',
             '#ifndef HUB_SOURCE_BADGE_ASSETS_H', '#define HUB_SOURCE_BADGE_ASSETS_H',
             '#include <stddef.h>',
             'enum { HUB_BADGE_SOURCE, HUB_BADGE_BROWSE, HUB_BADGE_WAVEFORM };']
    names = []
    for family in ('source', 'browse', 'waveform'):
        family_names = []
        for number in range(1, 9):
            data = badge(family, number)
            name = 'hub_badge_' + family + '_' + str(number)
            family_names.append(name)
            lines.append('static const unsigned char ' + name + '[] = {')
            for offset in range(0, len(data), 16):
                lines.append('    ' + ','.join('0x%02x' % byte for byte in data[offset:offset + 16]) + ',')
            lines.append('};')
            if args.preview_dir:
                args.preview_dir.mkdir(parents=True, exist_ok=True)
                (args.preview_dir / (name + '.png')).write_bytes(data)
        names.append(family_names)
    lines += ['struct hub_badge_png { const unsigned char *data; size_t size; };',
              'static const struct hub_badge_png hub_badge_pngs[3][8] = {']
    for family_names in names:
        lines.append('    {' + ','.join('{%s,sizeof %s}' % (name, name) for name in family_names) + '},')
    lines += ['};', 'static const unsigned char *source_badge_asset(int family,int display_number,size_t *length) {',
              '    if (length) *length=0;',
              '    if (family<0 || family>2 || display_number<1 || display_number>8) return NULL;',
              '    const struct hub_badge_png *asset=&hub_badge_pngs[family][display_number-1];',
              '    if (length) *length=asset->size;', '    return asset->data;', '}', '#endif', '']
    (root / 'linkfake/source-badge-assets.h').write_text('\n'.join(lines), encoding='ascii')

if __name__ == '__main__':
    main()

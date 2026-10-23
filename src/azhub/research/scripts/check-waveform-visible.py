#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Assert visible waveform content in an unscaled EP147 two-deck screenshot.

This checks both graph areas, excluding source labels, controls, time displays,
the white playhead and the overview's bottom phrase strip. It does not prove
audio correctness, analysis identity or waveform coverage in other UI layouts.
Requires Pillow. Screenshots remain local; no image is rewritten or uploaded.
"""
import argparse
import json
from pathlib import Path
from PIL import Image


def measure(image, box):
    region = image.crop(box).convert('RGB')
    width, height = region.size
    pixels = region.load()
    count = 0
    columns, rows = set(), set()
    for y in range(height):
        for x in range(width):
            channels = pixels[x, y]
            # Native grid/playhead are neutral; graph samples are colored.
            if max(channels) >= 70 and max(channels)-min(channels) >= 45:
                count += 1
                columns.add(x)
                rows.add(y)
    # A flat colored baseline, marker or isolated label cannot satisfy all three.
    visible = count >= 1000 and len(columns) >= 150 and len(rows) >= 12
    return {'colored_pixels': count, 'occupied_columns': len(columns),
            'occupied_rows': len(rows), 'visible': visible, 'box': box}


def inspect(path, deck):
    with Image.open(path) as image:
        if image.size != (1280, 800):
            raise ValueError('Expected unscaled 1280x800 EP147 two-deck screenshot')
        detail = (410, 112, 1055, 230) if deck == 1 else (410, 315, 1055, 435)
        overview = (30, 730, 620, 770) if deck == 1 else (670, 730, 1250, 770)
        result = {'deck': deck, 'detail': measure(image, detail),
                  'overview': measure(image, overview)}
        result['visible'] = result['detail']['visible'] and result['overview']['visible']
        return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('screenshot', type=Path)
    parser.add_argument('--deck', type=int, choices=(1, 2), required=True)
    args = parser.parse_args()
    result = inspect(args.screenshot, args.deck)
    print(json.dumps(result, indent=2))
    raise SystemExit(0 if result['visible'] else 1)

#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Check real native-positive and blank-hub-negative local screenshot controls.

Screenshots must come from the same 1280x800 two-deck comparison capture.
This exercises the CLI exit status as well as both independent graph assertions.
It does not establish media identity; retain that provenance separately.
"""
import argparse
import json
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('comparison', type=Path)
    parser.add_argument('--native-deck', type=int, choices=(1, 2), default=1)
    args = parser.parse_args()
    checker = Path(__file__).with_name('check-waveform-visible.py')
    results = {}
    for label, deck, visible in [('native', args.native_deck, True),
                                 ('blank_hub', 3-args.native_deck, False)]:
        run = subprocess.run([sys.executable, str(checker), str(args.comparison),
                              '--deck', str(deck)], capture_output=True, text=True,
                             timeout=30)
        assert run.returncode == (0 if visible else 1), (label, run.stdout, run.stderr)
        result = json.loads(run.stdout)
        assert result['visible'] is visible, result
        assert result['detail']['visible'] is visible, result
        assert result['overview']['visible'] is visible, result
        results[label] = result
    print(json.dumps(results, indent=2))


if __name__ == '__main__':
    main()

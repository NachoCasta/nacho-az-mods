#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Cross-check C framing against the existing Python research wire codec."""
import random
import struct
import subprocess
import sys
import link_db_wire as wire


class Stream:
    def __init__(self, data):
        self.data = data

    def recv(self, size):
        part, self.data = self.data[:size], self.data[size:]
        return part


def main():
    assert len(sys.argv) == 2, "C_TEST_EXECUTABLE"
    rng = random.Random(147)
    frames, expected = [], []
    for tx in range(100):
        args = []
        for i in range(rng.randrange(17)):
            choice = rng.randrange(3)
            if choice == 0:
                args.append(rng.randrange(1, 2**32))
            elif choice == 1:
                args.append("Fixture \U0001f3b5 \u00e1" * rng.randrange(5))
            else:
                args.append(bytes(rng.randrange(256) for _ in range(1 + rng.randrange(32))))
        if tx % 10 == 0:
            args = [0, b"", 7]
        kind = 0x4101 if tx % 2 else 0x3000
        frame = wire.packet(tx, kind, args)
        if tx % 3 == 0:
            width = 16 if len(args) > 12 else 12
            frame = frame[:16] + struct.pack("!I", len(args)) + frame[20:20+len(args)] + frame[20+width:]
        frames.append(frame)
        expected.append({"transaction": tx, "type": kind, "arguments": args})
    result = subprocess.run([sys.argv[1], "normalize"], input=b"".join(frames), capture_output=True, check=True)
    stream = Stream(result.stdout)
    for message in expected:
        assert wire.message(stream) == message
    assert not stream.data
    print("PASS:100 Python-generated mixed/Unicode/empty/short-type-array messages normalize through C and decode identically")


if __name__ == "__main__":
    main()

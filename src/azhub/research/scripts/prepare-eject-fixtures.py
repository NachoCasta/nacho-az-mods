#!/usr/bin/env python3
"""Create disposable empty FAT fixtures using only the localhost emulator.

Writes four 32 MiB images under guest /tmp and downloads them to ignored local/.
Builds one two-partition MBR image from those FAT volumes. No mounting, QMP device
changes, real AZ access, music/firmware copying or package installation occurs.
"""
import argparse
import importlib.util
from pathlib import Path
import shutil
import struct
import subprocess
import sys

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("suite", HERE / "test-safe-eject-emulator.py")
t = importlib.util.module_from_spec(spec)
spec.loader.exec_module(t)


def run(output):
    root = Path(__file__).resolve().parents[4]
    output = output.resolve()
    assert output.is_relative_to(root / "local")
    assert t.ssh("uname -r").strip() == "6.6.138-dirty"
    output.mkdir(parents=True, exist_ok=True)
    for letter in "CDEF":
        remote = f"/tmp/eject-fallback-{letter}.img"
        t.ssh(f"dd if=/dev/zero of={remote} bs=1048576 count=32 2>/dev/null; /sbin/mkfs.vfat -n EJECT_{letter} {remote}")
        subprocess.run([sys.executable, str(HERE / "eject-ui-emu.py"), "get", remote,
                        str(output / f"eject-fallback-{letter}.img")], check=True)
    sectors = 65536
    starts = (2048, 2048 + sectors)
    target = output / "eject-two-partitions.img"
    with target.open("wb") as out:
        out.truncate((starts[1] + sectors) * 512)
        mbr = bytearray(512)
        for index, (start, letter) in enumerate(zip(starts, "CD")):
            image = output / f"eject-fallback-{letter}.img"
            assert image.stat().st_size == sectors * 512
            with image.open("rb") as src:
                boot = src.read(512)
            assert boot[510:512] == b"\x55\xaa"
            kind = 0x0E if boot[54:59] == b"FAT16" else 0x0C
            mbr[446 + 16 * index:462 + 16 * index] = struct.pack(
                "<B3sB3sII", 0, b"\xfe\xff\xff", kind, b"\xfe\xff\xff", start, sectors)
        mbr[510:512] = b"\x55\xaa"
        out.seek(0)
        out.write(mbr)
        for start, letter in zip(starts, "CD"):
            out.seek(start * 512)
            with (output / f"eject-fallback-{letter}.img").open("rb") as src:
                shutil.copyfileobj(src, out)
    print("Disposable FAT fixtures ready under", output)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    run(args.output)

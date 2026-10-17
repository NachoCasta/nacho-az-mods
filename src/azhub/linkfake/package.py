#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Package the already-built SDK mod and the exact AZ kernel asset."""
import hashlib
import json
from pathlib import Path
import zipfile

ROOT = Path(__file__).resolve().parent
manifest = json.loads((ROOT / "azmod.json").read_text(encoding="utf-8"))
mod_id = manifest["id"]
files = {
    "azmod.json": ROOT / "azmod.json",
    mod_id + ".so": ROOT / (mod_id + ".so"),
    "assets/azhub.ko": ROOT.parent / "azhub.ko",
    "assets/mount-hub.sh": ROOT / "assets/mount-hub.sh",
    "README.md": ROOT / "AZ-TEST.md",
    "LICENSE": ROOT.parents[2] / "LICENSE",
}
# Prevent silently packaging a host build or a different kernel asset.
for name in (mod_id + ".so", "assets/azhub.ko"):
    data = files[name].read_bytes()
    if data[:6] != b"\x7fELF\x02\x01" or int.from_bytes(data[18:20], "little") != 183:
        raise SystemExit(name + ": expected little-endian AArch64 ELF64")
if b"vermagic=4.4.194 SMP preempt mod_unload aarch64" not in files["assets/azhub.ko"].read_bytes():
    raise SystemExit("unexpected kernel module vermagic")
out = ROOT / "dist"
out.mkdir(exist_ok=True)
archive = out / (mod_id + "-" + manifest["version"] + "-dev.zip")
with zipfile.ZipFile(archive, "w", compression=zipfile.ZIP_DEFLATED) as package:
    for name, path in files.items():
        entry = zipfile.ZipInfo(name, date_time=(2026, 10, 17, 0, 0, 0))
        entry.compress_type = zipfile.ZIP_DEFLATED
        entry.external_attr = 0o100644 << 16
        package.writestr(entry, path.read_bytes())
with zipfile.ZipFile(archive) as package:
    assert package.testzip() is None
    assert set(package.namelist()) == set(files)
digest = hashlib.sha256(archive.read_bytes()).hexdigest()
archive.with_suffix(".zip.sha256").write_text(digest + "  " + archive.name + "\n", encoding="ascii")
print(archive)
print("sha256=" + digest)

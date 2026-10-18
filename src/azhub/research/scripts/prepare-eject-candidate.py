#!/usr/bin/env python3
"""Prepare and validate a local eject candidate using the pinned SDK packer.

Copies only the manifest, explicit ELF, icon, license and existing mod assets into
a separate staging directory. Does not build, deploy, create tags, publish, update
the root catalog or change the source manifest. Output must be inside ignored local/.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(binary, version, packer, output):
    root = Path(__file__).resolve().parents[4]
    source = root / "src/azhub/linkfake"
    output = output.resolve()
    assert output.is_relative_to(root / "local"), "Candidate output must stay in ignored local/"
    manifest = json.loads((source / "azmod.json").read_text(encoding="utf-8"))
    assert manifest["id"] == "usb-hub-support"
    assert manifest["firmware"] == ["ep147-9e9454e357389ece"]
    assert re.fullmatch(r"(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)", version)
    catalog = json.loads((root / "azmods-repo.json").read_text(encoding="utf-8"))
    public = next(item for item in catalog["mods"] if item["id"] == manifest["id"])
    assert tuple(map(int, version.split("."))) > tuple(map(int, public["version"].split(".")))
    stage = output / "mod"
    stage.mkdir(parents=True, exist_ok=True)
    manifest["version"] = version
    # Preserve the reviewed source description, including resource disclosures.
    assert len(manifest["description"].encode()) <= 599
    (stage / "azmod.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    shutil.copy2(binary, stage / "usb-hub-support.so")
    for name in ("icon.png", "LICENSE"):
        shutil.copy2(source / name, stage / name)
    shutil.copytree(source / "assets", stage / "assets", dirs_exist_ok=True)
    assert {p.relative_to(stage / "assets").as_posix() for p in (stage / "assets").rglob("*") if p.is_file()} == {
        "azhub.ko", "azhub-LICENSE.txt", "mount-hub.sh"}
    for command in (("validate", str(stage)), ("package", str(stage), "--out", str(output / "dist"))):
        subprocess.run([sys.executable, str(packer), *command], check=True)
    package = output / "dist" / f"usb-hub-support-{version}.azmod"
    subprocess.run([sys.executable, str(packer), "validate", str(package)], check=True)
    record = {"version": version, "status": "Local candidate; not published or hardware accepted",
              "binary_sha256": sha(binary), "package_sha256": sha(package),
              "package_bytes": package.stat().st_size, "firmware": manifest["firmware"],
              "source_manifest_unchanged": True, "catalog_unchanged": True,
              "assets": {p.name: sha(p) for p in (stage / "assets").iterdir() if p.is_file()}}
    (output / "candidate.json").write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(record, indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--packer", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    run(args.binary, args.version, args.packer, args.output)

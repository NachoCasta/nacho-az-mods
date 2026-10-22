#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Local legacy cue wire control with explicit private source/fixture manifests."""
from pathlib import Path
import json
import select
import shutil
import subprocess
import sys
import tempfile


def main():
    assert len(sys.argv) == 6, "SERVER CLASSPATH PDB PRIVATE_SNAPSHOT PRIVATE_FIXTURE_MANIFEST"
    tracks = json.loads(Path(sys.argv[4]).read_text(encoding="utf-8"))["tracks"]
    manifest = Path(sys.argv[5])
    fixtures = json.loads(manifest.read_text())["matches"]
    selected = []
    with tempfile.TemporaryDirectory(prefix="azhub-cue-export-") as directory:
        root = Path(directory)
        pdb = root / "PIONEER/rekordbox/export.pdb"
        pdb.parent.mkdir(parents=True)
        shutil.copyfile(sys.argv[3], pdb)
        for fixture in fixtures:
            path = fixture["private_source"]
            marker = path.index("/PIONEER/")
            relative = path[marker + 1:]
            assert ".." not in Path(relative).parts and not Path(relative).is_absolute()
            matches = [row for row in tracks if row["analysis_path"].lstrip("/") == relative]
            assert len(matches) == 1, "Fixture must belong to exactly one PDB track"
            target = root / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            source = manifest.parent / fixture["file"]
            shutil.copyfile(source, target)
            shutil.copyfile(source.with_suffix(".EXT"), target.with_suffix(".EXT"))
            selected.append((matches[0]["id"], source))
        server = subprocess.Popen([sys.argv[1], directory], stdin=subprocess.PIPE,
                                  stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        try:
            assert select.select([server.stdout], [], [], 15)[0], "Readiness timeout"
            port = int(server.stdout.readline())
            for identity, source in selected:
                result = subprocess.run(["java", "-cp", sys.argv[2], "DbCueControl", str(port),
                                         str(identity), str(source)], capture_output=True, text=True, timeout=20)
                if result.stderr:
                    print(result.stderr, file=sys.stderr)
                assert result.returncode == 0, "Independent cue control failed"
                print(result.stdout.strip())
            server.stdin.write("Q")
            server.stdin.flush()
            assert server.wait(timeout=5) == 0
        finally:
            if server.poll() is None:
                server.kill()
                server.wait(timeout=5)
            errors = server.stderr.read()
            if errors:
                print(errors, file=sys.stderr)
            for stream in (server.stdin, server.stdout, server.stderr):
                stream.close()


if __name__ == "__main__":
    main()

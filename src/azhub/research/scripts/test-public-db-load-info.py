#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Local load-info control with one explicitly selected copied audio fixture."""
from pathlib import Path
import json
import select
import shutil
import subprocess
import sys
import tempfile


def main():
    assert len(sys.argv) == 6, "SERVER CLASSPATH PDB AUDIO PRIVATE_SNAPSHOT"
    tracks = json.loads(Path(sys.argv[5]).read_text(encoding="utf-8"))["tracks"]
    matches = [row for row in tracks if row["file_size"] == Path(sys.argv[4]).stat().st_size]
    assert len(matches) == 1, "Audio size must identify exactly one explicit fixture track"
    track = matches[0]
    relative = track["path"].lstrip("/")
    assert relative and not Path(relative).is_absolute() and ".." not in Path(relative).parts
    with tempfile.TemporaryDirectory(prefix="azhub-load-info-") as directory:
        for source, path in [(sys.argv[3], "PIONEER/rekordbox/export.pdb"), (sys.argv[4], relative)]:
            target = Path(directory, path)
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, target)
        server = subprocess.Popen([sys.argv[1], directory], stdin=subprocess.PIPE,
                                  stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        try:
            assert select.select([server.stdout], [], [], 15)[0], "Readiness timeout"
            port = int(server.stdout.readline())
            client = subprocess.run(["java", "-cp", sys.argv[2], "DbLoadInfoControl", str(port),
                                     sys.argv[3], str(track["id"])], capture_output=True, text=True, timeout=20)
            if client.stderr:
                print(client.stderr, file=sys.stderr)
            assert client.returncode == 0, "Independent load-info control failed"
            print(client.stdout.strip())
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

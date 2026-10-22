#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Local analysis response control using explicitly copied private fixtures."""
from pathlib import Path
import json
import select
import shutil
import socket
import subprocess
import sys
import tempfile


def main():
    assert len(sys.argv) == 7, "SERVER CLASSPATH PDB PRIVATE_SNAPSHOT FIXTURE_DIRECTORY ORACLE_DIRECTORY"
    tracks = json.loads(Path(sys.argv[4]).read_text(encoding="utf-8"))["tracks"]
    matches = [row for row in tracks if row["file_size"] == 10881597]
    assert len(matches) == 1, "Selected source fixture must be unique"
    track = matches[0]
    relative = track["analysis_path"].lstrip("/")
    assert relative.endswith(".DAT") and not Path(relative).is_absolute() and ".." not in Path(relative).parts
    with tempfile.TemporaryDirectory(prefix="azhub-analysis-export-") as directory:
        pdb = Path(directory, "PIONEER/rekordbox/export.pdb")
        pdb.parent.mkdir(parents=True)
        shutil.copyfile(sys.argv[3], pdb)
        for extension in ("DAT", "EXT", "2EX"):
            target = Path(directory, relative[:-3] + extension)
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(Path(sys.argv[5], "fixture." + extension), target)
        server = subprocess.Popen([sys.argv[1], directory], stdin=subprocess.PIPE,
                                  stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        try:
            assert select.select([server.stdout], [], [], 15)[0], "Readiness timeout"
            port = int(server.stdout.readline())
            clients = []
            # An accepted peer that never sends its greeting must not block
            # independent sessions, including their large analysis replies.
            with socket.create_connection(("127.0.0.1", port), timeout=5) as stalled:
                try:
                    for _ in range(2):
                        clients.append(subprocess.Popen(
                            ["java", "-cp", sys.argv[2], "DbAnalysisControl", str(port),
                             str(track["id"]), sys.argv[6], sys.argv[5]], stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, text=True))
                    for client in clients:
                        output, errors = client.communicate(timeout=25)
                        if errors:
                            print(errors, file=sys.stderr)
                        assert client.returncode == 0, "Independent analysis control failed"
                        print(output.strip())
                    print("PASS: two concurrent analysis clients complete with a stalled greeting peer")
                finally:
                    for client in clients:
                        if client.poll() is None:
                            client.kill()
                        client.communicate(timeout=5)
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

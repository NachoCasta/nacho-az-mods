#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Read-only C live-index browsing using unchanged Beat Link over local TCP."""
from pathlib import Path
import select
import shutil
import subprocess
import sys
import tempfile


def main():
    assert len(sys.argv) == 5, "SERVER CLASSPATH PDB ORACLE_TSV"
    with tempfile.TemporaryDirectory(prefix="azhub-db-library-") as directory:
        pdb = Path(directory, "PIONEER/rekordbox/export.pdb")
        pdb.parent.mkdir(parents=True)
        shutil.copyfile(sys.argv[3], pdb)
        server = subprocess.Popen([sys.argv[1], directory], stdin=subprocess.PIPE,
                                  stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        try:
            assert select.select([server.stdout], [], [], 15)[0], "Readiness timeout"
            port = int(server.stdout.readline())
            client = subprocess.run(["java", "-cp", sys.argv[2], "DbLibraryControl", str(port),
                                     sys.argv[3], sys.argv[4]], capture_output=True, text=True, timeout=105)
            if client.stderr:
                print(client.stderr, file=sys.stderr)
            assert client.returncode == 0, "Independent browse failed"
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

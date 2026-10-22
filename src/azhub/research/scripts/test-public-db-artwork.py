#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Local artwork response control using an explicit prepared fixture root."""
import select
import subprocess
import sys


def main():
    assert len(sys.argv) == 6, "SERVER CLASSPATH FIXTURE_ROOT ART_ID JPEG_FILE"
    server = subprocess.Popen([sys.argv[1], sys.argv[3]], stdin=subprocess.PIPE,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try:
        # Large private PDB fixtures on Windows-mounted WSL paths can take
        # longer while checking thousands of absent audio/analysis/art files.
        assert select.select([server.stdout], [], [], 60)[0], "Readiness timeout"
        port = int(server.stdout.readline())
        client = subprocess.run(["java", "-cp", sys.argv[2], "DbArtworkControl", str(port),
                                 sys.argv[4], sys.argv[5]], capture_output=True, text=True, timeout=20)
        if client.stderr:
            print(client.stderr, file=sys.stderr)
        assert client.returncode == 0, "Independent artwork control failed"
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

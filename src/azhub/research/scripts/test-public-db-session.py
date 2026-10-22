#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Local C handshake with an idle socket and unchanged Beat Link client."""
import select
import socket
import subprocess
import sys


def main():
    assert len(sys.argv) == 3, "SERVER JAVA_CLASSPATH"
    server = subprocess.Popen([sys.argv[1]], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try:
        assert select.select([server.stdout], [], [], 3)[0], "Server readiness timeout"
        port = int(server.stdout.readline())
        with socket.create_connection(("127.0.0.1", port), timeout=2) as idle:
            idle.sendall(b"\x11")
            client = subprocess.run(["java", "-cp", sys.argv[2], "DbSessionControl", str(port)],
                                    capture_output=True, text=True, timeout=6)
            assert client.returncode == 0, client.stderr
            print(client.stdout.strip())
        output, errors = server.communicate(timeout=5)
        assert server.returncode == 0, errors
        print(output.strip())
    finally:
        if server.poll() is None:
            server.kill()
            server.wait(timeout=3)
        server.stdout.close()
        server.stderr.close()


if __name__ == "__main__":
    main()

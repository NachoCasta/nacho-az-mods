#!/usr/bin/env python3
"""Run inside a fresh `unshare -n` namespace; no emulator/physical NIC access."""
from pathlib import Path
import hashlib
import shutil
import select
import socket
import subprocess
import sys
import tempfile


def main():
    assert len(sys.argv) in (3, 5), "Pass C server, Java classpath and optional fixture/virtual path"
    assert Path("/proc/self/ns/net").readlink() != Path("/proc/1/ns/net").readlink()
    # Existing sysfs mounts can describe the initial namespace after unshare.
    # Query the process's actual network namespace instead.
    assert {name for _, name in socket.if_nameindex()} == {"lo"}
    subprocess.run(["ip", "link", "set", "lo", "up"], check=True)
    with tempfile.TemporaryDirectory(prefix="azhub-crate-c-service-") as directory:
        relative = Path(sys.argv[4]) if len(sys.argv) == 5 else Path("Music", "Álbum 🎵", "track.bin")
        assert not relative.is_absolute() and ".." not in relative.parts
        path = Path(directory, relative)
        path.parent.mkdir(parents=True)
        java_args = []
        if len(sys.argv) == 5:
            fixture = Path(sys.argv[3])
            assert fixture.is_file() and fixture.stat().st_size <= 32 * 1024 * 1024
            shutil.copyfile(fixture, path)
            java_args = [relative.as_posix(), str(path.stat().st_size), hashlib.sha256(path.read_bytes()).hexdigest()]
        else:
            path.write_bytes(bytes((i * 37 + i // 256) % 256 for i in range(30001)))
        server = subprocess.Popen([sys.argv[1], directory, relative.as_posix(), "--isolated-portmapper"],
                                  stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        try:
            assert select.select([server.stdout], [], [], 10)[0], "No server readiness"
            ready = server.stdout.readline().split()
            assert len(ready) == 2 and ready[0] == b"111", (ready, server.poll())
            client = subprocess.run(["java", "-cp", sys.argv[2], "LocalFileServiceClient", *java_args],
                                    capture_output=True, text=True, timeout=45)
            print(client.stdout, end="")
            print(client.stderr, end="", file=sys.stderr)
            assert client.returncode == 0, client.returncode
            server.stdin.write(b"Q")
            server.stdin.flush()
            assert server.wait(timeout=5) == 0
            print("PASS: unchanged Crate Digger library mounts and fetches exact fixture bytes from C UDP service")
        finally:
            if server.poll() is None:
                server.kill()
                server.wait(timeout=5)
            errors = server.stderr.read().decode(errors="replace")
            if errors:
                print(errors, file=sys.stderr)
            for stream in (server.stdin, server.stdout, server.stderr):
                stream.close()


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""PDB-populated C export control inside a fresh loopback-only namespace."""
import hashlib
import os
from pathlib import Path
import select
import shutil
import socket
import struct
import subprocess
import sys
import tempfile


def main():
    assert len(sys.argv) in (5, 6), "SERVER CLASSPATH PDB AUDIO [ENTITY_ORACLE_FOR_COMBINED_SERVER]"
    combined = len(sys.argv) == 6
    assert Path("/proc/self/ns/net").readlink() != Path("/proc/1/ns/net").readlink()
    assert {name for _, name in socket.if_nameindex()} == {"lo"}
    subprocess.run(["ip", "link", "set", "lo", "up"], check=True)
    with tempfile.TemporaryDirectory(prefix="azhub-library-export-") as directory:
        fixtures = [
            ("database", "PIONEER/rekordbox/export.pdb", Path(sys.argv[3])),
            ("audio", "Contents/Aaliyah/Are You That Somebody/Aaliyah - Are You That Somebody-1.mp3", Path(sys.argv[4])),
        ]
        for _, relative, source in fixtures:
            assert source.is_file() and source.stat().st_size <= 32 * 1024 * 1024
            target = Path(directory, relative)
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, target)
        Path(directory, "not-exported.txt").write_text("Unrelated fixture")
        mode = ["--isolated-combined"] if combined else ["--library", "--isolated-portmapper"]
        server = subprocess.Popen([sys.argv[1], directory, *mode],
                                  stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                  env=dict(os.environ, AZ_PUBLIC_RPC_STATS="1"))
        clients = []
        try:
            assert select.select([server.stdout], [], [], 15)[0], "Server readiness timeout"
            ready = server.stdout.readline().split()
            assert len(ready) == 2, (ready, server.poll())
            if combined:
                assert ready[1] == b"111"
                with socket.create_connection(("127.0.0.1", 12523), timeout=3) as query:
                    query.sendall(b"\x00\x00\x00\x0fRemoteDBServer\x00")
                    port_reply = b""
                    while len(port_reply) < 2:
                        chunk = query.recv(2-len(port_reply))
                        assert chunk, "Port query closed without full reply"
                        port_reply += chunk
                    assert struct.unpack(">H", port_reply)[0] == int(ready[0])
                    assert query.recv(1) == b""
                with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as mount:
                    mount.settimeout(3)
                    mount.sendto(struct.pack(">11I", 1, 0, 2, 100005, 1, 1, 0, 0, 0, 0, 2) + b"/C\0\0",
                                 ("127.0.0.1", 111))
                    reply, _ = mount.recvfrom(9000)
                    assert reply[:28] == struct.pack(">7I", 1, 1, 0, 0, 0, 0, 0)
                    assert len(reply) == 60
                    root_handle = reply[28:]
                browser = subprocess.Popen(["java", "-cp", sys.argv[2], "DbLibraryControl",
                                             ready[0].decode(), str(fixtures[0][2]), sys.argv[5]],
                                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                clients.append(("metadata", browser))
            else:
                assert ready[0] == b"111"
                root_handle = bytes.fromhex(ready[1].decode())
            for kind, relative, source in fixtures:
                digest = hashlib.sha256(source.read_bytes()).hexdigest()
                client = subprocess.Popen(["java", "-cp", sys.argv[2], "LocalFileServiceClient",
                                           relative, str(source.stat().st_size), digest],
                                          stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                clients.append((kind, client))
            for kind, client in clients:
                output, errors = client.communicate(timeout=45)
                print(kind + ": " + output.decode().strip())
                if errors:
                    print(errors.decode(), file=sys.stderr)
                assert client.returncode == 0, (kind, client.returncode)
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as client:
                client.settimeout(3)

                def request(procedure, arguments):
                    client.sendto(struct.pack(">10I", 71, 0, 2, 100003, 2, procedure, 0, 0, 0, 0) + arguments,
                                  ("127.0.0.1", 111))
                    response, _ = client.recvfrom(9000)
                    assert struct.unpack(">6I", response[:24]) == (71, 1, 0, 0, 0, 0)
                    return response[24:]

                name = "not-exported.txt".encode("utf-16le")
                body = request(4, root_handle + struct.pack(">I", len(name)) + name + bytes(-len(name) % 4))
                assert body == struct.pack(">I", 2)
                # Mutation in owned temporary fixture, never the emulator USB.
                with Path(directory, fixtures[0][1]).open("ab") as output:
                    output.write(b"changed")
                assert request(1, root_handle) == struct.pack(">I", 70)
            if combined:
                with socket.create_connection(("127.0.0.1", 12523), timeout=3) as query:
                    try:
                        query.sendall(b"\x00\x00\x00\x0fRemoteDBServer\x00")
                        assert query.recv(2) == b"", "Withdrawn source still publishes its DB port"
                    except (ConnectionResetError, BrokenPipeError):
                        pass
                print("PASS: DB port lookup matches metadata endpoint and withdraws on source mutation")
            server.stdin.write(b"Q")
            server.stdin.flush()
            assert server.wait(timeout=5) == 0
            print("PASS: concurrent unchanged Crate Digger PDB/audio transfers from C PDB-populated catalog, hidden unrelated file and post-ready mutation STALE")
            if combined:
                print("PASS: unchanged Beat Link browsing and Crate Digger transfers use one combined library generation")
        finally:
            for _, client in clients:
                if client.poll() is None:
                    client.kill()
                    client.wait(timeout=5)
                if client.stdout:
                    client.stdout.close()
                if client.stderr:
                    client.stderr.close()
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

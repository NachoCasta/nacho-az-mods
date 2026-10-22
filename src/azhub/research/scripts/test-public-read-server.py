#!/usr/bin/env python3
"""Independent localhost wire client for public-read-server (Linux harness)."""
import hashlib
from pathlib import Path
import select
import socket
import struct
import subprocess
import sys
import tempfile


def line(process):
    if not select.select([process.stdout], [], [], 10)[0]:
        raise AssertionError("Server handshake timed out")
    value = process.stdout.readline()
    assert value, "Server closed its output"
    return value.strip()


def main():
    assert len(sys.argv) == 2, "Pass the locally built server executable"
    content = bytes((index * 37 + index // 256) % 256 for index in range(30001))
    with tempfile.TemporaryDirectory(prefix="azhub-independent-read-") as directory:
        relative = Path("Music", "Álbum 🎵", "track.bin")
        path = Path(directory, relative)
        path.parent.mkdir(parents=True)
        path.write_bytes(content)
        process = subprocess.Popen([sys.argv[1], directory, relative.as_posix()], stdin=subprocess.PIPE,
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        try:
            port, encoded = line(process).split()
            endpoint = ("127.0.0.1", int(port))
            handle = bytes.fromhex(encoded.decode("ascii"))
            assert len(handle) == 32
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as client:
                client.bind(("127.0.0.1", 0))
                client.settimeout(3)
                xid = 0

                def request(procedure, arguments, program=100003, version=2):
                    nonlocal xid
                    xid += 1
                    # Encode independently of every server header/helper.
                    packet = struct.pack(">10I", xid, 0, 2, program, version, procedure, 0, 0, 0, 0) + arguments
                    client.sendto(packet, endpoint)
                    reply, sender = client.recvfrom(9000)
                    assert sender == endpoint and len(reply) >= 24
                    header = struct.unpack(">6I", reply[:24])
                    assert header[:5] == (xid, 1, 0, 0, 0), header
                    return header[5], reply[24:]

                root_handle = handle
                for program, version in ((100003, 2), (100005, 1)):
                    status, body = request(3, struct.pack(">4I", program, version, 17, 0), 100000, 2)
                    assert status == 0 and body == struct.pack(">I", endpoint[1])
                status, body = request(5, b"", 100005, 1)
                assert status == 0 and body == struct.pack(">2I", 1, 3) + b"/C/\0" + bytes(8)
                mount_path = struct.pack(">I", 3) + b"/C/\0"
                status, body = request(1, mount_path, 100005, 1)
                assert status == 0 and body == bytes(4) + root_handle
                status, body = request(3, mount_path, 100005, 1)
                assert status == 0 and not body
                status, body = request(1, root_handle)
                assert status == 0 and len(body) == 72
                assert struct.unpack_from(">I", body, 4)[0] == 2
                for component in relative.parts:
                    status, listing = request(16, handle + struct.pack(">2I", 0, 8192))
                    assert status == 0 and listing[:8] == struct.pack(">2I", 0, 1)
                    entry_id, name_length = struct.unpack_from(">2I", listing, 8)
                    listed = listing[16:16 + name_length].decode("utf-16le")
                    assert entry_id and listed == component
                    end = 16 + ((name_length + 3) & ~3)
                    cookie, present, eof = struct.unpack_from(">3I", listing, end)
                    assert cookie and present == 0 and eof == 1
                    encoded_name = (component + "\0").encode("utf-16le")
                    arguments = handle + struct.pack(">I", len(encoded_name)) + encoded_name
                    arguments += bytes(-len(encoded_name) % 4)
                    status, body = request(4, arguments)
                    assert status == 0 and len(body) == 104 and body[:4] == bytes(4)
                    handle = body[4:36]
                status, body = request(1, handle)
                assert status == 0 and len(body) == 72
                assert struct.unpack_from(">I", body, 24)[0] == len(content)
                # Invalid high surrogate and hidden path never become OS paths.
                status, body = request(4, root_handle + struct.pack(">I", 2) + b"\x00\xd8\0\0")
                assert status == 4 and not body
                hidden = "hidden".encode("utf-16le")
                status, body = request(4, root_handle + struct.pack(">I", len(hidden)) + hidden)
                assert status == 0 and body == struct.pack(">I", 2)
                reconstructed = bytearray()
                while len(reconstructed) < len(content):
                    status, body = request(6, handle + struct.pack(">3I", len(reconstructed), 8192, 0))
                    assert status == 0 and len(body) >= 76
                    attributes = struct.unpack(">19I", body[:76])
                    assert attributes[0] == 0 and attributes[1] == 1
                    assert attributes[6] == len(content)
                    count = attributes[18]
                    assert 0 < count <= 8192
                    assert len(body) == 76 + ((count + 3) & ~3)
                    assert not any(body[76 + count:]), "Nonzero XDR padding"
                    reconstructed.extend(body[76:76 + count])
                assert bytes(reconstructed) == content
                status, body = request(6, handle + struct.pack(">3I", len(content), 8192, 0))
                assert status == 0 and len(body) == 76 and body[-4:] == bytes(4)
                status, body = request(8, handle + bytes(48))
                assert status == 3 and not body and path.read_bytes() == content
                altered = bytes([handle[0] ^ 1]) + handle[1:]
                status, body = request(6, altered + struct.pack(">3I", 0, 8192, 0))
                assert status == 0 and body == struct.pack(">I", 70)
                process.stdin.write(b"E")
                process.stdin.flush()
                assert line(process) == b"WITHDRAWN"
                status, body = request(6, handle + struct.pack(">3I", 0, 8192, 0))
                assert status == 0 and body == struct.pack(">I", 70)
                status, body = request(1, mount_path, 100005, 1)
                assert status == 0 and body == struct.pack(">I", 2)
                status, body = request(5, b"", 100005, 1)
                assert status == 0 and body == bytes(4)
                assert path.read_bytes() == content
            process.stdin.write(b"Q")
            process.stdin.flush()
            assert process.wait(timeout=5) == 0, process.stderr.read().decode()
            print("PASS: independent Python UDP portmapper/mount/export/UMNT, Unicode READDIR/LOOKUP/GETATTR,30001-byte READ, write refusal and withdrawal STALE")
            print("payload_sha256=" + hashlib.sha256(content).hexdigest())
        finally:
            if process.poll() is None:
                process.kill()
                process.wait(timeout=5)
            for stream in (process.stdin, process.stdout, process.stderr):
                stream.close()


if __name__ == "__main__":
    main()

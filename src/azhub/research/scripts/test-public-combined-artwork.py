#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Loopback shared-generation TCP artwork/UDP mount and withdrawal control."""
import select
import socket
import struct
import subprocess
import sys
import time


def main():
    assert len(sys.argv) == 6, "SERVER CLASSPATH FIXTURE_ROOT ART_ID JPEG_FILE"
    server = subprocess.Popen([sys.argv[1], sys.argv[3], "--combined"],
                              stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE, text=True)
    client = None
    udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp.settimeout(2)
    try:
        assert select.select([server.stdout], [], [], 60)[0], "Readiness timeout"
        tcp_port, udp_port = map(int, server.stdout.readline().split())
        sequence = 0

        def rpc(program, version, procedure, arguments=b""):
            nonlocal sequence
            sequence += 1
            packet = struct.pack(">10I", sequence, 0, 2, program, version,
                                 procedure, 0, 0, 0, 0) + arguments
            udp.sendto(packet, ("127.0.0.1", udp_port))
            reply, peer = udp.recvfrom(16384)
            assert peer == ("127.0.0.1", udp_port)
            assert reply[:24] == struct.pack(">6I", sequence, 1, 0, 0, 0, 0)
            return reply[24:]

        mount = rpc(100005, 1, 1, struct.pack(">I", 2) + b"/C\0\0")
        assert len(mount) == 36 and mount[:4] == bytes(4)
        handle = mount[4:]
        client = subprocess.Popen(["java", "-cp", sys.argv[2], "DbArtworkControl",
                                   str(tcp_port), sys.argv[4], sys.argv[5]],
                                  stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        checks = 0
        deadline = time.monotonic() + 25
        while client.poll() is None:
            assert time.monotonic() < deadline, "Client deadline"
            assert rpc(100003, 2, 1, handle)[:4] == bytes(4)
            checks += 1
            time.sleep(0.01)
        output, errors = client.communicate()
        assert client.returncode == 0, errors
        assert checks > 0
        server.stdin.write("E")
        server.stdin.flush()
        deadline = time.monotonic() + 3
        while rpc(100003, 2, 1, handle)[:4] != struct.pack(">I", 70):
            assert time.monotonic() < deadline, "Withdrawal did not return STALE"
        assert rpc(100005, 1, 5) == bytes(4), "Withdrawn export remains listed"
        print(output.strip())
        print(f"PASS: {checks} concurrent UDP attribute checks; withdrawal returns STALE and empty exports")
        server.stdin.write("Q")
        server.stdin.flush()
        assert server.wait(timeout=5) == 0
    finally:
        udp.close()
        for process in (client, server):
            if process is not None:
                if process.poll() is None:
                    process.kill()
                    process.wait(timeout=5)
                for stream in (process.stdin, process.stdout, process.stderr):
                    if stream is not None:
                        stream.close()


if __name__ == "__main__":
    main()

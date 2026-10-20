#!/usr/bin/env python3
"""Frame a pre-created isolated Linux TAP over stdio for QEMU socket transport.

Run inside the dedicated azhub-link network namespace. Network namespace and TAP
creation are explicit separate setup operations. No physical interface is opened.
Stdout contains binary frames only; diagnostics must go to stderr.
"""
import fcntl
import os
import select
import struct
import subprocess
import sys


def main():
    if sys.argv[1:] == ['--prepare-isolated-namespace']:
        # Keep WSL alive from namespace creation through the binary relay.
        subprocess.run(['ip', 'netns', 'add', 'azhub-link'], check=True)
        commands = [
            ['ip', '-n', 'azhub-link', 'link', 'set', 'lo', 'up'],
            ['ip', 'netns', 'exec', 'azhub-link', 'ip', 'tuntap', 'add', 'dev', 'azlink0', 'mode', 'tap'],
            ['ip', '-n', 'azhub-link', 'addr', 'add', '192.0.2.2/24', 'brd', '192.0.2.255', 'dev', 'azlink0'],
            ['ip', '-n', 'azhub-link', 'link', 'set', 'azlink0', 'address', '02:00:00:00:02:02'],
            ['ip', '-n', 'azhub-link', 'link', 'set', 'azlink0', 'up'],
        ]
        for command in commands:
            subprocess.run(command, check=True, stdout=sys.stderr)
        os.execvp('ip', ['ip', 'netns', 'exec', 'azhub-link', 'python3', __file__])
    if sys.argv[1:]:
        raise SystemExit('Unexpected arguments')
    if os.stat('/proc/self/ns/net').st_ino == os.stat('/proc/1/ns/net').st_ino:
        raise SystemExit('Refusing to attach TAP in the host network namespace')
    fd = os.open('/dev/net/tun', os.O_RDWR)
    fcntl.ioctl(fd, 0x400454ca, struct.pack('16sH', b'azlink0', 0x0002 | 0x1000))
    buffer = b''
    try:
        while True:
            ready, _, _ = select.select([fd, 0], [], [])
            if fd in ready:
                frame = os.read(fd, 65536)
                sys.stdout.buffer.write(struct.pack('!I', len(frame)) + frame)
                sys.stdout.buffer.flush()
            if 0 in ready:
                block = os.read(0, 65536)
                if not block:
                    break
                buffer += block
                while len(buffer) >= 4:
                    size = struct.unpack('!I', buffer[:4])[0]
                    if not 14 <= size <= 65536:
                        raise ValueError(f'Invalid frame length {size}')
                    if len(buffer) < size + 4:
                        break
                    frame, buffer = buffer[4:size + 4], buffer[size + 4:]
                    if os.write(fd, frame) != len(frame):
                        raise OSError('Incomplete TAP frame write')
    finally:
        os.close(fd)


if __name__ == '__main__':
    main()

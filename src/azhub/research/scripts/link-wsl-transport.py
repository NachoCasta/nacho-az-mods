#!/usr/bin/env python3
"""Connect local QEMU Ethernet to a pre-created isolated WSL TAP namespace.

Windows-only. Expects QEMU listening on loopback:15100 and the explicit WSL
azhub-link/azlink0 setup. Does not restart the emulator or configure networking.
"""
from pathlib import Path
import socket
import subprocess
import threading


def main():
    script = Path(__file__).with_name('link-tap-stdio.py').resolve()
    if script.drive.upper() != 'C:':
        raise SystemExit('Set up an explicit WSL path for a non-C workspace first')
    linux_script = '/mnt/c/' + script.as_posix()[3:]
    command = ['wsl', '-u', 'root', '-e', 'python3', linux_script,
               '--prepare-isolated-namespace']
    with socket.create_connection(('127.0.0.1', 15100), timeout=10) as network:
        network.settimeout(None)
        process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                   bufsize=0)

        def from_tap():
            try:
                while True:
                    block = process.stdout.read(65536)
                    if not block:
                        break
                    network.sendall(block)
            finally:
                network.shutdown(socket.SHUT_RDWR)

        thread = threading.Thread(target=from_tap, daemon=True)
        thread.start()
        print('Isolated QEMU-to-WSL TAP transport connected', flush=True)
        try:
            while True:
                block = network.recv(65536)
                if not block:
                    break
                pending = memoryview(block)
                while pending:
                    written = process.stdin.write(pending)
                    if not written:
                        raise BrokenPipeError('TAP transport stopped')
                    pending = pending[written:]
        finally:
            process.stdin.close()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.terminate()
            thread.join(timeout=5)


if __name__ == '__main__':
    main()

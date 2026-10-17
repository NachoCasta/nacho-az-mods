"""Minimal read-only ONC RPC servers for the fake-player experiment:
portmap v2 (UDP 111), mountd v1 (UDP 40000) and NFS v2 (UDP 2049), serving one directory.

usage: py nfsd.py <seconds> <root_dir> [deck_ip]
Every call is logged with its program, procedure and arguments. Any MNT path is mapped
to <root_dir>. Unknown calls get PROC_UNAVAIL. Never writes anything.
"""
import os
import select
import socket
import stat
import struct
import sys
import time

secs = float(sys.argv[1])
ROOT = os.path.abspath(sys.argv[2])
only = sys.argv[3] if len(sys.argv) > 3 else None
PMAP, MOUNT, NFS = 100000, 100005, 100003
MOUNT_PORT, NFS_PORT = 40000, 2049
t0 = time.time()


def log(msg):
    print(f'{time.time() - t0:6.1f}s {msg}', flush=True)


# ---- XDR helpers
class R:
    def __init__(self, b):
        self.b, self.o = b, 0

    def u32(self):
        v = stru
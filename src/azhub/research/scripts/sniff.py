"""Passively listen for Pro DJ Link UDP traffic (ports 50000-50004) for N seconds.

usage: py sniff.py [seconds] [only_ip]
Prints one line per packet: source, port, length, Pro DJ Link type byte, first bytes.
Sends nothing.
"""
import select
import socket
import sys
import time

secs = float(sys.argv[1]) if len(sys.argv) > 1 else 10
only = sys.argv[2] if len(sys.argv) > 2 else None
MAGIC = b'Qspt1WmJOL'
socks = []
for port in (50000, 50001, 50002, 50004):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    try:
        s.bind(('', port))
    except OSError as e:
        print(f'bind {port}: {e}')
        continue
    socks.append(s)
end = time.time() + secs
seen = {}
while time.time() < end:
    r, _, _ = select.select(socks, [], [], 0.5)
    for s in r:
        data, (ip, sport) = s.recvfrom(4096)
        if only and ip != only:
            continue
        port = s.getsockname()[1]
        kind = data[10] if data.startswith(MAGIC) and len(data) > 10 else None
        key = (ip, port, kind, len(data))
        seen[key] = seen.get(key, 0) + 1
        if seen[key] <= 2:
            name = data[0x0b:0x1f].split(b'\0')[0].decode(errors='replace') if kind is not None else ''
            print(f'{ip}:{sport} -> :{port} len={len(data)} type={kind if kind is None else hex(kind)} '
                  f'name={name!r} head={data[:48].hex()}')
print('summary:')
for (ip, port, kind, ln), n in sorted(seen.items(), key=str):
    print(f'  {ip} :{port} type={kind if kind is None else hex(kind)} len={ln} x{n}')

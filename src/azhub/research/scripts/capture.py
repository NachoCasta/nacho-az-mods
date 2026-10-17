"""Capture full Pro DJ Link packets from one IP: one sample per (port, type, length).

usage: py capture.py <seconds> <ip> [ports=50000,50001,50002]
"""
import select
import socket
import sys
import time

secs, ip = float(sys.argv[1]), sys.argv[2]
ports = [int(p) for p in (sys.argv[3] if len(sys.argv) > 3 else '50000,50001,50002').split(',')]
socks = []
for port in ports:
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(('', port))
    socks.append(s)
end = time.time() + secs
done = set()
while time.time() < end:
    r, _, _ = select.select(socks, [], [], 0.5)
    for s in r:
        data, (src, sport) = s.recvfrom(4096)
        if src != ip:
            continue
        key = (s.getsockname()[1], data[10] if len(data) > 10 else None, len(data))
        if key in done:
            continue
        done.add(key)
        print(f':{key[0]} type={key[1]:#04x} len={key[2]} from :{sport}')
        for i in range(0, len(data), 16):
            print(f'  {i:04x}: {data[i:i + 16].hex(" ")}')

"""Experiment: announce a fake Pro DJ Link player and log everything the deck sends to it.

usage: py fakecdj.py <seconds> <my_ip> <deck_ip> [player=3] [name=CDJ-3000]
Sends a keep-alive (type 0x06, the deck's own layout with our number/MAC/IP) every 1.5 s
to the broadcast address on UDP 50000. Logs every UDP packet from the deck on 50000-50004
and every TCP connection attempt on 1051 / 12523 (dbserver), without answering anything.
"""
import select
import socket
import sys
import time

secs = float(sys.argv[1])
my_ip, deck_ip = sys.argv[2], sys.argv[3]
player = int(sys.argv[4]) if len(sys.argv) > 4 else 3
name = (sys.argv[5] if len(sys.argv) > 5 else 'CDJ-3000').encode()
MAGIC = b'Qspt1WmJOL'
mac = bytes([0x02, 0x0a, 0x2d, 0x00, 0x00, player])  # locally administered
bcast = my_ip.rsplit('.', 1)[0] + '.255'


def keepalive():
    p = bytearray(54)
    p[0:10] = MAGIC
    p[0x0a] = 0x06
    p[0x0c:0x0c + len(name)] = name
    p[0x20:0x24] = bytes([0x01, 0x03, 0x00, 0x36])
    p[0x24] = player
    p[0x25] = 0x01          # player
    p[0x26:0x2c] = mac
    p[0x2c:0x30] = socket.inet_aton(my_ip)
    p[0x30] = 0x01
    p[0x34] = 0x01
    p[0x35] = 0x00
    return bytes(p)


udp = []
for port in (50000, 50001, 50002, 50004):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    s.bind(('', port))
    udp.append(s)
tcp = []
for port in (1051, 12523):
    t = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    t.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    try:
        t.bind((my_ip, port))
        t.listen(4)
        tcp.append(t)
    except OSError as e:
        print(f'tcp {port}: {e}')
tx = udp[0]
ka = keepalive()
print('keepalive:', ka.hex(' '))
end = time.time() + secs
next_ka = 0
seen = {}
while time.time() < end:
    now = time.time()
    if now >= next_ka:
        tx.sendto(ka, (bcast, 50000))
        next_ka = now + 1.5
    r, _, _ = select.select(udp + tcp, [], [], 0.2)
    for s in r:
        if s in tcp:
            c, addr = s.accept()
            c.settimeout(1.0)
            try:
                d = c.recv(4096)
            except OSError:
                d = b''
            print(f'{time.time() - end + secs:6.1f}s TCP :{s.getsockname()[1]} from {addr} sent {d[:64].hex(" ")}')
            c.close()
            continue
        data, (src, sport) = s.recvfrom(4096)
        if src != deck_ip:
            continue
        port = s.getsockname()[1]
        kind = data[10] if data.startswith(MAGIC) and len(data) > 10 else None
        key = (port, kind, len(data))
        seen[key] = seen.get(key, 0) + 1
        if key[1] not in (0x03, 0x06) or seen[key] == 1:
            t = time.time() - end + secs
            print(f'{t:6.1f}s :{port} type={kind if kind is None else hex(kind)} len={len(data)} from :{sport}')
            if key[1] not in (0x03, 0x06):
                for i in range(0, len(data), 16):
                    print(f'    {i:04x}: {data[i:i + 16].hex(" ")}')
print('summary:')
for (port, kind, ln), n in sorted(seen.items(), key=str):
    print(f'  :{port} type={kind if kind is None else hex(kind)} len={ln} x{n}')

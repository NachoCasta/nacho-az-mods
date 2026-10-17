"""Experiment 2: a fake player that also claims a loaded USB.

usage: py fakecdj2.py <seconds> <my_ip> <deck_ip> [player=3]

- keep-alive (0x06) every 1.5 s, broadcast UDP 50000 (same layout as experiment 1)
- once the deck sends us its first CDJ status (0x0a), that packet becomes the template for
  our own status: device number -> ours, USB local = loaded, link available = 1. Sent to the
  deck on UDP 50002 every 200 ms.
- media slot query (0x05) from the deck -> media slot response (0x06) for USB.
- logs every packet type the deck sends (first sighting with hex), TCP connects on
  1051/12523/111/2049 (first bytes) and UDP on 111/2049.
Nothing is written on the deck.
"""
import select
import socket
import struct
import sys
import time

secs = float(sys.argv[1])
my_ip, deck_ip = sys.argv[2], sys.argv[3]
player = int(sys.argv[4]) if len(sys.argv) > 4 else 3
NAME = b'CDJ-3000'
MAGIC = b'Qspt1WmJOL'
mac = bytes([0x02, 0x0a, 0x2d, 0x00, 0x00, player])
bcast = my_ip.rsplit('.', 1)[0] + '.255'
t0 = time.time()


def log(msg):
    print(f'{time.time() - t0:6.1f}s {msg}', flush=True)


def hexdump(data, limit=0x200):
    for i in range(0, min(len(data), limit), 16):
        print(f'      {i:04x}: {data[i:i + 16].hex(" ")}')


def keepalive():
    p = bytearray(54)
    p[0:10] = MAGIC
    p[0x0a] = 0x06
    p[0x0c:0x0c + len(NAME)] = NAME
    p[0x20:0x24] = bytes([0x01, 0x03, 0x00, 0x36])
    p[0x24] = player
    p[0x25] = 0x01
    p[0x26:0x2c] = mac
    p[0x2c:0x30] = socket.inet_aton(my_ip)
    p[0x30] = 0x01
    p[0x34] = 0x01
    return bytes(p)


def status_from(template):
    p = bytearray(template)
    p[0x0b:0x1f] = NAME.ljust(0x14, b'\0')
    p[0x21] = player
    p[0x24] = player
    p[0x27] = 0x00        # idle
    p[0x28:0x2b] = b'\0\0\0'  # no track loaded
    p[0x6a] = 0x04        # Ua
    p[0x6f] = 0x00        # Ul: USB loaded
    p[0x73] = 0x04        # Sl: no SD
    p[0x75] = 0x01        # L: link media available
    p[0xb7] = 0x01        # Mp: USB present (guess: bit 0)
    return bytes(p)


def slot_response(query):
    # query: header, name, D at 0x21, lenr, IP at 0x24?, Dr, Sr near the end
    sr = query[-1]
    dr = query[-5] if len(query) >= 5 else player
    p = bytearray(0x24 + 0x9c)
    p[0:10] = MAGIC
    p[0x0a] = 0x06
    p[0x0b:0x0b + len(NAME)] = NAME
    p[0x1f] = 0x01
    p[0x20] = 0x00
    p[0x21] = player
    p[0x22:0x24] = struct.pack('>H', 0x9c)
    p[0x24:0x28] = socket.inet_aton(my_ip)
    p[0x28:0x2c] = bytes([0, 0, 0, player])
    p[0x2c - 4:0x2c] = bytes([0, 0, 0, sr])
    name = 'HUB USB'.encode('utf-16-be')
    p[0x2c:0x2c + len(name)] = name
    date = '2026-10-16'.encode('utf-16-be')
    p[0x6c:0x6c + len(date)] = date
    p[0xa6:0xa8] = struct.pack('>H', 100)   # tracks
    p[0xa8] = 0x00
    p[0xaa] = 0x01                            # rekordbox
    p[0xae:0xb0] = struct.pack('>H', 5)     # playlists
    p[0xb0:0xb8] = struct.pack('>Q', 128 * 10 ** 9)
    p[0xb8:0xc0] = struct.pack('>Q', 64 * 10 ** 9)
    return bytes(p), dr, sr


udp = {}
for port in (50000, 50001, 50002, 50004, 111, 2049):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    try:
        s.bind(('', port))
        udp[s] = port
    except OSError as e:
        log(f'udp {port}: {e}')
tcp = {}
for port in (1051, 12523, 111, 2049):
    t = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    t.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    try:
        t.bind((my_ip, port))
        t.listen(4)
        tcp[t] = port
    except OSError as e:
        log(f'tcp {port}: {e}')
tx = next(s for s, p in udp.items() if p == 50000)
st_tx = next(s for s, p in udp.items() if p == 50002)
ka = keepalive()
status = None
next_ka = next_st = 0
seen = {}
end = t0 + secs
while time.time() < end:
    now = time.time()
    if now >= next_ka:
        tx.sendto(ka, (bcast, 50000))
        next_ka = now + 1.5
    if status and now >= next_st:
        st_tx.sendto(status, (deck_ip, 50002))
        next_st = now + 0.2
    r, _, _ = select.select(list(udp) + list(tcp), [], [], 0.05)
    for s in r:
        if s in tcp:
            c, addr = s.accept()
            c.settimeout(1.0)
            try:
                d = c.recv(4096)
            except OSError:
                d = b''
            log(f'TCP :{tcp[s]} connect from {addr}, first bytes:')
            hexdump(d, 0x80)
            c.close()
            continue
        data, (src, sport) = s.recvfrom(4096)
        if src != deck_ip:
            continue
        port = udp[s]
        kind = data[10] if data.startswith(MAGIC) and len(data) > 10 else None
        key = (port, kind, len(data))
        seen[key] = seen.get(key, 0) + 1
        if kind == 0x0a and status is None and data[0x21] != player:
            status = status_from(data)
            log(f'got deck status template (player {data[0x21]}, {len(data)} B); '
                f'deck fields Ua={data[0x6a]:#x} Ul={data[0x6f]:#x} L={data[0x75]:#x} Mp={data[0xb7]:#x}')
        if port == 50002 and kind == 0x05:
            resp, dr, sr = slot_response(data)
            st_tx.sendto(resp, (deck_ip, 50002))
            log(f'media query (Dr={dr} Sr={sr}) -> answered')
            hexdump(data)
        if seen[key] == 1 and kind not in (0x03, 0x06, 0x0a, 0x0b):
            log(f'UDP :{port} type={kind if kind is None else hex(kind)} len={len(data)} from :{sport}')
            hexdump(data)
log('summary:')
for (port, kind, ln), n in sorted(seen.items(), key=str):
    print(f'  :{port} type={kind if kind is None else hex(kind)} len={ln} x{n}')

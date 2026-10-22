#!/usr/bin/env python3
"""Bounded localhost QEMU/TAP bridge preserving NAT management and peer TCP.

Requires the prepared isolated azhub-link namespace and QEMU mirror/injection
channels. A temporary QEMU ingress redirect suppresses NAT packets claiming the
TAP peer's address; otherwise slirp resets valid peer TCP sessions. Other NAT
traffic, including SSH, returns through the existing guest injection channel.
No guest files, NICs, addresses, firmware or application processes are changed.
"""
import argparse
import ipaddress
import json
from pathlib import Path
import socket
import struct
import subprocess
import threading
import time
import tempfile
import os
import select


class Frames:
    def __init__(self):
        self.pending = bytearray()

    def feed(self, data):
        self.pending.extend(data)
        frames = []
        while len(self.pending) >= 4:
            size = struct.unpack('!I', self.pending[:4])[0]
            if not 14 <= size <= 65536:
                raise ValueError('Invalid Ethernet frame size')
            if len(self.pending) < size + 4:
                break
            frames.append(bytes(self.pending[4:size + 4]))
            del self.pending[:size + 4]
        return frames

    def finish(self):
        if self.pending:
            raise ValueError('Truncated framed stream')


def framed(frame):
    if not 14 <= len(frame) <= 65536:
        raise ValueError('Invalid Ethernet frame size')
    return struct.pack('!I', len(frame)) + frame


from link_fixture_network import verify_namespace, fixture_addresses,source_mac


def owned_peer(frame,addresses=(bytes([192,0,2,2]),)):
    """Only the TAP owns192.0.2.2; NAT must not impersonate it."""
    if len(frame) >= 34 and frame[12:14] == b'\x08\x00':
        ihl = (frame[14] & 15) * 4
        total = int.from_bytes(frame[16:18], 'big')
        if frame[14] >> 4 == 4 and ihl >= 20 and ihl <= total <= len(frame) - 14:
            return frame[26:30] in addresses
    if len(frame) >= 42 and frame[12:14] == b'\x08\x06':
        if frame[14:20] == b'\x00\x01\x08\x00\x06\x04':
            return frame[28:32] in addresses
    return False




def fixture_dhcp_clients(value,parent=None):
    slots=[int(part) for part in value.split(',')]
    if not 1<=len(slots)<=8 or len(slots)!=len(set(slots)) or any(slot<0 or slot>7 for slot in slots):
        raise ValueError('Distinct fixture slots0..7 required')
    return tuple(source_mac(parent,slot) if parent else bytes([2,0,0,0,0x12,3+slot]) for slot in slots)


def fixture_dhcp_reply(frame,clients=(bytes.fromhex('020000001203'),)):
    """Only NAT DHCP replies naming explicitly selected virtual fixture MACs."""
    if len(frame)<34 or frame[12:14]!=b'\x08\x00' or frame[14]>>4!=4:return False
    ihl=(frame[14]&15)*4;total=int.from_bytes(frame[16:18],'big')
    if ihl<20 or total>len(frame)-14 or total<ihl+8+240 or frame[23]!=17:return False
    if int.from_bytes(frame[20:22],'big')&0x3fff:return False
    udp=14+ihl;length=int.from_bytes(frame[udp+4:udp+6],'big')
    if frame[udp:udp+4]!=b'\x00\x43\x00\x44' or length<248 or length>total-ihl:return False
    bootp=udp+8
    return (frame[bootp:bootp+3]==b'\x02\x01\x06' and
            frame[bootp+28:bootp+34] in clients and
            frame[bootp+236:bootp+240]==b'\x63\x82\x53\x63')


def qmp(command, arguments=None):
    # Fixed automation endpoint; never connect to a physical controller.
    with socket.create_connection(('127.0.0.1', 4446), timeout=5) as connection:
        connection.settimeout(5)
        stream = connection.makefile('rwb')
        stream.readline()
        for request in ({'execute': 'qmp_capabilities'},
                        {'execute': command, 'arguments': arguments or {}}):
            stream.write(json.dumps(request).encode() + b'\n')
            stream.flush()
            while True:
                line = stream.readline()
                if not line:
                    raise EOFError('QMP closed before reply')
                result = json.loads(line)
                if 'error' in result:
                    raise RuntimeError((command, result['error']))
                if 'return' in result:
                    break
        return result['return']


def write_all(stream, data):
    remaining = memoryview(data)
    while remaining:
        count = stream.write(remaining)
        if not count:
            raise BrokenPipeError('TAP input closed')
        remaining = remaining[count:]


def connect_local_socket(path):
    """Windows Python may omit AF_UNIX even though Winsock/QEMU support it."""
    if hasattr(socket,'AF_UNIX'):
        stream=socket.socket(socket.AF_UNIX,socket.SOCK_STREAM)
        try:
            stream.settimeout(5);stream.connect(str(path));return stream
        except BaseException:stream.close();raise
    if os.name!='nt':raise OSError('Local socket support unavailable')
    import ctypes
    encoded=str(path).encode('utf-8')
    if len(encoded)>=108:raise ValueError('Local socket path exceeds sockaddr_un')
    class Address(ctypes.Structure):
        _fields_=[('family',ctypes.c_ushort),('path',ctypes.c_char*108)]
    winsock=ctypes.WinDLL('ws2_32')
    winsock.connect.argtypes=[ctypes.c_size_t,ctypes.c_void_p,ctypes.c_int]
    address=Address(1,encoded)
    stream=socket.socket(1,socket.SOCK_STREAM)
    try:
        stream.settimeout(5)
        if winsock.connect(stream.fileno(),ctypes.byref(address),ctypes.sizeof(address)):
            error=winsock.WSAGetLastError()
            if error not in (10035,10036):raise OSError(error,'Local socket connect failed')
            _,ready,failed=select.select([],[stream],[stream],5)
            if not ready or failed:raise OSError('Local socket connection timeout/failure')
            error=stream.getsockopt(socket.SOL_SOCKET,socket.SO_ERROR)
            if error:raise OSError(error,'Local socket connection failed')
        return stream
    except BaseException:stream.close();raise

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seconds', type=int, required=True)
    parser.add_argument('--stop-file', type=Path, help='Explicit supervisor request for graceful transport cleanup')
    parser.add_argument('--capture-port', type=int, default=15101)
    parser.add_argument('--inject-port', type=int, default=15102)
    parser.add_argument('--nat-port', type=int, default=15103,help='Legacy TCP backend port, only with --nat-tcp')
    parser.add_argument('--nat-unix',action='store_true',default=True,help='Use a temporary local socket (default)')
    parser.add_argument('--nat-tcp',action='store_false',dest='nat_unix',help='Explicit legacy TCP backend for transport diagnostics')
    parser.add_argument('--fixture-macvlan',action='store_true')
    parser.add_argument('--fixture-address-count',type=int,choices=range(1,9),default=1)
    parser.add_argument('--fixture-dhcp',action='store_true',help='Suppress only NAT DHCP replies to fixture MAC02:00:00:00:12:03; TAP server owns its lease')
    parser.add_argument('--fixture-dhcp-slots',default='0',help='Bounded selected fixture slots; affects only NAT DHCP suppression')
    parser.add_argument('--controller-mac',help='Explicit isolated controller MAC for selected DHCP identities')
    parser.add_argument('--fixture-link-local',action='store_true',help='Explicit secondary169.254.2.2/16 on the isolated TAP only')
    args = parser.parse_args()
    try:dhcp_clients=fixture_dhcp_clients(args.fixture_dhcp_slots,args.controller_mac)
    except ValueError as error:parser.error(str(error))
    ports = [args.capture_port, args.inject_port, args.nat_port]
    if not 5 <= args.seconds <= 3600 or len(set(ports)) != 3 or any(not 1024 <= p <= 65535 for p in ports):
        parser.error('Use5..3600 seconds and three distinct unprivileged loopback ports')
    script = Path(__file__).with_name('link-tap-stdio.py').resolve()
    if script.drive.upper() != 'C:':
        parser.error('Review explicit WSL path mapping before using another drive')
    prefix = ['wsl.exe', '-u', 'root', '-e', 'ip', 'netns', 'exec', 'azhub-link']
    check = subprocess.run(prefix + ['ip', '-j', '-d', 'addr', 'show'], capture_output=True, check=True)
    verify_namespace(json.loads(check.stdout),args.fixture_address_count,args.fixture_macvlan,args.fixture_link_local)
    owned_addresses=tuple(socket.inet_aton(a) for a in fixture_addresses(args.fixture_address_count))
    if args.fixture_link_local:owned_addresses+=(socket.inet_aton('169.254.2.2'),)
    channel, guard = 'azhubtapnat', 'azhubtapguard'
    if any(row.get('label') == channel for row in qmp('query-chardev')):
        raise RuntimeError('Existing NAT channel requires owner inspection; never replace it')
    sockets, threads = [], []
    nat_directory=nat_path=None
    created = installed = False
    child = timer = None
    stopped = threading.Event()
    inject_lock = threading.Lock()
    errors = []
    counts = dict(guest_to_tap=0, tap_to_guest=0, nat_to_guest=0, nat_peer_suppressed=0,nat_dhcp_suppressed=0)
    deadline = time.monotonic() + args.seconds
    try:
        capture = socket.create_connection(('127.0.0.1', args.capture_port), timeout=5)
        sockets.append(capture)
        inject = socket.create_connection(('127.0.0.1', args.inject_port), timeout=5)
        sockets.append(inject)
        if args.nat_unix:
            nat_directory=Path(tempfile.mkdtemp(prefix='azhub-nat-'))
            nat_path=nat_directory/'n.sock'
            address={'type':'unix','data':{'path':str(nat_path)}}
        else:address={'type': 'inet', 'data': {'host': '127.0.0.1', 'port': str(args.nat_port)}}
        qmp('chardev-add', {'id': channel, 'backend': {'type': 'socket', 'data': {
            'addr': address,
            'server': True, 'wait': False}}})
        created = True
        if args.nat_unix:
            nat=connect_local_socket(nat_path);sockets.append(nat)
        else:nat = socket.create_connection(('127.0.0.1', args.nat_port), timeout=5)
        if not args.nat_unix:sockets.append(nat)
        capture.settimeout(1)
        nat.settimeout(1)
        child = subprocess.Popen(prefix + ['python3', '/mnt/c/' + script.as_posix()[3:]],
                                 stdin=subprocess.PIPE, stdout=subprocess.PIPE, bufsize=0)
        timer = threading.Timer(args.seconds + 10, lambda: child.terminate() if child.poll() is None else None)
        timer.daemon = True
        timer.start()

        def supervisor_stop():
            while not stopped.wait(.2):
                if args.stop_file and args.stop_file.exists():
                    stopped.set()
                    child.stdin.close()
                    return
        if args.stop_file:
            threading.Thread(target=supervisor_stop, daemon=True).start()

        def socket_reader(connection, management):
            records = Frames()
            try:
                while not stopped.is_set() and time.monotonic() < deadline:
                    try:
                        data = connection.recv(65536)
                    except socket.timeout:
                        continue
                    if not data:
                        records.finish()
                        break
                    for frame in records.feed(data):
                        if management:
                            if args.fixture_dhcp and fixture_dhcp_reply(frame,dhcp_clients):
                                counts['nat_dhcp_suppressed'] += 1
                                continue
                            if owned_peer(frame,owned_addresses):
                                counts['nat_peer_suppressed'] += 1
                                continue
                            with inject_lock:
                                inject.sendall(framed(frame))
                            counts['nat_to_guest'] += 1
                        else:
                            write_all(child.stdin, framed(frame))
                            counts['guest_to_tap'] += 1
            except Exception as error:
                if not stopped.is_set():
                    errors.append(str(error))
            finally:
                stopped.set()
                if not management:
                    child.stdin.close()

        for connection, management in ((capture, False), (nat, True)):
            thread = threading.Thread(target=socket_reader, args=(connection, management), daemon=True)
            threads.append(thread)
            thread.start()
        qmp('object-add', {'qom-type': 'filter-redirector', 'id': guard, 'netdev': 'net0',
                          'queue': 'tx', 'position': 'head', 'outdev': channel})
        installed = True
        print(json.dumps({'ready': True, 'seconds': args.seconds, 'physical_NIC': False}), flush=True)
        records = Frames()
        while not stopped.is_set():
            block = child.stdout.read(65536)
            if not block:
                records.finish()
                break
            for frame in records.feed(block):
                with inject_lock:
                    inject.sendall(framed(frame))
                counts['tap_to_guest'] += 1
    finally:
        # Restore ordinary NAT before closing the stream that carries management.
        if installed:
            try:
                qmp('object-del', {'id': guard})
            except Exception as error:
                errors.append('Restore NAT: ' + str(error))
        stopped.set()
        for connection in sockets:
            connection.close()
        for thread in threads:
            thread.join(timeout=3)
        if child:
            try:
                child.wait(timeout=5)
            except subprocess.TimeoutExpired:
                child.terminate()
                child.wait(timeout=5)
        if timer:
            timer.cancel()
        if created:
            try:
                qmp('chardev-remove', {'id': channel})
            except Exception as error:
                errors.append('Remove capture channel: ' + str(error))
        if nat_directory:
            try:
                if nat_path.exists():nat_path.unlink()
                nat_directory.rmdir()
            except OSError as error:errors.append('Remove owned local socket: '+str(error))
        print(json.dumps({'frames': counts, 'errors': errors}), flush=True)
    if errors:
        raise SystemExit(1)


if __name__ == '__main__':
    main()

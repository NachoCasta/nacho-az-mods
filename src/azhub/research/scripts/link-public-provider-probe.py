#!/usr/bin/env python3
"""Isolated host-side public metadata/file prototype; not live device export."""
import importlib.util
import json
from pathlib import Path
import socket
import time
import threading
import struct
import argparse
import select
import link_db_wire as protocol
from link_snapshot_backend import SnapshotBackend
from link_fixture_files import FixtureFiles
from link_activation import Activation
from link_session_pool import SessionPool
from link_fixture_network import verify_namespace, fixture_addresses

BIND="192.0.2.2"
LABEL="Hub USB1 Research"
DEVICE=None


def bind_socket(sock,port):
    if DEVICE is not None:
        sock.setsockopt(socket.SOL_SOCKET,socket.SO_BINDTODEVICE,DEVICE.encode()+b'\0')
    sock.bind((BIND,port))


def emit(event,**fields):
    print(json.dumps(dict(event=event,monotonic_ns=time.monotonic_ns(),**fields)),flush=True)


def client_session(stop,client,port,number,guard,backend):
    with client:
        client.settimeout(2)
        try:
            emit('connection',port=port)
            if port==12523:
                query=guard.exact(client,19)
                if query!=b'\x00\x00\x00\x0fRemoteDBServer\x00':raise RuntimeError('Invalid port query')
                client.sendall(struct.pack('!H',1051));return
            if guard.field(client)!=1:raise RuntimeError('Invalid greeting')
            client.sendall(guard.number(1))
            menus={};extended=False
            for _ in range(512):
                if stop.is_set():break
                if not select.select([client],[],[],.5)[0]:continue
                request=protocol.message(client)
                kind=request['type'];tx=request['transaction']
                emit('db_request',kind=kind,arguments=len(request['arguments']))
                if kind in (0x1000,0x1004,0x1400,0x2002,0x3000):
                    emit('db_context',kind=kind,transaction=tx,
                         numeric_arguments=[v if isinstance(v,int) else None for v in request['arguments']])
                if kind==0x1105:emit('playlist_query',numeric_arguments=request['arguments'])
                if kind==0:extended=len(request['arguments'])==2 and request['arguments'][1]==20
                if backend is not None:
                    for response,arguments in backend.request(request,menus):
                        if response in (0x4000,0x4003):emit('db_result',request=kind,response=response,arguments=arguments)
                        if response==0x4101 and extended:
                            track=backend.tracks.get(arguments[1]) if arguments[6]&0xffff==4 else None
                            arguments=arguments+[0,2,'',track['bpm100'] if track else 0]
                        client.sendall(protocol.packet(tx,response,arguments))
                    if kind==0x100:break
                elif kind==0:client.sendall(protocol.packet(tx,0x4000,[0,number]))
                elif kind==0x1000:client.sendall(protocol.packet(tx,0x4000,[kind,0]))
                elif kind==0x100:break
                else:client.sendall(protocol.packet(tx,0x4003,[kind,0]))
        except (OSError,EOFError,ValueError,RuntimeError) as error:
            emit('connection_end',port=port,error=type(error).__name__,reason=str(error))

def service(stop,port,number,guard,backend):
    pool=SessionPool(8)
    try:
        with socket.socket() as server:
            server.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
            bind_socket(server,port);server.listen(8);server.settimeout(.5)
            emit('listener_ready',port=port)
            while not stop.is_set():
                try:client,address=server.accept()
                except socket.timeout:continue
                if address[0]!='192.0.2.15':
                    client.close();continue
                if not pool.submit(client,lambda owned:client_session(stop,owned,port,number,guard,backend)):
                    emit('connection_capacity',port=port)
    finally:
        pool.close()


def status_service(stop,number,track_count,claimed):
    activation=Activation(number)
    with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as receiver:
        bind_socket(receiver,50002);receiver.settimeout(.5)
        emit('listener_ready',port=50002)
        while not stop.is_set():
            repeat=activation.poll(time.monotonic())
            if repeat is not None:receiver.sendto(repeat,('192.0.2.15',50002))
            try:query,address=receiver.recvfrom(2048)
            except socket.timeout:continue
            if address[0]!='192.0.2.15' or len(query)<36:continue
            kind=query[10];emit('status_query',kind=kind,bytes=len(query))
            if kind in (0x10,5,0x46):emit('status_context',kind=kind,source=BIND,fields=list(query[31:48]))
            if kind==0x46:
                claimed.set()
                answer=activation.request(time.monotonic())
                receiver.sendto(answer,('192.0.2.15',50002))
                emit('activation_reply',kind=answer[10],count=activation.count)
            elif kind==0x10:
                answer=bytearray(296);answer[:10]=b'Qspt1WmJOL';answer[10]=0x11
                answer[11:20]=b'rekordbox';answer[31:38]=bytes((1,1,number,1,4,number,1))
                name=LABEL.encode('utf-16be');answer[40:40+len(name)]=name
                receiver.sendto(answer,('192.0.2.15',50002))
                emit('hello_reply',source=BIND,number=number,label=LABEL)
            elif kind==5 and len(query)>=48:
                answer=bytearray(192);answer[:10]=b'Qspt1WmJOL';answer[10]=6
                answer[11:20]=b'rekordbox';answer[31:36]=bytes((1,1,number,0,156))
                struct.pack_into('!II',answer,36,number,query[47])
                name=LABEL.encode('utf-16be');answer[44:44+len(name)]=name
                struct.pack_into('!H',answer,166,min(track_count,65535))
                answer[170]=1
                receiver.sendto(answer,('192.0.2.15',50002))
                emit('media_reply',slot=query[47],tracks=track_count)



def rpc_service(stop,port,files=None,self_client=False):
    def words(*v):return struct.pack('!'+'I'*len(v),*v)
    def blob(v):return words(len(v))+v+b'\0'*((-len(v))%4)
    with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as receiver:
        bind_socket(receiver,port);receiver.settimeout(.5)
        emit('listener_ready',port=port)
        while not stop.is_set():
            try:data,address=receiver.recvfrom(4096)
            except socket.timeout:continue
            allowed=('192.0.2.15','192.0.2.2') if self_client else ('192.0.2.15',)
            if address[0] not in allowed or len(data)<40:continue
            xid,kind,rpcver,program,version,procedure=struct.unpack_from('!6I',data)
            if kind or rpcver!=2:continue
            offset=24
            for _ in range(2):
                if offset+8>len(data):break
                size=struct.unpack_from('!I',data,offset+4)[0];offset+=8+((size+3)//4)*4
            if offset>len(data):continue
            emit('rpc_request',port=port,program=program,version=version,procedure=procedure)
            result=None
            if program==100000 and procedure==3 and len(data)>=offset+16:
                requested=struct.unpack_from('!I',data,offset)[0]
                result=words(40000 if requested==100005 else 2049 if requested==100003 else 0)
            elif program==100005:
                if procedure==5:result=words(1)+blob(b'/C/')+words(1)+blob(b'192.0.2.2/255.255.0.0')+words(0,0)
                elif procedure==1:result=files.mount(data[offset:]) if files else words(0)+b'\x11'*32
                elif procedure in (0,3,4):result=b''
            elif program==100003:
                if files is not None:result=files.request(procedure,data[offset:])
                elif procedure==0:result=b''
                elif procedure==1:result=words(0,2,0o40555,1,0,0,0,4096,0,0,1,1,0,0,0,0,0,0)
                elif procedure==16:result=words(0,0,1)  # Empty directory, EOF.
                elif procedure==17:result=words(0,8192,4096,1,1,1)
                else:result=words(2)  # Empty control export: no files.
            if result is None:receiver.sendto(words(xid,1,0,0,0,3),address)
            else:receiver.sendto(words(xid,1,0,0,0,0)+result,address)


def main():
    global BIND,LABEL,DEVICE
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--address',choices=fixture_addresses(8),default='192.0.2.2')
    parser.add_argument('--fixture-macvlan',action='store_true')
    parser.add_argument('--fixture-address-count',type=int,choices=range(1,9),default=1)
    parser.add_argument('--label',default='Hub USB1 Research')
    parser.add_argument('--number',type=int,choices=(17,18,*range(41,49)),default=17)
    parser.add_argument('--library-snapshot',type=Path)
    parser.add_argument('--fixture-files',type=Path,help='Ignored local JSON mapping virtual paths to explicit host files')
    parser.add_argument('--fixture-self-client',action='store_true',help='Allow isolated same-namespace file client and portmapper111 control')
    parser.add_argument('--seconds',type=int,default=40)
    args=parser.parse_args()
    if not 1<=args.seconds<=180:parser.error('Expected duration1..180')
    backend=SnapshotBackend(args.library_snapshot,args.number) if args.library_snapshot else None
    spec=importlib.util.spec_from_file_location('guard',Path(__file__).with_name('link-library-peer.py'))
    guard=importlib.util.module_from_spec(spec);spec.loader.exec_module(guard);
    import os,subprocess
    if os.stat('/proc/self/ns/net').st_ino==os.stat('/proc/1/ns/net').st_ino:raise RuntimeError('Host namespace refused')
    verify_namespace(json.loads(subprocess.check_output(['ip','-j','-d','address','show'])),args.fixture_address_count,args.fixture_macvlan)
    if args.address not in fixture_addresses(args.fixture_address_count):raise RuntimeError('Address not in fixture')
    if not 1<=len(args.label)<=32:raise ValueError('Invalid label length')
    BIND=args.address;LABEL=args.label
    if args.fixture_macvlan:
        index=fixture_addresses(args.fixture_address_count).index(BIND)+1
        if args.number!=40+index:raise ValueError('Provider number must match virtual MAC')
        DEVICE='azpub'+str(index)
    files=FixtureFiles(json.loads(args.fixture_files.read_text())) if args.fixture_files else None
    stop=threading.Event();claimed=threading.Event()
    workers=[threading.Thread(target=service,args=(stop,port,args.number,guard,backend)) for port in (12523,1051)]
    workers.append(threading.Thread(target=status_service,args=(stop,args.number,backend.track_count if backend else 0,claimed)))
    rpc_ports=(50111,40000,2049,111) if args.fixture_self_client else (50111,40000,2049)
    workers.extend(threading.Thread(target=rpc_service,args=(stop,port,files,args.fixture_self_client)) for port in rpc_ports)
    for worker in workers:worker.start()
    # Protocol fields independently encoded from observed computer announcements.
    # This tests discovery only and must never be used as a real export server.
    data=bytearray(50);data[:10]=b'Qspt1WmJOL';data[10]=2
    name=b'rekordbox';data[12:12+len(name)]=name
    data[32:36]=bytes((1,3,0,50));data[36:40]=socket.inet_aton(BIND)
    data[40:46]=bytes((2,0,0,0,0x11,args.number));data[46:50]=bytes((args.number,6,4,1))
    with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as sender:
        bind_socket(sender,50000);sender.setsockopt(socket.SOL_SOCKET,socket.SO_BROADCAST,1)
        sender.settimeout(.001)
        initial=bytearray(37);initial[:36]=data[:36];initial[10]=10
        initial[33]=2;initial[35]=37;initial[36]=3
        sender.sendto(initial,('192.0.2.255',50000));time.sleep(.3)
        for step in range(1,4):
            claim=bytearray(44);claim[:36]=data[:36];claim[10]=0;claim[35]=44
            claim[36:38]=bytes((step,4));claim[38:44]=data[40:46]
            sender.sendto(claim,('192.0.2.255',50000));time.sleep(.3)
        for step in range(1,4):
            claim=bytearray(data);claim[47]=step
            sender.sendto(claim,('192.0.2.255',50000));time.sleep(.3)
        for step in range(1,4):
            claim=bytearray(42);claim[:36]=data[:36];claim[10]=4;claim[33]=2;claim[35]=42
            claim[36:38]=bytes((args.number,step))
            sender.sendto(claim,('192.0.2.255',50000));time.sleep(.3)
        for tick in range(args.seconds):
            data[46]=args.number
            if tick%3==0:
                alive=bytearray(54);alive[:36]=data[:36];alive[10]=6;alive[35]=54
                alive[36:38]=bytes((args.number,1));alive[38:44]=data[40:46]
                alive[44:48]=socket.inet_aton(BIND);alive[48]=2
                alive[52:54]=bytes((4,8))
                sender.sendto(alive,('192.0.2.255',50000))
            sender.sendto(data,('192.0.2.255',50000))
            until=time.monotonic()+1
            while time.monotonic()<until:
                try:
                    reply,address=sender.recvfrom(512)
                    if address[0]=='192.0.2.15' and len(reply)>=36:
                        emit('discovery_reply',kind=reply[10],bytes=len(reply),fields=list(reply[36:50]))
                except socket.timeout:time.sleep(.01)
    stop.set()
    for worker in workers:worker.join(timeout=3)
    if files is not None:files.close()
    emit('provider_probe_finished',number=args.number,announcements=args.seconds,library_service='snapshot metadata' if backend else 'empty control',file_service='read-only fixture' if files else 'empty control')


if __name__=='__main__':main()

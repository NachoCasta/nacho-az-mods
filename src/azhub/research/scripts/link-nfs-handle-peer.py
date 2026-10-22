#!/usr/bin/env python3
"""Hold a native fixture NFSv2 handle across eject; isolated emulator only."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import socket
import struct
import time


def words(*values):return struct.pack('!'+'I'*len(values),*values)
def opaque(value):return words(len(value))+value+b'\0'*((-len(value))%4)


class RPC:
    def __init__(self):
        self.socket=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
        self.socket.bind(('192.0.2.2',0));self.socket.settimeout(1)
        self.serial=10000
    def call(self,port,program,version,procedure,args=b''):
        self.serial+=1;xid=self.serial
        packet=words(xid,0,2,program,version,procedure,0,0,0,0)+args
        for _ in range(3):
            self.socket.sendto(packet,('192.0.2.15',port))
            deadline=time.monotonic()+1
            while time.monotonic()<deadline:
                try:data,address=self.socket.recvfrom(65536)
                except socket.timeout:break
                if address!=('192.0.2.15',port) or len(data)<24:continue
                if struct.unpack_from('!III',data)!=(xid,1,0):continue
                size=struct.unpack_from('!I',data,16)[0];offset=20+((size+3)//4)*4
                if len(data)<offset+4 or struct.unpack_from('!I',data,offset)[0]!=0:
                    raise RuntimeError('RPC response rejected')
                return data[offset+4:]
        raise TimeoutError('Native RPC request timed out')
    def port(self,program,version):
        data=self.call(111,100000,2,3,words(program,version,17,0))
        port=struct.unpack('!I',data)[0]
        if not 1<=port<=65535:raise RuntimeError('Invalid portmapper response')
        return port


def emit(event,**fields):print(json.dumps(dict(event=event,monotonic_ns=time.monotonic_ns(),**fields)),flush=True)
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mount-wait',type=float,default=0,help='Explicit bounded wait for native export admission; log every status13 retry')
    parser.add_argument('--samples',type=int,default=9)
    parser.add_argument('--interval',type=float,default=5)
    args=parser.parse_args()
    if not 2<=args.samples<=64 or not .2<=args.interval<=10 or not 0<=args.mount_wait<=60:
        parser.error('Bounded samples2..64 and interval0.2..10 required')
    spec=importlib.util.spec_from_file_location('guard',Path(__file__).with_name('link-library-peer.py'))
    guard=importlib.util.module_from_spec(spec);spec.loader.exec_module(guard);guard.isolated()
    rpc=RPC()
    try:
        mount=rpc.port(100005,1);nfs=rpc.port(100003,2)
        mount_deadline=time.monotonic()+args.mount_wait
        while True:
            body=rpc.call(mount,100005,1,1,opaque('/C/'.encode('utf-16le')))
            if len(body)<4:raise RuntimeError('Truncated MOUNT status')
            mount_status=struct.unpack_from('!I',body)[0]
            if not mount_status:break
            emit('mount_rejected',export='/C/',nfs_status=mount_status)
            if mount_status!=13 or time.monotonic()>=mount_deadline:
                raise RuntimeError('Native /C/ MOUNT rejected: status='+str(mount_status))
            time.sleep(max(0,min(2,mount_deadline-time.monotonic())))
        if len(body)<36:raise RuntimeError('Truncated MOUNT handle')
        handle=body[4:36]
        for name in ('Contents','Aaliyah','Are You That Somebody','Aaliyah - Are You That Somebody-1.mp3'):
            body=rpc.call(nfs,100003,2,4,handle+opaque(name.encode('utf-16le')))
            if len(body)<36 or struct.unpack_from('!I',body)[0]:raise RuntimeError('Lookup failed')
            handle=body[4:36]
        for index in range(args.samples):
            try:
                body=rpc.call(nfs,100003,2,6,handle+words(index*1024,1024,0))
                status=struct.unpack_from('!I',body)[0]
                fields={'sample':index,'nfs_status':status}
                if status==0:
                    if len(body)<76:raise RuntimeError('Truncated READ reply')
                    size=struct.unpack_from('!I',body,72)[0];data=body[76:76+size]
                    if len(data)!=size:raise RuntimeError('Truncated READ data')
                    fields.update(bytes=size,sha256=hashlib.sha256(data).hexdigest())
                emit('read',**fields)
            except TimeoutError:emit('read_timeout',sample=index)
            if index+1<args.samples:time.sleep(args.interval)
    finally:rpc.socket.close()


if __name__=='__main__':main()

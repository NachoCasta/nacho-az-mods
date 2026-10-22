#!/usr/bin/env python3
"""Bounded isolated native DB requester/session capacity control; no private slots."""
import importlib.util
import json
from pathlib import Path
import socket
import struct


def main():
    spec=importlib.util.spec_from_file_location('wire',Path(__file__).with_name('link-library-peer.py'))
    wire=importlib.util.module_from_spec(spec);spec.loader.exec_module(wire);wire.isolated()
    def packet(tx,kind,args):
        tags=b'\x06'*len(args)+b'\x00'*(12-len(args))
        return wire.number(0x872349ae)+wire.number(tx)+wire.number(kind,2)+wire.number(len(args),1)+b'\x14'+struct.pack('!I',12)+tags+b''.join(wire.number(x) for x in args)
    def field(sock):
        tag=wire.exact(sock,1)[0]
        if tag in (15,16,17):return int.from_bytes(wire.exact(sock,{15:1,16:2,17:4}[tag]),'big')
        if tag in (20,38):
            size=int.from_bytes(wire.exact(sock,4),'big')*(2 if tag==38 else 1)
            if size>65536:raise ValueError('Oversized field')
            data=wire.exact(sock,size)
            return data if tag==20 else {'string_bytes':size}
        raise ValueError('Unknown field')
    def message(sock):
        if field(sock)!=0x872349ae:raise ValueError('Magic mismatch')
        tx,kind,count=field(sock),field(sock),field(sock);tags=field(sock)
        if not isinstance(count,int) or count>12 or not isinstance(tags,bytes) or len(tags)<count:raise ValueError('Invalid header')
        return tx,kind,[field(sock) for _ in range(count)]
    def emit(**kw):print(json.dumps(kw),flush=True)
    with socket.create_connection(('192.0.2.15',12523),timeout=3) as query:
        query.sendall(b'\x00\x00\x00\x0fRemoteDBServer\x00');port=int.from_bytes(wire.exact(query,2),'big')
    if not 1024<=port<=65535:raise ValueError('Invalid server port')
    clients={};counts={}
    try:
        for requester in (3,7):
            client=socket.create_connection(('192.0.2.15',port),timeout=3);clients[requester]=client
            client.sendall(wire.number(1));assert field(client)==1
            client.sendall(packet(0xfffffffe,0,[requester]));_,kind,args=message(client)
            emit(event='setup',requester=requester,reply=kind,arguments=args)
            if kind!=0x4000:raise RuntimeError('Native setup refused')
            command=0x1004
            dmst=(requester<<24)|(1<<16)|(3<<8)|1
            client.sendall(packet(1,command,[dmst,0] if command==0x1004 else [dmst,0,0xffffff]));_,reply,values=message(client)
            emit(event='menu',requester=requester,reply=reply,command=command,count=values[1] if len(values)>1 else None)
            if reply!=0x4000:raise RuntimeError('Native menu refused')
            counts[requester]=values[1]
        for requester in (3,7):
            client=clients[requester];dmst=(requester<<24)|(1<<16)|(3<<8)|1
            client.sendall(packet(2,0x3000,[dmst,0,64,0,counts[requester],0]));items=[]
            for _ in range(66):
                _,kind,values=message(client)
                if kind==0x4101:items.append(values[6])
                if kind==0x4201:break
            else:raise RuntimeError('Missing menu footer')
            emit(event='render',requester=requester,item_types=items)
            if len(items)!=64:raise RuntimeError('Unexpected render count')
    finally:
        for client in clients.values():
            try:client.sendall(packet(0xfffffffe,0x100,[]))
            except OSError:pass
            client.close()


if __name__=='__main__':main()

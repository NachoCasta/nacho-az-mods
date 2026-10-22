# SPDX-License-Identifier: GPL-3.0-only
"""Bounded dbserver field/message framing for isolated research services."""
import struct


def exact(sock,size):
    result=bytearray()
    while len(result)<size:
        part=sock.recv(size-len(result))
        if not part:raise EOFError('Truncated field')
        result.extend(part)
    return bytes(result)


def number(value,size=4):return bytes(({1:15,2:16,4:17}[size],))+value.to_bytes(size,'big')
def field(sock):
    tag=exact(sock,1)[0]
    if tag in (15,16,17):return int.from_bytes(exact(sock,{15:1,16:2,17:4}[tag]),'big')
    if tag not in (20,38):raise ValueError('Unknown field tag')
    size=int.from_bytes(exact(sock,4),'big')*(2 if tag==38 else 1)
    if size>65536:raise ValueError('Oversized field')
    data=exact(sock,size)
    if tag==20:return data
    if not data.endswith(b'\0\0'):raise ValueError('Missing string terminator')
    return data[:-2].decode('utf-16be')


def packet(tx,kind,args):
    if len(args)>16:raise ValueError('Too many arguments')
    tags=bytearray();fields=bytearray()
    for index,value in enumerate(args):
        if isinstance(value,int):tags.append(6);fields.extend(number(value))
        elif isinstance(value,str):
            data=value.encode('utf-16be')+b'\0\0';tags.append(2)
            fields.extend(b'\x26'+struct.pack('!I',len(data)//2)+data)
        elif isinstance(value,bytes):
            tags.append(3)
            if not value:
                if index==0 or type(args[index-1]) is not int or args[index-1]!=0:raise ValueError('Empty blob requires preceding zero length')
            else:fields.extend(b'\x14'+struct.pack('!I',len(value))+value)
        else:raise ValueError('Unsupported field')
    width=16 if len(tags)>12 else 12
    tags.extend(b'\0'*(width-len(tags)))
    return number(0x872349ae)+number(tx)+number(kind,2)+number(len(args),1)+b'\x14'+struct.pack('!I',width)+tags+fields


def message(sock):
    if field(sock)!=0x872349ae:raise ValueError('Invalid magic')
    tx,kind,count,tags=field(sock),field(sock),field(sock),field(sock)
    if not isinstance(count,int) or count>16 or not isinstance(tags,bytes) or len(tags) not in (count,12,16):
        raise ValueError('Invalid argument header')
    args=[]
    for tag in tags[:count]:
        value=b'' if tag==3 and args and type(args[-1]) is int and args[-1]==0 else field(sock)
        args.append(value)
    for tag,value in zip(tags,args):
        if not ((isinstance(value,int) and tag in (4,5,6)) or (isinstance(value,str) and tag==2) or (isinstance(value,bytes) and tag==3)):
            raise ValueError(f'Argument type mismatch ({tag:#x}/{type(value).__name__})')
    return {'transaction':tx,'type':kind,'arguments':args}

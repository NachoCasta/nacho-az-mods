#!/usr/bin/env python3
"""Record RPC call headers/ports on isolated TAP, never file data or paths."""
import argparse
import json
import os
import socket
import struct
import time


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seconds',type=float,default=30)
    args=parser.parse_args()
    if not 0<args.seconds<=3600: parser.error('Expected bounded duration')
    if os.stat('/proc/self/ns/net').st_ino==os.stat('/proc/1/ns/net').st_ino:
        raise SystemExit('Refusing host network namespace')
    pending={}
    with socket.socket(socket.AF_PACKET,socket.SOCK_RAW,socket.htons(3)) as capture:
        capture.bind(('azlink0',0));capture.settimeout(1)
        deadline=time.monotonic()+args.seconds
        while time.monotonic()<deadline:
            try: frame,_=capture.recvfrom(65536)
            except socket.timeout: continue
            if len(frame)<42 or frame[12:14]!=b'\x08\x00':continue
            ip=frame[14:];ihl=(ip[0]&15)*4
            total=struct.unpack_from('!H',ip,2)[0]
            if ip[0]>>4!=4 or ihl<20 or total>len(ip) or ip[9]!=17 or total<ihl+8:continue
            if struct.unpack_from('!H',ip,6)[0]&0x3fff:continue
            source=socket.inet_ntoa(ip[12:16]);dest=socket.inet_ntoa(ip[16:20])
            if {source,dest}!={'192.0.2.2','192.0.2.15'}:continue
            sport,dport,length,_=struct.unpack_from('!HHHH',ip,ihl)
            if length<32 or ihl+length>total:continue
            data=ip[ihl+8:ihl+length];xid,kind=struct.unpack_from('!II',data)
            event={'monotonic_ns':time.monotonic_ns(),'source':source,'destination':dest,
                   'source_port':sport,'destination_port':dport,'payload_bytes':len(data),'xid':xid}
            if kind==0 and len(data)>=40:
                version,program,pversion,procedure=struct.unpack_from('!IIII',data,8)
                if version!=2 or program not in (100000,100003,100005):continue
                event.update(kind='call',program=program,version=pversion,procedure=procedure)
                if len(pending)>=32768:raise RuntimeError('RPC observation capacity exceeded')
                pending[(source,sport,dest,dport,xid)]=(program,pversion,procedure)
            elif kind==1:
                key=(dest,dport,source,sport,xid);call=pending.pop(key,None)
                if call is None:continue
                event.update(kind='reply',program=call[0],version=call[1],procedure=call[2],
                             reply_state=struct.unpack_from('!I',data,8)[0])
            else:continue
            print(json.dumps(event),flush=True)


if __name__=='__main__':main()

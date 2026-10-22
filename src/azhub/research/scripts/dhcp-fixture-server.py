#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Bounded OFFER/ACK fixture; refuses any topology beyond isolated TAP.

Not a production DHCP server. Serves explicitly selected fixture MACs and
documentation addresses with60/600-second leases. Refresh mode is bounded to
600 seconds by default, or1200 for explicit extended native UI controls. An optional initially absent marker resumes withheld slot0 replies;
the server reads that marker but never creates it. No persistent lease store.
"""
import argparse
import json
from pathlib import Path
import socket
import struct
import subprocess
import time
import select
from link_fixture_network import verify_namespace,source_mac

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--address-flow',action='store_true',help='Verify UDP reachability after the proposed address is acquired')
parser.add_argument('--arp-conflict',action='store_true',help='Inject a foreign claim when the fixture probes the granted address')
parser.add_argument('--retry-after-decline',action='store_true',help='After DECLINE offer a second nonconflicting fixture address')
parser.add_argument('--duration-seconds',type=int,choices=(600,1200),default=600,help='Bounded fixture lifetime; extended native UI controls only')
parser.add_argument('--lease-seconds',type=int,choices=(60,600),default=60,help='Explicit bounded fixture lease')
parser.add_argument('--withhold-offer',action='store_true',help='Observe DISCOVER without replying, for pending-client cancellation controls')
parser.add_argument('--link-local-echo',action='store_true',help='Withhold DHCP and echo only an eligible link-local client on the isolated TAP')
parser.add_argument('--withhold-all',action='store_true',help='Observe all declared derived clients without replies on the isolated link-local fixture')
parser.add_argument('--reconnect',action='store_true',help='Serve a second incarnation with the same client identity and address')
parser.add_argument('--no-echo',action='store_true',help='Use independent library clients instead of the address-flow UDP echo')
parser.add_argument('--source-slots',help='Comma-separated isolated fixture slots0..7, with independent MAC/IP grants')
parser.add_argument('--controller-mac',help='Explicit isolated controller MAC for derived source identities')
parser.add_argument('--duplicate-slot',type=int,choices=range(1,8),help='Offer the selected source the address already granted to slot0')
parser.add_argument('--native-address',action='store_true',help='Offer the emulator native address for a rejection control')
parser.add_argument('--refresh',action='store_true',help='Keep serving same-identity source grants during the bounded fixture lifetime')
parser.add_argument('--withhold-refresh-after',type=int,choices=range(1,9),help='Withhold slot0 replies after this many ACKs until the owned resume marker exists')
parser.add_argument('--resume-refresh-marker',type=Path,help='Initially absent absolute marker in the ignored evidence directory')
parser.add_argument('--change-refresh-marker',type=Path,help='Switch slot0 grants to192.0.2.4 once this initially absent marker exists')
parser.add_argument('--link-local-return-marker',type=Path,help='Initially absent isolated marker enabling only slot0 DHCP after IPv4LL publication')
args=parser.parse_args()
if args.link_local_echo:assert args.withhold_offer and not (args.address_flow or args.source_slots or args.reconnect)
if args.withhold_all:assert args.source_slots and args.refresh and not args.link_local_echo
if args.link_local_return_marker is not None:
    assert args.withhold_all and args.link_local_return_marker.is_absolute() and not args.link_local_return_marker.exists()
if args.controller_mac:assert args.source_slots
if args.change_refresh_marker is not None:
    assert args.refresh and args.withhold_refresh_after is None
    assert args.change_refresh_marker.is_absolute() and not args.change_refresh_marker.exists()
if args.withhold_refresh_after is not None:
    assert args.refresh and args.resume_refresh_marker is not None
    assert args.resume_refresh_marker.is_absolute() and not args.resume_refresh_marker.exists()
else:assert args.resume_refresh_marker is None
if args.refresh:assert args.source_slots and args.lease_seconds==60 and args.duplicate_slot is None and not args.native_address
if args.native_address:assert not (args.source_slots or args.reconnect or args.arp_conflict or args.address_flow or args.withhold_offer)
if args.duplicate_slot is not None:assert args.source_slots and not args.reconnect
if args.withhold_offer:assert not args.address_flow and not args.arp_conflict and not args.retry_after_decline
if args.arp_conflict:assert args.address_flow
if args.retry_after_decline:assert args.arp_conflict
verify_namespace(json.loads(subprocess.check_output(['ip','-j','-d','addr','show'])),link_local=args.link_local_echo or args.withhold_all)
mac=bytes.fromhex('020000001203')
server=socket.inet_aton('192.0.2.2');offered=socket.inet_aton('192.0.2.15' if args.native_address else '192.0.2.3')
cookie=b'\x63\x82\x53\x63'
echo=conflict_socket=decline_stream=None
if args.arp_conflict:
    conflict_socket=socket.socket(socket.AF_PACKET,socket.SOCK_RAW,socket.htons(3))
    conflict_socket.bind(('azlink0',0));conflict_socket.settimeout(1)
if args.link_local_echo or (args.address_flow and (not args.arp_conflict or args.retry_after_decline) and not args.no_echo):
    echo=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    echo.setsockopt(socket.SOL_SOCKET,socket.SO_BINDTODEVICE,b'azlink0\0')
    echo.bind(('169.254.2.2' if args.link_local_echo else '192.0.2.2',49000));echo.settimeout(20)
def options(data):
    result={};offset=240
    while offset<len(data):
        code=data[offset];offset+=1
        if code==255:return result
        if code==0:continue
        if offset>=len(data):return None
        length=data[offset];offset+=1
        if offset+length>len(data) or code in result:return None
        result[code]=data[offset:offset+length];offset+=length
    return None

def multiple_sources():
    slots=[int(value) for value in args.source_slots.split(',')]
    assert len(slots)==len(set(slots)) and 2<=len(slots)<=8 and 0 in slots and all(0<=slot<8 for slot in slots)
    if args.change_refresh_marker is not None:assert 1 not in slots,'Changed address belongs to slot1'
    assert not (args.address_flow or args.arp_conflict or args.retry_after_decline or args.withhold_offer)
    clients={(source_mac(args.controller_mac,slot) if args.controller_mac else bytes([2,0,0,0,0x12,3+slot])):slot for slot in slots}
    transactions={};grants={slot:0 for slot in slots}
    required={slot:2 if args.reconnect and slot==0 else 1 for slot in slots}
    with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as stream:
        stream.setsockopt(socket.SOL_SOCKET,socket.SO_BINDTODEVICE,b'azlink0\0')
        stream.setsockopt(socket.SOL_SOCKET,socket.SO_BROADCAST,1)
        stream.bind(('0.0.0.0',67));stream.settimeout(1)
        print(json.dumps({'ready':True,'fixture_server':'192.0.2.2','source_slots':slots}),flush=True)
        deadline=time.monotonic()+args.duration_seconds
        while args.refresh or any(grants[slot]<required[slot] for slot in slots):
            assert time.monotonic()<deadline,'Incomplete multi-source fixture grants'
            try:data,peer=stream.recvfrom(1500)
            except socket.timeout:continue
            if len(data)<240 or data[:3]!=b'\x01\x01\x06' or data[236:240]!=cookie:continue
            client=data[28:34]
            if client not in clients:continue
            slot=clients[client];opts=options(data)
            if opts is None:continue
            assert opts.get(61)==b'\x01'+client
            if args.withhold_all and not (slot==0 and args.link_local_return_marker is not None and args.link_local_return_marker.exists()):
                if opts.get(53)==b'\x04':
                    print(json.dumps({'DHCPDECLINE_received':True,'slot':slot,'xid':data[4:8].hex()}),flush=True)
                if opts.get(53)==b'\x01':
                    print(json.dumps({'DISCOVER_waiting':True,'slot':slot,'xid':data[4:8].hex(),'client':client.hex(),'monotonic':time.monotonic()}),flush=True)
                continue
            address=bytes([192,0,2,3 if slot==args.duplicate_slot else 3+slot]);xid=data[4:8]
            if slot==0 and args.change_refresh_marker is not None and args.change_refresh_marker.exists():
                assert 1 not in slots,'Changed fixture address must not belong to another selected source'
                address=socket.inet_aton('192.0.2.4')
            if (slot==0 and args.withhold_refresh_after is not None and
                    grants[slot]>=args.withhold_refresh_after and not args.resume_refresh_marker.exists()):
                if opts.get(53)==b'\x01':
                    transactions.pop(client,None)
                    print(json.dumps({'refresh_DISCOVER_waiting':True,'slot':slot,'xid':xid.hex(),
                        'client':client.hex(),'grants':grants[slot],'monotonic':time.monotonic()}),flush=True)
                continue
            if opts.get(53)==b'\x01':kind=2;transactions[client]=xid
            elif opts.get(53)==b'\x03' and transactions.get(client)==xid and opts.get(50)==address and opts.get(54)==server:kind=5
            else:continue
            header=bytearray(236);header[:4]=b'\x02\x01\x06\x00';header[4:8]=xid
            header[10:12]=b'\x80\x00';header[16:20]=address;header[20:24]=server;header[28:34]=client
            payload=bytes(header)+cookie+bytes([53,1,kind,54,4])+server+bytes([51,4])+struct.pack('!I',args.lease_seconds)+bytes([1,4,255,255,255,0,255])
            stream.sendto(payload,('255.255.255.255',68))
            if kind==5:
                grants[slot]+=1;transactions.pop(client,None)
            print(json.dumps({'reply':'OFFER' if kind==2 else 'ACK','slot':slot,'client':client.hex(),'address':socket.inet_ntoa(address),'grants':grants[slot]}),flush=True)
        print(json.dumps({'all_source_grants_complete':True,'grants':grants}),flush=True)

if args.source_slots:
    multiple_sources()
    raise SystemExit(0)

def exchange(offered,keep_decline=False,declined_at=None):
  with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as stream:
    stream.setsockopt(socket.SOL_SOCKET,socket.SO_BINDTODEVICE,b'azlink0\0')
    stream.setsockopt(socket.SOL_SOCKET,socket.SO_BROADCAST,1)
    stream.bind(('0.0.0.0',67));stream.settimeout(1)
    if declined_at is None:print(json.dumps({'ready':True,'fixture_server':'192.0.2.2'}),flush=True)
    deadline=time.monotonic()+(args.duration_seconds if args.reconnect else 40);offered_xid=None
    while time.monotonic()<deadline:
        if args.link_local_echo:
            readable,_,_=select.select([stream,echo],[],[],1)
            if echo in readable:
                data,peer=echo.recvfrom(256);ip=struct.unpack('!I',socket.inet_aton(peer[0]))[0]
                assert 0xa9fe0100<=ip<=0xa9fefeff and peer[0]!='169.254.2.2' and data==b'AZHUB-ADDRESS-READY'
                echo.sendto(b'AZHUB-ADDRESS-OK',peer)
                print(json.dumps({'link_local_UDP_roundtrip':True,'peer':peer[0]}),flush=True);return None
            if stream not in readable:continue
        try:data,peer=stream.recvfrom(1500)
        except socket.timeout:continue
        if len(data)<240 or data[:3]!=b'\x01\x01\x06' or data[28:34]!=mac or data[236:240]!=cookie:continue
        opts=options(data)
        if opts is None:continue
        kind=opts.get(53);xid=data[4:8]
        assert opts.get(61)==b'\x01'+mac,'Unexpected native client identifier'
        if kind==b'\x01':
            if args.withhold_offer:
                print(json.dumps({'DISCOVER_waiting':True,'xid':xid.hex(),'client':mac.hex()}),flush=True)
                continue
            reply=2;offered_xid=xid
            if declined_at is not None:
                print(json.dumps({'new_discovery_after_decline_seconds':time.monotonic()-declined_at}),flush=True)
        elif kind==b'\x03' and xid==offered_xid and opts.get(50)==offered and opts.get(54)==server:reply=5
        else:continue
        header=bytearray(236);header[:4]=b'\x02\x01\x06\x00';header[4:8]=xid
        header[10:12]=b'\x80\x00';header[16:20]=offered;header[20:24]=server;header[28:34]=mac
        payload=bytes(header)+cookie+bytes([53,1,reply,54,4])+server+bytes([51,4])+struct.pack('!I',args.lease_seconds)+bytes([1,4,255,255,255,0,255])
        stream.sendto(payload,('255.255.255.255',68))
        print(json.dumps({'reply':'OFFER' if reply==2 else 'ACK','xid':xid.hex(),'client':mac.hex(),'address':socket.inet_ntoa(offered),'lease':args.lease_seconds}),flush=True)
        if reply==5:
            if keep_decline:
                follow=stream.dup();follow.settimeout(10);return follow
            return None
    else:raise SystemExit('No complete fixture DHCP exchange')
decline_stream=exchange(offered,args.arp_conflict)
if args.link_local_echo:
    echo.close();raise SystemExit(0)
if conflict_socket:
    with conflict_socket:
        deadline=time.monotonic()+15
        while time.monotonic()<deadline:
            try:frame=conflict_socket.recv(2048)
            except socket.timeout:continue
            if (len(frame)>=42 and frame[6:12]==mac and frame[12:14]==b'\x08\x06' and
                    frame[14:22]==b'\x00\x01\x08\x00\x06\x04\x00\x01' and
                    frame[28:32]==b'\0'*4 and frame[38:42]==offered):break
        else:raise SystemExit('No fixture ARP probe observed')
        own=bytes.fromhex('020000000202')
        claim=mac+own+b'\x08\x06'+b'\x00\x01\x08\x00\x06\x04\x00\x02'+own+offered+mac+b'\0'*4
        conflict_socket.send(claim)
        print(json.dumps({'foreign_ARP_claim_injected':True,'address':'192.0.2.3'}),flush=True)
        with decline_stream:
            data,peer=decline_stream.recvfrom(1500)
            declined_at=time.monotonic()
            assert len(data)>=300 and data[:4]==b'\x01\x01\x06\x00' and data[28:34]==mac
            assert data[4:8]!=b'\0'*4 and data[8:28]==b'\0'*20
            assert data[236:240]==cookie and peer==('0.0.0.0',68),peer
            declined=options(data)
            assert declined and declined.get(53)==b'\x04' and declined.get(50)==offered
            assert declined.get(54)==server and declined.get(61)==b'\x01'+mac
            assert declined.get(56)==b'Address conflict'
            assert set(declined)=={53,50,54,61,56}
            print(json.dumps({'DHCPDECLINE_received':True,'requested_address':'192.0.2.3',
                              'source':'0.0.0.0','client_identifier_matches_grant':True}),flush=True)
        deadline=time.monotonic()+2
        while time.monotonic()<deadline:
            try:frame=conflict_socket.recv(2048)
            except socket.timeout:continue
            if len(frame)<42 or frame[6:12]!=mac:continue
            assert not (frame[12:14]==b'\x08\x06' and frame[28:32]==offered),'Conflicting client announced address'
            assert not (frame[12:14]==b'\x08\x00' and frame[26:30]==offered),'Conflicting client used IPv4'
        print(json.dumps({'no_fixture_ARP_claim_or_IPv4_use_after_conflict':True}),flush=True)
    if args.retry_after_decline:
        offered=socket.inet_aton('192.0.2.4')
        exchange(offered,declined_at=declined_at)
        print(json.dumps({'reacquisition_after_decline':True,'new_address':'192.0.2.4'}),flush=True)
if args.reconnect:
    assert not args.arp_conflict and not args.withhold_offer
    exchange(offered)
    print(json.dumps({'reconnected_client_granted':True,'address':socket.inet_ntoa(offered)}),flush=True)
if echo:
    with echo:
        data,peer=echo.recvfrom(256)
        assert peer[0]==socket.inet_ntoa(offered) and data==b'AZHUB-ADDRESS-READY',(peer,data)
        echo.sendto(b'AZHUB-ADDRESS-OK',peer)
        print(json.dumps({'configured_address_UDP_roundtrip':True,'peer':peer[0]}),flush=True)

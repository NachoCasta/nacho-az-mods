#!/usr/bin/env python3
"""Explicit bounded link-local DB-port query; no AZ writes, reload or track load."""
import argparse
import ipaddress
import json
import socket
import time

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--local',required=True)
p.add_argument('--target',required=True)
p.add_argument('--seconds',type=int,default=30)
a=p.parse_args()
local=ipaddress.IPv4Address(a.local);target=ipaddress.IPv4Address(a.target)
assert local.is_link_local and target.is_link_local and local!=target
assert 1<=a.seconds<=120
end=time.monotonic()+a.seconds
while time.monotonic()<end:
    start=time.monotonic();phase='connect'
    row={'time_unix':time.time(),'target':str(target),'local':str(local)}
    with socket.socket() as connection:
        connection.settimeout(1)
        connection.bind((str(local),0))
        try:
            connection.connect((str(target),12523));phase='query'
            connection.sendall(bytes([0,0,0,15])+b'RemoteDBServer\0')
            reply=connection.recv(2)
            if len(reply)!=2:raise ValueError('Incomplete DB port response')
            row.update(connected=True,metadata_port=int.from_bytes(reply,'big'))
        except (OSError,ValueError) as error:
            row.update(failed_phase=phase,error=type(error).__name__,message=str(error))
    row['elapsed_seconds']=time.monotonic()-start
    print(json.dumps(row),flush=True)
    time.sleep(max(0,1-(time.monotonic()-start)))

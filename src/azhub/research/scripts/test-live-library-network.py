#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Reconstruct owned emulator live-file control using explicit local artifacts.

Run from repository root on Windows. Never targets the physical controller.
Required prepared Java classes contain LocalFileServiceClient; module must be
azhub_test_macvlan built against the exact emulator6.6 kernel, not target4.4.
"""
from pathlib import Path
import argparse, hashlib, importlib.util, json, os, queue, subprocess, sys, threading, time, uuid

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--server',type=Path,required=True)
parser.add_argument('--module',type=Path,required=True)
parser.add_argument('--classes',type=Path,required=True)
parser.add_argument('--evidence-directory',type=Path,required=True)
parser.add_argument('--combined-provider',action='store_true',help='Use the source worker with public discovery and metadata')
parser.add_argument('--nat-port',type=int,default=15188,help='Unoccupied loopback test transport port')
args=parser.parse_args()
assert 1024<=args.nat_port<=65535
root=Path.cwd().resolve(); out=args.evidence_directory.resolve()
assert os.name=='nt' and root.drive.upper()=='C:'
assert out.is_relative_to(root/'local/diagnostics') and not out.exists()
for artifact in (args.server,args.module,args.classes):
    assert artifact.resolve().is_relative_to(root) and artifact.exists()
out.mkdir(parents=True)
base='/mnt/c/'+root.as_posix()[3:]
spec=importlib.util.spec_from_file_location('emu',root/'src/azhub/research/scripts/eject-ui-emu.py')
emu=importlib.util.module_from_spec(spec);spec.loader.exec_module(emu)
az=emu.azemu
assert az.SSH_PORT==2222 and az.QMP==('127.0.0.1',4446)
rc,kernel=az.ssh('uname -r',timeout=15);assert rc==0 and kernel.strip()=='6.6.138-dirty'
commands=[]; files=[]; scope=module=link=moved=False
bridge=server=None; clients=[]
token=uuid.uuid4().hex
remote='/tmp/azhub-live-server-'+token
remote_module='/tmp/azhub-live-macvlan-'+token+'.ko'
ns='azhub-pubtest'

def guest(command,check=True):
    r=subprocess.run(az._ssh_base()+[command],capture_output=True,text=True,timeout=25)
    commands.append({'command':command,'exit':r.returncode,'stdout':r.stdout,'stderr':r.stderr})
    if check:assert r.returncode==0,(command,r.stderr)
    return r

def host(*args,check=True):
    r=subprocess.run(['wsl.exe','-u','root','--exec',*args],capture_output=True,text=True,timeout=20)
    if check:assert r.returncode==0,(args,r.stderr)
    return r

def launch(command,name,stdin=False):
    stdout=(out/(name+'.stdout')).open('wb');stderr=(out/(name+'.stderr')).open('wb')
    files.extend([stdout,stderr])
    return subprocess.Popen(command,stdin=subprocess.PIPE if stdin else subprocess.DEVNULL,stdout=stdout,stderr=stderr)

def server_line(timeout):
    pending=queue.Queue()
    threading.Thread(target=lambda:pending.put(server.stdout.readline()),daemon=True).start()
    return pending.get(timeout=timeout).decode().strip()

try:
    assert ns not in guest('ip netns list').stdout
    assert not host('ip','netns','list').stdout.strip()
    assert 'azhub_test_macvlan' not in guest('cat /proc/modules').stdout
    links=json.loads(guest('ip -j link show').stdout)
    assert all(row['ifname']!='azpub0' for row in links)
    assert next(row for row in links if row['ifname']=='eth0')['address']=='52:54:00:12:34:56'
    az.push_file(str(args.server.resolve()),remote)
    az.push_file(str(args.module.resolve()),remote_module)
    guest('insmod '+remote_module);module=True
    guest('ip netns add '+ns);scope=True
    guest('ip link add link eth0 name azpub0 address 02:00:00:00:12:03 type macvlan mode bridge');link=True
    guest('ip link set azpub0 netns '+ns);moved=True
    guest('ip -n '+ns+' link set lo up')
    guest('ip -n '+ns+' addr add 192.0.2.3/24 brd 192.0.2.255 dev azpub0')
    guest('ip -n '+ns+' link set azpub0 up')
    guest('ip -n '+ns+' -j -d addr show')
    host('ip','netns','add','azhub-link');scope_host=True
    host('ip','-n','azhub-link','link','set','lo','up')
    host('ip','netns','exec','azhub-link','ip','tuntap','add','dev','azlink0','mode','tap')
    host('ip','-n','azhub-link','addr','add','192.0.2.2/24','brd','192.0.2.255','dev','azlink0')
    host('ip','-n','azhub-link','link','set','azlink0','address','02:00:00:00:02:02')
    host('ip','-n','azhub-link','link','set','azlink0','up')
    transport_seconds=150 if args.combined_provider else 90
    bridge=launch([sys.executable,str(root/'src/azhub/research/scripts/link-primary-tap-transport.py'),
                   '--seconds',str(transport_seconds),'--nat-port',str(args.nat_port)],'transport')
    deadline=time.monotonic()+15
    while '"ready": true' not in (out/'transport.stdout').read_text():
        assert bridge.poll() is None,'Transport ended before readiness'
        assert time.monotonic()<deadline,'Transport readiness timeout'
        time.sleep(.2)
    stderr=(out/'server.stderr').open('wb');files.append(stderr)
    server_mode=' --emulator-provider' if args.combined_provider else ' --library --emulator-link'
    server_env='AZHUB_RESEARCH_SECONDS=180 ' if args.combined_provider else ''
    server=subprocess.Popen(az._ssh_base()+[server_env+'AZ_PUBLIC_RPC_STATS=1 ip netns exec '+ns+' '+remote+
                            ' /media/usb/sdc1'+server_mode],
                            stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=stderr)
    ready=server_line(25);(out/'server-ready.txt').write_text(ready+'\n')
    assert len(ready.split())==2 and ready.split()[1 if args.combined_provider else 0]=='111',ready
    print('LIVE_SERVICE_READY',flush=True)
    fixtures=[('database','PIONEER/rekordbox/export.pdb','sharing-hub0-export.pdb'),
              ('audio','Contents/Aaliyah/Are You That Somebody/Aaliyah - Are You That Somebody-1.mp3','sharing-hub0-audio.mp3')]
    classpath=base+'/'+args.classes.resolve().relative_to(root).as_posix()+':'+base+'/local/link-peer-build/dependencies/*'
    if args.combined_provider:
        command=['wsl.exe','-u','root','--exec','ip','netns','exec','azhub-link','java','-cp',classpath,
                 'LiveProviderControl',base+'/local/diagnostics/sharing-hub0-export.pdb',
                 base+'/local/diagnostics/sharing-pdb-tables-v1/java-entities.tsv']
        clients.append(('discovery_and_menus',launch(command,'discovery-and-menus')))
    for kind,relative,fixture in fixtures:
        path=root/'local/diagnostics'/fixture
        command=['wsl.exe','-u','root','--exec','ip','netns','exec','azhub-link','java','-cp',classpath,
                 'LocalFileServiceClient',relative,str(path.stat().st_size),hashlib.sha256(path.read_bytes()).hexdigest(),'--emulator-link']
        clients.append((kind,launch(command,kind)))
    for kind,client in clients:
        client.wait(timeout=120 if args.combined_provider else 65);assert client.returncode==0,(kind,client.returncode)
        print(kind+'_TRANSFER_PASS',flush=True)
    lifecycle=None
    if args.combined_provider:
        command=['wsl.exe','-u','root','--exec','ip','netns','exec','azhub-link','java','-cp',classpath,
                 'ProviderLifecycleControl',ready.split()[0]]
        lifecycle=launch(command,'lifecycle',stdin=True)
        clients.append(('lifecycle',lifecycle))
        deadline=time.monotonic()+15
        while 'READY:' not in (out/'lifecycle.stdout').read_text():
            assert lifecycle.poll() is None,'Lifecycle control ended before readiness'
            assert time.monotonic()<deadline,'Lifecycle readiness timeout'
            time.sleep(.05)
    server.stdin.write(b'E');server.stdin.flush();assert server_line(10)=='WITHDRAWN'
    if lifecycle:
        lifecycle.stdin.write(b'W');lifecycle.stdin.flush()
        lifecycle.wait(timeout=30);assert lifecycle.returncode==0,'External withdrawal control failed'
    server.stdin.write(b'Q');server.stdin.flush();server.wait(timeout=10);assert server.returncode==0
    (out/'result.json').write_text(json.dumps({'live_root':'emulator hub0','transfers':['database','audio'],
        'C_server_aarch64':True,'withdrawal_drained':True,'combined_provider':args.combined_provider,
        'peer_withdrawal_checks':args.combined_provider,
        'physical_AZ_touched':False},indent=2)+'\n')
    print('LIVE_EXPORT_PASS',flush=True)
    bridge.wait(timeout=transport_seconds+10);assert bridge.returncode==0
finally:
    for _,child in clients:
        if child.poll() is None:child.terminate();child.wait(timeout=10)
        if child.stdin:child.stdin.close()
    if server and server.poll() is None:
        server.stdin.close()
        try:server.wait(timeout=10)
        except subprocess.TimeoutExpired:server.terminate();server.wait(timeout=10)
    if bridge and bridge.poll() is None:
        emu.qmp('object-del',{'id':'azhubtapguard'})
        bridge.terminate();bridge.wait(timeout=10)
        emu.qmp('chardev-remove',{'id':'azhubtapnat'})
    if 'scope_host' in globals() and scope_host:
        # This namespace was created in this scope and contains only owned helpers.
        pids=host('ip','netns','pids','azhub-link').stdout.split()
        for pid in pids:
            assert pid.isdigit();host('kill','-TERM',pid,check=False)
        host('ip','netns','del','azhub-link')
    if scope:
        # Closing the SSH client does not prove its remote child exited.
        # This namespace contains only this scope's explicitly pushed server.
        pids=guest('ip netns pids '+ns).stdout.split()
        for pid in pids:
            assert pid.isdigit()
            command=guest('cat /proc/'+pid+'/cmdline').stdout.split('\x00')[0]
            assert command==remote,('Unexpected process in owned namespace',pid,command)
            guest('kill -TERM '+pid)
        deadline=time.monotonic()+5
        while guest('ip netns pids '+ns).stdout.strip():
            assert time.monotonic()<deadline,'Owned server did not terminate'
            time.sleep(.1)
        if link and moved:guest('ip -n '+ns+' link del azpub0');link=False
        guest('ip netns del '+ns)
    if link:guest('ip link del azpub0')
    if module:guest('rmmod azhub_test_macvlan')
    guest('rm -f -- '+remote+' '+remote+'.new '+remote_module+' '+remote_module+'.new')
    guest('ip netns list');guest('cat /proc/modules')
    (out/'guest-commands.json').write_text(json.dumps(commands,indent=2)+'\n')
    for handle in files:handle.close()
    print('OWNED_SCOPE_CLEANED',flush=True)

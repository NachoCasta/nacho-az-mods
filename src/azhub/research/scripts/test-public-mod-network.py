#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Temporarily install an explicit sharing candidate in the localhost emulator.

Requires empty decks, the existing protected USB fixtures and matching SDK assets.
Creates an isolated test network, verifies the mapped shared-object hash and uses
independent Beat Link/Crate Digger clients for discovery, menus and exact files.
Restores the previous emulator mod/approval files and removes owned networking.
No physical AZ access, publication or external loaded-deck eject acceptance.
"""
from pathlib import Path
import argparse, hashlib, importlib.util, json, os, re, subprocess, sys, time, uuid

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--mod',dest='server',type=Path,required=True,help='Explicit sharing-emulator shared object')
parser.add_argument('--module',type=Path,required=True)
parser.add_argument('--classes',type=Path,required=True)
parser.add_argument('--evidence-directory',type=Path,required=True)
parser.add_argument('--combined-provider',action='store_true',help='Required explicit discovery/menu integration control')
parser.add_argument('--nat-port',type=int,default=15188,help='Unoccupied loopback test transport port')
parser.add_argument('--eject-source',action='store_true',help='Eject/reconnect the protected functionalstickA fixture through native UI')
parser.add_argument('--sources',type=int,choices=(1,2),default=1,help='Number of existing live fixture sources to export')
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
bridge=None; clients=[]
fixture_need_reset=fixture_detached=False
extra_scopes=[]
token=uuid.uuid4().hex
remote='/tmp/azhub-mod-network-'+token
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

def wait_for(predicate,description,timeout=40):
    deadline=time.monotonic()+timeout
    while not predicate():
        assert time.monotonic()<deadline,description
        time.sleep(.2)

def fixture_guard():
    blocks=emu.qmp('query-block',{})['return']
    assert any(row.get('qdev')=='/machine/peripheral/functionalstickA/functionalstickA.0/legacy[0]' and
               row.get('inserted',{}).get('node-name')=='uibadgeA' for row in blocks)
    assert emu.qmp('qom-get',{'path':'/machine/peripheral/functionalstickA','property':'port'})['return']=='2.1'
    nodes=emu.qmp('query-named-block-nodes',{})['return']
    node=next(row for row in nodes if row['node-name']=='uibadgeA')
    assert node['drv']=='qcow2','Requires a protected fixture overlay'
    backing=node['image']
    while 'backing-image' in backing:backing=backing['backing-image']
    assert backing['format']=='raw'
    assert any(row['drv']=='raw' and row.get('ro') and
               row.get('image',{}).get('filename')==backing['filename'] for row in nodes),'Base image must remain read-only'

def replug_fixture():
    global fixture_detached,fixture_need_reset
    if not fixture_detached:
        result=emu.qmp('device_del',{'id':'functionalstickA'});assert 'error' not in result,result
        fixture_detached=True
    wait_for(lambda:guest('test ! -e /sys/bus/usb/devices/3-2.1',check=False).returncode==0,'Fixture detach')
    result=emu.qmp('device_add',{'driver':'usb-storage','id':'functionalstickA','drive':'uibadgeA',
                               'bus':'functionalxhci.0','port':'2.1'})
    assert 'error' not in result,result
    fixture_detached=False
    wait_for(lambda:'/media/usb/sdc1' in guest('cat /proc/mounts').stdout,'Fixture remount')
    fixture_need_reset=False


try:
    assert guest('systemctl is-active EP147').stdout.strip()=='active','Requires the normal emulator service'
    if args.eject_source:fixture_guard()
    assert ns not in guest('ip netns list').stdout
    assert not host('ip','netns','list').stdout.strip()
    assert 'azhub_test_macvlan' not in guest('cat /proc/modules').stdout
    links=json.loads(guest('ip -j link show').stdout)
    assert all(row['ifname']!='azpub0' for row in links)
    assert next(row for row in links if row['ifname']=='eth0')['address']=='52:54:00:12:34:56'
    assert az.push_file(str(args.server.resolve()),remote)
    assert az.push_file(str(args.module.resolve()),remote_module)
    guest('insmod '+remote_module);module=True
    guest('ip netns add '+ns);scope=True
    guest('ip link add link eth0 name azpub0 address 02:00:00:00:12:03 type macvlan mode bridge');link=True
    guest('ip link set azpub0 netns '+ns);moved=True
    guest('ip -n '+ns+' link set lo up')
    guest('ip -n '+ns+' addr add 192.0.2.3/24 brd 192.0.2.255 dev azpub0')
    guest('ip -n '+ns+' link set azpub0 up')
    guest('ip -n '+ns+' -j -d addr show')
    for source in range(1,args.sources):
        extra={'namespace':ns+str(source),'interface':'azpub'+str(source),'link':False,'moved':False}
        assert all(row['ifname']!=extra['interface'] for row in links)
        guest('ip netns add '+extra['namespace']);extra_scopes.append(extra)
        guest('ip link add link eth0 name '+extra['interface']+' address 02:00:00:00:12:%02x'%(3+source)+' type macvlan mode bridge');extra['link']=True
        guest('ip link set '+extra['interface']+' netns '+extra['namespace']);extra['moved']=True
        guest('ip -n '+extra['namespace']+' link set lo up')
        guest('ip -n '+extra['namespace']+' addr add 192.0.2.'+str(3+source)+'/24 brd 192.0.2.255 dev '+extra['interface'])
        guest('ip -n '+extra['namespace']+' link set '+extra['interface']+' up')
    host('ip','netns','add','azhub-link');scope_host=True
    host('ip','-n','azhub-link','link','set','lo','up')
    host('ip','netns','exec','azhub-link','ip','tuntap','add','dev','azlink0','mode','tap')
    host('ip','-n','azhub-link','addr','add','192.0.2.2/24','brd','192.0.2.255','dev','azlink0')
    host('ip','-n','azhub-link','link','set','azlink0','address','02:00:00:00:02:02')
    host('ip','-n','azhub-link','link','set','azlink0','up')
    transport_seconds=300 if args.sources>1 else 210
    bridge=launch([sys.executable,str(root/'src/azhub/research/scripts/link-primary-tap-transport.py'),
                   '--seconds',str(transport_seconds),'--nat-port',str(args.nat_port),'--nat-unix'],'transport')
    deadline=time.monotonic()+15
    while '"ready": true' not in (out/'transport.stdout').read_text():
        assert bridge.poll() is None,'Transport ended before readiness'
        assert time.monotonic()<deadline,'Transport readiness timeout'
        time.sleep(.2)
    assert args.combined_provider
    owners=re.findall(r'launch (/media/usb/sd[a-z]+[0-9]+/MODS/community-mods/usb-hub-support)/assets/mount-hub\.sh:',
                      guest('cat /tmp/azmod-community/usb-hub-support.log').stdout)
    assert owners,'Requires the running mod to identify its actual USB assets'
    package=owners[-1]+'/usb-hub-support.so'
    stage='/home/root/settings/az-usbload/community/usb-hub-support/usb-hub-support.so'
    approved='/home/root/settings/azmod-community/approved'
    starting='/home/root/settings/azmod-community/.starting'
    unit='EP147-sharing-'+token
    backups=remote+'-backup'
    restore_files=(package,stage,approved,starting)
    original_hashes=[guest('if test -e '+path+'; then sha256sum '+path+'; else echo ABSENT; fi').stdout.split()[0] for path in restore_files]
    assert all(value!='ABSENT' for value in original_hashes[:3]),'Requires an existing emulator mod installation and approval file'
    guest('mkdir '+backups)
    for index,path in enumerate(restore_files):
        if original_hashes[index]!='ABSENT':guest('cp '+path+' '+backups+'/'+str(index))
    mod_restore=True
    guest('systemctl stop EP147')
    guest('cp '+remote+' '+package+' && cp '+remote+' '+stage)
    digest=hashlib.sha256(args.server.read_bytes()).hexdigest()
    assert guest('sha256sum '+package).stdout.split()[0]==digest
    assert guest('sha256sum '+stage).stdout.split()[0]==digest
    guest("printf 'usb-hub-support "+digest+"\\n' >> "+approved+"; echo 0 > "+starting)
    guest('systemd-run --unit='+unit+' --property=EnvironmentFile=/home/root/settings/az-usbload/az-preload.env '
          '--setenv=AZHUB_PUBLIC_EMULATOR=1 --setenv=AZHUB_PUBLIC_EMULATOR_COUNT='+str(args.sources)+' --setenv=AZHUB_EJECT_UI_PROTOTYPE=1 --setenv=AZHUB_EJECT_STATUS=1 --setenv=AZHUB_EJECT_TRACE_LOAD=1 '
          '--setenv=AZHUB_EJECT_PORT2_ROOT=/sys/bus/usb/devices/3-2 /home/root/scripts/apl_start.sh')
    deadline=time.monotonic()+60
    while True:
        metadata_ports=[]
        for source in range(args.sources):
            namespace=ns+str(source) if source else ns
            sockets=guest('ip netns exec '+namespace+' cat /proc/net/tcp').stdout.splitlines()[1:]
            ports=[int(line.split()[1].split(':')[1],16) for line in sockets if line.split()[3]=='0A']
            if 12523 in ports and len(ports)==2:metadata_ports.append(next(port for port in ports if port!=12523))
        if len(metadata_ports)==args.sources:break
        assert time.monotonic()<deadline,guest('tail -40 /tmp/azmod-community/usb-hub-support.log').stdout
        time.sleep(.5)
    metadata=metadata_ports[0]
    ready=str(metadata)+' 111'
    (out/'server-ready.txt').write_text(ready+'\n')
    pid=guest('pidof EP147').stdout.strip()
    assert pid.isdigit()
    assert 'mod=usb-hub-support|1|' in guest('cat /tmp/azmod-community/loaded').stdout
    mapped=guest('grep usb-hub-support.so /proc/'+pid+'/maps | head -1').stdout.split()[0]
    assert guest('sha256sum /proc/'+pid+'/map_files/'+mapped).stdout.split()[0]==digest
    (out/'mod-loaded.json').write_text(json.dumps({'pid':pid,'sha256':digest,'metadata_ports':metadata_ports})+'\n')
    print('LIVE_SERVICE_READY',flush=True)
    fixtures=[('database','PIONEER/rekordbox/export.pdb','sharing-hub0-export.pdb'),
              ('audio','Contents/Aaliyah/Are You That Somebody/Aaliyah - Are You That Somebody-1.mp3','sharing-hub0-audio.mp3')]
    classpath=base+'/'+args.classes.resolve().relative_to(root).as_posix()+':'+base+'/local/link-peer-build/dependencies/*'
    command=['wsl.exe','-u','root','--exec','ip','netns','exec','azhub-link','java','-cp',classpath,
             'LiveProvidersControl' if args.sources>1 else 'LiveProviderControl',
             base+'/local/diagnostics/sharing-hub0-export.pdb',base+'/local/diagnostics/sharing-pdb-tables-v1/java-entities.tsv']
    if args.sources>1:command.append(str(args.sources))
    clients.append(('discovery_and_menus',launch(command,'discovery-and-menus')))
    for source in range(args.sources):
        suffix='-source'+str(source+1) if source else ''
        source_flag='--emulator-link-'+str(source+1)
        for kind,relative,fixture in fixtures:
            path=root/'local/diagnostics'/fixture
            command=['wsl.exe','-u','root','--exec','ip','netns','exec','azhub-link','java','-cp',classpath,
                     'LocalFileServiceClient',relative,str(path.stat().st_size),hashlib.sha256(path.read_bytes()).hexdigest(),source_flag]
            clients.append((kind+suffix,launch(command,kind+suffix)))
    for kind,client in clients:
        client.wait(timeout=180 if args.sources>1 else 120);assert client.returncode==0,(kind,client.returncode)
        print(kind+'_TRANSFER_PASS',flush=True)
    if args.eject_source:
        command=['wsl.exe','-u','root','--exec','ip','netns','exec','azhub-link','java','-cp',classpath,
                 'ProviderLifecycleControl',str(metadata),'--retire-and-reconnect']
        if args.sources>1:command.append('--neighbor')
        lifecycle=launch(command,'lifecycle',stdin=True);clients.append(('lifecycle',lifecycle))
        wait_for(lambda:'READY:' in (out/'lifecycle.stdout').read_text(),'External retained handle',15)
        neighbor=None
        if args.sources>1:
            command=['wsl.exe','-u','root','--exec','ip','netns','exec','azhub-link','java','-cp',classpath,
                     'ProviderNeighborControl',str(metadata_ports[1])]
            neighbor=launch(command,'neighbor',stdin=True);clients.append(('neighbor',neighbor))
            wait_for(lambda:'READY:' in (out/'neighbor.stdout').read_text(),'Neighbor retained handles and session',20)
        before=json.loads(guest('cat /run/azhub-eject-status').stdout)
        assert str(before['pid'])==pid and all(not deck['loaded'] for deck in before['decks'])
        assert before['devices'][0]['disk']=='sdc' and before['devices'][0]['port']==2
        original_mounts={line.split()[1] for line in guest('cat /proc/mounts').stdout.splitlines()
                         if line.split()[1].startswith('/media/usb/') and line.split()[1]!='/media/usb/sdc1'}
        selector_offset=len(guest('cat /tmp/azmod-community/usb-hub-support.log').stdout)
        subprocess.run([sys.executable,str(root/'src/azhub/research/scripts/eject-ui-emu.py'),'stop','2','.5'],check=True)
        def selector_open():
            history=guest('cat /tmp/azmod-community/usb-hub-support.log').stdout
            return 'eject-ui selector port=2' in (history[selector_offset:] if len(history)>=selector_offset else history)
        wait_for(selector_open,'Native selector')
        fixture_need_reset=True
        subprocess.run([sys.executable,str(emu.tools/'az-drive.py'),'knobclick'],check=True)
        wait_for(lambda:any(item['disk']=='sdc' and item['state']==2 for item in
                           json.loads(guest('cat /run/azhub-eject-status').stdout)['devices']),'Native safe eject')
        mounted={line.split()[1] for line in guest('cat /proc/mounts').stdout.splitlines()}
        assert '/media/usb/sdc1' not in mounted and original_mounts.issubset(mounted)
        lifecycle.stdin.write(b'W');lifecycle.stdin.flush()
        if neighbor:neighbor.stdin.write(b'W');neighbor.stdin.flush()
        wait_for(lambda:'WITHDRAWN:' in (out/'lifecycle.stdout').read_text(),'External retirement',30)
        if neighbor:wait_for(lambda:'WITHDRAWN:' in (out/'neighbor.stdout').read_text(),'Neighbor survives withdrawal',10)
        replug_fixture()
        wait_for(lambda:(':%04X'%12523) in guest('ip netns exec '+ns+' cat /proc/net/tcp').stdout,'Public port after reconnect')
        lifecycle.stdin.write(b'R');lifecycle.stdin.flush();lifecycle.wait(timeout=25)
        assert lifecycle.returncode==0,'External reconnect/old-handle check'
        if neighbor:
            neighbor.stdin.write(b'R');neighbor.stdin.flush();neighbor.wait(timeout=15)
            assert neighbor.returncode==0,'Neighbor continuity/root identity'
        print('INTEGRATED_EJECT_RECONNECT_PASS',flush=True)
    assert guest('pidof EP147').stdout.strip()==pid
    (out/'integrated-mod.log').write_text(guest('cat /tmp/azmod-community/usb-hub-support.log').stdout)
    (out/'integrated-status.json').write_text(guest('cat /run/azhub-eject-status').stdout)
    (out/'result.json').write_text(json.dumps({'live_root':'emulator hub0','transfers':['database','audio'],
        'integrated_mod_aarch64':True,'withdrawal_drained':args.eject_source,'combined_provider':args.combined_provider,
        'peer_withdrawal_checks':args.eject_source,'integrated_mod':True,'live_sources':args.sources,
        'neighbor_continuity':args.eject_source and args.sources>1,
        'physical_AZ_touched':False},indent=2)+'\n')
    print('LIVE_EXPORT_PASS',flush=True)
    bridge.wait(timeout=transport_seconds+10);assert bridge.returncode==0
finally:
    for _,child in clients:
        if child.poll() is None:child.terminate();child.wait(timeout=10)
        if child.stdin:child.stdin.close()
    if globals().get('mod_restore'):
        if fixture_need_reset:replug_fixture()
        guest('systemctl stop '+unit,check=False)
        for index,path in enumerate(restore_files):
            if original_hashes[index]=='ABSENT':guest('rm -f -- '+path)
            else:
                guest('cp '+backups+'/'+str(index)+' '+path)
                assert guest('sha256sum '+path).stdout.split()[0]==original_hashes[index]
        guest('systemctl start EP147')
        for index in range(len(restore_files)):guest('rm -f '+backups+'/'+str(index))
        guest('rmdir '+backups)
        print('ORIGINAL_MOD_RESTORED',flush=True)
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
    for extra in reversed(extra_scopes):
        assert not guest('ip netns pids '+extra['namespace']).stdout.strip(),'Unexpected process in owned namespace'
        if extra['link']:
            guest(('ip -n '+extra['namespace'] if extra['moved'] else 'ip')+' link del '+extra['interface'])
        guest('ip netns del '+extra['namespace'])
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

#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Native emulator udhcpc OFFER/ACK with reporting-only hook on isolated TAP.

Does not apply its proposed address, install a mod or touch physical hardware.
Explicit matching emulator MACVLAN module is required. All owned resources are
removed; native application PID, DNS, routes and DHCP processes must survive.
"""
from pathlib import Path
import argparse,importlib.util,json,os,subprocess,sys,time,uuid

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--module',type=Path,required=True)
p.add_argument('--evidence-directory',type=Path,required=True)
p.add_argument('--acquirer',type=Path,help='Explicit pinned AArch64 supervised acquisition control; otherwise use raw native client')
p.add_argument('--network-helper',type=Path,help='Explicit standalone helper for supervised acquisition')
p.add_argument('--address-flow',action='store_true',help='Explicit isolated ARP/configure/UDP/retire control')
p.add_argument('--arp-conflict',action='store_true',help='Expect ARP rejection before applying the DHCP-granted address')
p.add_argument('--retry-after-decline',action='store_true',help='After rejecting the first address wait and acquire a second address')
p.add_argument('--library',action='store_true',help='Serve one live hub library after DHCP/ARP using a standalone worker')
p.add_argument('--classes',type=Path,help='Prepared independent Java discovery/menu/file controls')
p.add_argument('--allocate-network',action='store_true',help='Acquirer owns anonymous namespace and MACVLAN creation; no guest network prepared by runner')
p.add_argument('--allocated-ready',action='store_true',help='Allocated controller also probes ARP, configures, exchanges UDP and retires its address')
p.add_argument('--broker-allocate',action='store_true',help='Exercise ALLOCATE dispatch and real allocator callbacks for the library trial')
p.add_argument('--no-offer',action='store_true',help='Withhold all DHCP replies and validate the supervised native no-offer terminal sequence')
p.add_argument('--link-local',action='store_true',help='After native no-offer, configure/probe an owned IPv4LL address, exchange direct UDP and retire')
args=p.parse_args();root=Path.cwd().resolve();out=args.evidence_directory.resolve()
assert os.name=='nt' and root.drive.upper()=='C:'
assert out.is_relative_to(root/'local/diagnostics') and not out.exists()
assert args.module.resolve().is_relative_to(root) and args.module.is_file()
if args.acquirer:assert args.acquirer.resolve().is_relative_to(root) and args.acquirer.is_file()
if args.network_helper:assert args.acquirer and args.network_helper.resolve().is_relative_to(root) and args.network_helper.is_file()
if args.link_local:assert args.allocate_network and args.allocated_ready and not (args.library or args.no_offer or args.arp_conflict)
if args.no_offer:assert args.acquirer and args.network_helper and not (args.address_flow or args.allocate_network or args.library)
if args.address_flow:assert args.network_helper and args.acquirer
if args.arp_conflict:assert args.address_flow
if args.retry_after_decline:assert args.arp_conflict
if args.library:
 assert (args.address_flow or args.allocated_ready) and not args.arp_conflict and args.classes
 assert args.classes.resolve().is_relative_to(root) and args.classes.is_dir()
if args.allocate_network:assert args.acquirer and args.network_helper and not args.address_flow and (not args.library or args.allocated_ready)
if args.allocated_ready:assert args.allocate_network
if args.broker_allocate:assert args.allocated_ready and args.library
out.mkdir(parents=True)
s=importlib.util.spec_from_file_location('e',root/'src/azhub/research/scripts/eject-ui-emu.py')
e=importlib.util.module_from_spec(s);s.loader.exec_module(e);az=e.azemu
assert az.SSH_PORT==2222 and az.QMP==('127.0.0.1',4446)
commands=[];handles=[];children=[];bridge=acquisition=None
module=scope=link=moved=host_scope=False
token=uuid.uuid4().hex;ns='azhub-dhcp-'+token[:8]
remote_module='/tmp/azhub-dhcp-'+token+'.ko';hook='/tmp/azhub-dhcp-'+token+'.sh'
acquirer='/tmp/azhub-dhcp-'+token+'-acquire'
network_helper='/tmp/azhub-dhcp-'+token+'-helper'
control='/tmp/azhub-dhcp-'+token+'-library'
def guest(cmd,check=True,timeout=20):
 r=subprocess.run(az._ssh_base()+[cmd],capture_output=True,text=True,timeout=timeout)
 commands.append({'command':cmd,'exit':r.returncode,'stdout':r.stdout,'stderr':r.stderr})
 if check:assert r.returncode==0,(cmd,r.stdout,r.stderr)
 return r
def host(*cmd,check=True):
 r=subprocess.run(['wsl.exe','-u','root','--exec',*cmd],capture_output=True,text=True,timeout=20)
 if check:assert r.returncode==0,(cmd,r.stderr)
 return r
def launch(cmd,name):
 stdout=(out/(name+'.stdout')).open('wb');stderr=(out/(name+'.stderr')).open('wb');handles.extend([stdout,stderr])
 child=subprocess.Popen(cmd,stdin=subprocess.DEVNULL,stdout=stdout,stderr=stderr);children.append(child);return child
def ready(path,child,timeout=15):
 deadline=time.monotonic()+timeout
 while '"ready": true' not in path.read_text():
  assert child.poll() is None,('Readiness child exited',child.returncode)
  assert time.monotonic()<deadline,'Readiness timeout'
  time.sleep(.1)
def native_state():
 return {name:guest(cmd).stdout for name,cmd in [('PID','pidof EP147'),('DNS','sha256sum /etc/resolv.conf'),
         ('routes','ip route show table all'),('DHCP_PIDs','pidof udhcpc')]}
try:
 assert guest('uname -r').stdout.strip()=='6.6.138-dirty'
 assert guest('systemctl is-active EP147').stdout.strip()=='active'
 assert not host('ip','netns','list').stdout.strip()
 assert not guest('ip netns list').stdout.strip()
 assert 'azhub_test_macvlan' not in guest('cat /proc/modules').stdout
 links=json.loads(guest('ip -j link show').stdout)
 assert next(row for row in links if row['ifname']=='eth0')['address']=='52:54:00:12:34:56'
 assert all(row['ifname']!='azpub0' for row in links)
 before=native_state()
 if args.library:
  guest('test -f /media/usb/sdc1/PIONEER/rekordbox/export.pdb')
  guest('test ! -e '+control+'.ready && test ! -e '+control+'.done')
 assert az.push_file(str(args.module.resolve()),remote_module)
 assert az.push_file(str(root/'src/azhub/research/scripts/public-dhcp-event-hook.sh'),hook)
 guest('chmod 700 '+hook)
 if args.acquirer:
  assert az.push_file(str(args.acquirer.resolve()),acquirer)
  guest('chmod 700 '+acquirer)
 if args.network_helper:
  assert az.push_file(str(args.network_helper.resolve()),network_helper)
  guest('chmod 700 '+network_helper)
 guest('insmod '+remote_module);module=True
 if not args.allocate_network:
  guest('ip netns add '+ns);scope=True
  guest('ip link add link eth0 name azpub0 address 02:00:00:00:12:03 type macvlan mode bridge');link=True
  guest('ip link set azpub0 netns '+ns);moved=True
  guest('ip -n '+ns+' link set lo up');guest('ip -n '+ns+' link set azpub0 up')
 host('ip','netns','add','azhub-link');host_scope=True
 host('ip','netns','exec','azhub-link','ip','tuntap','add','dev','azlink0','mode','tap')
 host('ip','-n','azhub-link','link','set','lo','up')
 host('ip','-n','azhub-link','addr','add','192.0.2.2/24','brd','192.0.2.255','dev','azlink0')
 host('ip','-n','azhub-link','link','set','azlink0','address','02:00:00:00:02:02')
 host('ip','-n','azhub-link','link','set','azlink0','up')
 if args.link_local:host('ip','-n','azhub-link','addr','add','169.254.2.2/16','brd','169.254.255.255','dev','azlink0')
 bridge=launch([sys.executable,str(root/'src/azhub/research/scripts/link-primary-tap-transport.py'),
                '--seconds','210' if args.library else '60','--fixture-dhcp']+(['--fixture-link-local'] if args.link_local else []),'transport')
 ready(out/'transport.stdout',bridge)
 base='/mnt/c/'+root.as_posix()[3:]
 server_command=['wsl.exe','-u','root','--exec','ip','netns','exec','azhub-link','python3',
                base+'/src/azhub/research/scripts/dhcp-fixture-server.py']
 if args.no_offer or args.link_local:server_command.append('--withhold-offer')
 if args.link_local:server_command.append('--link-local-echo')
 if (args.address_flow or args.allocated_ready) and not args.link_local:server_command.append('--address-flow')
 if args.arp_conflict:server_command.append('--arp-conflict')
 if args.retry_after_decline:server_command.append('--retry-after-decline')
 if args.library:server_command.extend(['--lease-seconds','600'])
 server=launch(server_command,'server')
 ready(out/'server.stdout',server)
 print('ISOLATED_DHCP_READY',flush=True)
 cmd='AZHUB_DHCP_EVENT_FD=3 AZHUB_DHCP_INTERFACE=azpub0 ip netns exec '+ns+' /usr/sbin/udhcpc -B -f -n -q -t 3 -T 2 -s '+hook+' -i azpub0 3>&1'
 if args.acquirer:cmd=acquirer+(' --emulator-no-offer' if args.no_offer else ' --emulator')+' /var/run/netns/'+ns+' '+hook
 if args.network_helper:cmd+=' '+network_helper
 if args.allocate_network:cmd=acquirer+' --emulator '+hook+' '+network_helper
 if args.link_local:cmd+=' --link-local'
 elif args.allocated_ready:cmd+=(' --broker-library' if args.broker_allocate else ' --library' if args.library else ' --ready')
 if args.address_flow:cmd+=(' --address-library' if args.library else ' --address-retry' if args.retry_after_decline else ' --address-conflict' if args.arp_conflict else ' --address-flow')
 if args.library:
  cmd='AZHUB_ADDRESS_LIBRARY_CONTROL='+control+' '+cmd
  acquisition=launch(az._ssh_base()+[cmd],'acquisition')
  deadline=time.monotonic()+45
  while True:
   status=guest('cat '+control+'.ready',check=False)
   if not status.returncode:
    published=json.loads(status.stdout)
    assert published['ready'] and published['address']=='192.0.2.3' and published['player']==41
    assert 1024<published['metadata_port']<65536
    (out/'library-ready.json').write_text(status.stdout);break
   assert acquisition.poll() is None,('Acquisition ended before library readiness',(out/'acquisition.stderr').read_text())
   assert time.monotonic()<deadline,'Leased library readiness timeout'
   time.sleep(.2)
  print('LEASED_LIBRARY_READY',flush=True)
  classpath=base+'/'+args.classes.resolve().relative_to(root).as_posix()+':'+base+'/local/link-peer-build/dependencies/*'
  prefix=['wsl.exe','-u','root','--exec','ip','netns','exec','azhub-link','java','-cp',classpath]
  observers=[('discovery_and_menus',launch(prefix+['LiveProviderControl',base+'/local/diagnostics/sharing-hub0-export.pdb',
                  base+'/local/diagnostics/sharing-pdb-tables-v1/java-entities.tsv'],'discovery-and-menus'))]
  import hashlib
  for name,relative,fixture in [('database','PIONEER/rekordbox/export.pdb','sharing-hub0-export.pdb'),
       ('audio','Contents/Aaliyah/Are You That Somebody/Aaliyah - Are You That Somebody-1.mp3','sharing-hub0-audio.mp3')]:
   local=root/'local/diagnostics'/fixture
   observers.append((name,launch(prefix+['LocalFileServiceClient',relative,str(local.stat().st_size),
       hashlib.sha256(local.read_bytes()).hexdigest(),'--emulator-link-1'],name)))
  for name,child in observers:
   child.wait(timeout=150);assert child.returncode==0,(name,child.returncode)
   print('LEASED_'+name.upper()+'_PASS',flush=True)
  guest(': > '+control+'.done')
  acquisition.wait(timeout=15);assert acquisition.returncode==0,(out/'acquisition.stderr').read_text()
  result=subprocess.CompletedProcess(cmd,0,(out/'acquisition.stdout').read_text(),(out/'acquisition.stderr').read_text())
  commands.append({'command':cmd,'exit':0,'stdout':result.stdout,'stderr':result.stderr})
 else:result=guest(cmd,timeout=40 if args.retry_after_decline else 20)
 (out/'client-events.txt').write_text(result.stdout)
 expected='1\tdeconfig\tazpub0\t-\t-\t-\t-\n1\tbound\tazpub0\t192.0.2.3\t255.255.255.0\t60\t192.0.2.2\n'
 if args.link_local:
  grant=json.loads(result.stdout)
  assert grant['native_no_offer'] and grant['IPv4LL_configured_and_retired'] and grant['direct_UDP_roundtrip'] and grant['no_default_route']
  assert '"link_local_UDP_roundtrip": true' in (out/'server.stdout').read_text()
  assert '"reply"' not in (out/'server.stdout').read_text()
 elif args.no_offer:
  grant=json.loads(result.stdout)
  assert grant['native_no_offer'] and grant['deconfig_then_leasefail'] and grant['child_reaped'] and grant['pipe_EOF'] and not grant['address_applied']
  assert '"DISCOVER_waiting": true' in (out/'server.stdout').read_text()
  assert '"reply"' not in (out/'server.stdout').read_text()
 elif args.allocate_network:
  grant=json.loads(result.stdout)
  assert grant['allocated_namespace_interface_native_DHCP'] and grant['address']=='192.0.2.3'
  assert grant['not_public_ready']==(not args.allocated_ready) and grant['configured_and_retired']==args.allocated_ready
  assert grant['allocated_library_served']==args.library
  if args.broker_allocate:assert grant['broker_allocate_callbacks']
 elif args.acquirer:
  grant=json.loads(result.stdout)
  assert grant['supervised_grant'] and grant['address']==('192.0.2.4' if args.retry_after_decline else '192.0.2.3') and grant['server']=='192.0.2.2'
  seconds=600 if args.library else 60
  assert grant['lease_seconds']==seconds and grant['expires_ms']==grant['started_ms']+seconds*1000
  assert grant['started_ms']<=grant['completed_ms']<grant['expires_ms']
  assert grant['lease_candidate_not_ready']
  assert grant['address_flow_applied_then_removed']==(args.address_flow and (not args.arp_conflict or args.retry_after_decline))
  assert grant['arp_conflict_before_configuration']==args.arp_conflict
  assert grant['reacquired_after_decline']==args.retry_after_decline
  if args.retry_after_decline:assert grant['post_decline_wait_ms']>=10000
  assert grant['address_library_served']==args.library
 else:assert result.stdout==expected,result.stdout
 if not args.no_offer:
  server.wait(timeout=5);assert server.returncode==0
 if not args.allocate_network:
  addresses=json.loads(guest('ip -n '+ns+' -j address show dev azpub0').stdout)
  assert not any(a.get('family')=='inet' for row in addresses for a in row.get('addr_info',[]))
  assert not guest('ip netns pids '+ns).stdout.strip()
 else:assert not guest('ip netns list').stdout.strip()
 after=native_state();assert before==after,(before,after)
 (out/'result.json').write_text(json.dumps({'native_DHCP_OFFER_ACK':not (args.no_offer or args.link_local),'native_DHCP_no_offer':args.no_offer or args.link_local,'IPv4LL_configured_and_retired':args.link_local,'reporting_only_bound_event':not (args.no_offer or args.link_local),
       'proposal_not_applied':not (args.address_flow or args.allocated_ready) or (args.arp_conflict and not args.retry_after_decline),
       'address_flow_applied_then_removed':args.allocated_ready or (args.address_flow and (not args.arp_conflict or args.retry_after_decline)),
       'arp_conflict_before_configuration':args.arp_conflict,
       'reacquired_after_decline':args.retry_after_decline,
       'standalone_leased_library':args.library,
       'native_state_unchanged':True,'supervised_acquisition':bool(args.acquirer),
       'standalone_helper':bool(args.network_helper),'broker_allocate_callbacks':args.broker_allocate,'physical_AZ_touched':False},indent=2))
 print('LIVE_LINK_LOCAL_PASS' if args.link_local else 'LIVE_DHCP_NO_OFFER_PASS' if args.no_offer else 'LIVE_DHCP_OFFER_ACK_PASS',flush=True)
 if args.arp_conflict:print('ARP_CONFLICT_REJECT_PASS',flush=True)
 if args.retry_after_decline:print('POST_DECLINE_REACQUISITION_PASS',flush=True)
 if args.library:print('LEASED_LIBRARY_AND_RETIREMENT_PASS',flush=True)
 if not (args.no_offer or args.link_local):
  bridge.wait(timeout=220 if args.library else 70);assert bridge.returncode==0
finally:
 if acquisition and acquisition.poll() is None:
  guest(': > '+control+'.done',check=False)
  try:acquisition.wait(timeout=15)
  except subprocess.TimeoutExpired:pass
 if scope:
  for pid in guest('ip netns pids '+ns).stdout.split():
   assert pid.isdigit()
   name=guest('cat /proc/'+pid+'/cmdline').stdout.split('\0')[0]
   assert name in ('/usr/sbin/udhcpc',acquirer,network_helper,'/sbin/ip'),('Unexpected owned-namespace process',pid,name)
   guest('kill -TERM '+pid)
  if link and moved:guest('ip -n '+ns+' link del azpub0');link=False
  guest('ip netns del '+ns)
 if bridge and bridge.poll() is None:
  e.qmp('object-del',{'id':'azhubtapguard'});bridge.terminate();bridge.wait(timeout=10)
  e.qmp('chardev-remove',{'id':'azhubtapnat'})
 if host_scope:
  for pid in host('ip','netns','pids','azhub-link').stdout.split():
   assert pid.isdigit();host('kill','-TERM',pid,check=False)
  for child in children:
   if child.poll() is None:child.wait(timeout=10)
  host('ip','netns','del','azhub-link')
 if link:guest('ip link del azpub0')
 if module:guest('rmmod azhub_test_macvlan')
 guest('rm -f '+hook+' '+hook+'.new '+remote_module+' '+remote_module+'.new '+acquirer+' '+acquirer+'.new '+network_helper+' '+network_helper+'.new '+control+'.ready '+control+'.done')
 (out/'guest-commands.json').write_text(json.dumps(commands,indent=2))
 for handle in handles:handle.close()
 print('OWNED_SCOPE_CLEANED',flush=True)

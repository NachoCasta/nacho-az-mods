#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Explicit localhost emulator SDK launch/IPC control with exact rollback.

Requires empty native decks (check the emulator UI first). Temporarily installs
an explicit probe mod/helper; never connects to or changes a physical AZ.
Default mode only acknowledges a namespace. Explicit --service-module prepares
a temporary service fixture; --allocate-service delegates guest network/DHCP/ARP
ownership to the helper and supplies only the isolated host DHCP/client fixture.
"""
from pathlib import Path
import argparse,hashlib,importlib.util,json,os,re,subprocess,time,uuid,sys,struct

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--mod',type=Path,required=True)
p.add_argument('--helper',type=Path,required=True)
p.add_argument('--evidence-directory',type=Path,required=True)
p.add_argument('--service-module',type=Path,help='Explicit service-mode control with temporary emulator network and synthetic library')
p.add_argument('--library-root',help='Explicit mounted emulator fixture root, exported through a temporary bind alias')
p.add_argument('--classes',type=Path,help='Independent prepared Java client classes, required with --library-root')
p.add_argument('--allocate-service',action='store_true',help='Helper owns network/DHCP/ARP through ALLOCATE; runner supplies no guest network')
p.add_argument('--cancel-dhcp',action='store_true',help='Withhold OFFER and drop IPC only after observing the pending native DHCP client')
p.add_argument('--arp-conflict',action='store_true',help='Inject initial address conflict and verify DECLINE/no publication through the managed helper')
p.add_argument('--retry-conflict',action='store_true',help='Recover after DECLINE with retained backoff and the same public identity')
p.add_argument('--inventory-gate',action='store_true',help='Observe the matching native source and gate its unmount on managed retirement')
p.add_argument('--eject-selected',action='store_true',help='Use the native selector to eject the exported fixture after independent clients complete')
p.add_argument('--reconnect-selected',action='store_true',help='Replug the owned virtual stick and verify new publication with stale old handles')
p.add_argument('--capacity-source-count',type=int,choices=(5,8),help='Complete integrated clients/files for every prepared native hub source; explicit capacity control')
p.add_argument('--integrated-whole-hub',action='store_true',help='After complete eight-source clients, native long STOP withdraws all hub exports and mounts while preserving native USB reads')
p.add_argument('--whole-hub-withdrawal-only',action='store_true',help='Separate bounded whole-hub lifecycle control; skip already-separate full menu/file capacity workload, retain real MP3/status and eight-source sessions')
p.add_argument('--whole-hub-native-admission',action='store_true',help='Require diagnostic native-store admission of the paired loaded track before whole-hub eject')
p.add_argument('--native-file-control',action='store_true',help='Independent complete native USB1 PDB/audio and native USB2 mod-file reads alongside integrated hub clients')
p.add_argument('--native-local-playback',action='store_true',help='Fresh selected native USB UI load and advancing local engine while native/hub independent file clients run')
p.add_argument('--native-local-usb',type=int,choices=(1,2),default=1,help='Native local playback fixture port; USB2 requires --native-local-playback')
p.add_argument('--two-sources',action='store_true',help='Use existing matching native roots in slots0/4 and preserve the neighbor across eject/replug')
p.add_argument('--topology-source-row',type=int,choices=range(-1,8),help='Explicit previously reviewed emulator source row; each load still proves its physical backing')
p.add_argument('--local-deck-topology',action='store_true',help='Native two-On/four-Off/two-On loaded engine control alongside integrated clients')
p.add_argument('--local-playback',action='store_true',help='Keep a native hub deck looping while external clients browse/fetch and eject/replug its source')
p.add_argument('--reject-duplicate-grant',action='store_true',help='Reject a duplicate second-source lease while preserving the original source')
p.add_argument('--reject-native-grant',action='store_true',help='Offer the emulator native IPv4 address and require rejection before publication')
p.add_argument('--refresh-grants',action='store_true',help='Use60-second grants and verify real same-generation refresh while clients stay active')
p.add_argument('--eject-pending-refresh',action='store_true',help='Eject slot0 while a withheld refresh client is live; preserve slot4 and replug')
p.add_argument('--change-refresh-address',action='store_true',help='Change slot0 DHCP address and verify retirement/recovery with the original neighbor session')
p.add_argument('--player-collision',action='store_true',help='Inject a foreign player41 claim after full clients; require retirement without identity retry')
p.add_argument('--recover-player-collision',action='store_true',help='After confirmed player collision retirement, select a free fixture number and revalidate the library')
p.add_argument('--dynamic-identity',action='store_true',help='Use controller-derived source MACs with the separate dynamic-policy helper')
p.add_argument('--native-inventory',action='store_true',help='Select source roots from the existing native mod inventory instead of fixture disk declarations')
p.add_argument('--inventory-engine',action='store_true',help='Use the reusable source0-independent inventory through SDK launch/IPC')
p.add_argument('--candidate-runtime',action='store_true',help='Use candidate subnet admission, network monitoring, random ACD timing and SDK-owned lifetime in the emulator')
p.add_argument('--native-network-change',action='store_true',help='Add/remove an owned secondary emulator native address and verify helper-initiated complete retirement')
p.add_argument('--recover-native-network',action='store_true',help='After graceful network retirement, re-launch with a fresh SDK session and revalidate both libraries')
p.add_argument('--native-ipv4-label',action='store_true',help='Label the owned emulator IPv4LL address eth0:avahi to reproduce native address enumeration')
p.add_argument('--link-local',action='store_true',help='Withhold DHCP and validate two managed IPv4LL libraries using the isolated link-local peer')
p.add_argument('--link-local-conflict',action='store_true',help='Inject a managed IPv4LL conflict and verify new-address recovery with original neighbor sessions')
p.add_argument('--link-local-eject',action='store_true',help='Use native eject/replug with link-local withdrawal observer and fresh independent clients')
p.add_argument('--link-local-dhcp-return',action='store_true',help='Restore DHCP only for slot0, verify IPv4LL retirement and fresh routed library')
p.add_argument('--integrated-runtime',action='store_true',help='Run the automatic controller without SDK probe variables, masks or result markers')
p.add_argument('--integrated-loaded-eject',action='store_true',help='Integrated eject/replug with real MP3 handle and continuing synthetic external loaded status')
p.add_argument('--loaded-status-wire-only',action='store_true',help='Explicit non-diagnostic loaded-eject control: require wire status, do not claim native status-store admission')
p.add_argument('--integrated-eject',action='store_true',help='Exercise native eject/replug and original neighbor sessions using the integrated controller')
p.add_argument('--verify-log-retention',action='store_true',help='Seed old owned emulator test log families before mode cycle and require their removal on restart')
p.add_argument('--integrated-network-recovery',action='store_true',help='After integrated graceful network retirement, verify fresh automatic helper/session and complete independent libraries')
p.add_argument('--integrated-mode-cycle',action='store_true',help='Use native Utility UI for On/Off/On, external withdrawal and fresh-session complete clients')
p.add_argument('--native-status-eject',action='store_true',help='Explicit native loaded-source STOP1/STALE/replug baseline in emulator only')
p.add_argument('--native-status-held-file',action='store_true',help='Pair native status observer with real held NFS reads; no eject')
p.add_argument('--native-status-observer',action='store_true',help='Run the explicit read-only native status-store diagnostic and synthetic peer, with no sharing helper')
args=p.parse_args();root=Path.cwd().resolve();out=args.evidence_directory.resolve()
source_slots=list(range(args.capacity_source_count)) if args.capacity_source_count else [0,4]
fixture_slot_text=','.join(map(str,source_slots))
if args.native_status_eject:assert args.native_status_held_file
if args.native_status_held_file:assert args.native_status_observer
if args.native_status_observer:assert args.classes and not (args.service_module or args.library_root or args.integrated_runtime or args.allocate_service)
if args.topology_source_row is not None:assert args.local_deck_topology
if args.native_file_control:assert args.integrated_runtime and not (args.local_deck_topology or args.integrated_eject or args.integrated_mode_cycle)
if args.native_local_playback:assert args.native_file_control
if args.native_local_usb==2:assert args.native_local_playback
if args.integrated_whole_hub:assert args.capacity_source_count==8 and args.native_file_control
if args.whole_hub_withdrawal_only:assert args.integrated_whole_hub and not (args.native_local_playback or args.refresh_grants)
if args.whole_hub_native_admission:assert args.integrated_whole_hub and args.mod.name.endswith('sharing-runtime-observer-emulator.so')
if args.capacity_source_count:assert args.integrated_runtime and args.two_sources and not (args.local_deck_topology or args.integrated_eject or args.integrated_mode_cycle or args.local_playback)
if args.local_deck_topology:assert args.integrated_mode_cycle and not args.local_playback
if args.verify_log_retention:assert args.integrated_mode_cycle
if args.integrated_loaded_eject:assert args.integrated_eject and not args.local_playback
if args.loaded_status_wire_only:assert args.integrated_loaded_eject
if args.integrated_network_recovery:assert args.integrated_runtime and not (args.integrated_eject or args.integrated_mode_cycle or args.capacity_source_count or args.local_playback)
if args.integrated_mode_cycle:assert args.integrated_runtime and not args.integrated_eject
if args.integrated_eject:assert args.integrated_runtime and args.reconnect_selected
if args.integrated_runtime:
 assert args.candidate_runtime and args.two_sources and not (args.link_local or args.player_collision or args.native_network_change or args.change_refresh_address or args.eject_pending_refresh)
 if args.local_playback:assert args.integrated_eject and not args.integrated_mode_cycle
if args.library_root:assert args.service_module and args.classes and re.fullmatch(r'/media/usb/sd[a-z]+1',args.library_root)
if args.allocate_service:assert args.library_root and args.service_module
if args.cancel_dhcp:assert args.allocate_service
if args.arp_conflict:assert args.allocate_service and not args.cancel_dhcp
if args.retry_conflict:assert args.arp_conflict and args.library_root
if args.inventory_gate:assert args.allocate_service and not (args.arp_conflict or args.cancel_dhcp)
if args.eject_selected:assert args.inventory_gate
if args.reconnect_selected:assert args.eject_selected
if args.two_sources:assert args.inventory_gate and (args.reconnect_selected or args.reject_duplicate_grant)
if args.reject_duplicate_grant:assert args.two_sources and not (args.eject_selected or args.local_playback or args.arp_conflict)
if args.local_playback:assert args.inventory_gate and args.eject_selected
if args.link_local_dhcp_return:assert args.link_local and not (args.link_local_conflict or args.link_local_eject)
if args.link_local_eject:assert args.link_local and args.reconnect_selected and not args.link_local_conflict
if args.link_local_conflict:assert args.link_local
if args.native_ipv4_label:assert args.link_local
if args.link_local:assert args.candidate_runtime and not (args.native_network_change or args.player_collision or args.local_playback or args.change_refresh_address or args.eject_pending_refresh)
if args.recover_native_network:assert args.native_network_change
if args.native_network_change:assert args.candidate_runtime and not (args.player_collision or args.local_playback or args.change_refresh_address)
if args.candidate_runtime:assert args.inventory_engine and args.helper.name.endswith("candidate-emulator")
if args.inventory_engine:assert args.native_inventory
if args.native_inventory:assert args.dynamic_identity
if args.dynamic_identity:assert args.two_sources and (args.refresh_grants or args.whole_hub_withdrawal_only) and not (args.player_collision or args.reject_duplicate_grant or args.eject_pending_refresh)
if args.recover_player_collision:assert args.player_collision
if args.player_collision:assert args.two_sources and not (args.change_refresh_address or args.eject_pending_refresh or args.local_playback)
if args.change_refresh_address:assert args.refresh_grants and args.two_sources and not (args.eject_pending_refresh or args.local_playback)
if args.eject_pending_refresh:assert args.refresh_grants and args.reconnect_selected and not args.local_playback
if args.refresh_grants:assert args.two_sources and not (args.reject_duplicate_grant or args.reject_native_grant)
if args.reject_native_grant:assert args.allocate_service and not (args.two_sources or args.inventory_gate or args.arp_conflict or args.cancel_dhcp)
assert os.name=='nt' and out.is_relative_to(root/'local/diagnostics') and not out.exists()
for file in (args.mod,args.helper):assert file.resolve().is_relative_to(root) and file.is_file()
out.mkdir(parents=True)
s=importlib.util.spec_from_file_location('e',root/'src/azhub/research/scripts/eject-ui-emu.py')
e=importlib.util.module_from_spec(s);s.loader.exec_module(e);az=e.azemu
assert az.SSH_PORT==2222 and az.QMP==('127.0.0.1',4446)
commands=[];namespace=changed=module=fixture=link=bound=host_scope=False;token=uuid.uuid4().hex
bridge=None;children=[];handles=[];secondary_address=False;native_link_local=False
native_mode_off=False
topology=None
native_playback=None
pending_clients=[];playback_samples=[];local_suite=None;playback_pid=None;playback_content_id=None
remote='/tmp/azhub-sdk-'+token;backups=remote+'-rollback'
unit='EP147-sdk-probe-'+token
result_file='/run/azhub-sdk-probe-'+token+'.json'
restore_files=[];hashes=[];helper_log=None;probe_marker=None
ready_file='/run/azhub-sdk-service-'+token+'.ready';done_file='/run/azhub-sdk-service-'+token+'.done'
base='/mnt/c/'+root.as_posix()[3:]
if args.integrated_loaded_eject or args.integrated_whole_hub or args.native_status_held_file:
 verification_classpath=base+'/'+args.classes.resolve().relative_to(root).as_posix()+':'+base+'/local/link-peer-build/dependencies/*'
 verification=subprocess.run(['wsl.exe','--exec','java','-cp',verification_classpath,'LoadedDeckStatusControl','--verify-fixture',base+'/local/diagnostics/sharing-hub0-export.pdb'],capture_output=True,text=True,timeout=30)
 (out/'loaded-track-binding.log').write_text(verification.stdout+verification.stderr)
 assert verification.returncode==0,verification.stdout+verification.stderr

def host(*cmd,check=True):
 r=subprocess.run(['wsl.exe','-u','root','--exec',*cmd],capture_output=True,text=True,timeout=20)
 if check:assert r.returncode==0,(cmd,r.stderr)
 return r

def launch(cmd,name,interactive=False):
 stdout=(out/(name+'.stdout')).open('wb');stderr=(out/(name+'.stderr')).open('wb');handles.extend((stdout,stderr))
 child=subprocess.Popen(cmd,stdin=subprocess.PIPE if interactive else subprocess.DEVNULL,stdout=stdout,stderr=stderr);children.append(child);return child
def guest(cmd,check=True,timeout=20):
 r=subprocess.run(az._ssh_base()+[cmd],capture_output=True,text=True,timeout=timeout)
 commands.append({'command':cmd,'exit':r.returncode,'stdout':r.stdout,'stderr':r.stderr})
 if check:assert r.returncode==0,(cmd,r.stdout,r.stderr)
 return r
def persisted_link_mode():
 # Parse only the named setting in memory; never log/copy the complete settings file.
 result=subprocess.run(az._ssh_base()+['od -v -b /home/root/settings/SETTINGS.DAT'],capture_output=True,text=True,timeout=20)
 assert result.returncode==0,result.stderr
 data=bytes(int(byte,8) for line in result.stdout.splitlines() for byte in line.split()[1:])
 key=b'ProDjLinkMode\0';assert data.count(key)==1
 at=data.index(key)+len(key);assert data[at:at+3]==bytes([1,5,1])
 mode=int.from_bytes(data[at+3:at+7],'little');assert mode in (0,1)
 return mode

def native_link_choice(mode):
 assert mode in (0,1)
 if persisted_link_mode()==mode:return
 drive=[sys.executable,'C:/Users/icast/xdj-az-emu/tools/az-drive.py']
 touch=[sys.executable,str(root/'src/azhub/research/scripts/eject-ui-emu.py'),'tap']
 subprocess.run(drive+['source'],check=True)
 subprocess.run(drive+['panel','menu'],check=True)
 subprocess.run(touch+['700','1295'],check=True)
 subprocess.run(touch+['1700','296' if mode else '196'],check=True)
 subprocess.run(drive+['shot','native-link-'+str(mode)+'-confirmation'],env=dict(os.environ,AZ_SHOT_DIR=str(out)),check=True)
 subprocess.run(touch+['1080','998'],check=True)
 time.sleep(1)
 subprocess.run(drive+['source'],check=True)
 limit=time.monotonic()+15
 while persisted_link_mode()!=mode:
  assert time.monotonic()<limit,'Native Utility did not persist the requested Link mode';time.sleep(.2)

def application_pid():
 main=guest('systemctl show '+unit+' -p MainPID').stdout.strip()
 assert main.startswith('MainPID=') and main[8:].isdigit()
 parent=main[8:]
 if guest('cat /proc/'+parent+'/comm').stdout.strip()=='EP147':return parent
 pid=guest('pgrep -P '+parent+' -x EP147').stdout.strip();assert pid.isdigit(),pid
 assert guest('readlink /proc/'+pid+'/exe').stdout.strip().endswith('/EP147')
 return pid

def probe_events(completion=None):
 global probe_marker,helper_log
 logs=guest('cat /tmp/azmod-community/usb-hub-support.log.1 /tmp/azmod-community/usb-hub-support.log 2>/dev/null',check=False).stdout
 if args.native_status_observer:
  if probe_marker is None:
   candidate_marker='native-status-observer installed pid='+application_pid()+';'
   assert candidate_marker in logs
   probe_marker=candidate_marker
  assert probe_marker in logs
  return logs.rsplit(probe_marker,1)[1]
 if args.integrated_runtime:
  if probe_marker is None:
   launches=list(re.finditer(r'public-sharing managed launch pid=(\d+)',logs));assert launches
   candidate_marker=launches[-1].group(0)
   command=guest('cat /proc/'+launches[-1].group(1)+'/cmdline').stdout.split('\0')
   assert len(command)>=5 and command[0]==asset and command[1]=='--broker-candidate-emulator'
   assert re.fullmatch(r'/run/azhub-share-[0-9]+-[0-9a-f]{16}/control.sock',command[2])
   assert re.fullmatch(r'[0-9a-f]{32}',command[3]) and command[4]==application_pid()
   helper_log=owners[-1]+'/data/public-sharing-'+command[3]+'.log'
   probe_marker=candidate_marker
  assert probe_marker in logs
  return logs.rsplit(probe_marker,1)[1]
 if probe_marker is None:
  if completion is not None:
   assert completion['run']==token and completion['managed_pid']>0 and completion['peer_pid']>0
   probe_marker='sdk-broker-probe managed launch pid='+str(completion['managed_pid'])
   assert probe_marker in logs
   current=logs.rsplit(probe_marker,1)[1]
   assert 'accepted helper pid='+str(completion['peer_pid'])+' ' in current
  else:
   launches=list(re.finditer(r'sdk-broker-probe managed launch pid=(\d+)',logs));assert launches
   candidate_marker=launches[-1].group(0)
   current=logs.rsplit(candidate_marker,1)[1]
   peer=re.search(r'accepted helper pid=(\d+)',current);assert peer
   command=guest('cat /proc/'+peer.group(1)+'/cmdline').stdout
   assert '/assets/usb-hub-support-network-helper.so' in command and '-'+token[:8]+'/control.sock' in command
   probe_marker=candidate_marker
 assert probe_marker in logs,'Current probe marker rotated beyond retained logs'
 return logs.rsplit(probe_marker,1)[1]
try:
 assert guest('uname -r').stdout.strip()=='6.6.138-dirty'
 assert guest('systemctl is-active EP147').stdout.strip()=='active'
 if args.integrated_mode_cycle:assert persisted_link_mode()==1,'Mode-cycle control requires native Link On initially'
 loaded=guest('cat /tmp/azmod-community/loaded').stdout
 assert 'suspended=0' in loaded and 'mod=usb-hub-support|1|' in loaded,'Resume community mods through the framework UI before testing'
 if args.integrated_eject and args.library_root:
  device=args.library_root.rsplit('/',1)[1]
  physical=guest('readlink -f /sys/class/block/'+device).stdout.strip()
  assert '/3-2.1/' in physical, 'Selected library root must match protected source0 port3-2.1; Linux sdX names can change after hotplug: '+physical

 active=guest('systemctl show EP147 -p ActiveEnterTimestampMonotonic').stdout.strip()
 assert re.fullmatch(r'ActiveEnterTimestampMonotonic=[0-9]+',active)
 stable_after=int(active.split('=')[1])/1_000_000+150
 if float(guest('cat /proc/uptime').stdout.split()[0])<stable_after:
  print('WAITING_FOR_STABLE_NORMAL_STARTUP',flush=True)
 while float(guest('cat /proc/uptime').stdout.split()[0])<stable_after:time.sleep(1)
 assert guest('systemctl show EP147 -p ActiveEnterTimestampMonotonic').stdout.strip()==active
 (out/'normal-startup-stability.json').write_text(json.dumps({'active_enter_monotonic':active,'minimum_service_age_seconds':150,'framework_suspended':False},indent=2))
 native_before={key:guest(cmd).stdout for key,cmd in [('address','ip -o -4 addr show dev eth0'),('routes','ip -4 route show'),('dns','sha256sum /etc/resolv.conf'),('namespace','ls -Lid /proc/self/ns/net'),('parent_flags','cat /sys/class/net/eth0/flags')]}
 (out/'native-before.json').write_text(json.dumps(native_before,indent=2))
 if args.link_local:
  assert '169.254.2.15/' not in guest('ip -o -4 addr show dev eth0').stdout
  guest('ip -4 addr add 169.254.2.15/16 brd 169.254.255.255 dev eth0'+(' label eth0:avahi' if args.native_ipv4_label else ''));native_link_local=True
 dynamic_parent=guest('cat /sys/class/net/eth0/address').stdout.strip() if args.dynamic_identity else None
 if dynamic_parent:assert re.fullmatch(r'(?:[0-9a-f]{2}:){5}[0-9a-f]{2}',dynamic_parent)
 assert not guest('ip netns list').stdout.strip()
 if args.service_module:
  assert 'azhub_test_macvlan' not in guest('cat /proc/modules').stdout
  assert guest('test ! -e /tmp/azhub-sdk-library').returncode==0
  assert hashlib.sha256(args.service_module.read_bytes()).hexdigest()=='d125c73ce850c91a8485f32a4ef52ea318f3fd5b1d7eb712f18c510654e268ad'
 assert guest('test ! -e '+result_file).returncode==0
 owners=re.findall(r'launch (/media/usb/sd[a-z]+[0-9]+/MODS/community-mods/usb-hub-support)/assets/mount-hub\.sh:',
                   guest('cat /tmp/azmod-community/usb-hub-support.log').stdout)
 if not owners:
  candidates=guest('for path in /media/usb/sd*1/MODS/community-mods/usb-hub-support; do [ -f "$path/assets/mount-hub.sh" ] && [ -f "$path/usb-hub-support.so" ] && printf "%s\\n" "$path"; done',check=False).stdout.splitlines()
  assert len(candidates)==1 and re.fullmatch(r'/media/usb/sd[a-z]+1/MODS/community-mods/usb-hub-support',candidates[0]),candidates
  owners=candidates
 assert owners
 helper_log=owners[-1]+'/data/sdk-broker-'+token+'.log'
 assert guest('test ! -e '+helper_log).returncode==0
 package=owners[-1]+'/usb-hub-support.so'
 stage='/home/root/settings/az-usbload/community/usb-hub-support/usb-hub-support.so'
 approved='/home/root/settings/azmod-community/approved'
 starting='/home/root/settings/azmod-community/.starting'
 asset=owners[-1]+'/assets/usb-hub-support-network-helper.so'
 assert guest('test ! -e '+asset).returncode==0,'Existing helper requires a separately reviewed rollback'
 restore_files=[package,stage,approved,starting,asset]
 if args.allocate_service:
  hook_asset=owners[-1]+'/assets/public-dhcp-event-hook.so'
  assert guest('test ! -e '+hook_asset).returncode==0
  restore_files.append(hook_asset)
 hashes=[guest('if test -e '+path+'; then sha256sum '+path+'; else echo ABSENT; fi').stdout.split()[0] for path in restore_files]
 assert hashes[0]==hashes[1]=='a3d6c7ba438494008edf66fdda43d4be5ae702ed3afa4cdcb57791b9964d1add'
 assert hashes[2]!='ABSENT'
 guest('mkdir '+backups)
 for i,path in enumerate(restore_files):
  if hashes[i]!='ABSENT':guest('cp '+path+' '+backups+'/'+str(i))
 (out/'original-files.json').write_text(json.dumps({'paths':restore_files,'sha256':hashes},indent=2))
 assert az.push_file(str(args.mod.resolve()),remote+'.so')
 assert az.push_file(str(args.helper.resolve()),remote+'-helper')
 if not args.allocate_service and not args.native_status_observer:
  guest('ip netns add azhub-sdk-probe');namespace=True
 if args.service_module:
  assert az.push_file(str(args.service_module.resolve()),remote+'.ko')
  assert guest('sha256sum '+remote+'.ko').stdout.split()[0]==hashlib.sha256(args.service_module.read_bytes()).hexdigest()
  guest('insmod '+remote+'.ko');module=True
  if not args.allocate_service:
   guest('ip link add link eth0 name azpub7 address 02:00:00:00:12:73 type macvlan mode bridge');link=True
   guest('ip link set azpub7 netns azhub-sdk-probe');link=False
   guest('ip netns exec azhub-sdk-probe ip link set azpub7 up')
   guest('ip netns exec azhub-sdk-probe ip addr add 192.0.2.3/24 broadcast 192.0.2.255 dev azpub7')
  if not args.inventory_gate:
   guest('mkdir /tmp/azhub-sdk-library');fixture=True
  if args.library_root:
   expected=hashlib.sha256((root/'local/diagnostics/sharing-hub0-export.pdb').read_bytes()).hexdigest()
   assert guest('sha256sum '+args.library_root+'/PIONEER/rekordbox/export.pdb').stdout.split()[0]==expected
   if args.two_sources:assert guest('sha256sum /media/usb/sdg1/PIONEER/rekordbox/export.pdb').stdout.split()[0]==expected
   if args.inventory_gate:assert args.library_root=='/media/usb/sdc1' or args.integrated_eject
   else:
    guest('mount --bind '+args.library_root+' /tmp/azhub-sdk-library');bound=True
   assert not host('ip','netns','list').stdout.strip()
   host('ip','netns','add','azhub-link');host_scope=True
   host('ip','netns','exec','azhub-link','ip','tuntap','add','dev','azlink0','mode','tap')
   host('ip','-n','azhub-link','link','set','lo','up')
   host('ip','-n','azhub-link','addr','add','192.0.2.2/24','brd','192.0.2.255','dev','azlink0')
   host('ip','-n','azhub-link','link','set','azlink0','address','02:00:00:00:02:02')
   host('ip','-n','azhub-link','link','set','azlink0','up')
   if args.link_local:host('ip','-n','azhub-link','addr','add','169.254.2.2/16','brd','169.254.255.255','dev','azlink0')
   bridge=launch([sys.executable,str(root/'src/azhub/research/scripts/link-primary-tap-transport.py'),'--seconds','1200' if args.local_deck_topology else '600' if args.reconnect_selected else '240']+(['--fixture-link-local'] if args.link_local else [])+(['--controller-mac',dynamic_parent] if dynamic_parent else [])+(['--fixture-dhcp']+(['--fixture-dhcp-slots',fixture_slot_text] if args.two_sources else []) if args.allocate_service else []),'transport')
   limit=time.monotonic()+15
   while '"ready": true' not in (out/'transport.stdout').read_text():
    assert bridge.poll() is None and time.monotonic()<limit;time.sleep(.1)
   if args.allocate_service:
    server=launch(['wsl.exe','-u','root','--exec','ip','netns','exec','azhub-link','python3',base+'/src/azhub/research/scripts/dhcp-fixture-server.py','--duration-seconds','1200' if args.local_deck_topology else '600','--lease-seconds','60' if args.refresh_grants else '600']+(['--controller-mac',dynamic_parent] if dynamic_parent else [])+(['--refresh'] if args.refresh_grants else [])+(['--withhold-all'] if args.link_local else [])+(['--link-local-return-marker',base+'/'+(out/'return-dhcp').relative_to(root).as_posix()] if args.link_local_dhcp_return else [])+(['--change-refresh-marker',base+'/'+(out/'change-refresh').relative_to(root).as_posix()] if args.change_refresh_address else [])+(['--withhold-refresh-after','4','--resume-refresh-marker',base+'/'+(out/'resume-refresh').relative_to(root).as_posix()] if args.eject_pending_refresh else [])+(['--native-address'] if args.reject_native_grant else ['--source-slots','0,4','--duplicate-slot','4'] if args.reject_duplicate_grant else (['--source-slots',fixture_slot_text,'--reconnect'] if args.two_sources else ['--reconnect']) if args.reconnect_selected else ['--withhold-offer'] if args.cancel_dhcp else (['--address-flow','--arp-conflict']+(['--retry-after-decline','--no-echo'] if args.retry_conflict else [])) if args.arp_conflict else []),'dhcp-server')
    limit=time.monotonic()+15
    while '"ready": true' not in (out/'dhcp-server.stdout').read_text():
     assert server.poll() is None and time.monotonic()<limit;time.sleep(.1)
  else:
   guest('mkdir -p /tmp/azhub-sdk-library/PIONEER/rekordbox')
   pdb=bytearray(1024)
   for offset,value in ((5,2),(8,1),(36,1),(40,1),(516,1)):pdb[offset]=value
   local_pdb=out/'synthetic.pdb';local_pdb.write_bytes(pdb)
   assert az.push_file(str(local_pdb),'/tmp/azhub-sdk-library/PIONEER/rekordbox/export.pdb')
 expected_inode=0 if args.native_status_observer else int(guest('ls -Lid '+(args.library_root if args.inventory_gate else '/tmp/azhub-sdk-library' if args.allocate_service else '/var/run/netns/azhub-sdk-probe')).stdout.split()[0])
 guest('systemctl stop EP147');changed=True
 guest('cp '+remote+'.so '+package+' && cp '+remote+'.so '+stage)
 guest('cp '+remote+'-helper '+asset+' && chmod 700 '+asset)
 if args.allocate_service:
  hook=root/'src/azhub/research/scripts/public-dhcp-event-hook.sh'
  assert az.push_file(str(hook),hook_asset)
  guest('chmod 700 '+hook_asset)
  assert guest('sha256sum '+hook_asset).stdout.split()[0]==hashlib.sha256(hook.read_bytes()).hexdigest()
 digest=hashlib.sha256(args.mod.read_bytes()).hexdigest()
 helper_digest=hashlib.sha256(args.helper.read_bytes()).hexdigest()
 assert guest('sha256sum '+package).stdout.split()[0]==digest
 assert guest('sha256sum '+stage).stdout.split()[0]==digest
 assert guest('sha256sum '+asset).stdout.split()[0]==helper_digest
 guest("printf 'usb-hub-support "+digest+"\\n' >> "+approved+"; echo 0 > "+starting)
 eject_environment=''
 if args.eject_selected:
  hubs=guest('for path in /sys/bus/usb/devices/*; do [ -f "$path/bDeviceClass" ] || continue; [ "$(cat "$path/bDeviceClass")" = 09 ] && readlink -f "$path"; done',check=False).stdout.splitlines()
  hubs=[path for path in hubs if re.fullmatch(r'[0-9]+-[0-9]+(?:\.[0-9]+)*',path.rsplit('/',1)[-1])]
  assert len(hubs)==1 and re.fullmatch(r'/sys/devices/[A-Za-z0-9_./:-]+',hubs[0]),hubs
  eject_environment='--setenv=AZHUB_EJECT_PORT2_ROOT='+hubs[0]+' '
 run_command=('systemd-run --unit='+unit+' --property=EnvironmentFile=/home/root/settings/az-usbload/az-preload.env '+
       eject_environment+'--setenv=AZHUB_PUBLIC_SDK_PROBE=1 '+('--setenv=AZHUB_SDK_SERVICE_PROBE=1 ' if args.service_module else '')+
       ('--setenv=AZHUB_SDK_ALLOCATE_PROBE=1 ' if args.allocate_service else '')+
       ('--setenv=AZHUB_SDK_CANCEL_DHCP=1 ' if args.cancel_dhcp else '')+
       ('--setenv=AZHUB_SDK_EXPECT_CONFLICT=1 ' if args.arp_conflict else '')+
       ('--setenv=AZHUB_SDK_EXPECT_REJECT=99 ' if args.reject_native_grant else '')+
       ('--setenv=AZHUB_SDK_RETRY_CONFLICT=1 ' if args.retry_conflict else '')+
       ('--setenv=AZHUB_SDK_INVENTORY=1 ' if args.inventory_gate else '')+
       ('--setenv=AZHUB_SDK_INVENTORY_MASK=11 ' if args.two_sources else '')+
       ('--setenv=AZHUB_EJECT_STATUS=1 ' if args.eject_selected else '')+
       ('--setenv=AZHUB_SDK_PLAYER_RECOVERY=1 ' if args.recover_player_collision else '')+
       ('--setenv=AZHUB_SDK_DYNAMIC_IDENTITY=1 ' if args.dynamic_identity else '')+
       ('--setenv=AZHUB_SDK_NATIVE_INVENTORY=1 ' if args.native_inventory else '')+
       ('--setenv=AZHUB_SDK_INVENTORY_ENGINE=1 ' if args.inventory_engine else '')+
       ('--setenv=AZHUB_SDK_CANDIDATE_RUNTIME=1 ' if args.candidate_runtime else '')+
       ('--setenv=AZHUB_SDK_RECOVER_NETWORK=1 ' if args.recover_native_network else '')+
       ('--setenv=AZHUB_EJECT_TRACE_LOAD=1 ' if args.local_playback or args.local_deck_topology or args.native_local_playback else '')+
       ('--setenv=AZHUB_SDK_SERVICE_HOLD=1 ' if args.library_root else '')+
       '--setenv=AZHUB_SDK_PROBE_RUN='+token+' /home/root/scripts/apl_start.sh')
 if args.integrated_runtime or args.native_status_observer:
  run_command=re.sub(r'--setenv=AZHUB_(?:PUBLIC_SDK_PROBE|SDK_[A-Z_]+)=[^ ]+ ', '',run_command)
 guest(run_command)
 if args.native_status_observer or args.integrated_loaded_eject or args.local_deck_topology or args.native_file_control:
  # Match the normal service's supported ExecStartPost. Already-mounted media
  # otherwise remains absent from native NFS export registration after restart.
  guest('/usr/sbin/az-usbload --post-start',timeout=90)
  (out/'framework-poststart.log').write_text(guest('cat /tmp/az-usbload-poststart.log').stdout)
  if args.native_status_held_file:
   current_pid=application_pid()
   raw=guest('dd if=/proc/'+current_pid+'/mem bs=1 skip='+str(0x3fd58f0)+' count=76 2>/dev/null | od -v -b').stdout
   table=bytes(int(value,8) for line in raw.splitlines() for value in line.split()[1:]);assert len(table)==76
   registration={'pid':current_pid,'lengths':struct.unpack_from('<III',table),'roots':[table[16+20*i:16+20*(i+1)].split(b'\0')[0].decode('ascii') for i in range(3)]}
   (out/'native-root-registration.json').write_text(json.dumps(registration,indent=2))
   assert all(registration['lengths'][:2]),'Supported post-start did not register both native NFS roots'

 if args.native_status_observer:
  limit=time.monotonic()+60
  while True:
   try:events=probe_events()
   except AssertionError:events=''
   if events:break
   # The install marker can exist before the first status sample.
   if probe_marker:break
   assert time.monotonic()<limit;time.sleep(.2)
  print('NATIVE_OBSERVER_SETTLING',flush=True);time.sleep(15)
  loaded=guest('cat /tmp/azmod-community/loaded').stdout
  (out/'native-observer-loaded.txt').write_text(loaded)
  assert 'suspended=0' in loaded and 'mod=usb-hub-support|1|' in loaded, 'Native observer was not loaded successfully; refusing synthetic stimulus'
  assert 'native-status-observer installed pid='+application_pid()+';'==probe_marker, 'Observer marker does not match the current application'
  child=launch([sys.executable,str(root/'src/azhub/research/scripts/test-loaded-status-emulator.py'),'--classes',str(args.classes),'--source','native1','--seconds','120' if args.native_status_eject else '60','--unit',unit,'--evidence-directory',str(out/'wire')]+(['--hold-file'] if args.native_status_held_file else [])+(['--native-eject'] if args.native_status_eject else []),'native-peer-scope')
  child.wait(timeout=240 if args.native_status_eject else 150);assert child.returncode==0,(out/'native-peer-scope.stderr').read_text()
  (out/'native-status-store.log').write_text(probe_events())
 if args.cancel_dhcp:
  limit=time.monotonic()+15
  while '"DISCOVER_waiting": true' not in (out/'dhcp-server.stdout').read_text():
   assert server.poll() is None and time.monotonic()<limit;time.sleep(.1)
  for child in guest('pidof udhcpc').stdout.split():
   assert child.isdigit()
   if hook_asset in guest('cat /proc/'+child+'/cmdline').stdout:pending_clients.append(int(child))
  assert len(pending_clients)==1,pending_clients
  assert guest('test ! -e '+ready_file).returncode==0
  (out/'pending-client.json').write_text(json.dumps({'native_DHCP_client_pid':pending_clients[0],'discover_observed':True,'no_OFFER':True},indent=2))
  guest(': > '+done_file)
 elif args.library_root and not args.reject_native_grant and (not args.arp_conflict or args.retry_conflict):
  limit=time.monotonic()+45
  if args.integrated_runtime:
   while True:
    try:events=probe_events()
    except AssertionError:events=''
    if all('public-sharing source='+str(slot)+' generation=1 phase=2 error=0' in events for slot in (source_slots if args.capacity_source_count else (0,4))):break
    assert time.monotonic()<limit;time.sleep(.2)
   assert guest("tr '\\0' '\\n' < /proc/"+application_pid()+"/environ | grep -E '^AZHUB_(PUBLIC_SDK_PROBE|SDK_)'",check=False).returncode==1
  else:
   while guest('test -e '+('/run/azhub-sdk-service-'+token+'.multi' if args.two_sources and not args.reject_duplicate_grant else ready_file),check=False).returncode:
    assert time.monotonic()<limit;time.sleep(.2)
  if args.local_playback:
   local_spec=importlib.util.spec_from_file_location('local_suite',root/'src/azhub/research/scripts/test-safe-eject-emulator.py')
   local_suite=importlib.util.module_from_spec(local_spec);local_spec.loader.exec_module(local_suite)
   local_suite.logs=probe_events
   def sample_playback(label):
    global playback_pid,playback_content_id
    state=json.loads(guest('cat /run/azhub-eject-status').stdout)
    if playback_pid is None:playback_pid=state['pid']
    assert state['pid']==playback_pid and guest('readlink /proc/'+str(playback_pid)+'/exe').stdout.strip().endswith('/EP147')
    deck=state['decks'][0]
    if playback_content_id is None:playback_content_id=deck['content_id']
    assert deck['content_id']==playback_content_id and playback_content_id>0
    playback_samples.append({'case':label,'clock_ms':state['clock_ms'],'pid':state['pid'],'deck':deck})
    assert deck['loaded']==1 and deck['playing']==1
    return deck
   local_suite.center_knob()
   shot_env=dict(os.environ,AZ_SHOT_DIR=str(out))
   local_suite.drive('source')
   subprocess.run([sys.executable,'C:/Users/icast/xdj-az-emu/tools/az-drive.py','shot','native-source-menu'],env=shot_env,check=True)
   selection=out/'native-source-row.txt';limit=time.monotonic()+90
   print('NATIVE_SOURCE_SELECTION_REQUIRED '+str(selection),flush=True)
   while not selection.exists():
    assert time.monotonic()<limit,'Review the saved emulator source menu and provide its verified hub row offset'
    time.sleep(.2)
   row=int(selection.read_text().strip());assert -1<=row<=7
   local_suite.browse_a(source_row=row)
   shot_env=dict(os.environ,AZ_SHOT_DIR=str(out))
   subprocess.run([sys.executable,'C:/Users/icast/xdj-az-emu/tools/az-drive.py','shot','native-before-load'],env=shot_env,check=True)
   marker=len(probe_events());local_suite.load(1,delay=1)
   limit=time.monotonic()+30
   while not json.loads(guest('cat /run/azhub-eject-status').stdout)['decks'][0]['loaded']:
    assert time.monotonic()<limit;time.sleep(.3)
   local_suite.assert_load_disk(marker,1,'sdc')
   deck=json.loads(guest('cat /run/azhub-eject-status').stdout)['decks'][0]
   if not deck['playing']:local_suite.drive('press',1,'play')
   time.sleep(2);local_suite.drive('press',1,'loop4');time.sleep(1)
   sample_playback('before independent clients')
  if args.local_deck_topology:
   topology_spec=importlib.util.spec_from_file_location('topology',root/'src/azhub/research/scripts/local-deck-topology-control.py')
   topology_module=importlib.util.module_from_spec(topology_spec);topology_spec.loader.exec_module(topology_module)
   topology=topology_module.LocalDeckTopology(root,out,guest,probe_events)
   topology.row=args.topology_source_row
   topology.begin_on()
  classpath=base+'/'+args.classes.resolve().relative_to(root).as_posix()+':'+base+'/local/link-peer-build/dependencies/*'
  link_local_options=[]
  if args.link_local:
   candidates=re.findall(r'allocate link-local candidate slot=(\d+) generation=\d+ address=(169\.254\.[0-9]+\.[0-9]+)',guest('cat '+helper_log).stdout)
   addresses={int(slot):address for slot,address in candidates};assert all(slot in addresses for slot in (0,4))
   assert addresses[0]!=addresses[4]
   (out/'link-local-addresses.json').write_text(json.dumps(addresses,indent=2))
   link_local_options=['-Dazhub.testLinkLocalPeer=true']+['-Dazhub.testSourceAddress'+str(slot)+'='+addresses[slot] for slot in (0,4)]
  prefix=['wsl.exe','-u','root','--exec','ip','netns','exec','azhub-link','java']+link_local_options+(['-Dazhub.testSourceAddress=192.0.2.4'] if args.retry_conflict else [])+['-cp',classpath]
  if args.native_local_playback:
   spec=importlib.util.spec_from_file_location('native_playback_control',root/'src/azhub/research/scripts/local-deck-topology-control.py')
   control=importlib.util.module_from_spec(spec);spec.loader.exec_module(control)
   native_playback=control.LocalDeckTopology(root,out,guest,probe_events,expected_disk='sdb' if args.native_local_usb==1 else 'sda')
   assert all(not deck['loaded'] for deck in native_playback.state()['decks'])
   native_playback.row=args.native_local_usb-2;native_playback.select(1);native_playback.browse();native_playback.load(1)
   native_playback.play_loop(1);native_playback.waveform();native_playback.sample('native USB'+str(args.native_local_usb)+' fresh local playback',(1,))
  observers=[] if args.whole_hub_withdrawal_only else [launch(prefix+['LiveProvidersControl' if args.two_sources and not args.reject_duplicate_grant else 'LiveProviderControl',base+'/local/diagnostics/sharing-hub0-export.pdb',base+'/local/diagnostics/sharing-pdb-tables-v1/java-entities.tsv']+(['--indices='+fixture_slot_text] if args.two_sources and not args.reject_duplicate_grant else []),'discovery-and-menus')]
  if args.native_file_control and not args.whole_hub_withdrawal_only:
   # Native exports are served by EP147, independently of the public helper.
   for name,relative,local in [('database','PIONEER/rekordbox/export.pdb','sharing-hub0-export.pdb'),('audio','Contents/Aaliyah/Are You That Somebody/Aaliyah - Are You That Somebody-1.mp3','sharing-hub0-audio.mp3')]:
    artifact=root/'local/diagnostics'/local
    observers.append(launch(prefix+['LocalFileServiceClient',relative,str(artifact.stat().st_size),hashlib.sha256(artifact.read_bytes()).hexdigest(),'--emulator-native-usb1'],'native-usb1-'+name))
   observers.append(launch(prefix+['LocalFileServiceClient','MODS/community-mods/usb-hub-support/usb-hub-support.so',str(args.mod.stat().st_size),digest,'--emulator-native-usb2'],'native-usb2-mod-file'))
   if args.native_local_usb==2:
    for name,relative,local in [('database','PIONEER/rekordbox/export.pdb','sharing-hub0-export.pdb'),('audio','Contents/Aaliyah/Are You That Somebody/Aaliyah - Are You That Somebody-1.mp3','sharing-hub0-audio.mp3')]:
     artifact=root/'local/diagnostics'/local
     observers.append(launch(prefix+['LocalFileServiceClient',relative,str(artifact.stat().st_size),hashlib.sha256(artifact.read_bytes()).hexdigest(),'--emulator-native-usb2'],'native-usb2-'+name))
  for name,relative,local in ([] if args.whole_hub_withdrawal_only else [('database','PIONEER/rekordbox/export.pdb','sharing-hub0-export.pdb'),('audio','Contents/Aaliyah/Are You That Somebody/Aaliyah - Are You That Somebody-1.mp3','sharing-hub0-audio.mp3')]):
   artifact=root/'local/diagnostics'/local
   observers.append(launch(prefix+['LocalFileServiceClient',relative,str(artifact.stat().st_size),hashlib.sha256(artifact.read_bytes()).hexdigest(),'--emulator-link-1'],name))
  if args.two_sources and not args.reject_duplicate_grant and not args.whole_hub_withdrawal_only:
   for name,relative,local in [('database','PIONEER/rekordbox/export.pdb','sharing-hub0-export.pdb'),('audio','Contents/Aaliyah/Are You That Somebody/Aaliyah - Are You That Somebody-1.mp3','sharing-hub0-audio.mp3')]:
    artifact=root/'local/diagnostics'/local
    observers.append(launch(prefix+['LocalFileServiceClient',relative,str(artifact.stat().st_size),hashlib.sha256(artifact.read_bytes()).hexdigest(),'--emulator-link-5'],'neighbor-'+name))
  if args.capacity_source_count and not args.whole_hub_withdrawal_only:
   for source in source_slots:
    if source in (0,4):continue
    for kind,relative,local in [('database','PIONEER/rekordbox/export.pdb','sharing-hub0-export.pdb'),('audio','Contents/Aaliyah/Are You That Somebody/Aaliyah - Are You That Somebody-1.mp3','sharing-hub0-audio.mp3')]:
     artifact=root/'local/diagnostics'/local
     observers.append(launch(prefix+['LocalFileServiceClient',relative,str(artifact.stat().st_size),hashlib.sha256(artifact.read_bytes()).hexdigest(),'--emulator-link-'+str(source+1)],'capacity-'+str(source)+'-'+kind))
  for child in observers:
   if args.local_playback:
    limit=time.monotonic()+(210 if args.two_sources else 150)
    while child.poll() is None:
     assert time.monotonic()<limit
     sample_playback('external clients active');time.sleep(1)
   elif native_playback:
    limit=time.monotonic()+210
    while child.poll() is None:
     assert time.monotonic()<limit
     native_playback.observe('native and hub remote clients active',(1,));time.sleep(1)
   elif topology:
    limit=time.monotonic()+210
    while child.poll() is None:
     assert time.monotonic()<limit
     topology.observe('initial remote clients active',(1,2));time.sleep(1)
   else:child.wait(timeout=210 if args.two_sources else 150)
   assert child.returncode==0,child.returncode
  if native_playback:
   native_playback.sample('native and hub clients completed',(1,))
   if not args.integrated_whole_hub:
    native_playback.unload(1)
    assert all(not deck['loaded'] for deck in native_playback.state()['decks'])
  if args.local_playback:sample_playback('external clients completed')
  if args.refresh_grants and not args.link_local:
   refreshed=guest('cat '+helper_log).stdout
   for source in source_slots:assert re.search(r'allocate refresh accepted slot='+str(source)+r' generation=1 lease_generation=1 count=[1-9]',refreshed),refreshed
   (out/'refresh-during-clients.log').write_text(refreshed)
  if args.integrated_whole_hub:
   state=json.loads(guest('cat /run/azhub-eject-status').stdout)
   if native_playback:
    native_playback.observe('native playback before whole-hub eject',(1,))
    assert all(not deck['loaded'] for deck in state['decks'][1:])
   else:assert all(not deck['loaded'] for deck in state['decks'])
   hubs=[item for item in state['devices'] if item['port']==2]
   assert len(hubs)==8
   roots={'/media/usb/'+item['disk']+'1' for item in hubs}
   before=guest('cat /proc/mounts').stdout.splitlines()
   native=[line for line in before if re.fullmatch(r'/media/usb/sd[a-z]+1',line.split()[1]) and line.split()[1] not in roots]
   assert len(native)==2 and roots.issubset({line.split()[1] for line in before})
   whole_prefix=prefix[:prefix.index('-cp')]+['-Dazhub.lifecycleWaitSeconds=100']+prefix[prefix.index('-cp'):]
   withdrawal=launch(whole_prefix+['WholeHubWithdrawalControl'],'whole-hub-withdrawal',interactive=True)
   limit=time.monotonic()+45
   while 'READY: eight independent' not in (out/'whole-hub-withdrawal.stdout').read_text():
    assert withdrawal.poll() is None and time.monotonic()<limit,(out/'whole-hub-withdrawal.stderr').read_text()
    time.sleep(.2)
   if args.whole_hub_native_admission:
    limit=time.monotonic()+30
    while True:
     current=probe_events()
     if 'accepted=1 b0=3 b1=0 b4=41 b5=4 b6=1 word8_le=1884 ' in current:
      (out/'whole-hub-native-admission.log').write_text(current);break
     assert withdrawal.poll() is None and time.monotonic()<limit
     time.sleep(.2)
   # Whole-hub STOP uses eject_stop_hook/gesture polling, not selector encoder
   # ownership. Do not add a short-selector precondition to the long gesture;
   # the existing SDK frame guards and selector refusal remain unchanged.
   subprocess.run([sys.executable,str(root/'src/azhub/research/scripts/eject-ui-emu.py'),'stop','2','2.3'],check=True)
   limit=time.monotonic()+120
   while True:
    after=guest('cat /proc/mounts').stdout.splitlines()
    assert set(native).issubset(after),'Native USB mount changed during whole-hub eject'
    assert application_pid()==str(state['pid']),'EP147 restarted during whole-hub eject'
    if native_playback:native_playback.observe('native playback while hub unmounts',(1,))
    if roots.isdisjoint({line.split()[1] for line in after}):break
    assert withdrawal.poll() is None and time.monotonic()<limit
    time.sleep(.3)
   withdrawal.stdin.write(b'W');withdrawal.stdin.flush();withdrawal.wait(timeout=45)
   assert withdrawal.returncode==0,(out/'whole-hub-withdrawal.stderr').read_text()
   assert 'PASS: all eight' in (out/'whole-hub-withdrawal.stdout').read_text()
   assert 'LOADED_STATUS_COMPLETE:' in (out/'whole-hub-withdrawal.stdout').read_text()
   checks=[]
   for name,relative,artifact,flag in [('database','PIONEER/rekordbox/export.pdb',root/'local/diagnostics/sharing-hub0-export.pdb','--emulator-native-usb1'),('audio','Contents/Aaliyah/Are You That Somebody/Aaliyah - Are You That Somebody-1.mp3',root/'local/diagnostics/sharing-hub0-audio.mp3','--emulator-native-usb1'),('mod-file','MODS/community-mods/usb-hub-support/usb-hub-support.so',args.mod,'--emulator-native-usb2')]:
    checks.append(launch(prefix+['LocalFileServiceClient',relative,str(artifact.stat().st_size),hashlib.sha256(artifact.read_bytes()).hexdigest(),flag],'whole-hub-native-'+name))
   for name,relative,local in [('database','PIONEER/rekordbox/export.pdb','sharing-hub0-export.pdb'),('audio','Contents/Aaliyah/Are You That Somebody/Aaliyah - Are You That Somebody-1.mp3','sharing-hub0-audio.mp3')]:
    artifact=root/'local/diagnostics'/local
    checks.append(launch(prefix+['LocalFileServiceClient',relative,str(artifact.stat().st_size),hashlib.sha256(artifact.read_bytes()).hexdigest(),'--emulator-native-usb2'],'whole-hub-native-usb2-'+name))
   for child in checks:child.wait(timeout=60);assert child.returncode==0
   if native_playback:
    native_playback.sample('native playback after whole-hub eject',(1,));native_playback.unload(1)
    assert not native_playback.state()['decks'][0]['loaded']
   (out/'whole-hub-eject.json').write_text(json.dumps({'hub_roots_unmounted':sorted(roots),'native_mounts_preserved':native,'native_complete_reads_after_eject':True,'same_EP147_pid':state['pid'],'independent_discovery_withdrawn':8,'retained_metadata_sessions_closed':8,'foreign_handle_STALE_checks_before_eject':56,'real_MP3_handle_and_synthetic_loaded_status':True,'paired_track_id':1884,'paired_source_slot':4,'native_playback_through_eject':bool(native_playback),'native_status_store_instrumented':args.whole_hub_native_admission,'manual_unmount_or_external_unload':False},indent=2))
  if args.link_local:
   limit=time.monotonic()+85
   while True:
    records=[json.loads(line) for line in (out/'dhcp-server.stdout').read_text().splitlines() if line.endswith('}')]
    attempts={slot:{record['xid'] for record in records if record.get('slot')==slot and record.get('DISCOVER_waiting')} for slot in (0,4)}
    if all(len(attempts[slot])>=2 for slot in (0,4)):break
    assert server.poll() is None and time.monotonic()<limit;time.sleep(.3)
   assert not any('reply' in record for record in records)
   (out/'link-local-dhcp-attempts.json').write_text(json.dumps({str(slot):sorted(values) for slot,values in attempts.items()},indent=2))
  if args.reject_duplicate_grant:
   observed=probe_events()
   assert 'inventory status generation=1 phase=0 error=99 slot=4' in observed
   assert not re.search(r'inventory status generation=\d+ phase=2 .*slot=4',observed)
   assert guest('test ! -e /run/azhub-sdk-service-'+token+'.multi').returncode==0
   (out/'duplicate-grant.json').write_text(json.dumps({'original_source_full_clients_passed':True,'duplicate_source_EMPTY_error':99,'duplicate_source_never_ACTIVE':True,'both_ACTIVE_marker_absent':True},indent=2))
  if args.eject_pending_refresh:
   limit=time.monotonic()+120
   while not any(json.loads(line).get('slot')==0 and json.loads(line).get('grants',0)>=4 for line in (out/'dhcp-server.stdout').read_text().splitlines()):
    assert server.poll() is None and time.monotonic()<limit;time.sleep(.2)
  neighbor=None
  if args.two_sources and (not args.integrated_runtime or args.integrated_eject) and not args.reject_duplicate_grant and not (args.native_network_change or (args.link_local and not (args.link_local_conflict or args.link_local_eject or args.link_local_dhcp_return))):
   neighbor_port=re.search(r'SOURCE: index=4 DB port=(\d+)',(out/'discovery-and-menus.stdout').read_text()).group(1)
   neighbor_prefix=prefix[:prefix.index('-cp')]+['-Dazhub.neighborIndex=4']+(['-Dazhub.lifecycleWaitSeconds=100'] if args.link_local_dhcp_return or args.integrated_loaded_eject else [])+prefix[prefix.index('-cp'):]
   neighbor=launch(neighbor_prefix+['ProviderNeighborControl',neighbor_port],'neighbor',interactive=True)
   limit=time.monotonic()+15
   while 'READY:' not in (out/'neighbor.stdout').read_text():
    assert neighbor.poll() is None and time.monotonic()<limit,(out/'neighbor.stderr').read_text()
    time.sleep(.2)
  if args.integrated_mode_cycle:
   metadata_port=re.search(r'SOURCE: index=0 DB port=(\d+)',(out/'discovery-and-menus.stdout').read_text()).group(1)
   observer=launch(prefix+['ProviderLifecycleControl',metadata_port,'--drain-only'],'mode-withdrawal',interactive=True)
   limit=time.monotonic()+15
   while 'READY:' not in (out/'mode-withdrawal.stdout').read_text():
    assert observer.poll() is None and time.monotonic()<limit;time.sleep(.2)
   initial_pid=application_pid();initial_peer=int(probe_marker.rsplit('=',1)[1]);initial_log=helper_log
   seeded_logs=[]
   if args.verify_log_retention:
    data=helper_log.rsplit('/',1)[0]
    for old_token in ('00000000000000000000000000000000','00000000000000000000000000000001'):
     for suffix in ('.log','.log.1'):
      path=data+'/public-sharing-'+old_token+suffix
      guest('test ! -e '+path)
      guest('printf retention-control > '+path)
      guest('touch -t 200001010000 '+path)
      seeded_logs.append(path)
    (out/'log-retention-seeded.json').write_text(json.dumps({'paths':seeded_logs,'mtime':'2000-01-01T00:00'},indent=2))

   (out/'mode-old-helper-path.txt').write_text(initial_log)
   mounts_before=guest('cat /proc/mounts').stdout.splitlines()
   native_mode_off=True;native_link_choice(0)
   limit=time.monotonic()+35
   while 'public-sharing graceful retirement complete; restart scheduled' not in probe_events():
    assert time.monotonic()<limit;time.sleep(.2)
   off_events=probe_events()
   assert 'mode recovery: native completion mode=0' in off_events
   assert 'public-sharing native Link Off; retiring public sources' in off_events
   assert guest('test ! -e /proc/'+str(initial_peer),check=False).returncode==0
   assert not guest('ip netns list').stdout.strip()
   observer.stdin.write(b'W');observer.stdin.flush();observer.wait(timeout=35);assert observer.returncode==0
   assert application_pid()==initial_pid
   mounts_after=guest('cat /proc/mounts').stdout.splitlines()
   for line in mounts_before:
    if re.fullmatch(r'/media/usb/sd[a-z]+1',line.split()[1]):assert line in mounts_after
   (out/'mode-off.log').write_text(off_events)
   (out/'mode-first-helper.log').write_text(guest('cat '+initial_log).stdout)
   if topology:topology.while_off()
   native_link_choice(1);native_mode_off=False
   probe_marker=None
   limit=time.monotonic()+65
   while True:
    try:events=probe_events()
    except AssertionError:events=''
    if all('public-sharing source='+str(slot)+' generation=1 phase=2 error=0' in events for slot in (0,4)):break
    assert time.monotonic()<limit;time.sleep(.2)
   new_peer=int(probe_marker.rsplit('=',1)[1]);assert new_peer!=initial_peer and helper_log!=initial_log
   if args.verify_log_retention:
    for path in seeded_logs:guest('test ! -e '+path)
    listing=guest('ls -ln '+helper_log.rsplit('/',1)[0]+'/public-sharing-*.log*').stdout
    files=[line.split()[-1] for line in listing.splitlines() if line.startswith('-') and re.fullmatch(r'public-sharing-[0-9a-f]{32}\.log(?:\.1)?',line.split()[-1].rsplit('/',1)[-1])]
    families={re.sub(r'\.1$','',path) for path in files}
    assert families=={initial_log,helper_log},families
    (out/'log-retention-verified.json').write_text(json.dumps({'old_seed_families_removed':True,'families':sorted(families),'files':files},indent=2))

   assert application_pid()==initial_pid
   fresh=[launch(prefix+['LiveProvidersControl',base+'/local/diagnostics/sharing-hub0-export.pdb',base+'/local/diagnostics/sharing-pdb-tables-v1/java-entities.tsv','--indices=0,4'],'mode-recovered-menus')]
   for source in (0,4):
    for name,relative,local in [('database','PIONEER/rekordbox/export.pdb','sharing-hub0-export.pdb'),('audio','Contents/Aaliyah/Are You That Somebody/Aaliyah - Are You That Somebody-1.mp3','sharing-hub0-audio.mp3')]:
     artifact=root/'local/diagnostics'/local
     fresh.append(launch(prefix+['LocalFileServiceClient',relative,str(artifact.stat().st_size),hashlib.sha256(artifact.read_bytes()).hexdigest(),'--emulator-link-'+str(source+1)],'mode-recovered-'+str(source)+'-'+name))
   if topology:
    limit=time.monotonic()+210
    while any(child.poll() is None for child in fresh):
     topology.observe('recovered remote clients active',(1,2))
     assert time.monotonic()<limit,'Recovered clients did not finish within the bounded control'
     time.sleep(1)
   for child in fresh:child.wait(timeout=210);assert child.returncode==0
   assert persisted_link_mode()==1
   if topology:topology.returned_on()
   (out/'mode-cycle.json').write_text(json.dumps({'native_application_pid':initial_pid,'old_helper_pid':initial_peer,'new_helper_pid':new_peer,'fresh_IPC_session':True,'old_endpoints_withdrawn':True,'native_and_hub_backing_mounts_preserved':True,'initial_and_final_user_mode':1,'local_loaded_decks_tested':args.local_deck_topology},indent=2))
   (out/'mode-old-helper-path.txt').write_text(initial_log)
  if args.link_local_dhcp_return:
   metadata_port=re.search(r'SOURCE: index=0 DB port=(\d+)',(out/'discovery-and-menus.stdout').read_text()).group(1)
   observer_prefix=prefix[:prefix.index('-cp')]+['-Dazhub.lifecycleWaitSeconds=100']+prefix[prefix.index('-cp'):]
   withdrawal=launch(observer_prefix+['ProviderLifecycleControl',metadata_port,'--drain-only'],'dhcp-return-withdrawal',interactive=True)
   limit=time.monotonic()+15
   while 'READY:' not in (out/'dhcp-return-withdrawal.stdout').read_text():
    assert withdrawal.poll() is None and time.monotonic()<limit;time.sleep(.2)
   (out/'return-dhcp').touch(exist_ok=False)
   limit=time.monotonic()+85
   while 'inventory status generation=2 phase=2 error=0 slot=0' not in probe_events():
    assert neighbor.poll() is None and time.monotonic()<limit;time.sleep(.2)
   events=probe_events();assert 'inventory acknowledged retirement generation=1 error=102 slot=0' in events
   (out/'dhcp-return-generations.log').write_text(events)
   withdrawal.stdin.write(b'W');withdrawal.stdin.flush()
   neighbor.stdin.write(b'WR');neighbor.stdin.flush();neighbor.wait(timeout=15);assert neighbor.returncode==0
   withdrawal.wait(timeout=35);assert withdrawal.returncode==0
   changed_prefix=[item for item in prefix if not item.startswith('-Dazhub.testSourceAddress0=')]
   recovered=[launch(changed_prefix+['LiveProviderControl',base+'/local/diagnostics/sharing-hub0-export.pdb',base+'/local/diagnostics/sharing-pdb-tables-v1/java-entities.tsv'],'dhcp-return-menus')]
   for name,relative,local in [('database','PIONEER/rekordbox/export.pdb','sharing-hub0-export.pdb'),('audio','Contents/Aaliyah/Are You That Somebody/Aaliyah - Are You That Somebody-1.mp3','sharing-hub0-audio.mp3')]:
    artifact=root/'local/diagnostics'/local
    recovered.append(launch(changed_prefix+['LocalFileServiceClient',relative,str(artifact.stat().st_size),hashlib.sha256(artifact.read_bytes()).hexdigest(),'--emulator-link-1'],'dhcp-return-'+name))
   for child in recovered:child.wait(timeout=150);assert child.returncode==0
   replies=[json.loads(line) for line in (out/'dhcp-server.stdout').read_text().splitlines() if line.endswith('}')]
   assert any(row.get('reply')=='ACK' and row.get('slot')==0 for row in replies)
   assert not any(row.get('reply') and row.get('slot')==4 for row in replies)
   (out/'dhcp-return.json').write_text(json.dumps({'old_address':addresses[0],'new_address':'192.0.2.3','acknowledged_retired_generation':1,'active_generation':2,'neighbor_original_sessions_preserved':True,'old_endpoints_withdrawal_observed':True},indent=2))
  if args.link_local_conflict:
   old_address=addresses[0]
   from link_fixture_network import source_mac
   injection=r"""import socket
mac=bytes.fromhex(%r);own=bytes.fromhex('020000000202');address=socket.inet_aton(%r)
frame=mac+own+b'\x08\x06'+b'\x00\x01\x08\x00\x06\x04\x00\x02'+own+address+mac+b'\0'*4
with socket.socket(socket.AF_PACKET,socket.SOCK_RAW,socket.htons(3)) as stream:
 stream.bind(('azlink0',0));assert stream.send(frame)==len(frame)
print('LINK_LOCAL_FOREIGN_ARP_CLAIM_SENT')
""" % (source_mac(dynamic_parent,0).hex(),old_address)
   injected=host('ip','netns','exec','azhub-link','python3','-c',injection)
   (out/'link-local-conflict-injection.stdout').write_text(injected.stdout)
   limit=time.monotonic()+40
   while 'inventory status generation=2 phase=2 error=0 slot=0' not in probe_events():
    assert neighbor.poll() is None and time.monotonic()<limit;time.sleep(.2)
   events=probe_events();assert 'inventory acknowledged retirement generation=1 error=98 slot=0' in events
   (out/'link-local-conflict-generations.log').write_text(events)
   replacements=re.findall(r'allocate link-local candidate slot=0 generation=2 address=(169\.254\.[0-9]+\.[0-9]+)',guest('cat '+helper_log).stdout)
   assert len(replacements)==1 and replacements[0]!=old_address
   neighbor.stdin.write(b'WR');neighbor.stdin.flush();neighbor.wait(timeout=15);assert neighbor.returncode==0
   changed_prefix=[('-Dazhub.testSourceAddress0='+replacements[0]) if item.startswith('-Dazhub.testSourceAddress0=') else item for item in prefix]
   recovered=[launch(changed_prefix+['LiveProviderControl',base+'/local/diagnostics/sharing-hub0-export.pdb',base+'/local/diagnostics/sharing-pdb-tables-v1/java-entities.tsv'],'link-local-recovered-menus')]
   for name,relative,local in [('database','PIONEER/rekordbox/export.pdb','sharing-hub0-export.pdb'),('audio','Contents/Aaliyah/Are You That Somebody/Aaliyah - Are You That Somebody-1.mp3','sharing-hub0-audio.mp3')]:
    artifact=root/'local/diagnostics'/local
    recovered.append(launch(changed_prefix+['LocalFileServiceClient',relative,str(artifact.stat().st_size),hashlib.sha256(artifact.read_bytes()).hexdigest(),'--emulator-link-1'],'link-local-recovered-'+name))
   for child in recovered:child.wait(timeout=150);assert child.returncode==0
   assert 'DHCPDECLINE_received' not in (out/'dhcp-server.stdout').read_text()
   (out/'link-local-conflict.json').write_text(json.dumps({'old_address':old_address,'new_address':replacements[0],'acknowledged_retired_generation':1,'empty_status_directly_logged':True,'active_generation':2,'neighbor_original_sessions_preserved':True,'fixture_observed_DHCP_DECLINE':False,'old_handle_withdrawal_observed':False},indent=2))
  if args.native_network_change:
   metadata_port=re.search(r'DB port=(\d+)',(out/'discovery-and-menus.stdout').read_text()).group(1)
   observer=launch(prefix+['ProviderLifecycleControl',metadata_port,'--drain-only'],'network-withdrawal',interactive=True)
   limit=time.monotonic()+15
   while 'READY:' not in (out/'network-withdrawal.stdout').read_text():
    assert observer.poll() is None and time.monotonic()<limit;time.sleep(.2)
   initial_events=probe_events()
   initial_peer=int(re.search(r'accepted helper pid=(\d+)',initial_events).group(1))
   initial_command=guest('cat /proc/'+str(initial_peer)+'/cmdline').stdout.split('\0')
   mounts_before=guest('cat /proc/mounts').stdout.splitlines()
   assert '192.0.2.99/' not in guest('ip -o -4 addr show dev eth0').stdout
   guest('ip -4 addr add 192.0.2.99/24 dev eth0');secondary_address=True
   (out/'native-network-injected.txt').write_text(guest('ip -o -4 addr show dev eth0').stdout)
   limit=time.monotonic()+30
   while ('restart scheduled attempt=1 after confirmed retirement' not in probe_events()) if args.recover_native_network else bool(guest('test -e '+result_file,check=False).returncode):
    assert time.monotonic()<limit;time.sleep(.2)
   assert guest('test ! -e /proc/'+str(initial_peer),check=False).returncode==0
   assert not guest('ip netns list').stdout.strip()
   guest('ip -4 addr del 192.0.2.99/24 dev eth0');secondary_address=False
   observer.stdin.write(b'W');observer.stdin.flush();observer.wait(timeout=35)
   assert observer.returncode==0,(out/'network-withdrawal.stderr').read_text()
   assert 'native network changed; retiring owned sources' in guest('cat '+helper_log).stdout
   if not args.recover_native_network:assert not guest('ip netns list').stdout.strip()
   mounts_after=guest('cat /proc/mounts').stdout.splitlines()
   for line in mounts_before:
    if re.fullmatch(r'/media/usb/sd[a-z]+1',line.split()[1]):assert line in mounts_after
   (out/'native-network-change.json').write_text(json.dumps({'secondary_address':'192.0.2.99/24','owned_namespaces_retired':True,'native_USB_mounts_preserved':True,'secondary_address_removed':True,'automatic_recovery_tested':args.recover_native_network},indent=2))
   if args.recover_native_network:
    limit=time.monotonic()+60
    while guest('test -e /run/azhub-sdk-service-'+token+'.multi',check=False).returncode:
     assert time.monotonic()<limit;time.sleep(.3)
    events=probe_events();assert re.search(r'managed launch pid=\d+ attempt=1',events)
    new_peer=int(re.findall(r'accepted helper pid=(\d+)',events)[-1]);assert new_peer!=initial_peer
    new_command=guest('cat /proc/'+str(new_peer)+'/cmdline').stdout.split('\0')
    assert len(initial_command)>3 and len(new_command)>3 and initial_command[3]!=new_command[3]
    (out/'network-recovery-sessions.json').write_text(json.dumps({'old_peer':initial_peer,'new_peer':new_peer,'old_peer_gone_before_relaunch':True},indent=2))
    resumed=[launch(prefix+['LiveProvidersControl',base+'/local/diagnostics/sharing-hub0-export.pdb',base+'/local/diagnostics/sharing-pdb-tables-v1/java-entities.tsv','--indices=0,4'],'network-recovered-menus')]
    for source in (0,4):
     for name,relative,local in [('database','PIONEER/rekordbox/export.pdb','sharing-hub0-export.pdb'),('audio','Contents/Aaliyah/Are You That Somebody/Aaliyah - Are You That Somebody-1.mp3','sharing-hub0-audio.mp3')]:
      artifact=root/'local/diagnostics'/local
      resumed.append(launch(prefix+['LocalFileServiceClient',relative,str(artifact.stat().st_size),hashlib.sha256(artifact.read_bytes()).hexdigest(),'--emulator-link-'+str(source+1)],'network-recovered-'+str(source)+'-'+name))
    for child in resumed:child.wait(timeout=210);assert child.returncode==0
  if args.player_collision:
   collision_code="""import socket,struct
payload=bytearray(50);payload[:10]=b'Qspt1WmJOL';payload[10]=2;payload[12:21]=b'rekordbox';payload[32:36]=bytes([1,3,0,50]);payload[36:40]=socket.inet_aton('192.0.2.2');payload[40:46]=bytes.fromhex('0200000012fe');payload[46:50]=bytes([41,6,4,1])
udp=struct.pack('!HHHH',50000,50000,8+len(payload),0)+payload
ip=bytearray(struct.pack('!BBHHHBBH4s4s',0x45,0,20+len(udp),1,0,64,17,0,socket.inet_aton('192.0.2.2'),socket.inet_aton('192.0.2.3')))
checksum=sum(struct.unpack('!10H',ip));checksum=(checksum&65535)+(checksum>>16);checksum=(checksum&65535)+(checksum>>16);ip[10:12]=struct.pack('!H',(~checksum)&65535)
frame=bytes.fromhex('0200000012030200000002020800')+ip+udp
with socket.socket(socket.AF_PACKET,socket.SOCK_RAW,socket.htons(3)) as stream:
 stream.bind(('azlink0',0));assert stream.send(frame)==len(frame)
print('FOREIGN_PLAYER_41_CLAIM_SENT')
"""
   injected=host('ip','netns','exec','azhub-link','python3','-c',collision_code)
   (out/'collision-injection.stdout').write_text(injected.stdout)
   limit=time.monotonic()+20
   while 'inventory status generation=1 phase=0 error=17 slot=0' not in probe_events():
    assert neighbor.poll() is None and time.monotonic()<limit;time.sleep(.2)
   time.sleep(2)
   events=probe_events()
   if not args.recover_player_collision:assert not re.search(r'inventory status generation=[2-9][0-9]* .*slot=0',events)
   else:
    limit=time.monotonic()+30
    while 'inventory status generation=2 phase=2 error=0 slot=0' not in probe_events():
     assert neighbor.poll() is None and time.monotonic()<limit;time.sleep(.2)
    events=probe_events();assert 'player replacement old=41 new=42 retired_generation=1' in events
   (out/'collision-retirement.log').write_text(events)
   neighbor.stdin.write(b'WR');neighbor.stdin.flush();neighbor.wait(timeout=15);assert neighbor.returncode==0
   if args.recover_player_collision:
    recovered_prefix=prefix[:prefix.index('-cp')]+['-Dazhub.testSourceNumber=42']+prefix[prefix.index('-cp'):]
    recovered=launch(recovered_prefix+['LiveProviderControl',base+'/local/diagnostics/sharing-hub0-export.pdb',base+'/local/diagnostics/sharing-pdb-tables-v1/java-entities.tsv'],'replacement-menus')
    recovered.wait(timeout=150);assert recovered.returncode==0
    for name,relative,local in [('database','PIONEER/rekordbox/export.pdb','sharing-hub0-export.pdb'),('audio','Contents/Aaliyah/Are You That Somebody/Aaliyah - Are You That Somebody-1.mp3','sharing-hub0-audio.mp3')]:
     artifact=root/'local/diagnostics'/local
     fresh=launch(recovered_prefix+['LocalFileServiceClient',relative,str(artifact.stat().st_size),hashlib.sha256(artifact.read_bytes()).hexdigest(),'--emulator-link-1'],'replacement-'+name)
     fresh.wait(timeout=60);assert fresh.returncode==0
  if args.change_refresh_address:
   metadata_port=re.search(r'DB port=(\d+)',(out/'discovery-and-menus.stdout').read_text()).group(1)
   withdrawal=launch(prefix+['ProviderLifecycleControl',metadata_port,'--drain-only'],'changed-withdrawal',interactive=True)
   limit=time.monotonic()+15
   while 'READY:' not in (out/'changed-withdrawal.stdout').read_text():
    assert withdrawal.poll() is None and time.monotonic()<limit;time.sleep(.2)
   (out/'change-refresh').touch(exist_ok=False)
   limit=time.monotonic()+45
   while 'inventory status generation=2 phase=2 error=0 slot=0' not in probe_events():
    assert neighbor.poll() is None and time.monotonic()<limit;time.sleep(.2)
   events=probe_events()
   assert 'inventory status generation=1 phase=0 error=102 slot=0' in events,events
   (out/'changed-generation.log').write_text(events)
   withdrawal.stdin.write(b'W');withdrawal.stdin.flush()
   neighbor.stdin.write(b'WR');neighbor.stdin.flush()
   neighbor.wait(timeout=15);assert neighbor.returncode==0
   withdrawal.wait(timeout=30);assert withdrawal.returncode==0
   changed_prefix=prefix[:prefix.index('-cp')]+['-Dazhub.testSourceAddress=192.0.2.4']+prefix[prefix.index('-cp'):]
   recovered=[launch(changed_prefix+['LiveProviderControl',base+'/local/diagnostics/sharing-hub0-export.pdb',base+'/local/diagnostics/sharing-pdb-tables-v1/java-entities.tsv'],'changed-menus')]
   for name,relative,local in [('database','PIONEER/rekordbox/export.pdb','sharing-hub0-export.pdb'),('audio','Contents/Aaliyah/Are You That Somebody/Aaliyah - Are You That Somebody-1.mp3','sharing-hub0-audio.mp3')]:
    artifact=root/'local/diagnostics'/local
    recovered.append(launch(changed_prefix+['LocalFileServiceClient',relative,str(artifact.stat().st_size),hashlib.sha256(artifact.read_bytes()).hexdigest(),'--emulator-link-1'],'changed-'+name))
   for child in recovered:child.wait(timeout=150);assert child.returncode==0
  if args.eject_selected and (not args.integrated_runtime or args.integrated_eject) and not (args.change_refresh_address or args.player_collision or args.native_network_change or (args.link_local and not args.link_local_eject)):
   metadata_port=re.search(r'SOURCE: index=0 DB port=(\d+)' if args.two_sources else r'DB port=(\d+)',(out/'discovery-and-menus.stdout').read_text()).group(1)
   withdrawal_prefix=prefix[:prefix.index('-cp')]+(['-Dazhub.loadedFileControl=true','-Dazhub.loadedStatusControl=true','-Dazhub.lifecycleWaitSeconds=100'] if args.integrated_loaded_eject else [])+prefix[prefix.index('-cp'):]
   remote_wire=None
   if args.integrated_loaded_eject:
    remote_wire=launch(['wsl.exe','-u','root','--exec','ip','netns','exec','azhub-link','python3',base+'/src/azhub/research/scripts/link-packet-observer.py','--seconds','180'],'remote-loaded-wire')
   withdrawal=launch(withdrawal_prefix+['ProviderLifecycleControl',metadata_port,'--retire-and-reconnect' if args.reconnect_selected and not args.link_local_eject else '--drain-only'],'withdrawal',interactive=True)
   limit=time.monotonic()+(35 if args.integrated_loaded_eject else 15)
   while 'READY:' not in (out/'withdrawal.stdout').read_text():
    assert withdrawal.poll() is None and time.monotonic()<limit,(out/'withdrawal.stderr').read_text()
    time.sleep(.2)
   remote_status=None
   if args.integrated_loaded_eject:
    assert 'FILE_READY:' in (out/'withdrawal.stdout').read_text()
    remote_status=withdrawal
    limit=time.monotonic()+30
    while True:
     assert remote_status.poll() is None and time.monotonic()<limit,(out/'withdrawal.stderr').read_text()
     if args.loaded_status_wire_only:
      rows=[json.loads(line) for line in (out/'remote-loaded-wire.stdout').read_text().splitlines() if line.endswith('}')]
      frames=[row for row in rows if row.get('source')=='192.0.2.2' and row.get('link_type')==10 and row.get('byte_21')==3 and row.get('byte_28')==41 and row.get('byte_29')==4 and row.get('word_2c')==1884]
      counts={target:sum(row.get('destination')==target for row in frames) for target in ('192.0.2.3','192.0.2.15')}
      if 'LOADED_STATUS_READY:' in (out/'withdrawal.stdout').read_text() and all(count>=10 for count in counts.values()):
       (out/'remote-loaded-wire-ready.json').write_text(json.dumps(counts,indent=2));break
      time.sleep(.2);continue
     logs=guest('cat /tmp/azmod-community/usb-hub-support.log.1 /tmp/azmod-community/usb-hub-support.log 2>/dev/null || true').stdout
     marker='native-status-observer installed pid='+application_pid()+';'
     if marker in logs:
      current=logs.rsplit(marker,1)[1]
      if 'LOADED_STATUS_READY:' in (out/'withdrawal.stdout').read_text() and 'accepted=1 b0=3 b1=0 b4=41 b5=4 b6=1 word8_le=1884 ' in current:
       (out/'remote-loaded-native-accepted.log').write_text(current);break
     time.sleep(.2)
   def telemetry():return json.loads(guest('cat /run/azhub-eject-status').stdout)
   before=telemetry();disk=args.library_root.rsplit('/',1)[1][:-1]
   devices=[item for item in before['devices'] if item['port']==2]
   index=next(i for i,item in enumerate(devices) if item['disk']==disk)
   mounts_before=guest('cat /proc/mounts').stdout.splitlines()
   neighbors=[line for line in mounts_before if re.fullmatch(r'/media/usb/sd[a-z]+1',line.split()[1]) and line.split()[1]!=args.library_root]
   assert neighbors
   if args.eject_pending_refresh:
    limit=time.monotonic()+40
    while True:
     waiting=[json.loads(line) for line in (out/'dhcp-server.stdout').read_text().splitlines() if 'refresh_DISCOVER_waiting' in line]
     if waiting:
      found=[]
      for child in guest('pidof udhcpc',check=False).stdout.split():
       assert child.isdigit()
       command=guest('cat /proc/'+child+'/cmdline',check=False).stdout
       if hook_asset in command and '\0azpub0\0' in command:found.append(int(child))
      if len(found)==1:
       pending_clients.extend(found)
       (out/'pending-refresh-client.json').write_text(json.dumps({'pid':found[0],'discover':waiting[-1],'observed_before_eject':True},indent=2));break
     assert server.poll() is None and time.monotonic()<limit;time.sleep(.1)
   subprocess.run([sys.executable,str(root/'src/azhub/research/scripts/eject-ui-emu.py'),'stop','2','0.5'],check=True)
   time.sleep(1)
   if index:subprocess.run([sys.executable,str(root/'src/azhub/research/scripts/eject-ui-emu.py'),'knob','cw',str(index)],check=True)
   subprocess.run([sys.executable,'C:/Users/icast/xdj-az-emu/tools/az-drive.py','knobclick'],check=True)
   limit=time.monotonic()+60
   while True:
    after=telemetry()
    if any(item['disk']==disk and item['state']==2 for item in after['devices']):break
    assert time.monotonic()<limit,guest('tail -50 /tmp/azmod-community/usb-hub-support.log').stdout
    time.sleep(.3)
   if args.eject_pending_refresh:
    for child in pending_clients:assert guest('test ! -e /proc/'+str(child),check=False).returncode==0
    (out/'resume-refresh').touch(exist_ok=False)
   if args.local_playback:sample_playback('selected backing ejected')
   events=probe_events()
   match=re.search((r'public-sharing inventory source=(\d+) root=' if args.integrated_runtime else r'sdk-broker-probe inventory source=(\d+) root=')+re.escape(args.library_root),events);assert match
   slot=match.group(1)
   assert events.index(('public-sharing acknowledged retirement source='+slot+' generation=1 error=0') if args.integrated_runtime else ('sdk-broker-probe inventory retired source='+slot))<events.index('safe-eject source-closed source='+slot)
   mounts_after=guest('cat /proc/mounts').stdout.splitlines()
   assert all(line in mounts_after for line in neighbors)
   assert not any(line.split()[1]==args.library_root for line in mounts_after)
   if neighbor:neighbor.stdin.write(b'W');neighbor.stdin.flush()
   withdrawal.stdin.write(b'W');withdrawal.stdin.flush()
   if args.reconnect_selected:
    limit=time.monotonic()+30
    while 'WITHDRAWN:' not in (out/'withdrawal.stdout').read_text():
     assert withdrawal.poll() is None and time.monotonic()<limit,(out/'withdrawal.stderr').read_text()
     time.sleep(.2)
    assert args.library_root=='/media/usb/sdc1' or args.integrated_eject
    assert 'return' in e.qmp('device_del',{'id':'functionalstickA'})
    limit=time.monotonic()+15
    while guest('test ! -e /sys/bus/usb/devices/3-2.1',check=False).returncode:
     assert time.monotonic()<limit;time.sleep(.2)
    assert 'return' in e.qmp('device_add',{'driver':'usb-storage','id':'functionalstickA','drive':'uibadgeA','bus':'functionalxhci.0','port':'2.1'})
    limit=time.monotonic()+45
    while (('public-sharing source=0 generation=2 phase=2 error=0' if args.integrated_runtime else 'inventory status generation=2 phase=2 error=0 slot=0') not in probe_events()):
     assert time.monotonic()<limit;time.sleep(.3)
    if neighbor:neighbor.stdin.write(b'R');neighbor.stdin.flush()
    if args.link_local_eject:
     withdrawal.wait(timeout=5);assert withdrawal.returncode==0
     replacements=re.findall(r'allocate link-local candidate slot=0 generation=2 address=(169\.254\.[0-9]+\.[0-9]+)',guest('cat '+helper_log).stdout)
     assert len(replacements)==1
     prefix=[('-Dazhub.testSourceAddress0='+replacements[0]) if item.startswith('-Dazhub.testSourceAddress0=') else item for item in prefix]
     fresh_menus=launch(prefix+['LiveProviderControl',base+'/local/diagnostics/sharing-hub0-export.pdb',base+'/local/diagnostics/sharing-pdb-tables-v1/java-entities.tsv'],'reconnected-menus')
     fresh_audio=root/'local/diagnostics/sharing-hub0-audio.mp3'
     fresh_audio_client=launch(prefix+['LocalFileServiceClient','Contents/Aaliyah/Are You That Somebody/Aaliyah - Are You That Somebody-1.mp3',str(fresh_audio.stat().st_size),hashlib.sha256(fresh_audio.read_bytes()).hexdigest(),'--emulator-link-1'],'reconnected-audio')
     (out/'link-local-reconnected-address.json').write_text(json.dumps({'old_address':addresses[0],'new_address':replacements[0],'old_handle_rechecked_at_new_address':False},indent=2))
    else:withdrawal.stdin.write(b'R');withdrawal.stdin.flush()
    artifact=root/'local/diagnostics/sharing-hub0-export.pdb'
    fresh=launch(prefix+['LocalFileServiceClient','PIONEER/rekordbox/export.pdb',str(artifact.stat().st_size),hashlib.sha256(artifact.read_bytes()).hexdigest(),'--emulator-link-1'],'reconnected-database')
    fresh.wait(timeout=60);assert fresh.returncode==0
    if args.link_local_eject:
     fresh_audio_client.wait(timeout=60);assert fresh_audio_client.returncode==0
     fresh_menus.wait(timeout=150);assert fresh_menus.returncode==0
   withdrawal.wait(timeout=35);assert withdrawal.returncode==0,(out/'withdrawal.stderr').read_text()
   if neighbor:
    neighbor.wait(timeout=15);assert neighbor.returncode==0,(out/'neighbor.stderr').read_text()
   if remote_status:
    assert 'LOADED_STATUS_COMPLETE:' in (out/'withdrawal.stdout').read_text(),'Loaded status did not complete with the lifecycle client'
    assert 'FILE_STALE:' in (out/'withdrawal.stdout').read_text()
    remote_wire.terminate();remote_wire.wait(timeout=10)
    rows=[json.loads(line) for line in (out/'remote-loaded-wire.stdout').read_text().splitlines()]
    frames=[row for row in rows if row.get('source')=='192.0.2.2' and row.get('link_type')==10 and row.get('byte_21')==3 and row.get('byte_28')==41 and row.get('byte_29')==4 and row.get('word_2c')==1884]
    destinations={target:sum(row.get('destination')==target for row in frames) for target in ('192.0.2.3','192.0.2.15')}
    assert all(count>=10 for count in destinations.values()),destinations
    (out/'remote-loaded-eject.json').write_text(json.dumps({'native_store_accepted':None if args.loaded_status_wire_only else True,'native_store_admission_observed':not args.loaded_status_wire_only,'public_source':41,'source_slot':4,'track_id':1884,'first_MP3_block_exact':True,'old_MP3_handle_STALE_after_reconnect':True,'status_continued_through_eject_reconnect':True,'wire_packets_by_destination':destinations,'manual_external_unload':False,'actual_CDJ_acceptance':False,'audio_renderer':False},indent=2))
   (out/'native-eject.json').write_text(json.dumps({'before':before,'after':after,'source':int(slot),'root':args.library_root,'retirement_before_source_close':True,'backing_unmounted':True,'neighbor_mounts_preserved':neighbors,'remote_loaded_deck_tested':args.integrated_loaded_eject},indent=2))
  if args.local_playback:
   sample_playback('after eject/replug observers')
   assert max(item['deck']['pos_s'] for item in playback_samples)-min(item['deck']['pos_s'] for item in playback_samples)>.1
   (out/'local-playback.json').write_text(json.dumps({'samples':playback_samples,'physical_backing_verified':'sdc','native_four_beat_loop':True,'audio_signal_captured':False},indent=2))
  if not args.integrated_runtime:guest(': > '+done_file)
 if args.native_status_observer:
  pid=application_pid();mapped=guest('grep usb-hub-support.so /proc/'+pid+'/maps | head -1').stdout.split()[0]
  assert guest('sha256sum /proc/'+pid+'/map_files/'+mapped).stdout.split()[0]==digest
  (out/'mod.log').write_text(probe_events());result={}
 elif args.integrated_runtime:
  pid=application_pid();mapped=guest('grep usb-hub-support.so /proc/'+pid+'/maps | head -1').stdout.split()[0]
  assert guest('sha256sum /proc/'+pid+'/map_files/'+mapped).stdout.split()[0]==digest
  initial_peer=int(probe_marker.rsplit('=',1)[1])
  assert '192.0.2.99/' not in guest('ip -o -4 addr show dev eth0').stdout
  guest('ip -4 addr add 192.0.2.99/24 dev eth0');secondary_address=True
  limit=time.monotonic()+30
  while 'public-sharing graceful retirement complete; restart scheduled' not in probe_events():
   assert time.monotonic()<limit;time.sleep(.2)
  assert guest('test ! -e /proc/'+str(initial_peer),check=False).returncode==0
  assert not guest('ip netns list').stdout.strip()
  if args.integrated_network_recovery:
   old_peer=initial_peer;old_log=helper_log
   old_command=guest('cat '+old_log).stdout
   (out/'network-retired-helper.log').write_text(old_command)
   guest('ip -4 addr del 192.0.2.99/24 dev eth0');secondary_address=False
   probe_marker=None
   limit=time.monotonic()+75
   while True:
    try:
     events=probe_events();new_peer=int(probe_marker.rsplit('=',1)[1])
     ready=new_peer!=old_peer and all('public-sharing source='+str(slot)+' generation=1 phase=2 error=0' in events for slot in (0,4))
    except (AssertionError,ValueError):ready=False;probe_marker=None
    if ready:break
    assert time.monotonic()<limit;time.sleep(.3)
   assert helper_log!=old_log and application_pid()==pid
   resumed=[launch(prefix+['LiveProvidersControl',base+'/local/diagnostics/sharing-hub0-export.pdb',base+'/local/diagnostics/sharing-pdb-tables-v1/java-entities.tsv','--indices=0,4'],'integrated-recovered-menus')]
   for source in (0,4):
    for name,relative,local in [('database','PIONEER/rekordbox/export.pdb','sharing-hub0-export.pdb'),('audio','Contents/Aaliyah/Are You That Somebody/Aaliyah - Are You That Somebody-1.mp3','sharing-hub0-audio.mp3')]:
     artifact=root/'local/diagnostics'/local
     resumed.append(launch(prefix+['LocalFileServiceClient',relative,str(artifact.stat().st_size),hashlib.sha256(artifact.read_bytes()).hexdigest(),'--emulator-link-'+str(source+1)],'integrated-recovered-'+str(source)+'-'+name))
   for child in resumed:child.wait(timeout=210);assert child.returncode==0
   (out/'integrated-network-recovery.json').write_text(json.dumps({'old_peer':old_peer,'new_peer':new_peer,'fresh_helper_log_and_token':True,'same_application_pid':pid,'complete_menus_and_four_exact_files':True},indent=2))
   initial_peer=new_peer
   guest('ip -4 addr add 192.0.2.99/24 dev eth0');secondary_address=True
   limit=time.monotonic()+30
   while 'public-sharing graceful retirement complete; restart scheduled' not in probe_events():
    assert time.monotonic()<limit;time.sleep(.2)
   assert guest('test ! -e /proc/'+str(initial_peer),check=False).returncode==0
   assert not guest('ip netns list').stdout.strip()
  (out/'mod.log').write_text(probe_events())
  guest('systemctl stop '+unit)
  result={'managed_pid':initial_peer,'peer_pid':initial_peer}
 else:
  deadline=time.monotonic()+60
  while True:
   reply=guest('cat '+result_file,check=False)
   if not reply.returncode:break
   assert time.monotonic()<deadline,guest('tail -35 /tmp/azmod-community/usb-hub-support.log').stdout
   time.sleep(.3)
  result=json.loads(reply.stdout)
  (out/'probe-result.json').write_text(reply.stdout)
  (out/'mod.log').write_text(probe_events(result))
  assert result['run']==token and result['success'] and result['phase']==5,result
  assert result['root_inode' if args.allocate_service else 'namespace_inode']==expected_inode
  pid=application_pid()
  mapped=guest('grep usb-hub-support.so /proc/'+pid+'/maps | head -1').stdout.split()[0]
  assert guest('sha256sum /proc/'+pid+'/map_files/'+mapped).stdout.split()[0]==digest
 for child in (set((result['managed_pid'],result['peer_pid'])) if not args.native_status_observer else set()):
  assert child>0 and guest('test ! -e /proc/'+str(child),check=False).returncode==0
 for child in pending_clients:assert guest('test ! -e /proc/'+str(child),check=False).returncode==0
 if args.cancel_dhcp:
  assert guest('test ! -e '+ready_file).returncode==0
  assert 'hint=2' not in (out/'mod.log').read_text().rsplit('sdk-broker-probe managed launch pid=',1)[-1]
 if args.arp_conflict:
  server.wait(timeout=10);assert server.returncode==0
  observed=(out/'dhcp-server.stdout').read_text()
  assert '"DHCPDECLINE_received": true' in observed and '"no_fixture_ARP_claim_or_IPv4_use_after_conflict": true' in observed
  if args.retry_conflict:
   events=[json.loads(line) for line in observed.splitlines() if line.startswith('{')]
   assert any(e.get('reacquisition_after_decline') and e.get('new_address')=='192.0.2.4' for e in events)
   assert any(e.get('new_discovery_after_decline_seconds',-1)>=10 for e in events)
  else:
   assert guest('test ! -e '+ready_file).returncode==0
   assert 'hint=2' not in (out/'mod.log').read_text().rsplit('sdk-broker-probe managed launch pid=',1)[-1]
 if args.reject_native_grant:
  server.wait(timeout=10);assert server.returncode==0
  assert '"address": "192.0.2.15"' in (out/'dhcp-server.stdout').read_text()
  assert guest('test ! -e '+ready_file).returncode==0
  events=(out/'mod.log').read_text()
  assert 'error=99' in events and 'hint=2' not in events
 (out/'result.json').write_text(json.dumps({'SDK_launch_and_IPC':not args.native_status_observer,'native_status_observer':args.native_status_observer,'integrated_runtime':args.integrated_runtime,'integrated_native_eject':args.integrated_eject,'integrated_mode_cycle':args.integrated_mode_cycle,'integrated_network_recovery':args.integrated_network_recovery,'probe_environment_used':not (args.integrated_runtime or args.native_status_observer),'mod_sha256':digest,
    'native_complete_file_control':args.native_file_control,
    'native_fresh_local_playback_control':args.native_local_playback,
    'native_local_playback_usb':args.native_local_usb if args.native_local_playback else None,
    'integrated_whole_hub_eject':args.integrated_whole_hub,
    'whole_hub_withdrawal_only':args.whole_hub_withdrawal_only,
    'capacity_complete_menu_file_clients_tested':bool(args.capacity_source_count) and not args.whole_hub_withdrawal_only,
    'helper_sha256':helper_digest,'native_application_pid':pid,'namespace_only_acknowledged':not bool(args.service_module) and not args.native_status_observer,
    'service_active_and_drained':bool(args.service_module) and not args.reject_native_grant and not (args.cancel_dhcp or (args.arp_conflict and not args.retry_conflict)),'helper_owned_allocation':args.allocate_service,
    'native_inventory_gate_enabled':args.inventory_gate,'two_managed_sources':args.two_sources,'capacity_sources_tested':args.capacity_source_count,'duplicate_grant_rejected':args.reject_duplicate_grant,'native_grant_rejected':args.reject_native_grant,'short_grant_refresh':args.refresh_grants and not args.link_local,'eject_pending_refresh':args.eject_pending_refresh,'changed_refresh_address':args.change_refresh_address,'player_collision':args.player_collision,'player_collision_recovery':args.recover_player_collision,'dynamic_identity':args.dynamic_identity,'native_inventory_roots':args.native_inventory,'reusable_inventory_engine':args.inventory_engine,'candidate_runtime':args.candidate_runtime,'native_network_change_retirement':args.native_network_change,'native_network_recovery':args.recover_native_network,'managed_link_local':args.link_local,'native_ipv4_label':args.native_ipv4_label,'managed_link_local_conflict':args.link_local_conflict,'managed_link_local_eject':args.link_local_eject,'managed_link_local_dhcp_return':args.link_local_dhcp_return,'initial_ARP_conflict_declined':args.arp_conflict,'conflict_recovery_same_identity':args.retry_conflict,
    'pending_DHCP_peer_loss_drained':args.cancel_dhcp,'pending_clients':pending_clients,
    'independent_real_library_clients':bool(args.library_root) and not args.reject_native_grant and not (args.cancel_dhcp or (args.arp_conflict and not args.retry_conflict)),
    'managed_and_peer_processes_gone':True,'physical_AZ_touched':False},indent=2))
 print('NATIVE_STATUS_OBSERVER_SCOPE_PASS' if args.native_status_observer else 'INTEGRATED_SHARING_RUNTIME_PASS' if args.integrated_runtime else 'SDK_MANAGED_HELPER_IPC_PASS',flush=True)
finally:
 if native_mode_off:
  try:native_link_choice(1);native_mode_off=False
  except (AssertionError,subprocess.SubprocessError):print('NATIVE_MODE_RESTORE_PENDING; continuing mandatory package/network cleanup',flush=True)
 if not probe_marker:
  completion=guest('cat '+result_file,check=False)
  if not completion.returncode:
   try:probe_events(json.loads(completion.stdout))
   except (AssertionError,ValueError,subprocess.TimeoutExpired):pass
 if probe_marker:
  try:(out/'scoped-mod.log').write_text(probe_events())
  except (AssertionError,subprocess.TimeoutExpired):pass
 if helper_log:
  reply=guest('cat '+helper_log,check=False)
  if not reply.returncode:(out/'managed-helper.log').write_text(reply.stdout)
 if args.recover_native_network and helper_log:
  recovered_log=helper_log[:-4]+'-r1.log'
  recovered=guest('cat '+recovered_log,check=False)
  if not recovered.returncode:(out/'recovered-helper.log').write_text(recovered.stdout)
 if changed:
  guest('systemctl stop '+unit,check=False)
  if native_link_local:
   guest('ip -4 addr del 169.254.2.15/16 dev eth0');native_link_local=False
  for i,path in enumerate(restore_files):
   if hashes[i]=='ABSENT':guest('rm -f -- '+path)
   else:
    guest('cp '+backups+'/'+str(i)+' '+path)
    assert guest('sha256sum '+path).stdout.split()[0]==hashes[i]
  if secondary_address:
   guest('ip -4 addr del 192.0.2.99/24 dev eth0');secondary_address=False
  guest('systemctl start EP147')
  native_after={key:guest(cmd).stdout for key,cmd in [('address','ip -o -4 addr show dev eth0'),('routes','ip -4 route show'),('dns','sha256sum /etc/resolv.conf'),('namespace','ls -Lid /proc/self/ns/net'),('parent_flags','cat /sys/class/net/eth0/flags')]}
  native_state_matches=native_after==native_before
  (out/'native-after.json').write_text(json.dumps(native_after,indent=2))
  print('ORIGINAL_BYTES_RESTORED',flush=True)
 if native_link_local:
  guest('ip -4 addr del 169.254.2.15/16 dev eth0');native_link_local=False
 if namespace:
  assert not guest('ip netns pids azhub-sdk-probe').stdout.strip()
  guest('ip netns del azhub-sdk-probe')
 if link:guest('ip link del azpub7')
 if module:guest('rmmod azhub_test_macvlan')
 if fixture:
  if bound:
   mounted=any(line.split()[1]=='/tmp/azhub-sdk-library' for line in guest('cat /proc/mounts').stdout.splitlines())
   if mounted:guest('umount /tmp/azhub-sdk-library')
   guest('rmdir /tmp/azhub-sdk-library')
  else:
   guest('rm -f /tmp/azhub-sdk-library/PIONEER/rekordbox/export.pdb')
   for path in ('/tmp/azhub-sdk-library/PIONEER/rekordbox','/tmp/azhub-sdk-library/PIONEER','/tmp/azhub-sdk-library'):guest('rmdir '+path)
 if bridge and bridge.poll() is None:
  e.qmp('object-del',{'id':'azhubtapguard'});bridge.terminate();bridge.wait(timeout=10)
  e.qmp('chardev-remove',{'id':'azhubtapnat'})
 if host_scope:
  for pid in host('ip','netns','pids','azhub-link').stdout.split():
   assert pid.isdigit();host('kill','-TERM',pid,check=False)
  for child in children:
   if child.poll() is None:child.terminate();child.wait(timeout=10)
  host('ip','netns','del','azhub-link')
 if hashes:
  for i,value in enumerate(hashes):
   if value!='ABSENT':guest('rm -f '+backups+'/'+str(i))
  guest('rmdir '+backups)
 guest('rm -f '+remote+'.so '+remote+'.so.new '+remote+'-helper '+remote+'-helper.new '+remote+'.ko '+remote+'.ko.new '+result_file+' '+ready_file+' '+done_file+' /run/azhub-sdk-service-'+token+'.multi')
 if (out/'mode-old-helper-path.txt').exists():
  old_log=(out/'mode-old-helper-path.txt').read_text();assert old_log.startswith(owners[-1]+'/data/public-sharing-') and old_log.endswith('.log')
  guest('rm -f '+old_log)
 if helper_log:
  guest('rm -f '+helper_log)
  if args.recover_native_network:guest('rm -f '+helper_log[:-4]+'-r1.log')
 (out/'guest-commands.json').write_text(json.dumps(commands,indent=2))
 for handle in handles:handle.close()
 print('OWNED_SCOPE_CLEANED',flush=True)
 if changed:assert native_state_matches,'Native network state changed; owned cleanup already completed'
 if native_mode_off:raise AssertionError('Native Link On restoration remains pending after owned cleanup')

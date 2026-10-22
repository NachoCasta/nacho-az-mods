#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Bounded synthetic remote-loaded status control; optional explicit native eject.

Uses only the existing localhost emulator and an owned isolated TAP. Records
wire headers and screenshots for separate native-admission review. Emitted
status is not decoded audio or an actual CDJ's loaded-track acceptance.
"""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--classes',required=True,type=Path)
p.add_argument('--source',choices=('native1','native2'),required=True)
p.add_argument('--seconds',type=int,choices=(30,60,120),default=60)
p.add_argument('--evidence-directory',required=True,type=Path)
p.add_argument('--hold-file',action='store_true',help='Read repeatedly through one real native NFS handle while loaded status continues; no eject')
p.add_argument('--native-eject',action='store_true',help='Explicit native STOP1 baseline with accepted status/held handle; restores owned virtual fixture')
p.add_argument('--unit',default='EP147',help='Existing owned emulator EP147 service or SDK probe unit')
args=p.parse_args();root=Path.cwd().resolve();out=args.evidence_directory.resolve()
assert re.fullmatch(r'EP147(?:-sdk-probe-[0-9a-f]{32})?',args.unit)
if args.native_eject:assert args.hold_file and args.seconds==120
if args.hold_file:assert args.source=='native1','Native /C/ control requires source1 USB slot3'
assert os.name=='nt' and out.is_relative_to(root/'local/diagnostics') and not out.exists()
classes=args.classes.resolve();assert classes.is_relative_to(root) and classes.is_dir()
out.mkdir(parents=True)
spec=importlib.util.spec_from_file_location('emulator',Path(__file__).with_name('eject-ui-emu.py'))
emulator=importlib.util.module_from_spec(spec);spec.loader.exec_module(emulator);az=emulator.azemu
assert az.SSH_PORT==2222 and az.QMP==('127.0.0.1',4446)
base='/mnt/c/'+root.as_posix()[3:];children=[];handles=[];owned=False;bridge=None;withdrawal=None
def host(*command):
 return subprocess.run(['wsl.exe','-u','root','--exec',*command],capture_output=True,text=True,check=True,timeout=20).stdout
def guest(command):
 code,output=az.ssh(command);assert code==0,(command,output);return output
def snapshot():
 main=guest('systemctl show '+args.unit+' -p MainPID').strip().split('=')[1];assert main.isdigit()
 pid=guest('pgrep -P '+main+' -x EP147').strip();assert pid.isdigit()
 assert guest('readlink /proc/'+pid+'/exe').strip().endswith('/EP147')
 return {key:guest(command) for key,command in {
  'pid':'echo '+pid,'modules':'cat /proc/modules','mounts':'cat /proc/mounts',
  'address':'ip -o -4 addr show dev eth0','routes':'ip -4 route show',
  'dns':'sha256sum /etc/resolv.conf','loaded':'cat /tmp/azmod-community/loaded'}.items()}
def launch(command,name):
 stdout=(out/(name+'.stdout')).open('wb');stderr=(out/(name+'.stderr')).open('wb');handles.extend((stdout,stderr))
 child=subprocess.Popen(command,stdout=stdout,stderr=stderr,stdin=subprocess.DEVNULL);children.append(child);return child
try:
 assert guest('uname -r').strip()=='6.6.138-dirty'
 before=snapshot();(out/'native-before.json').write_text(json.dumps(before,indent=2))
 if args.native_eject:
  helper_spec=importlib.util.spec_from_file_location('withdrawal',Path(__file__).with_name('native-loaded-withdrawal-control.py'))
  helper=importlib.util.module_from_spec(helper_spec);helper_spec.loader.exec_module(helper)
  withdrawal=helper.NativeWithdrawal(emulator,guest,out,before['pid'])

 assert not host('ip','netns','list').strip()
 host('ip','netns','add','azhub-link');owned=True
 for command in (
  ['ip','netns','exec','azhub-link','ip','tuntap','add','dev','azlink0','mode','tap'],
  ['ip','-n','azhub-link','link','set','lo','up'],
  ['ip','-n','azhub-link','addr','add','192.0.2.2/24','brd','192.0.2.255','dev','azlink0'],
  ['ip','-n','azhub-link','link','set','azlink0','address','02:00:00:00:02:02'],
  ['ip','-n','azhub-link','link','set','azlink0','up']):host(*command)
 bridge=launch([sys.executable,str(Path(__file__).with_name('link-primary-tap-transport.py')),'--seconds',str(args.seconds+60)],'transport')
 until=time.monotonic()+15
 while '"ready": true' not in (out/'transport.stdout').read_text():
  assert bridge.poll() is None and time.monotonic()<until;time.sleep(.1)
 prefix=['wsl.exe','-u','root','--exec','ip','netns','exec','azhub-link']
 observer=launch(prefix+['python3',base+'/src/azhub/research/scripts/link-packet-observer.py','--seconds',str(args.seconds+20)],'wire')
 peer=launch(prefix+['java','-cp',base+'/'+classes.relative_to(root).as_posix()+':'+base+'/local/link-peer-build/dependencies/*',
  'LoadedDeckStatusControl','--native-source'+args.source[-1],str(args.seconds)],'peer')
 until=time.monotonic()+25
 while 'READY:' not in (out/'peer.stdout').read_text():
  assert peer.poll() is None and time.monotonic()<until,(out/'peer.stderr').read_text();time.sleep(.2)
 held=None
 if args.hold_file:
  assert args.source=='native1','Native /C/ control requires source1 USB slot3'
  held=launch(prefix+['python3',base+'/src/azhub/research/scripts/link-nfs-handle-peer.py','--samples','9','--interval','5' if args.native_eject else '2','--mount-wait','30'],'held-file')

 if withdrawal:
  deadline=time.monotonic()+40
  while True:
   events=[json.loads(line) for line in (out/'held-file.stdout').read_text().splitlines()]
   if any(row.get('event')=='read' and row.get('sample')==0 and row.get('nfs_status')==0 and row.get('bytes')==1024 for row in events):break
   assert held.poll() is None and peer.poll() is None and time.monotonic()<deadline
   time.sleep(.2)
  withdrawal.withdraw()
  assert peer.poll() is None,'External loaded-status sender must remain live after STOP1'
 print('LOADED_STATUS_LIVE_FOR_EMULATOR_UI_REVIEW '+str(out),flush=True)
 subprocess.run([sys.executable,'C:/Users/icast/xdj-az-emu/tools/az-drive.py','shot','native-status-stimulus'],env=dict(os.environ,AZ_SHOT_DIR=str(out)),check=True)
 peer.wait(timeout=args.seconds+10);assert peer.returncode==0,(out/'peer.stderr').read_text()
 if held:
  held.wait(timeout=10);assert held.returncode==0,(out/'held-file.stderr').read_text()
  held_events=[json.loads(line) for line in (out/'held-file.stdout').read_text().splitlines()]
  reads=[row for row in held_events if row.get('event')=='read']
  assert len(reads)==9,reads
  if withdrawal:
   assert reads[0].get('nfs_status')==0 and reads[0].get('bytes')==1024
   statuses=[row.get('nfs_status') for row in reads]
   assert all(value in (0,70) for value in statuses) and statuses.count(70)>=4,statuses
   first_stale=statuses.index(70);assert all(value==70 for value in statuses[first_stale:]),statuses
   withdrawal.neighbors()
   withdrawal.note('retained_handle_STALE',statuses=statuses)
  else:
   assert all(row.get('nfs_status')==0 and row.get('bytes')==1024 for row in reads),reads

 observer.wait(timeout=25);assert observer.returncode==0
 rows=[json.loads(line) for line in (out/'wire.stdout').read_text().splitlines()]
 packets=[row for row in rows if row.get('source')=='192.0.2.2' and row.get('destination')=='192.0.2.15' and
  row.get('link_type')==10 and row.get('destination_port')==50002 and row.get('byte_21')==3 and
  row.get('byte_24')==3 and row.get('byte_28')==int(args.source[-1]) and row.get('byte_29')==3 and row.get('word_2c')==1884]
 assert len(packets)>=10
 if withdrawal:withdrawal.restore()
 after=snapshot();(out/'native-after.json').write_text(json.dumps(after,indent=2))
 if withdrawal:
  assert sorted(before['mounts'].splitlines())==sorted(after['mounts'].splitlines())
  assert {k:v for k,v in before.items() if k!='mounts'}=={k:v for k,v in after.items() if k!='mounts'}
 else:assert before==after
 (out/'result.json').write_text(json.dumps({'scope':('Native loaded-source STOP1 withdrawal with synthetic status and retained real NFS handle; not audio or physical-CDJ acceptance' if withdrawal else 'Synthetic loaded/playing status wire stimulus, not native admission/eject/audio or physical-CDJ acceptance'),
  'source_identity':int(args.source[-1]),'requester':3,'track_id':1884,'matching_wire_packets':len(packets),
  'peer_exit':peer.returncode,'observer_exit':observer.returncode,'native_snapshot_unchanged':True,
  'held_native_file_handle_reads':9 if args.hold_file else 0,
  'native_retained_handle_statuses':[row.get('nfs_status') for row in reads] if withdrawal else [],
  'native_admission_instrumented':bool(withdrawal),'eject_attempted':bool(withdrawal),'physical_AZ_touched':False},indent=2))
 print('LOADED_STATUS_WIRE_STIMULUS_PASS',flush=True)
finally:
 restoration_error=None
 if withdrawal:
  try:withdrawal.restore()
  except Exception as error:restoration_error=error
 if bridge and bridge.poll() is None:
  emulator.qmp('object-del',{'id':'azhubtapguard'});bridge.terminate();bridge.wait(timeout=10)
  emulator.qmp('chardev-remove',{'id':'azhubtapnat'})
 if owned:
  for pid in host('ip','netns','pids','azhub-link').split():
   assert re.fullmatch(r'[0-9]+',pid)
   subprocess.run(['wsl.exe','-u','root','--exec','kill','-TERM',pid],capture_output=True)
  for child in children:
   if child.poll() is None:child.terminate();child.wait(timeout=10)
  host('ip','netns','del','azhub-link')
 for handle in handles:handle.close()
 print('OWNED_STATUS_SCOPE_CLEANED',flush=True)
 if restoration_error:raise restoration_error

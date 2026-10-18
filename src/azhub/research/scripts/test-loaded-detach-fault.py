#!/usr/bin/env python3
"""Test the explicitly staged localhost delayed-native-receipt overlay; no physical AZ access.

Run from the repository root. Output stays in ignored local/diagnostics.
"""
import importlib.util,json,hashlib,re,time
from pathlib import Path
s=importlib.util.spec_from_file_location('t','src/azhub/research/scripts/test-safe-eject-emulator.py');t=importlib.util.module_from_spec(s);s.loader.exec_module(t)
assert t.ssh('uname -r').strip()=='6.6.138-dirty' and t.empty_decks()
base=Path('local/diagnostics/loaded-late-completion');binary=base/'source/out/usb-hub-support.so';digest=hashlib.sha256(binary.read_bytes()).hexdigest();assert t.ssh('sha256sum '+t.STAGE).split()[0]==digest
pid=t.current_pid();start=len(t.logs());t.center_knob();t.browse_a();marker=len(t.logs());t.load(1,delay=1)
t.wait_for(lambda:t.status()['decks'][0]['loaded']==1,'loaded fault target');t.wait_for(lambda:re.search(r'(?m)^safe-eject load-completed controller=.* result=(0|4)$',t.logs()[marker:]),'native load completion')
source=int(re.search(r'load-admitted source=(\d+)',t.logs()[marker:]).group(1));assert source in (0,1)
label=('HUB_LIB','HUB_LIB2')[source];disk=('sdb','sdc')[source];other=('sdc','sdb')[source];t.assert_load_disk(marker,1,disk)
def refusal():
 mark=t.request(label);t.wait_for(lambda:'USB tracks are still closing. Leave it connected and retry.' in t.logs()[mark:],'delayed receipt timeout')
 assert t.state_is(label,3) and t.present(disk) and t.present(other) and t.present('sda')
 assert 'safe to remove' not in t.logs()[mark:] and 'safe-eject withdrawal' not in t.logs()[mark:]
 return {'status':t.status(),'events':t.logs()[mark:]}
first=refusal();time.sleep(2);second=refusal()
events=t.logs()[start:];assert events.count('safe-eject native-detach ticket=')==1,events
# Release delivery of the actual recorded native owner/media/task serial. The
# native body already ran; never execute it twice to simulate delayed delivery.
t.ssh('test "$(uname -r)" = 6.6.138-dirty && touch /tmp/azhub-detach-test-release')
mark=t.request(label);t.wait_for(lambda:t.state_is(label,2),'retry consumes delayed native receipt')
assert not t.present(disk) and t.present(other) and t.present('sda') and t.status()['decks'][0]['loaded']==1
assert t.logs()[start:].count('safe-eject native-detach ticket=')==1
result={'scope':'Deliberate emulator-only delayed delivery overlay, not normal binary acceptance','base_binary_sha256':'9feec49c06f10dd01e191ed5094f68eb1e7e7b35cd9ed76d757a8477a37a945b','fault_binary_sha256':digest,'fault':'Forward actual native completion; hold its receipt outside pool; 1.5 second test timeout','pid':int(pid),'source_disk':disk,'first_timeout':first,'second_timeout':second,'released_retry_status':t.status(),'events':t.logs()[start:].splitlines(),'checks':['Native callback timeout never reports safe or starts server withdrawal','Backing remains mounted; other source and native USB1 remain available','Repeated pending retry does not enqueue native disconnect twice','Releasing the recorded actual receipt lets retry complete ordinary eject with track retained'],'real_az_access':False}
Path('docs/acceptance/usb-hub-support-0.2.1-native-timeout.json').write_text(json.dumps(result,indent=2)+'\n')
t.unload(1);assert t.empty_decks() and t.current_pid()==pid
print('PASS delayed actual native receipt: two refusals, one enqueue, successful released retry',flush=True)

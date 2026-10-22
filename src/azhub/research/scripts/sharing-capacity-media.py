#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Explicit owned emulator media attachment/removal; never a physical AZ tool.

Removal requires all partitions of each owned disk to be unmounted first through
the normal eject UI. No unmount, service stop, settings edit or forced removal.
"""
import argparse
import importlib.util
import json
from pathlib import Path
import re
import time
import uuid


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action',choices=('inspect','attach','detach','replug-all'))
    parser.add_argument('--eject-owned',action='store_true',help='Before detach, use the normal emulator selector to safely eject only mounted owned additions')
    args=parser.parse_args()
    assert not args.eject_owned or args.action=='detach'
    root=Path.cwd().resolve()
    directory=root/'local/integration/sharing-eight-source-overlays-v1'
    receipt=directory/'attached.json'
    spec=importlib.util.spec_from_file_location('capacity_emu',Path(__file__).with_name('eject-ui-emu.py'))
    emu=importlib.util.module_from_spec(spec);spec.loader.exec_module(emu)
    assert emu.azemu.SSH_PORT==2222 and emu.azemu.QMP==('127.0.0.1',4446)

    def guest(command):
        code,out=emu.azemu.ssh(command,timeout=20)
        assert code==0,(command,code,out)
        return out.strip()

    def qmp(command,arguments=None):
        reply=emu.qmp(command,arguments or {})
        assert 'error' not in reply,reply
        return reply['return']

    assert guest('uname -r')=='6.6.138-dirty'
    assert guest('cat /sys/bus/usb/devices/3-2/maxchild')=='8'
    blocks=qmp('query-block')
    summary=[{'node':x.get('inserted',{}).get('node-name'),'device':x.get('qdev'),
              'file':x.get('inserted',{}).get('file')} for x in blocks]
    if args.action=='inspect':
        print(json.dumps({'blocks':summary,'owned_receipt_exists':receipt.exists()},indent=2));return
    assert not guest("systemctl list-units --no-legend --state=running 'EP147-sdk-probe-*'"),'Managed test still running'
    state=json.loads(guest('cat /run/azhub-eject-status'))
    assert all(not deck['loaded'] for deck in state['decks']),'Empty emulator decks required'

    if args.action=='replug-all':
        # Restore deliberately ejected virtual media without replacing their
        # backends. No unmount, force, native-port mutation or file operation.
        attached=json.loads(receipt.read_text());assert attached['complete'] and not attached.get('detached')
        mounts=guest('cat /proc/self/mountinfo').splitlines()
        native=[line for line in guest('cat /proc/mounts').splitlines() if line.split()[1] in ('/media/usb/sda1','/media/usb/sdb1')]
        assert len(native)==2
        plan=[]
        for number in range(1,9):
            device='functionalstick'+'ABCDE'[number-1] if number<=5 else 'capacitystick'+str(number)
            matches=[x for x in summary if x['device'] and '/'+device+'/' in x['device']]
            assert len(matches)==1 and matches[0]['node']
            assert qmp('qom-get',{'path':'/machine/peripheral/'+device,'property':'port'})=='2.'+str(number)
            disks=guest('for p in /sys/bus/usb/devices/3-2.'+str(number)+'/*/host*/target*/*/block/*; do test ! -e "$p" || basename "$p"; done').splitlines()
            assert len(disks)==1 and re.fullmatch(r'sd[a-z]+',disks[0])
            disk=disks[0];ids=guest('cat /sys/class/block/'+disk+'/dev /sys/class/block/'+disk+'/'+disk+'*/dev').splitlines()
            assert all(line.split()[2] not in ids for line in mounts),'All eight media must be safely ejected first'
            plan.append({'port':number,'device':device,'node':matches[0]['node'],'disk':disk,'removed':False,'added':False})
        journal=directory/('replug-all-'+uuid.uuid4().hex+'.json')
        result={'scope':'Owned emulator hub media replug only','physical_AZ_touched':False,'devices':plan,'complete':False}
        def record():journal.write_text(json.dumps(result,indent=2)+'\n')
        record()
        for entry in plan:
            qmp('device_del',{'id':entry['device']});deadline=time.monotonic()+45
            while guest('test ! -e /sys/bus/usb/devices/3-2.'+str(entry['port'])+' && test ! -e /sys/class/block/'+entry['disk']+' && echo absent || true')!='absent':
                assert time.monotonic()<deadline;time.sleep(.3)
            entry['removed']=True;record()
        time.sleep(1)
        for entry in plan:
            arguments={'driver':'usb-storage','id':entry['device'],'drive':entry['node'],'bus':'functionalxhci.0','port':'2.'+str(entry['port'])}
            if entry['port']>=6:arguments['serial']='AZHUB-CAPACITY-'+str(entry['port'])
            qmp('device_add',arguments);entry['added']=True;record()
            deadline=time.monotonic()+45
            while guest('test -e /sys/bus/usb/devices/3-2.'+str(entry['port'])+' && echo present || true')!='present':
                assert time.monotonic()<deadline;time.sleep(.3)
        deadline=time.monotonic()+150
        while True:
            after=guest('cat /proc/mounts').splitlines();assert set(native).issubset(after)
            if all('/media/usb/'+entry['disk']+'1' in {line.split()[1] for line in after} for entry in plan):break
            assert time.monotonic()<deadline,'Hub media remount pending after virtual replug'
            time.sleep(1)
        result['complete']=True;result['native_mounts_preserved']=native;record()
        print(json.dumps({'receipt':str(journal),'result':result},indent=2));return

    def save(data):
        temporary=receipt.with_suffix('.tmp')
        temporary.write_text(json.dumps(data,indent=2)+'\n');temporary.replace(receipt)

    if args.action=='attach':
        assert not receipt.exists(),'Existing owned attachment receipt requires inspection'
        devices=qmp('qom-list',{'path':'/machine/peripheral'})
        names={x['name'] for x in devices}
        assert 'functionalhub' in names
        assert all('functionalstick'+letter in names for letter in 'ABCDE')
        assert all('capacitystick'+str(n) not in names for n in (6,7,8))
        assert not guest('for n in 6 7 8; do test ! -e /sys/bus/usb/devices/3-2.$n || echo occupied; done')
        data={'physical_AZ_touched':False,'devices':[],'complete':False}
        save(data)
        for number in (6,7,8):
            image=(directory/('source'+str(number)+'.qcow2')).resolve()
            assert image.is_relative_to(directory.resolve()) and image.is_file()
            node='sharingcapacity'+str(number);device='capacitystick'+str(number)
            entry={'port':number,'node':node,'device':device,'image':str(image),'block_added':False,'device_added':False}
            data['devices'].append(entry);save(data)
            qmp('blockdev-add',{'driver':'qcow2','node-name':node,'file':{'driver':'file','filename':image.as_posix()}})
            entry['block_added']=True;save(data)
            qmp('device_add',{'driver':'usb-storage','id':device,'drive':node,'bus':'functionalxhci.0',
                              'port':'2.'+str(number),'serial':'AZHUB-CAPACITY-'+str(number)})
            entry['device_added']=True;save(data)
            deadline=time.monotonic()+45
            while guest('test -e /sys/bus/usb/devices/3-2.'+str(number)+' && echo present || true')!='present':
                assert time.monotonic()<deadline,'Owned USB did not enumerate'
                time.sleep(.5)
        data['complete']=True;save(data)
    else:
        data=json.loads(receipt.read_text())
        assert data['physical_AZ_touched'] is False
        for entry in reversed(data['devices']):
            number=entry['port'];assert number in (6,7,8)
            assert entry['node']=='sharingcapacity'+str(number) and entry['device']=='capacitystick'+str(number)
            if entry['device_added']:
                matching=[x for x in summary if x['node']==entry['node']]
                assert len(matching)==1 and '/'+entry['device']+'/' in matching[0]['device']
                assert Path(matching[0]['file']).resolve()==Path(entry['image']).resolve()
                # Bind aliases retain the same major:minor as the partition.
                # Check mountinfo device identities, not only /dev/sdX text.
                disks=guest('for p in /sys/bus/usb/devices/3-2.'+str(number)+'/*/host*/target*/*/block/*; do test ! -e "$p" || basename "$p"; done').splitlines()
                assert len(disks)==1 and re.fullmatch(r'sd[a-z]+',disks[0]),disks
                disk=disks[0]
                ids=guest('cat /sys/class/block/'+disk+'/dev /sys/class/block/'+disk+'/'+disk+'*/dev').splitlines()
                mounts=guest('cat /proc/self/mountinfo').splitlines()
                if args.eject_owned and any(line.split()[2] in ids for line in mounts):
                    spec=importlib.util.spec_from_file_location('capacity_eject',Path(__file__).with_name('test-safe-eject-emulator.py'))
                    suite=importlib.util.module_from_spec(spec);spec.loader.exec_module(suite)
                    assert suite.empty_decks() and suite.current_pid()
                    suite.center_knob()
                    items=[item for item in suite.status()['devices'] if item['port']==2]
                    index=next(i for i,item in enumerate(items) if item['disk']==disk)
                    marker=len(suite.logs());suite.stop()
                    suite.wait_for(lambda:'eject-ui selector port=2' in suite.logs()[marker:],'owned media selector')
                    if index:suite.drive('knob','cw',index)
                    suite.drive('knobclick',delay=.1)
                    suite.wait_for(lambda:not suite.present(disk),'owned media safe backing unmount')
                    entry['normal_selector_eject_completed']=True;save(data)
                    mounts=guest('cat /proc/self/mountinfo').splitlines()
                assert all(line.split()[2] not in ids for line in mounts),'Safely eject the owned USB first'
                qmp('device_del',{'id':entry['device']})
                deadline=time.monotonic()+30
                while guest('test ! -e /sys/bus/usb/devices/3-2.'+str(number)+' && echo absent || true')!='absent':
                    assert time.monotonic()<deadline
                    time.sleep(.5)
                entry['device_added']=False;save(data)
            if entry['block_added']:
                qmp('blockdev-del',{'node-name':entry['node']})
                entry['block_added']=False;save(data)
        data['complete']=False;data['detached']=True;save(data)
    print(json.dumps(data,indent=2))


if __name__=='__main__':main()

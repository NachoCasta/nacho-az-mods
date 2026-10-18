#!/usr/bin/env python3
"""Emulator-only cached audio and different-stick replacement test.

Requires both original hub sticks safely ejected and empty decks. Replaces virtual
A with a separate protected copy of B, tests two cached media generations, then
restores original A. Optional audio stays under ignored local/ and is deleted
once numeric measurements are extracted. Never installs or accesses the real AZ.
"""
import argparse
import array
import datetime
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import re
import subprocess
import sys
import time
import uuid
import wave
spec=importlib.util.spec_from_file_location("suite",Path(__file__).with_name("test-safe-eject-emulator.py"))
t=importlib.util.module_from_spec(spec);spec.loader.exec_module(t)

def run(binary,output,audio_dir):
    assert t.ssh("uname -r").strip()=="6.6.138-dirty"
    digest=hashlib.sha256(binary.read_bytes()).hexdigest();assert t.ssh("sha256sum "+t.STAGE).split()[0]==digest
    pid=t.current_pid();assert t.empty_decks() and t.present("sda") and not t.present("sdb") and not t.present("sdc")
    start=len(t.logs());observations=[]
    def device(disk):return next((x for x in t.status()["devices"] if x["disk"]==disk),{})
    def snapshot(name):
        data={"case":name,"status":t.status(),"mounts":t.mounts()};observations.append(data);return data
    def wait(predicate,name):
        def alive():
            assert t.current_pid()==pid,"Unexpected restart"
            return predicate()
        return t.wait_for(alive,name)
    def load_a(deck):
        t.center_knob();t.browse_a();marker=len(t.logs());t.load(deck,delay=1)
        wait(lambda:t.status()["decks"][deck-1]["loaded"]==1,"known A/clone track loaded")
        wait(lambda:re.search(r"(?m)^safe-eject load-completed controller=.* result=(0|4)$",t.logs()[marker:]),"native completion")
        assert t.status()["decks"][deck-1]["content_id"]==495;t.assert_load_disk(marker,deck,"sdb")
        match=re.search("load-request deck="+str(deck)+r" location=(0x[0-9a-f]+):",t.logs()[marker:]);assert match
        return int(match.group(1),16)&0xffffffffffff
    def request_disk(disk):
        items=t.status()["devices"];index=next(i for i,x in enumerate(items) if x["disk"]==disk)
        marker=len(t.logs());t.stop();wait(lambda:"eject-ui selector port=2" in t.logs()[marker:],"physical selector")
        if index:t.drive("knob","cw",index)
        t.drive("knobclick",delay=.1)
        def complete():
            events=t.logs()[marker:]
            if "safe-eject result:" in events and "safe to remove" not in events:raise AssertionError(events)
            return device(disk).get("state")==2
        wait(complete,"selected disk safe");assert not t.present(disk)
    def play(deck):
        if not t.status()["decks"][deck-1]["playing"]:t.drive("press",deck,"play")
        wait(lambda:t.status()["decks"][deck-1]["playing"]==1,"playing cached track")
    t.replug("A");old_media=load_a(1);play(1)
    wait(lambda:t.status()["decks"][0]["pos_s"]>2,"A playback before eject")
    request_disk("sdb");assert not t.present("sdc") and t.present("sda")
    snapshot("all hub backings absent before cached seek and audio")
    before=t.status()["decks"][0]["pos_s"];t.drive("press",1,"search+",2.5)
    wait(lambda:t.status()["decks"][0]["pos_s"]>before+10,"cached seek")
    audio=None
    if audio_dir:
        root=Path(__file__).resolve().parents[4]
        audio_dir=audio_dir.resolve();assert audio_dir.is_relative_to(root/"local"),"Music capture must stay in ignored local/"
        audio_dir.mkdir(parents=True,exist_ok=True);path=audio_dir/("cached-"+uuid.uuid4().hex+".wav")
        tool=t.emu.tools/"az-capture.py";before_capture=snapshot("cached audio start")
        assert all(x["state"]==2 for x in before_capture["status"]["devices"])
        subprocess.run([sys.executable,str(tool),"start",str(path)],check=True)
        try:
            position=t.status()["decks"][0]["pos_s"]
            wait(lambda:t.status()["decks"][0]["pos_s"]>position+5,"linear cached playback during sample capture")
        finally:subprocess.run([sys.executable,str(tool),"stop"],check=True)
        after_capture=snapshot("cached audio end")
        assert before_capture["status"]["pid"]==after_capture["status"]["pid"]
        assert [x["generation"] for x in before_capture["status"]["devices"]]==[x["generation"] for x in after_capture["status"]["devices"]]
        assert not t.present("sdb") and not t.present("sdc")
        info=dict(line.split("=",1) for line in t.ssh("cat /tmp/master_out.capture.txt").splitlines() if "=" in line)
        with wave.open(str(path),"rb") as wav:
            assert wav.getnchannels()==8 and wav.getsampwidth()==4 and wav.getframerate()==44100
            data=array.array("i",wav.readframes(wav.getnframes()));statistics=[]
            for channel in range(8):
                values=data[channel::8]
                statistics.append({"channel":channel,"rms_full_scale":math.sqrt(sum(x*x for x in values)/max(1,len(values)))/(2**31),"peak_full_scale":max((abs(x) for x in values),default=0)/(2**31)})
            audio={"seconds":wav.getnframes()/44100,"format":"8-channel PCM32 at 44100 Hz; pre-mixer","dropped_blocks":int(info["dropped"]),"statistics":statistics,"raw_music_deleted":True}
        path.unlink()
        assert audio["dropped_blocks"]==0 and all(statistics[i]["rms_full_scale"]>1e-5 for i in (0,1)),audio
        assert all(statistics[i]["rms_full_scale"]==0 for i in range(2,8)),audio
    # A is already unmounted. Replace it with a different read-only backing,
    # not its old image: duplicate labels must not select the wrong device.
    base=t.emu.tools.parent/"build"
    t.snapshot_drive("ejectdriveA",base/"usblib.img",create=False)
    t.qmp("device_del",{"id":"ejectstickA"})
    wait(lambda:t.emu.azemu.ssh("test ! -e /sys/bus/usb/devices/2-1.1")[0]==0,"old A removed")
    t.snapshot_drive("ejectswapA",base/"hublib2.img")
    t.qmp("device_add",{"driver":"usb-storage","id":"ejectstickA","drive":"ejectswapA","bus":"ejectxhci.0","port":"1.1"})
    wait(lambda:t.present("sdb") and device("sdb").get("state")==0 and device("sdb").get("label")=="HUB_LIB2","different stick at original A port")
    assert device("sdc")["state"]==2 and t.status()["decks"][0]["playing"]
    snapshot("different physical image connected while old cached track plays")
    new_media=load_a(2);assert new_media!=old_media,"Distinct opaque native contexts required"
    play(2);wait(lambda:t.status()["decks"][1]["pos_s"]>2,"replacement track plays")
    snapshot("old and replacement media loaded on different decks")
    request_disk("sdb")
    assert t.present("sda") and not t.present("sdb") and not t.present("sdc")
    wait(lambda:t.status()["decks"][0]["playing"] and t.status()["decks"][1]["playing"],"both cached contexts survive replacement eject")
    snapshot("replacement unmounted with both cached contexts retained")
    for deck in (1,2):
        if t.status()["decks"][deck-1]["playing"]:t.drive("press",deck,"play")
        t.unload(deck)
    assert t.empty_decks()
    t.replug("A");wait(lambda:t.present("sdb") and device("sdb").get("label")=="HUB_LIB","original A image restored")
    request_disk("sdb")
    assert t.empty_decks() and t.present("sda") and t.current_pid()==pid
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text(json.dumps({"timestamp_utc":datetime.datetime.now(datetime.timezone.utc).isoformat(),"environment":"Local QEMU only; explicit protected COW backings","binary_sha256":digest,"pid":int(pid),"old_media":hex(old_media),"replacement_media":hex(new_media),"checks":["Playing A ejects and cached seek works with all hub filesystems absent","Different image on same hub port remains distinct from the old cached track","Duplicate volume labels select by disk identity rather than name","Replacement eject preserves old and new cached decks","Normal unload and original fixture restoration succeed"],"audio":audio,"observations":observations,"events":t.logs()[start:].splitlines(),"final_status":t.status()},indent=2)+"\n",encoding="utf-8")
    print("PASS cached audio and different-stick replacement",flush=True)

if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary",type=Path,required=True);parser.add_argument("--output",type=Path,required=True);parser.add_argument("--audio-dir",type=Path)
    args=parser.parse_args();run(args.binary,args.output,args.audio_dir)

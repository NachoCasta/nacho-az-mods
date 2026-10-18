#!/usr/bin/env python3
"""Loaded-track eject checks on the explicitly prepared localhost emulator.

Requires two protected fixture sticks (HUB_LIB / HUB_LIB2), native USB1, version
0.2.1 and telemetry/load tracing. Drives controls, ejects and reconnects virtual
sticks, and unloads decks at the end. No installation, real AZ access or publishing.
"""
import argparse
import datetime
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import time

spec=importlib.util.spec_from_file_location("suite",Path(__file__).with_name("test-safe-eject-emulator.py"))
t=importlib.util.module_from_spec(spec);spec.loader.exec_module(t)
LABELS=("HUB_LIB","HUB_LIB2")
DISKS=("sdb","sdc")

def run(binary,output):
    assert t.ssh("uname -r").strip()=="6.6.138-dirty"
    digest=hashlib.sha256(binary.read_bytes()).hexdigest()
    assert t.ssh("sha256sum "+t.STAGE).split()[0]==digest
    pid=t.current_pid()
    mapping=t.ssh("grep '/usb-hub-support.so$' /proc/"+pid+"/maps | head -1").split()[0]
    assert t.ssh("sha256sum /proc/"+pid+"/map_files/"+mapping).split()[0]==digest
    assert "mod=usb-hub-support|1|0.2.1|" in t.ssh("cat /tmp/azmod-community/loaded")
    assert t.empty_decks() and all(t.present(d) for d in ("sda","sdb","sdc"))
    start=len(t.logs());checks=[];observations=[]
    def wait(predicate,name,timeout=90):
        def alive():
            assert t.current_pid()==pid,"Application restarted during test"
            return predicate()
        return t.wait_for(alive,name,timeout)
    def snapshot(name):
        data=t.status();observations.append({"case":name,"status":data,"mounts":t.mounts()});return data
    def loaded_source(deck,row=0):
        t.center_knob();t.browse_a(source_row=row);marker=len(t.logs());t.load(deck,delay=1)
        wait(lambda:t.status()["decks"][deck-1]["loaded"]==1,"known fixture track loaded")
        wait(lambda:"load-completed" in t.logs()[marker:],"native load completed")
        assert t.status()["decks"][deck-1]["content_id"]==495
        matches=list(re.finditer(r"load-admitted source=(\d+) controller=0x[0-9a-f]+ deck="+str(deck),t.logs()[marker:]))
        assert matches,"Missing independent native admission trace"
        source=int(matches[-1].group(1));assert source in (0,1)
        # Calibrate either native row order, but never accept a title/CID as
        # physical identity. This separately checks actual alias device numbers.
        t.assert_load_disk(marker,deck,DISKS[source]);return source
    def request(source):
        marker=t.request(LABELS[source])
        def complete():
            events=t.logs()[marker:]
            if "safe-eject result:" in events and "safe to remove" not in events:raise AssertionError(events)
            return t.state_is(LABELS[source],2)
        wait(complete,"individual loaded-track eject")
        assert not t.present(DISKS[source]) and t.present("sda")
        return marker
    def play(deck):
        if not t.status()["decks"][deck-1]["playing"]:t.drive("press",deck,"play")
        wait(lambda:t.status()["decks"][deck-1]["playing"]==1,"deck playback")
    primary=loaded_source(1);other=1-primary;time.sleep(5)
    request(primary)
    assert t.present(DISKS[other]) and t.status()["decks"][0]["loaded"]==1
    snapshot("paused loaded track retained after individual unmount")
    play(1);wait(lambda:t.status()["decks"][0]["pos_s"]>3,"cached playback advances")
    before=t.status()["decks"][0]["pos_s"];t.drive("press",1,"search+",2.5)
    wait(lambda:t.status()["decks"][0]["pos_s"]>before+10,"cached SEARCH+ advances beyond local buffer")
    snapshot("search after USB unmount")
    t.drive("press",1,"loop4");time.sleep(3)
    positions=[]
    for _ in range(8):
        state=t.status()["decks"][0];assert state["loaded"] and state["playing"]
        positions.append(state["pos_s"]);time.sleep(.5)
    assert max(positions)-min(positions)<3.5 and any(b<a for a,b in zip(positions,positions[1:])),positions
    observations.append({"case":"four-beat loop after unmount","positions":positions})
    checks.append("Paused source eject retains deck; cached playback, SEARCH+ and four-beat loop work")
    assert loaded_source(2)==other,"Only the remaining hub source must load"
    play(2);wait(lambda:t.status()["decks"][1]["pos_s"]>2,"remaining source plays")
    request(other);assert not t.present(DISKS[primary])
    for deck in (0,1):assert t.status()["decks"][deck]["loaded"] and t.status()["decks"][deck]["playing"]
    wait(lambda:t.status()["decks"][1]["pos_s"]>8,"both cached decks survive all hub unmounts")
    snapshot("second source ejected while other cached deck plays")
    checks.append("Remaining source loads/plays independently; its eject preserves both loaded decks")
    for deck in (1,2):
        if t.status()["decks"][deck-1]["playing"]:t.drive("press",deck,"play")
        t.unload(deck)
    assert t.empty_decks()
    t.replug("A");t.replug("B")
    wait(lambda:all(t.present(d) for d in ("sdb","sdc")),"both fresh connections mounted")
    first=loaded_source(1);second=loaded_source(2,row=1)
    assert first!=second,"Two loaded decks must use different physical sticks"
    for deck in (1,2):play(deck)
    time.sleep(3);before=snapshot("two current-generation tracks playing before whole-hub hold")
    marker=t.whole_hub()
    wait(lambda:all(t.state_is(label,2) for label in LABELS),"whole hub with two loaded decks")
    assert not t.present("sdb") and not t.present("sdc") and t.present("sda")
    for deck in (0,1):assert t.status()["decks"][deck]["loaded"] and t.status()["decks"][deck]["playing"]
    wait(lambda:t.status()["decks"][1]["pos_s"]>before["decks"][1]["pos_s"]+3,"whole-hub cached playback continues")
    snapshot("whole hub unmounted; both decks continue")
    checks.append("Fresh-generation whole-hub hold unmounts both while both loaded decks continue")
    assert "frame filter switched off" not in t.logs()[start:]
    for deck in (1,2):
        if t.status()["decks"][deck-1]["playing"]:t.drive("press",deck,"play")
        t.unload(deck)
    assert t.empty_decks() and t.current_pid()==pid
    checks.append("Normal native unload clears both cached decks without restart")
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text(json.dumps({"timestamp_utc":datetime.datetime.now(datetime.timezone.utc).isoformat(),"environment":"Local QEMU only; protected COW images; no physical AZ access","binary_sha256":digest,"pid":int(pid),"checks":checks,"observations":observations,"events":t.logs()[start:].splitlines(),"audio_output_captured":False,"final_status":t.status()},indent=2)+"\n",encoding="utf-8")
    print("PASS loaded eject:",len(checks),"groups",flush=True)

if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary",type=Path,required=True);parser.add_argument("--output",type=Path,required=True)
    args=parser.parse_args();run(args.binary,args.output)

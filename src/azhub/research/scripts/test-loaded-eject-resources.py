#!/usr/bin/env python3
"""Observe loaded/eject/unload churn in the prepared localhost emulator only.

Requires original A/B COW fixtures and empty decks. Default mode reconnects and
loads A each cycle. load-unload mode keeps A connected throughout its cycles;
unload-first mode manually unloads before each ordinary eject/replug. Reads bounded own-mod metadata from the exact ELF mapping; reports derived
counts, never raw process memory. No installation, physical AZ or publishing.
"""
import argparse
import datetime
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import struct
import time
spec=importlib.util.spec_from_file_location("suite",Path(__file__).with_name("test-safe-eject-emulator.py"))
t=importlib.util.module_from_spec(spec);spec.loader.exec_module(t)

def symbols(binary):
    raw=binary.read_bytes();offset=struct.unpack_from("<Q",raw,40)[0];size,count=struct.unpack_from("<HH",raw,58)
    sections=[struct.unpack_from("<IIQQQQIIQQ",raw,offset+i*size) for i in range(count)];result={}
    for section in sections:
        if section[1]!=2:continue
        strings=sections[section[6]]
        for p in range(section[4],section[4]+section[5],section[9]):
            name,_,_,_,value,length=struct.unpack_from("<IBBHQQ",raw,p);start=strings[4]+name
            name=raw[start:raw.index(b"\0",start)].decode()
            if name in ("safe_detach_pool","safe_cached_decks"):result[name]=(value,length)
    assert result["safe_detach_pool"][1]==16*48 and result["safe_cached_decks"][1]==4*24
    return result

def run(binary,output,cycles,mode="detach"):
    assert mode in ("detach","load-unload","unload-first")
    assert 2<=cycles<=32 and t.ssh("uname -r").strip()=="6.6.138-dirty" and t.empty_decks()
    digest=hashlib.sha256(binary.read_bytes()).hexdigest();assert t.ssh("sha256sum "+t.STAGE).split()[0]==digest
    pid=t.current_pid();entries=t.ssh("cat /proc/"+pid+"/maps").splitlines()
    base=int(next(x.split("-",1)[0] for x in entries if "usb-hub-support.so" in x and x.split()[2]=="00000000"),16)
    metadata=symbols(binary);start=len(t.logs());samples=[]
    def own_bytes(name):
        offset,length=metadata[name]
        text=t.ssh("dd if=/proc/"+pid+"/mem bs=1 skip="+str(base+offset)+" count="+str(length)+" 2>/dev/null | od -v -x")
        words=[]
        for line in text.splitlines():
            parts=line.split()
            if len(parts)>1:words.extend(int(x,16) for x in parts[1:])
        result=b"".join(x.to_bytes(2,"little") for x in words);assert len(result)==length
        return result
    def sample(cycle):
        assert t.current_pid()==pid and t.empty_decks()
        data=t.ssh("cat /proc/"+pid+"/status; printf 'fd_count '; ls /proc/"+pid+"/fd | wc -l")
        values={}
        for key in ("VmRSS","VmData","Threads"):
            match=re.search(r"(?m)^"+key+r":\s+(\d+)",data);assert match;values[key]=int(match.group(1))
        values["fds"]=int(re.search(r"fd_count (\d+)",data).group(1));values["cycle"]=cycle
        pool=own_bytes("safe_detach_pool");proof=own_bytes("safe_cached_decks")
        values["native_pending"]=sum(struct.unpack_from("<i",pool,i*48+36)[0]==1 for i in range(16))
        values["native_ticket_references"]=sum(struct.unpack_from("<I",pool,i*48+32)[0] for i in range(16))
        values["cached_proofs"]=sum(struct.unpack_from("<i",proof,i*24+16)[0]!=0 for i in range(4))
        assert values["native_pending"]==0 and values["native_ticket_references"]<=4 and values["cached_proofs"]<=4,values
        samples.append(values);print(json.dumps(values),flush=True)
    marker=t.whole_hub();t.wait_for(lambda:t.state_is("HUB_LIB",2) and t.state_is("HUB_LIB2",2),"initial hub safe")
    sample(0)
    if mode=="load-unload": t.replug("A")
    for cycle in range(1,cycles+1):
        if mode!="load-unload": t.replug("A")
        t.center_knob();t.browse_a();marker=len(t.logs());t.load(1,delay=1)
        t.wait_for(lambda:t.status()["decks"][0]["loaded"]==1,"cycle loaded track")
        t.wait_for(lambda:re.search(r"(?m)^safe-eject load-completed controller=.* result=(0|4)$",t.logs()[marker:]),"cycle native completion")
        assert t.status()["decks"][0]["content_id"]==495;t.assert_load_disk(marker,1,"sdb")
        if mode=="unload-first": t.unload(1)
        if mode!="load-unload":
            mark=t.request("HUB_LIB");t.wait_for(lambda:t.state_is("HUB_LIB",2),"cycle loaded eject")
            assert not t.present("sdb") and not t.present("sdc") and t.present("sda") and t.status()["decks"][0]["loaded"]==(0 if mode=="unload-first" else 1)
            if mode=="unload-first": assert "safe-eject native-detach ticket=" not in t.logs()[start:]
        else:
            assert t.present("sdb") and not t.present("sdc") and t.present("sda")
            assert "safe-eject native-detach ticket=" not in t.logs()[start:]

        if t.status()["decks"][0]["playing"]:t.drive("press",1,"play")
        if mode!="unload-first": t.unload(1)
        time.sleep(2);sample(cycle)
    warm=samples[2:]
    assert max(x["fds"] for x in warm)-min(x["fds"] for x in warm)<=2,samples
    assert max(x["Threads"] for x in warm)==min(x["Threads"] for x in warm),samples
    marker=len(t.logs());t.stop();t.wait_for(lambda:"eject-ui selector port=2" in t.logs()[marker:],"post-stress selector")
    t.drive("knob","cw",1);t.drive("panel","back",.15)
    assert t.current_pid()==pid and "frame filter switched off" not in t.logs()[start:]
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text(json.dumps({"timestamp_utc":datetime.datetime.now(datetime.timezone.utc).isoformat(),"environment":"Local QEMU, protected COW fixtures only","binary_sha256":digest,"pid":int(pid),"mode":mode,"cycles":cycles,"samples":samples,"post_stress_selector_knob_back":True,"whole_mod_leak_free_claim":False,"scope":("Repeated native cache detach and manual unload" if mode=="detach" else "Repeated load/manual unload before ordinary empty-source eject and replug" if mode=="unload-first" else "Repeated ordinary load/manual unload on one connection; no detach or replug during cycles")+"; owned ticket/proof bounds plus aggregate native process observation. Proprietary firmware and allocator internals are not instrumented.","rss_warm_to_final_kib":samples[-1]["VmRSS"]-samples[2]["VmRSS"],"vmdata_warm_to_final_kib":samples[-1]["VmData"]-samples[2]["VmData"],"events":t.logs()[start:].splitlines()},indent=2)+"\n",encoding="utf-8")
    print("PASS resource observation mode="+mode,flush=True)

if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument("--binary",type=Path,required=True);parser.add_argument("--output",type=Path,required=True);parser.add_argument("--cycles",type=int,default=12)
    parser.add_argument("--mode",choices=("detach","load-unload","unload-first"),default="detach")
    args=parser.parse_args();run(args.binary,args.output,args.cycles,args.mode)

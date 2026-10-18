#!/usr/bin/env python3
"""Test actual stock exFAT-FUSE closure on an explicit localhost USB fixture.

Requires a disposable, mounted EJECT_EXFAT volume at /dev/sdd,
/media/usb/sdd, hub port 1.3, plus the installed candidate with empty decks.
Writes a small fixture file, creates busy holders and ejects that virtual USB.
Never connects to a physical AZ or installs a candidate.
"""
import argparse
import datetime
import hashlib
import importlib.util
import json
from pathlib import Path
import time

spec = importlib.util.spec_from_file_location("suite", Path(__file__).with_name("test-safe-eject-emulator.py"))
t = importlib.util.module_from_spec(spec)
spec.loader.exec_module(t)


def run(binary, output):
    digest = hashlib.sha256(binary.read_bytes()).hexdigest()
    assert t.ssh("uname -r").strip() == "6.6.138-dirty" and t.empty_decks()
    pid = t.current_pid()
    mapping = t.ssh(f"grep '/usb-hub-support.so$' /proc/{pid}/maps | head -1").split()[0]
    assert t.ssh(f"sha256sum /proc/{pid}/map_files/{mapping}").split()[0] == digest
    assert t.state_is("EJECT_EXFAT", 0)
    assert any(line.startswith("/dev/sdd /media/usb/sdd fuseblk ") for line in t.mounts())
    start = len(t.logs())
    checks = []
    t.ssh("printf 'safe eject exfat fixture\\n' > /media/usb/sdd/eject-proof.txt")
    for name, path, expected in [("raw", "/dev/sdd", "Another process is using the USB device"),
                                 ("file", "/media/usb/sdd/eject-proof.txt", "Device or resource busy")]:
        t.ssh(f"(exec 9<{path}; exec sleep 120) </dev/null >/tmp/eject-exfat-holder.log 2>&1 & echo $! >/tmp/eject-exfat-holder.pid")
        holder = int(t.ssh("cat /tmp/eject-exfat-holder.pid").strip())
        try:
            marker = t.request("EJECT_EXFAT")
            t.wait_for(lambda: expected in t.logs()[marker:], f"exFAT {name} busy refusal")
            assert "safe to remove" not in t.logs()[marker:]
            assert any(line.startswith("/dev/sdd /media/usb/sdd fuseblk ") for line in t.mounts())
        finally:
            t.ssh(f"kill {holder}")
        checks.append(f"exFAT {name} user prevents success while filesystem daemon remains available")
        print("PASS:", checks[-1], flush=True)
    marker = t.request("EJECT_EXFAT")
    t.wait_for(lambda: t.state_is("EJECT_EXFAT", 2), "exFAT normal unmount and daemon exit")
    assert not any(line.startswith("/dev/sdd ") for line in t.mounts())
    assert "unmounted path=/media/usb/sdd" in t.logs()[marker:]
    assert t.present("sda") and t.present("sdb") and t.present("sdc")
    time.sleep(10)
    assert not any(line.startswith("/dev/sdd ") for line in t.mounts())
    assert t.current_pid() == pid
    checks.append("Stock exFAT-FUSE daemon closes normally; device unmounts and remains inhibited; other sticks remain mounted")
    print("PASS:", checks[-1], flush=True)
    output.write_text(json.dumps({
        "timestamp_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "environment": "Local QEMU emulator; stock exFAT 1.4.0/libfuse 2.9.9; no physical AZ access",
        "experimental_binary_sha256": digest,
        "checks": checks,
        "events": t.logs()[start:].splitlines(),
        "final_status": t.status(),
    }, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    run(args.binary, args.output)

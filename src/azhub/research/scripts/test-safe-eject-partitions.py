#!/usr/bin/env python3
"""Localhost emulator: one physical USB, two real FAT partitions, partial retry.

Requires empty decks, no virtual device at hub port 1.3, the explicit candidate
and a disposable MBR image containing EJECT_C/EJECT_D FAT partitions. Mounts the
fixture without native USB announcements, writes tiny proof files, tests a busy
second partition and restarts EP147 once. No installation or physical AZ access.
"""
import argparse
import datetime
import hashlib
import importlib.util
import json
from pathlib import Path

spec = importlib.util.spec_from_file_location("suite", Path(__file__).with_name("test-safe-eject-emulator.py"))
t = importlib.util.module_from_spec(spec)
spec.loader.exec_module(t)


def mounted(partition):
    return any(line.startswith(f"/dev/{partition} ") for line in t.mounts())


def run(binary, image, output):
    digest = hashlib.sha256(binary.read_bytes()).hexdigest()
    assert t.ssh("uname -r").strip() == "6.6.138-dirty" and t.empty_decks()
    pid = t.current_pid()
    mapping = t.ssh(f"grep '/usb-hub-support.so$' /proc/{pid}/maps | head -1").split()[0]
    assert t.ssh(f"sha256sum /proc/{pid}/map_files/{mapping}").split()[0] == digest
    assert t.emu.azemu.ssh("test ! -e /sys/bus/usb/devices/2-1.3")[0] == 0
    start = len(t.logs())
    existing = next((node for node in t.qmp("query-named-block-nodes")
                     if node["node-name"] == "ejecttwoparts"), None)
    if existing:
        assert Path(existing["file"]).resolve() == image.resolve(), "Existing backend is another image"
    else:
        t.qmp("blockdev-add", {"driver": "raw", "node-name": "ejecttwoparts",
              "file": {"driver": "file", "filename": str(image.resolve()).replace("\\", "/")}})
    t.qmp("device_add", {"driver": "usb-storage", "id": "ejectpartitioned", "drive": "ejecttwoparts",
                         "bus": "ejectxhci.0", "port": "1.3"})
    t.wait_for(lambda: t.emu.azemu.ssh("test -b /dev/sdd1 -a -b /dev/sdd2")[0] == 0,
               "two physical partitions enumerated")
    for part in ("sdd1", "sdd2"):
        t.ssh(f"mkdir -p /media/usb/{part}; mountpoint -q /media/usb/{part} || mount -t vfat /dev/{part} /media/usb/{part}")
        t.ssh(f"printf 'partition fixture\\n' > /media/usb/{part}/eject-proof.txt")
    t.wait_for(lambda: t.state_is("EJECT_C", 0), "one physical inventory row")
    assert len([item for item in t.status()["devices"] if item["disk"] == "sdd"]) == 1
    t.ssh("(exec 9</media/usb/sdd2/eject-proof.txt; exec sleep 180) </dev/null >/tmp/eject-partition-holder.log 2>&1 & echo $! >/tmp/eject-partition-holder.pid")
    holder = int(t.ssh("cat /tmp/eject-partition-holder.pid").strip())
    try:
        marker = t.request("EJECT_C")
        t.wait_for(lambda: t.state_is("EJECT_C", 3), "partial partition unmount failure")
        assert "Device or resource busy" in t.logs()[marker:]
        assert "safe to remove" not in t.logs()[marker:]
        assert not mounted("sdd1") and mounted("sdd2")
    finally:
        t.ssh(f"kill {holder}")
    checks = ["Two FAT partitions are one physical USB selector item",
              "First partition unmounts but busy second partition prevents success and retains failed/inhibited state"]
    print("PASS: multi-partition partial failure preserves inhibition", flush=True)
    old = t.current_pid()
    restart_marker = len(t.logs())
    t.ssh("systemctl stop EP147; echo 0 > /home/root/settings/azmod-community/.starting; systemctl start EP147")
    t.wait_for(lambda: t.current_pid() and t.current_pid() != old, "new process")
    t.wait_for(lambda: str(json.loads(t.ssh("cat /run/azhub-eject-status"))["pid"]) == t.current_pid(), "fresh telemetry")
    t.wait_mod_ready(restart_marker, settle_seconds=10)
    t.wait_for(t.empty_decks, "native initialization")
    t.wait_for(lambda: t.state_is("EJECT_C", 3), "failed partition state restored")
    assert not mounted("sdd1") and mounted("sdd2")
    t.request("EJECT_C")
    t.wait_for(lambda: t.state_is("EJECT_C", 2), "retry completes remaining partition")
    assert not mounted("sdd1") and not mounted("sdd2") and t.present("sda")
    checks.append("Partial failure survives EP147 restart; retry closes the remaining partition and preserves native USB1")
    t.qmp("device_del", {"id": "ejectpartitioned"})
    t.wait_for(lambda: t.emu.azemu.ssh("test ! -e /sys/bus/usb/devices/2-1.3")[0] == 0, "fixture removed after safe eject")
    output.write_text(json.dumps({
        "timestamp_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "environment": "Local QEMU emulator only; disposable two-partition FAT image; no physical AZ access",
        "experimental_binary_sha256": digest,
        "checks": checks,
        "restart": {"previous_pid": old, "new_pid": t.current_pid()},
        "events": t.logs()[start:].splitlines(),
        "final_status": t.status(),
    }, indent=2) + "\n", encoding="utf-8")
    print("PASS: partition grouping, failure journal and retry", flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--image", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    run(args.binary, args.image, args.output)

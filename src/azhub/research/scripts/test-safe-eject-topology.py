#!/usr/bin/env python3
"""Explicit localhost-emulator topology and fallback-remount fault tests.

Requires empty decks, A/B mounted, candidate telemetry and the bounded test
reservation pause. Four disposable whole-volume FAT images named
eject-fallback-C.img through F.img must exist in --fixtures. Hotplugs virtual
devices, writes a fixture file and restarts EP147 once with empty decks.
Never installs or connects to a physical AZ.
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


def add(letter, fixtures):
    node = f"ejectfatdrive{letter}"
    if not any(item["node-name"] == node for item in t.qmp("query-named-block-nodes")):
        t.qmp("blockdev-add", {"driver": "raw", "node-name": node,
              "file": {"driver": "file", "filename": str((fixtures / f"eject-fallback-{letter}.img").resolve()).replace("\\", "/")}})
    t.qmp("device_add", {"driver": "usb-storage", "id": f"ejectfat{letter}", "drive": node,
                         "bus": "ejectxhci.0", "port": f"1.{ord(letter) - ord('A') + 1}"})
    disk = f"sd{chr(ord('d') + ord(letter) - ord('C'))}"
    t.wait_for(lambda: t.emu.azemu.ssh(f"test -b /dev/{disk}")[0] == 0, f"virtual {letter} block device")
    return disk


def remove(letter):
    t.qmp("device_del", {"id": f"ejectfat{letter}"})
    port = ord(letter) - ord('A') + 1
    t.wait_for(lambda: t.emu.azemu.ssh(f"test ! -e /sys/bus/usb/devices/2-1.{port}")[0] == 0,
               f"virtual {letter} disconnected")


def mounted(disk):
    return any(line.startswith(f"/dev/{disk} /media/usb/{disk} ") for line in t.mounts())


def run(binary, fixtures, output):
    digest = hashlib.sha256(binary.read_bytes()).hexdigest()
    assert t.ssh("uname -r").strip() == "6.6.138-dirty" and t.empty_decks()
    pid = t.current_pid()
    mapping = t.ssh(f"grep '/usb-hub-support.so$' /proc/{pid}/maps | head -1").split()[0]
    assert t.ssh(f"sha256sum /proc/{pid}/map_files/{mapping}").split()[0] == digest
    assert t.present("sdb") and t.present("sdc")
    start = len(t.logs())
    checks = []

    marker = t.whole_hub()
    t.wait_for(lambda: "safe-eject reserved targets=2" in t.logs()[marker:], "whole-hub reservation")
    assert add("C", fixtures) == "sdd"
    t.ssh("mkdir -p /media/usb/sdd; mount -t vfat /dev/sdd /media/usb/sdd")
    t.wait_for(lambda: "Hub changed during eject" in t.logs()[marker:], "new-child refusal")
    assert "Hub on USB 2 is safe to remove" not in t.logs()[marker:]
    assert t.state_is("HUB_LIB", 2) and t.state_is("HUB_LIB2", 2)
    assert t.state_is("EJECT_C", 0) and mounted("sdd") and t.present("sda")
    checks.append("New mounted child during whole-hub eject prevents a whole-hub success claim; completed children stay safe")
    print("PASS:", checks[-1], flush=True)
    marker = t.whole_hub()
    t.wait_for(lambda: "Hub on USB 2 is safe to remove" in t.logs()[marker:], "whole-hub retry includes C")
    assert t.state_is("EJECT_C", 2) and not mounted("sdd")
    checks.append("Whole-hub retry safely includes the newly connected device")

    remove("C")
    assert add("C", fixtures) == "sdd"
    t.ssh("mount -t vfat /dev/sdd /media/usb/sdd")
    t.wait_for(lambda: t.state_is("EJECT_C", 0), "C remounted after reconnect")
    marker = t.request("EJECT_C")
    t.wait_for(lambda: "safe-eject reserved targets=1" in t.logs()[marker:], "individual reservation")
    remove("C")
    t.wait_for(lambda: "safe-eject result:" in t.logs()[marker:], "disconnect error result")
    assert "safe to remove" not in t.logs()[marker:]
    # A disconnected filesystem can remain mounted after a udev race. The test
    # owns this disposable mount; use only ordinary unmount, never force/lazy.
    if mounted("sdd"):
        t.ssh("umount /media/usb/sdd")
    checks.append("Unexpected disconnect during an eject never reports safe removal")
    print("PASS:", checks[-1], flush=True)

    for letter in "CDEF":
        disk = add(letter, fixtures)
        assert disk == "sd" + chr(ord("d") + ord(letter) - ord("C"))
    t.wait_for(lambda: mounted("sdg"), "fallback worker automatically mounts sdg", timeout=30)
    t.wait_for(lambda: t.state_is("EJECT_F", 0), "fallback volume appears in selector")
    marker = t.request("EJECT_F")
    t.wait_for(lambda: t.state_is("EJECT_F", 2), "fallback volume ejected")
    assert not mounted("sdg")
    time.sleep(12)
    assert not mounted("sdg")
    checks.append("Fallback worker automatically mounts sdg, but cannot remount its ejected connection over four worker cycles")
    print("PASS:", checks[-1], flush=True)

    # Simulate an independent mount owner (for example a delayed stock udev
    # event) reopening the same still-connected device. A stored success must
    # not remain a current safe-to-remove assertion once a mount reappears.
    marker = len(t.logs())
    t.ssh("mount -t vfat /dev/sdg /media/usb/sdg")
    t.wait_for(lambda: t.state_is("EJECT_F", 3), "unexpected remount revokes safe status", timeout=5)
    assert mounted("sdg") and "safe to remove" not in t.logs()[marker:]
    marker = t.request("EJECT_F")
    t.wait_for(lambda: "safe-eject success" in t.logs()[marker:] and not mounted("sdg"),
               "retry actually unmounts reopened device")
    assert t.state_is("EJECT_F", 2)
    checks.append("Independent remount revokes completed safety; retry performs a real flush/unmount instead of reusing stale success")
    print("PASS:", checks[-1], flush=True)

    old = t.current_pid()
    restart_marker = len(t.logs())
    t.ssh("systemctl stop EP147; echo 0 > /home/root/settings/azmod-community/.starting; systemctl start EP147")
    t.wait_for(lambda: t.current_pid() and t.current_pid() != old, "new EP147 process")
    t.wait_for(lambda: str(json.loads(t.ssh("cat /run/azhub-eject-status"))["pid"]) == t.current_pid(), "fresh telemetry")
    t.wait_mod_ready(restart_marker)
    t.wait_for(t.empty_decks, "empty native decks after reload")
    t.wait_for(lambda: t.state_is("EJECT_F", 2), "persisted fallback eject")
    time.sleep(10)
    assert not mounted("sdg") and t.present("sda")
    checks.append("Fallback remount inhibition survives an EP147 restart with the virtual USB still attached")

    marker = t.whole_hub()
    t.wait_for(lambda: "Hub on USB 2 is safe to remove" in t.logs()[marker:], "safe fixture teardown")
    for letter in "CDEF":
        remove(letter)
    output.write_text(json.dumps({
        "timestamp_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "environment": "Local QEMU emulator only; extra devices are disposable empty FAT volumes, not simultaneous music-source acceptance",
        "experimental_binary_sha256": digest,
        "checks": checks,
        "restart": {"previous_pid": old, "new_pid": t.current_pid()},
        "events": t.logs()[start:].splitlines(),
        "final_status": t.status(),
    }, indent=2) + "\n", encoding="utf-8")
    print("PASS: topology and fallback boundaries", flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--fixtures", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    run(args.binary, args.fixtures, args.output)

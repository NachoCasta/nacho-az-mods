#!/usr/bin/env python3
"""Emulator-only raw-device refusal and native STOP regression after the E2E suite.

Requires the same explicitly installed experimental fixture, empty decks, both
hub sticks safely ejected and native USB1 mounted. Restores native USB1 after its
stock eject. Does not install or connect to a physical AZ.
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


def touch_request():
    marker = len(t.logs())
    index = next(i for i, item in enumerate(t.status()["devices"]) if item["label"] == "HUB_LIB2")
    t.stop()
    t.wait_for(lambda: "eject-ui selector port=2" in t.logs()[marker:], "touch selector")
    t.drive("tap", 850, 2 * (260 + index * 42))
    t.drive("tap", 1560, 1250)
    return marker


def run(binary, output):
    expected = hashlib.sha256(binary.read_bytes()).hexdigest()
    assert t.ssh("uname -r").strip() == "6.6.138-dirty"
    assert t.ssh(f"sha256sum {t.STAGE}").split()[0] == expected
    pid = t.current_pid()
    mapping = t.ssh(f"grep '/usb-hub-support.so$' /proc/{pid}/maps | head -1").split()[0]
    assert t.ssh(f"sha256sum /proc/{pid}/map_files/{mapping}").split()[0] == expected
    assert t.empty_decks() and t.present("sda") and not t.present("sdb") and not t.present("sdc")
    start = len(t.logs())
    t.replug("B")
    t.wait_for(lambda: t.present("sdc"), "B remounted after reconnection")
    t.ssh("(exec 9</dev/sdc; exec sleep 120) </dev/null >/tmp/eject-raw-holder.log 2>&1 & echo $! >/tmp/eject-raw-holder.pid")
    holder = int(t.ssh("cat /tmp/eject-raw-holder.pid").strip())
    try:
        marker = touch_request()
        t.wait_for(lambda: "Another process is using the USB device" in t.logs()[marker:], "raw-device refusal")
        assert t.present("sdc") and t.state_is("HUB_LIB2", 0)
        assert "withdrawal" not in t.logs()[marker:] and "safe to remove" not in t.logs()[marker:]
    finally:
        t.ssh(f"kill {holder}")
    touch_request()
    t.wait_for(lambda: t.state_is("HUB_LIB2", 2), "eject after raw-device user exits")
    assert not t.present("sdc")
    marker = len(t.logs())
    t.stop(0.4, port=1)
    time.sleep(1)
    assert t.present("sda")
    t.stop(2.3, port=1)
    t.wait_for(lambda: not t.present("sda"), "native USB1 long STOP eject")
    events = t.logs()[marker:]
    assert events.count("native-forward port=1 down=1") == 2
    assert events.count("native-forward port=1 down=0") == 2
    assert "selector port=1" not in events
    assert t.state_is("HUB_LIB", 2) and t.state_is("HUB_LIB2", 2)
    assert t.current_pid() == pid
    t.ssh("/home/root/scripts/device-mount.sh add sda1")
    t.wait_for(lambda: t.present("sda"), "native USB1 restored for cleanup")
    output.write_text(json.dumps({
        "timestamp_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "environment": "Local QEMU emulator only; no physical AZ access",
        "experimental_binary_sha256": expected,
        "checks": ["Touch selector submits actual eject requests",
                   "Raw block handle refuses before withdrawal and preserves availability",
                   "Eject succeeds after raw handle closes",
                   "Direct USB1 short/long STOP events forward intact and stock long eject unmounts USB1",
                   "Hub safe states remain intact and native USB1 is restored"],
        "events": t.logs()[start:].splitlines(),
    }, indent=2) + "\n", encoding="utf-8")
    print("PASS: raw-device refusal, retry and native USB1 regression")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    run(args.binary, args.output)

#!/usr/bin/env python3
"""Explicit localhost-emulator SOURCE focus and failed-load eject regressions.

Requires the installed candidate, both mounted hub fixtures, empty native decks
and the prepared hub-aware renotifier. Mutates only that emulator.
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
    expected = hashlib.sha256(binary.read_bytes()).hexdigest()
    assert t.ssh("uname -r").strip() == "6.6.138-dirty"
    pid = t.current_pid()
    mapping = t.ssh(f"grep '/usb-hub-support.so$' /proc/{pid}/maps | head -1").split()[0]
    assert t.ssh(f"sha256sum /proc/{pid}/map_files/{mapping}").split()[0] == expected
    t.wait_for(t.empty_decks, "empty decks")
    assert t.present("sdb") and t.present("sdc")
    checks = []
    start = len(t.logs())
    t.drive("source", delay=1)
    assert t.status()["view"] == 1
    t.ssh("(exec 9</dev/sdc; exec sleep 180) </dev/null >/tmp/eject-regression-holder.log 2>&1 & echo $! >/tmp/eject-regression-holder.pid")
    holder = int(t.ssh("cat /tmp/eject-regression-holder.pid").strip())
    try:
        for _ in range(2):
            marker = t.request("HUB_LIB2")
            t.wait_for(lambda: "Another process is using the USB device" in t.logs()[marker:], "busy refusal")
            time.sleep(1)
            assert t.status()["view"] == 1, "Selector must leave native SOURCE and Mod Menu unchanged"
            assert t.present("sdc") and t.state_is("HUB_LIB2", 0)
        checks.append("Repeated knob-selected requests from SOURCE preserve SOURCE and permit reopening")
        print("PASS:", checks[-1], flush=True)
    finally:
        t.ssh(f"kill {holder}")

    t.center_knob()
    t.browse_a()
    # Return from confirmed-present Algo mas to the fixture's first, missing file.
    t.drive("knob", "ccw", 34, delay=1)
    marker = len(t.logs())
    t.drive("load", 1, delay=2)
    t.wait_for(lambda: "safe-eject load-completed" in t.logs()[marker:] and "result=1" in t.logs()[marker:], "native missing-file failure")
    t.assert_load_disk(marker, 1, "sdb")
    failed = t.status()["decks"][0]
    assert failed["loaded"] == 1 and failed["content_id"] != 0
    marker = t.request("HUB_LIB")
    t.wait_for(lambda: t.state_is("HUB_LIB", 2), "eject after proven failed load")
    assert not t.present("sdb") and t.present("sdc")
    assert "stale-failed-metadata deck=1" in t.logs()[marker:]
    checks.append("Native completed load failure with stale SDK metadata permits actual safe eject")
    print("PASS:", checks[-1], flush=True)
    assert t.current_pid() == pid
    output.write_text(json.dumps({
        "timestamp_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "environment": "Local QEMU emulator only; no physical AZ access",
        "experimental_binary_sha256": expected,
        "checks": checks,
        "failed_load_sdk_snapshot": failed,
        "events": t.logs()[start:].splitlines(),
        "final_status": t.status(),
    }, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    run(args.binary, args.output)

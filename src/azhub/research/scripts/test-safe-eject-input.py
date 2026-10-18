#!/usr/bin/env python3
"""Localhost emulator: captured knob turns preserve native SOURCE/BROWSE state.

Requires empty decks and both mounted A/B fixtures in their original source
order. Exercises 100 turns inside each selector, native load afterward and busy
refusal. No installation, restart or physical AZ connection is performed.
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


def cancel_after_turns():
    marker = len(t.logs())
    original_view = t.status()["view"]
    t.stop()
    t.wait_for(lambda: "eject-ui selector port=2" in t.logs()[marker:], "selector opens")
    t.drive("knob", "cw", 100, delay=1)
    assert t.status()["view"] == original_view, "Native view changed while the selector owned the knob"
    t.drive("panel", "back", 0.15, delay=1)
    t.wait_for(lambda: "eject-ui cancel" in t.logs()[marker:], "selector cancels")
    assert t.status()["view"] == original_view, "Native view changed when selector focus was released"


def run(binary, output):
    digest = hashlib.sha256(binary.read_bytes()).hexdigest()
    assert t.ssh("uname -r").strip() == "6.6.138-dirty" and t.empty_decks()
    pid = t.current_pid()
    mapping = t.ssh(f"grep '/usb-hub-support.so$' /proc/{pid}/maps | head -1").split()[0]
    assert t.ssh(f"sha256sum /proc/{pid}/map_files/{mapping}").split()[0] == digest
    assert t.present("sdb") and t.present("sdc")
    start = len(t.logs())
    t.center_knob()
    t.drive("source", delay=1)
    t.drive("tap", 440, 230, delay=1)  # select native USB1, keep SOURCE open
    assert t.status()["view"] == 1
    cancel_after_turns()
    assert t.status()["view"] == 1, "Captured turns must not open Mod Menu on SOURCE"
    checks = ["100 captured turns and BACK preserve SOURCE without opening Mod Menu"]
    print("PASS:", checks[-1], flush=True)

    t.center_knob()
    t.browse_a()
    assert t.status()["view"] == 2
    # Establish the fixture selection before attributing a later mismatch to
    # capture. A successful native load also proves the backing source identity.
    before_capture = len(t.logs())
    t.load(2, delay=1)
    t.wait_for(lambda: t.status()["decks"][1]["loaded"] == 1, "fixture native load before capture")
    assert t.status()["decks"][1]["content_id"] == 495, "Fixture must select Algo mas before capture"
    t.assert_load_disk(before_capture, 2, "sdb")
    t.unload(2)
    t.drive("panel", "browse", 0.15, delay=1)
    assert t.status()["view"] == 2
    cancel_after_turns()
    assert t.status()["view"] == 2
    # Resume physical navigation after capture. A stale absolute counter would
    # jump on the first detent and would not return to the original track here.
    t.drive("knob", "cw", 1, delay=0.3)
    t.drive("knob", "ccw", 1, delay=0.3)
    load_marker = len(t.logs())
    t.load(2, delay=1)
    t.wait_for(lambda: t.status()["decks"][1]["loaded"] == 1, "native load after capture")
    assert t.status()["decks"][1]["content_id"] == 495, "Original Algo mas row must remain selected after capture"
    t.assert_load_disk(load_marker, 2, "sdb")
    marker = t.request("HUB_LIB")
    t.wait_for(lambda: "Unload" in t.logs()[marker:], "loaded track belongs to A")
    assert t.present("sdb") and t.present("sdc")
    time.sleep(5)
    t.unload(2)
    assert t.current_pid() == pid and t.empty_decks()
    assert "frame filter switched off" not in t.logs()[start:]
    checks.append("100 captured turns preserve the underlying BROWSE row; original Algo mas loads from A and still protects its source")
    checks.append("Native knob resumes with one detent each way; original track loads, with no frame-filter disable or process restart")
    output.write_text(json.dumps({
        "timestamp_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "environment": "Local QEMU emulator only; no physical AZ access",
        "experimental_binary_sha256": digest,
        "checks": checks,
        "events": t.logs()[start:].splitlines(),
        "final_status": t.status(),
    }, indent=2) + "\n", encoding="utf-8")
    print("PASS: semantic input capture and native navigation continuity", flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    run(args.binary, args.output)

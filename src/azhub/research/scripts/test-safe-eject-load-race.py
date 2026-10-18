#!/usr/bin/env python3
"""Race a native cached LOAD against reserved hub ejection in localhost QEMU.

Requires empty decks, mounted A/B FAT fixtures in their original source order,
the candidate with telemetry and AZHUB_EJECT_TEST_PAUSE_MS=8000. Drives native
deck 2 load/unload controls. Never installs or connects to a physical AZ.
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


def run(binary, output, source_row):
    digest = hashlib.sha256(binary.read_bytes()).hexdigest()
    assert t.ssh("uname -r").strip() == "6.6.138-dirty" and t.empty_decks()
    pid = t.current_pid()
    mapping = t.ssh(f"grep '/usb-hub-support.so$' /proc/{pid}/maps | head -1").split()[0]
    assert t.ssh(f"sha256sum /proc/{pid}/map_files/{mapping}").split()[0] == digest
    assert t.present("sdb") and t.present("sdc")
    start = len(t.logs())
    t.center_knob()
    t.browse_a(source_row)
    marker = len(t.logs())
    t.load(2, delay=1)
    t.wait_for(lambda: t.status()["decks"][1]["content_id"] == 495 and t.status()["decks"][1]["loaded"] == 1,
               "cached Algo mas on deck 2")
    t.assert_load_disk(marker, 2, "sdb")
    assert "deck=2" in t.logs()[marker:]
    marker = t.request("HUB_LIB")
    t.wait_for(lambda: "Unload" in t.logs()[marker:], "verify selected track actually belongs to A")
    assert t.present("sdb") and t.state_is("HUB_LIB", 0)
    time.sleep(5)
    t.unload(2)
    # Re-enter the browser and select the cached track through native controls.
    # Waveform -> BROWSE recenters its scroll; fixed bottom-row coordinates no
    # longer identify the track selected before unloading.
    t.center_knob()
    t.browse_a(source_row)
    assert t.status()["view"] == 2, "Native track list must own the LOAD selection"
    # Allow the native track-details prefetch to finish before reservation.
    # Otherwise the LOAD button is disabled before it can issue a request.
    time.sleep(5)
    marker = t.request("HUB_LIB")
    t.wait_for(lambda: "safe-eject reserved targets=1" in t.logs()[marker:], "source reservation")
    # Restore the native track-list focus after confirmation. The previous
    # no-touch attempt issued no native LOAD and did not exercise admission.
    t.drive("tap", 1100, 1350, delay=0.3)
    t.load(2, delay=0.1)
    t.wait_for(lambda: "safe-eject load-denied" in t.logs()[marker:], "native load admission refused after reservation")
    t.wait_for(lambda: t.state_is("HUB_LIB", 2), "eject completes after denied load")
    events = t.logs()[marker:]
    assert "load-denied stage=request" in events or "load-denied stage=track-loader" in events
    assert t.status()["decks"][1]["loaded"] == 0
    assert not t.present("sdb") and t.present("sdc")
    checks = ["Deck 2 native LOAD after reservation is denied before queuing or metadata changes",
              "Early rejection leaves the deck empty and permits safe eject; other source remains mounted"]
    print("PASS: reserved-source LOAD denied and eject completes", flush=True)

    # A is gone, so the first hub row is now B. Load it successfully after the
    # denied request to verify native recovery, then verify loaded-source refusal.
    t.center_knob()
    t.browse_a()
    load_marker = len(t.logs())
    t.load(2, delay=1)
    t.wait_for(lambda: t.status()["decks"][1]["content_id"] == 495 and t.status()["decks"][1]["loaded"] == 1,
               "B load after denied A load")
    t.assert_load_disk(load_marker, 2, "sdc")
    marker = t.request("HUB_LIB2")
    t.wait_for(lambda: "Unload" in t.logs()[marker:], "loaded B refuses eject")
    assert t.present("sdc")
    time.sleep(5)
    t.unload(2)
    assert t.empty_decks() and t.current_pid() == pid
    checks.append("Successful load after denial clears the failed-load exemption; actual loaded source refuses eject; native unload recovers")
    print("PASS: valid subsequent load still protects the actual source", flush=True)
    output.write_text(json.dumps({
        "timestamp_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "environment": "Local QEMU emulator only; no physical AZ access",
        "experimental_binary_sha256": digest,
        "checks": checks,
        "events": t.logs()[start:].splitlines(),
        "final_status": t.status(),
    }, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--source-row", type=int, choices=(0, 1), default=0)
    args = parser.parse_args()
    run(args.binary, args.output, args.source_row)

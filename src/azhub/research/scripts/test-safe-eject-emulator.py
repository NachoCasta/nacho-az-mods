#!/usr/bin/env python3
"""End-to-end safe-eject checks on the explicitly prepared localhost emulator.

This script hotplugs virtual sticks, drives native controls and restarts EP147
once with empty decks. Requires the experimental build, telemetry and
AZHUB_EJECT_TRACE_LOAD=1 for physical source verification, the hub-aware
renotifier fixture, and USB2 hub ports 1/2 holding usblib.img/hublib2.img. Before
starting, both hub sticks must already be unmounted; native USB1 must stay mounted.
No installation or physical-AZ connection is performed.
"""
import argparse
import datetime
import hashlib
import importlib.util
import json
import os
import shutil
import uuid
import re
from pathlib import Path
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("safeemu", HERE / "eject-ui-emu.py")
emu = importlib.util.module_from_spec(spec)
spec.loader.exec_module(emu)
LOG = "/tmp/azmod-community/usb-hub-support.log"
STAGE = "/home/root/settings/az-usbload/community/usb-hub-support/usb-hub-support.so"


def ssh(command):
    rc, out = emu.azemu.ssh(command, timeout=30)
    if rc:
        raise RuntimeError(f"Emulator command failed ({rc}): {command}\n{out}")
    return out


def qmp(command, args=None):
    result = emu.qmp(command, args or {})
    if "error" in result:
        raise RuntimeError(result["error"])
    return result["return"]


def drive(*args, delay=0.5):
    tool = HERE / "eject-ui-emu.py" if args[0] in ("tap", "knob") else emu.tools / "az-drive.py"
    subprocess.run([sys.executable, str(tool), *map(str, args)], check=True)
    time.sleep(delay)


def stop(seconds=0.5, port=2):
    subprocess.run([sys.executable, str(HERE / "eject-ui-emu.py"), "stop", str(port), str(seconds)], check=True)


def load(deck, delay=0.5):
    subprocess.run([sys.executable, str(HERE / "eject-ui-emu.py"), "load", str(deck), "0.35"], check=True)
    time.sleep(delay)


def wait_for(predicate, description, timeout=90):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        value = predicate()
        if value:
            return value
        time.sleep(0.3)
    raise AssertionError(f"Timed out: {description}")


def status():
    data = json.loads(ssh("cat /run/azhub-eject-status"))
    assert str(data["pid"]) == current_pid(), "Stale telemetry"
    return data


def mounts():
    return ssh("cat /proc/mounts").splitlines()


def present(disk):
    return any(line.startswith(f"/dev/{disk}1 /media/usb/{disk}1 ") for line in mounts())


def logs():
    return ssh(f"if test -f {LOG}; then cat {LOG}; fi")


def wait_mod_ready(log_offset, settle_seconds=0):
    # Inventory telemetry can precede completion of init and hook installation.
    # Match a new core log entry, not the previous process's loaded status file.
    def initialized():
        history = logs()
        # The SDK may rotate its capped log when a new process first logs.
        # A shorter replacement file starts at zero, not at the old byte offset.
        tail = history[log_offset:] if len(history) >= log_offset else history
        return re.search(r"(?m)^loaded \d+\.\d+\.\d+ on ep147-9e9454e357389ece \(\d+ hooks\)$", tail)
    wait_for(initialized, "mod initialization completed")
    # Cold-restart fixtures missed the first STOP DOWN even after this marker.
    # Settling is explicit; the subsequent input operation must still pass.
    if settle_seconds:
        time.sleep(settle_seconds)


def assert_load_disk(marker, deck, disk):
    # Identical fixture databases/titles cannot prove which USB was selected.
    # Resolve the actual native request's member/slot, then compare the bind
    # alias's filesystem identity with the requested physical backing volume.
    requests = re.findall(rf"safe-eject load-request deck={deck} location=(0x[0-9a-f]+):", logs()[marker:])
    assert requests, "Requires AZHUB_EJECT_TRACE_LOAD=1 and a native load request"
    media = int(requests[-1], 16)
    if disk in ('sda','sdb'):
        # Native fixtures use /C/ (USB1) and /B/ (USB2), rather than synthetic
        # /media/playerNN aliases. Require the exact native request form and
        # prove the actual root's filesystem/backing.
        expected = 0x1010200000001 if disk == 'sdb' else 0x1012200000001
        assert media == expected, ('Unexpected native USB request', disk, hex(media))
        root = ssh('readlink -f '+('/C' if disk == 'sdb' else '/B')).strip()
        assert root == '/media/usb/'+disk+'1', root
        table = {line.split()[4]: line.split()[2] for line in ssh('cat /proc/self/mountinfo').splitlines()}
        assert root in table and table[root] == ssh('cat /sys/class/block/'+disk+'1/dev').strip()
        return
    player = ((media >> 8) & 255) + 1
    leaf = {2: "usb", 0x22: "usb2", 0x42: "usb3", 3: "sd"}[(media >> 32) & 255]
    table = {line.split()[4]: line.split()[2] for line in ssh("cat /proc/self/mountinfo").splitlines()}
    alias, backing = f"/media/player{player:02d}/{leaf}", f"/media/usb/{disk}1"
    assert alias in table and backing in table and table[alias] == table[backing], (
        f"Native request belongs to player {player}/{leaf}, not fixture {disk}: "
        f"{table.get(alias)} != {table.get(backing)}")


def current_pid():
    # Native fork-before-exec helpers can briefly share the EP147 name. Select
    # the actual application child of the service's launcher, not every pidof
    # match; keep process changes observable rather than masking real restarts.
    rc, out = emu.azemu.ssh('unit_main=$(systemctl show EP147 -p MainPID); unit_main=${unit_main#MainPID=}; pgrep -P "$unit_main" -x EP147')
    return out.strip() if rc == 0 else ""


def device(label):
    return next((item for item in status()["devices"] if item["label"] == label), None)


def state_is(label, state):
    item = device(label)
    return item if item and item["state"] == state else None


def snapshot_drive(node, image, create=True):
    """Use a persistent named COW node; never hotplug a writable original image."""
    image = image.resolve()
    assert image.is_file(), "A fixture must be a regular local image file"
    nodes = qmp("query-named-block-nodes")
    raw = [item for item in nodes
           if item.get("image", {}).get("format") == "raw"
           and Path(item.get("file", "")).resolve() == image]
    assert all(item.get("ro") is True for item in raw), "Writable base image: restart with a protected fixture"
    existing = next((item for item in nodes if item["node-name"] == node), None)
    if existing:
        metadata = existing.get("image", {})
        backing = metadata.get("backing-image", {})
        assert metadata.get("format") == "qcow2" and backing.get("format") == "raw", "Fixture node is not a COW overlay"
        assert Path(backing["filename"]).resolve() == image and raw, "Unexpected or unverified fixture backing"
        filename = existing["file"]
        if filename.startswith("json:"):
            leaf = next(item for item in nodes if item["node-name"] == node + "-file")
            filename = leaf["file"]
        return Path(filename)
    if not create:
        return None
    tool = os.environ.get("AZHUB_QEMU_IMG") or shutil.which("qemu-img")
    if not tool and os.name == "nt":
        tool = str(Path(os.environ.get("ProgramFiles", "C:/Program Files")) / "qemu/qemu-img.exe")
    assert tool and Path(tool).is_file(), "Install qemu-img or set AZHUB_QEMU_IMG"
    output = HERE.parents[3] / "local/emulator-overlays"
    output.mkdir(parents=True, exist_ok=True)
    overlay = output / f"{node}-{uuid.uuid4().hex}.qcow2"
    subprocess.run([tool, "create", "-q", "-f", "qcow2", "-F", "raw", "-b", str(image), str(overlay)], check=True)
    qmp("blockdev-add", {"driver": "qcow2", "node-name": node,
                        "file": {"driver": "file", "node-name": node + "-file", "filename": overlay.as_posix()},
                        "backing": {"driver": "raw", "read-only": True,
                                    "file": {"driver": "file", "filename": image.as_posix(), "read-only": True}}})
    return snapshot_drive(node, image, create=False)


def replug(letter):
    name, node = f"ejectstick{letter}", f"ejectdrive{letter}"
    port = "1.1" if letter == "A" else "1.2"
    assert letter in ("A", "B")
    image = "usblib.img" if letter == "A" else "hublib2.img"
    image = emu.tools.parent / "build" / image
    snapshot_drive(node, image, create=False)
    qmp("device_del", {"id": name})
    wait_for(lambda: emu.azemu.ssh(f"test ! -e /sys/bus/usb/devices/2-{port}")[0] == 0,
             f"physical disappearance of virtual stick {letter}")
    time.sleep(0.5)
    snapshot_drive(node, image)
    qmp("device_add", {"driver": "usb-storage", "id": name, "drive": node,
                       "bus": "ejectxhci.0", "port": port})
    label = "HUB_LIB" if letter == "A" else "HUB_LIB2"
    return wait_for(lambda: state_is(label, 0), f"new generation of {label}")


def request(label):
    items = status()["devices"]
    index = next(i for i, item in enumerate(items) if item["label"] == label)
    offset = len(logs())
    stop()
    wait_for(lambda: "eject-ui selector port=2" in logs()[offset:], "USB STOP selector")
    if index:
        drive("knob", "cw", index)
    drive("knobclick", delay=0.1)
    return offset


def whole_hub():
    offset = len(logs())
    stop(2.3)
    return offset


def empty_decks():
    return all(deck["loaded"] == 0 for deck in status()["decks"])


def browse_a(source_row=0):
    # Exact measured native UI sequence for this two-stick fixture. Touch the
    # first visible track before using the knob, so category focus is not moved.
    drive("source", delay=1)
    drive("tap", 440, 410 + 180 * source_row, delay=1)
    drive("tap", 1500, 380, delay=1)
    drive("tap", 100, 350, delay=1.5)
    drive("tap", 1100, 310, delay=1)
    # Native source views retain their scroll position. The fixture's recent
    # selection is within the first 127 rows; return to its start explicitly.
    # Call center_knob before this helper so the emulator counter has room.
    drive("knob", "ccw", 127, delay=1)
    drive("knob", "cw", 34, delay=1)


def center_knob():
    # The emulator tool's absolute byte counter stops at 0/255. Recenter it while
    # our selector owns the knob, then cancel, so native list state is untouched.
    marker = len(logs())
    stop()
    wait_for(lambda: "eject-ui selector port=2" in logs()[marker:], "selector for encoder recenter")
    path = Path(emu.azemu.SOCK_DIR) / "knob.ctr"
    counter = int(path.read_text()) if path.exists() else 128
    if counter != 128:
        drive("knob", "ccw" if counter > 128 else "cw", abs(counter - 128))
    drive("panel", "back", 0.15)


def unload(deck):
    # Native waveform chevron -> native UNLOAD TRACK confirmation.
    if status()["view"] != 4:
        drive("panel", "browse", 1, delay=1)
    assert status()["view"] == 4, "Native waveform required for unload control"
    # Select a known layout before using its deck-header unload chevron.
    drive("tap", 440, 66, delay=0.8)
    y = 188 if deck == 1 else 596
    drive("tap", 310, y, delay=0.8)
    drive("tap", 1080, 1000, delay=0.8)
    wait_for(lambda: status()["decks"][deck - 1]["loaded"] == 0, f"native unload deck {deck}")


def run(binary, output, shots):
    expected = hashlib.sha256(binary.read_bytes()).hexdigest()
    assert ssh(f"sha256sum {STAGE}").split()[0] == expected
    assert ssh("uname -r").strip() == "6.6.138-dirty", "Only the local emulator is supported"
    pid = wait_for(current_pid, "EP147 startup")
    wait_for(lambda: emu.azemu.ssh(f"grep '/usb-hub-support.so$' /proc/{pid}/maps")[1].strip(), "prototype mapped")
    wait_for(lambda: str(json.loads(ssh("cat /run/azhub-eject-status"))["pid"]) == pid, "fresh prototype telemetry")
    mapping = ssh(f"grep '/usb-hub-support.so$' /proc/{pid}/maps | head -1").split()[0]
    assert ssh(f"sha256sum /proc/{pid}/map_files/{mapping}").split()[0] == expected
    assert present("sda") and not present("sdb") and not present("sdc"), "Start with both hub sticks safely unmounted"
    wait_for(empty_decks, "empty initial decks")
    offset = len(logs())
    checks, observations = [], []

    def check(message, **details):
        assert current_pid() == pid
        checks.append(message)
        if details:
            observations.append({"check": message, **details})
        print("PASS:", message, flush=True)

    def shot(name):
        time.sleep(0.8)
        qmp("screendump", {"filename": str((shots / name).resolve()), "format": "png"})

    first_a, first_b = replug("A"), replug("B")
    wait_for(lambda: present("sdb") and present("sdc"), "both hub mounts restored")
    check("New physical connection generations restore both ejected sticks")

    drive("panel", "browse", 1, delay=1)
    center_knob()
    browse_a()
    shot("safe-eject-e2e-track-selection.png")
    load_marker = len(logs())
    load(1, delay=1)
    loaded = wait_for(lambda: status()["decks"][0] if status()["decks"][0]["loaded"] == 1 else None, "Algo mas loaded")
    assert "Algo" in loaded["title"] and "más" in loaded["title"], loaded
    assert_load_disk(load_marker, 1, "sdb")
    check("Confirmed-present Algo mas loads from hub source A", deck=loaded)

    before = mounts()
    marker = request("HUB_LIB")
    wait_for(lambda: "Unload" in logs()[marker:], "busy loaded-source refusal")
    assert mounts() == before and device("HUB_LIB")["state"] == 0
    check("Loaded/paused source A refuses eject before withdrawal and remains mounted")

    drive("press", 1, "play", delay=1)
    playing = wait_for(lambda: status()["decks"][0] if status()["decks"][0]["playing"] == 1 else None, "deck 1 playback")
    marker = request("HUB_LIB2")
    wait_for(lambda: "safe-eject reserved" in logs()[marker:], "source B reserved")
    # Native controls remain usable while the coordinator is working.
    drive("panel", "browse", 1, delay=0.5)
    shot("safe-eject-e2e-background-ui.png")
    wait_for(lambda: state_is("HUB_LIB2", 2), "source B safe to remove")
    assert not present("sdc") and present("sdb") and present("sda")
    after = status()["decks"][0]
    assert after["loaded"] == 1 and after["playing"] == 1 and after["pos_s"] > playing["pos_s"] + 1
    assert after["content_id"] == playing["content_id"]
    assert "server-complete source=1 kind=0" in logs()[marker:]
    assert "server-complete source=1 kind=1" in logs()[marker:]
    check("Source B closes both independent servers and unmounts while source A playback advances", before=playing, after=after)
    time.sleep(10)
    assert not present("sdc")
    check("Ejected source B remains inhibited while still physically attached")

    marker = whole_hub()
    wait_for(lambda: "Unload" in logs()[marker:], "whole-hub busy refusal")
    assert present("sdb") and not present("sdc")
    check("Whole-hub hold refuses while source A is loaded, preserving already-ejected B")

    drive("press", 1, "play", delay=1)
    unload(1)
    marker = whole_hub()
    wait_for(lambda: "Hub on USB 2 is safe to remove." in logs()[marker:], "whole-hub completion")
    assert not present("sdb") and not present("sdc") and present("sda")
    shot("safe-eject-e2e-whole-hub.png")
    check("Native unload clears source ownership; long USB STOP ejects the whole hub without touching native USB1")

    # Real filesystem EBUSY: a separate process holds the backing mount open.
    new_a, new_b = replug("A"), replug("B")
    assert new_a["devnum"] != first_a["devnum"] and new_b["devnum"] != first_b["devnum"]
    wait_for(lambda: present("sdb") and present("sdc"), "fresh mounts for failure/retry")
    ssh("(exec 9</media/usb/sdc1/PIONEER/rekordbox/exportLibrary.db; exec sleep 120) </dev/null >/tmp/safe-eject-holder.log 2>&1 & echo $! >/tmp/safe-eject-holder.pid")
    holder = int(ssh("cat /tmp/safe-eject-holder.pid").strip())
    marker = request("HUB_LIB2")
    wait_for(lambda: state_is("HUB_LIB2", 3), "busy unmount failure")
    assert present("sdc") and "safe to remove" not in logs()[marker:]
    assert "Device or resource busy" in logs()[marker:]
    check("External open file causes real EBUSY; UI reports failure and never safe-to-remove")
    ssh(f"kill {holder}")
    marker = request("HUB_LIB2")
    wait_for(lambda: state_is("HUB_LIB2", 2), "retry after busy file closes")
    assert not present("sdc") and present("sdb")
    check("Retry finishes the quiesced source after the external busy handle closes")

    marker = whole_hub()
    wait_for(lambda: "Hub on USB 2 is safe to remove." in logs()[marker:], "final whole-hub eject")
    assert empty_decks() and not present("sdb") and not present("sdc") and present("sda")
    check("Whole-hub completion includes both successful and previously-ejected children")

    # Intentional emulator-only reload with empty decks; this is not a crash.
    old_pid = pid
    restart_marker = len(logs())
    ssh("systemctl stop EP147; printf '0\\n' > /home/root/settings/azmod-community/.starting; systemctl start EP147")
    def new_process():
        candidate = current_pid()
        return candidate if candidate and candidate != old_pid else None
    pid = wait_for(new_process, "new EP147 process")
    wait_for(lambda: "mod=usb-hub-support|1|" in ssh("cat /tmp/azmod-community/loaded"), "mod after reload")
    wait_for(lambda: str(json.loads(ssh("cat /run/azhub-eject-status"))["pid"]) == pid, "fresh telemetry after reload")
    wait_mod_ready(restart_marker)
    wait_for(lambda: state_is("HUB_LIB", 2) and state_is("HUB_LIB2", 2), "persisted completed eject states")
    time.sleep(10)
    assert not present("sdb") and not present("sdc") and present("sda")
    check("Completed ejection and remount inhibition survive an EP147 restart with sticks attached", previous_pid=old_pid, new_pid=pid)

    record = {
        "timestamp_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "environment": "Local QEMU emulator, kernel 6.6.138-dirty; FAT hub fixtures; no hardware access",
        "firmware_tag": "ep147-9e9454e357389ece",
        "experimental_binary_sha256": expected,
        "hub_ejection_implemented": True,
        "renotifier_fixture": "Nested USB storage excluded from native USB announcements",
        "checks": checks,
        "observations": observations,
        "events": logs()[offset:].splitlines(),
        "final_status": status(),
        "audio_output_captured": False,
        "screenshots": "Local ignored research/shots; no image bytes in this record",
    }
    output.write_text(json.dumps(record, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"PASS: {len(checks)} end-to-end checks; evidence: {output}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--shots", required=True, type=Path)
    args = parser.parse_args()
    assert args.shots.is_dir()
    run(args.binary, args.output, args.shots)

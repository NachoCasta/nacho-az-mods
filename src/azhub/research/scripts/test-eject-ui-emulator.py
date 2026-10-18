#!/usr/bin/env python3
"""Validate the installed interaction prototype on localhost emulator only.

Requires native USB1 plus a USB2 hub with two mounted sticks. Native USB1 long
STOP is exercised last and really uses stock ejection in the emulator. Hub
actions only record requests. No installation, service restart or AZ connection.
"""
import argparse
import datetime
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("ejectemu", HERE / "eject-ui-emu.py")
emu = importlib.util.module_from_spec(spec)
spec.loader.exec_module(emu)
LOG = "/tmp/azmod-community/usb-hub-support.log"


def ssh(command):
    rc, result = emu.azemu.ssh(command, timeout=20)
    if rc:
        raise RuntimeError(f"Emulator command failed ({rc}): {command}\n{result}")
    return result


def helper(*args):
    subprocess.run([sys.executable, str(HERE / "eject-ui-emu.py"), *args], check=True)


def drive(*args):
    subprocess.run([sys.executable, str(emu.tools / "az-drive.py"), *args], check=True)


def wait_for(predicate, description, timeout=30):
    deadline = time.monotonic()+timeout
    while time.monotonic()<deadline:
        result = predicate()
        if result:
            return result
        time.sleep(0.2)
    raise AssertionError(f"Timed out: {description}")


def mounts():
    return set(ssh("cat /proc/mounts").splitlines())


def run(binary, output, shots):
    expected = hashlib.sha256(binary.read_bytes()).hexdigest()
    staged = "/home/root/settings/az-usbload/community/usb-hub-support/usb-hub-support.so"
    assert ssh(f"sha256sum {staged}").split()[0] == expected
    wait_for(lambda: "mod=usb-hub-support|1|" in ssh("cat /tmp/azmod-community/loaded"), "mod loaded")
    pid = ssh("pidof EP147").strip()
    # Verify the exact executable mapping, not just the staged file's name.
    mapping = ssh(f"grep '/usb-hub-support.so$' /proc/{pid}/maps | head -1").split()[0]
    assert ssh(f"sha256sum /proc/{pid}/map_files/{mapping}").split()[0] == expected
    initial = mounts()
    for device in ("sda1", "sdb1", "sdc1"):
        assert any(line.startswith(f"/dev/{device} /media/usb/{device} ") for line in initial)
    offset = len(ssh(f"cat {LOG}"))

    def events():
        return ssh(f"cat {LOG}")[offset:]

    def shot(name):
        time.sleep(0.8)
        emu.qmp("screendump", {"filename": str((shots / name).resolve()), "format": "png"})

    checks = []
    # Leave the Mod Menu: the SDK correctly reserves its knob while open.
    drive("panel", "browse", "1")
    time.sleep(1)
    helper("stop", "2", "0.5")
    wait_for(lambda: "selector port=2 count=2" in events(), "hub selector")
    shot("eject-ui-selector.png")
    drive("knob", "cw", "1")
    drive("knobclick")
    wait_for(lambda: "request-one port=2 disk=sdc" in events(), "knob selected second stick")
    assert mounts() == initial
    checks.append("Short USB2 STOP opens two-stick selector; knob selects only second stick; mounts unchanged")

    helper("stop", "2", "0.5")
    wait_for(lambda: events().count("selector port=2 count=2") == 2, "second selector")
    drive("panel", "back", "0.15")
    wait_for(lambda: "eject-ui cancel" in events(), "BACK cancellation")
    assert events().count("request-one") == 1
    checks.append("BACK cancels without recording an eject request")

    helper("stop", "2", "0.5")
    wait_for(lambda: events().count("selector port=2 count=2") == 3, "touch selector")
    time.sleep(0.4)
    drive("tap", "850", "620")
    drive("tap", "1560", "1250")
    wait_for(lambda: events().count("request-one port=2 disk=sdc") == 2, "touch selected second stick")
    assert mounts() == initial
    checks.append("Touch selects second stick and submits request; mounts unchanged")

    helper("stop", "2", "2.3")
    wait_for(lambda: "request-all port=2" in events(), "whole-hub hold")
    time.sleep(0.8)
    assert events().count("request-all port=2") == 1
    assert events().count("selector port=2") == 3
    assert "native-forward port=2" not in events()
    assert mounts() == initial
    shot("eject-ui-whole-hub-request.png")
    checks.append("Long USB2 STOP records exactly one whole-hub request, without selector or native eject")

    # Touch Cancel also releases the browse knob; verify subsequent native input.
    helper("stop", "2", "0.5")
    wait_for(lambda: events().count("selector port=2 count=2") == 4, "cancel selector")
    drive("tap", "800", "1250")
    drive("panel", "browse", "1")
    shot("eject-ui-native-after-close.png")
    assert ssh("pidof EP147").strip() == pid
    checks.append("Touch Cancel closes selector; subsequent native BROWSE input runs with same EP147 process")

    helper("stop", "1", "0.4")
    wait_for(lambda: "native-forward port=1 down=0" in events(), "native short pair")
    assert mounts() == initial
    assert "selector port=1" not in events()
    helper("stop", "1", "2.3")
    wait_for(lambda: not any(line.startswith("/dev/sda1 /media/usb/sda1 ") for line in mounts()), "stock USB1 eject")
    final_mounts = mounts()
    for device in ("sdb1", "sdc1"):
        assert any(line.startswith(f"/dev/{device} /media/usb/{device} ") for line in final_mounts)
    assert events().count("native-forward port=1 down=1") == 2
    assert events().count("native-forward port=1 down=0") == 2
    assert "selector port=1" not in events() and "request-all port=1" not in events()
    assert ssh("pidof EP147").strip() == pid
    checks.append("Direct USB1 short/long pairs forwarded intact; stock long eject removes USB1, leaves both hub sticks mounted")
    result = {
        "timestamp_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "environment": "Local QEMU emulator, kernel 6.6.138-dirty; not hardware acceptance",
        "firmware_tag": "ep147-9e9454e357389ece",
        "prototype_sha256": expected,
        "ep147_pid": pid,
        "hub_port_fixture": "USB2 = /sys/bus/usb/devices/2-1; native USB1 = 1-1",
        "hold_threshold_ms": 1500,
        "hub_ejection_implemented": False,
        "checks": checks,
        "events": events().splitlines(),
        "screenshots": "Local ignored research/shots only; not included in this record",
    }
    output.write_text(json.dumps(result, indent=2)+"\n", encoding="utf-8")
    print(f"PASS: {len(checks)} emulator checks; evidence: {output}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--shots", required=True, type=Path)
    args = parser.parse_args()
    if not args.shots.is_dir():
        parser.error("Create a local ignored screenshot directory first")
    run(args.binary, args.output, args.shots)

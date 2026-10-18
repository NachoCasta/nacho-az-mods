#!/usr/bin/env python3
"""Explicitly replace an already installed localhost emulator eject candidate.

Stops/restarts EP147 and resets its rapid-restart quarantine counter. Requires
empty decks and an existing usb-hub-support test installation. No package creation,
real hardware connection or initial installation is performed.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path

spec = importlib.util.spec_from_file_location("suite", Path(__file__).with_name("test-safe-eject-emulator.py"))
t = importlib.util.module_from_spec(spec)
spec.loader.exec_module(t)


def run(binary):
    assert t.ssh("uname -r").strip() == "6.6.138-dirty"
    # SDK metadata can remain loaded after a proven failed file load. This helper
    # intentionally requires empty decks; clear/recover those before updating.
    assert t.empty_decks(), "Unload all decks before an emulator candidate update"
    digest = hashlib.sha256(binary.read_bytes()).hexdigest()
    package = "/media/usb/sda1/MODS/community-mods/usb-hub-support/usb-hub-support.so"
    assert t.emu.azemu.push_file(str(binary), package)
    assert t.ssh(f"sha256sum {package}").split()[0] == digest
    old = t.current_pid()
    restart_marker = len(t.logs())
    t.ssh(f"systemctl stop EP147; cp {package} {t.STAGE}.new; mv {t.STAGE}.new {t.STAGE}; "
          f"printf 'usb-hub-support {digest}\\n' >> /home/root/settings/azmod-community/approved; "
          "echo 0 > /home/root/settings/azmod-community/.starting; systemctl start EP147")
    t.wait_for(lambda: t.current_pid() and t.current_pid() != old, "new EP147 process")
    def fresh():
        return str(json.loads(t.ssh("cat /run/azhub-eject-status"))["pid"]) == t.current_pid()
    t.wait_for(fresh, "new candidate telemetry")
    t.wait_mod_ready(restart_marker, settle_seconds=10)
    t.wait_for(t.empty_decks, "native startup")
    print("Candidate staged and running in localhost emulator:", digest)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    args = parser.parse_args()
    run(args.binary)

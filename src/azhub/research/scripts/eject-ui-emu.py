#!/usr/bin/env python3
"""Local emulator helper; never connects to a physical AZ.

Set AZ_EMU_TOOLS to the local xdj-az-emu tools directory. Commands:
  ssh COMMAND; put LOCAL REMOTE; qmp EXECUTE [JSON_ARGUMENTS]; stop PORT SECONDS;
  load DECK SECONDS (repeated physical input frames)
  tap X Y (2560x1600 coordinates, sustained touch)
  knob cw|ccw DETENTS (one guest connection, native per-detent frames)
"""
import importlib.util
import json
import os
from pathlib import Path
import socket
import math
import sys
import time

tools = Path(os.environ.get("AZ_EMU_TOOLS", "C:/Users/icast/xdj-az-emu/tools"))
sys.path.insert(0, str(tools))
import azemu

if azemu.SSH_PORT != 2222 or azemu.QMP != ("127.0.0.1", 4446):
    raise SystemExit("Refusing non-default emulator endpoints")


def qmp(command, arguments):
    with socket.create_connection(azemu.QMP, timeout=10) as s:
        stream = s.makefile("rwb")
        stream.readline()
        for request in ({"execute": "qmp_capabilities"},
                        {"execute": command, "arguments": arguments}):
            stream.write(json.dumps(request).encode() + b"\n")
            stream.flush()
            while True:
                result = json.loads(stream.readline())
                if "return" in result or "error" in result:
                    break
        if __name__ == "__main__":
            print(json.dumps(result))
        return result


def hold_erp(frame, idle, bus, seconds):
    # Windows drive.send uses SSH per frame while EP147 is running. SSH latency
    # must not extend a requested short press into a native long hold. Time both
    # pressed and released states inside one guest shell instead.
    def phase(payload, duration):
        escaped = "".join("\\%03o" % byte for byte in bytes([bus] + list(payload)))
        ticks = max(1, math.ceil(duration * 100))
        clock = ("read up rest < /proc/uptime; "
                 "stamp=$(( ${up%.*} * 100 + 1${up#*.} - 100 ))")
        # Count elapsed guest time rather than iterations: spawning sleep under
        # TCG adds substantial overhead to every frame and must count as hold time.
        return (clock + f"; until=$((stamp+{ticks})); while :; do "
                f"printf '{escaped}' > /dev/subucom_ctrl || exit 1; "
                "sleep 0.02; " + clock + "; [ $stamp -lt $until ] || break; done")
    command = phase(frame, seconds) + "; " + phase(idle, 0.3)
    rc, out = azemu.ssh(command, timeout=30)
    if rc:
        raise RuntimeError(f"Emulator STOP injection failed ({rc}): {out}")


if __name__ == "__main__":
    action = sys.argv[1]
    if action == "ssh":
        rc, out = azemu.ssh(sys.argv[2], timeout=45)
        print(out, end="")
        raise SystemExit(rc)
    elif action == "put":
        raise SystemExit(0 if azemu.push_file(sys.argv[2], sys.argv[3]) else 1)
    elif action == "get":
        import subprocess
        import shlex
        result = subprocess.run(azemu._ssh_base() + ["cat " + shlex.quote(sys.argv[2])], capture_output=True, timeout=30)
        if result.returncode:
            raise SystemExit(result.stderr.decode(errors="replace"))
        Path(sys.argv[3]).write_bytes(result.stdout)
    elif action == "qmp":
        qmp(sys.argv[2], json.loads(sys.argv[3]) if len(sys.argv) > 3 else {})
    elif action == "knob":
        spec = importlib.util.spec_from_file_location("azdrive", tools / "az-drive.py")
        drive = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(drive)
        direction, count = sys.argv[2], int(sys.argv[3])
        if direction not in ("cw", "ccw") or not 0 <= count <= 253:
            raise SystemExit("Expected cw/ccw and 0..253 detents")
        counter = drive._knob_get()
        step = 1 if direction == "cw" else -1
        if not 1 <= counter + step * count <= 254:
            raise SystemExit("Recenter the emulator encoder before this operation")
        commands = ["framefile=$(mktemp /tmp/azhub-encoder.XXXXXX) || exit 1",
                    "trap 'rm -f \"$framefile\"' EXIT"]
        for n in range(count):
            counter += step
            frame = drive._mixer()
            frame[34] = counter
            frame[6] = 0x00 if n % 2 == 0 else 0x08
            payload = bytes([drive.BUS_MIXER] + list(frame))
            escaped = "".join("\\%03o" % byte for byte in payload)
            # The control device requires one complete frame per write. Shell
            # printf may split writes, so materialize first and let cat send it.
            commands.append(f"printf '{escaped}' > \"$framefile\" && "
                            "cat \"$framefile\" > /dev/subucom_ctrl || exit 1; sleep 0.12")
        # The same counter/strobe frames and spacing as upstream knob(), timed
        # inside the guest so per-frame SSH latency does not dominate navigation.
        rc, out = azemu.ssh("sh 2>&1", data=("\n".join(commands) + "\n").encode(), timeout=90)
        if rc:
            raise RuntimeError(f"Emulator encoder injection failed ({rc}): {out}")
        drive._knob_set(counter)
        drive.send(drive.BUS_MIXER, drive._mixer())
    elif action == "tap":
        spec = importlib.util.spec_from_file_location("azdrive", tools / "az-drive.py")
        drive = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(drive)
        x, y = int(sys.argv[2]), int(sys.argv[3])
        if not 0 <= x < drive.W or not 0 <= y < drive.H:
            raise SystemExit("Touch coordinates outside emulator screen")
        q = drive.Qmp()
        q.cmd({"execute": "input-send-event", "arguments": {"events": [
            {"type": "abs", "data": {"axis": "x", "value": round(x / drive.W * 0x7fff)}},
            {"type": "abs", "data": {"axis": "y", "value": round(y / drive.H * 0x7fff)}},
            {"type": "btn", "data": {"button": "left", "down": True}}]}})
        try:
            time.sleep(0.3)
        finally:
            q.cmd({"execute": "input-send-event", "arguments": {"events": [
                {"type": "btn", "data": {"button": "left", "down": False}}]}})
    elif action == "load":
        deck, seconds = int(sys.argv[2]), float(sys.argv[3])
        if deck not in (1, 2, 3, 4) or not 0 < seconds < 1:
            raise SystemExit("Expected deck 1..4 and short-press duration (0,1)")
        spec = importlib.util.spec_from_file_location("azdrive", tools / "az-drive.py")
        drive = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(drive)
        # A lone injected frame can be missed by EP147's native input sampling
        # under load. Hardware keeps sending the pressed state until release.
        drive._hold(drive._mixer(33, drive.LOAD_MASK[deck], True), seconds)
    elif action == "stop":
        port, seconds = int(sys.argv[2]), float(sys.argv[3])
        if port not in (1, 2) or not 0 < seconds <= 10:
            raise SystemExit("Expected port 1/2 and duration (0,10]")
        spec = importlib.util.spec_from_file_location("azdrive", tools / "az-drive.py")
        drive = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(drive)
        frame = drive.ERP_IDLE[:]
        frame[24] |= 1 << port
        hold_erp(frame, drive.ERP_IDLE, drive.BUS_ERP, seconds)
    else:
        raise SystemExit("Unknown command")

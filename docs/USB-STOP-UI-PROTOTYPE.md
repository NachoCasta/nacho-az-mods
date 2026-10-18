# USB STOP interaction prototype

Validated on 2026-10-17 in the local emulator only. This phase validates input
ownership and the selector; it does not implement safe ejection for hub storage.
Firmware compatibility remains exactly `ep147-9e9454e357389ece`.

## Observed behavior

- Short USB STOP on the hub connector opens a surface listing only that hub's
  mounted sticks. Rows show the volume label and hub port; multiple mounted
  partitions on one disk are grouped into one row.
- Touch or the browse knob selects a stick. Request eject closes the selector
  and records an individual request. BACK or the touch Cancel button closes it.
- Holding USB STOP for 1500 ms records one whole-hub request. Release does not
  open the short-press selector or submit a second request. This threshold is a
  prototype UI choice, not a measurement of the native firmware hold threshold.
- Hub gestures withhold both native press and release, preventing premature
  stock ejection. A direct USB retains its original handler and complete event
  pair, even if observed hub presence changes between press and release.
- Both request paths leave hub filesystems mounted. Feedback explicitly says the
  request was recorded and storage remains mounted. No safe-to-remove claim is made.

Inventory filesystem reads run on a worker. The native button hook only updates
gesture state; the core tick handles UI actions. Surface callbacks do no storage
teardown or filesystem scanning. The selector is dismissed before reporting a
request, and native BROWSE controls remain usable afterward. No playback or audio
continuity test was performed in this phase.

The SDK reserves the knob for the Mod Menu while it is open. In that case the
prototype hides its selector and asks the user to close that menu; it does not
steal focus. Normal native views support the selector. This is an explicit UI
limitation to consider before production integration.

## Implementation and isolation

`src/azhub/linkfake/eject-gesture.h` owns the per-port gesture state and press-pair
invariant. `eject-ui-prototype.h` contains the inventory worker, custom surface,
request preview, and semantic hook at `0x21aeb50`. The firmware vtable entry at
`0x31bbd78` is checked against that expected function before claiming the hook.
The core rejected `claim_vslot` for that read-only vtable range, so the prototype
uses the supported `claim_hook` API for the dispatch function instead.

The code is excluded from normal builds by `LF_EJECT_UI_PROTOTYPE`. Its explicit
build target is:

```sh
make -C src/azhub/linkfake eject-ui-prototype OUT=out-debug CC=/path/to/pinned/aarch64-linux-gnu-gcc
```

It additionally requires `AZHUB_EJECT_UI_PROTOTYPE` in EP147's environment, exact
emulator kernel release `6.6.138-dirty`, `/dev/subucom_ctrl`, and at least one
explicit connector-root fixture. The acceptance run used
`AZHUB_EJECT_PORT2_ROOT=/sys/bus/usb/devices/2-1`. This is a QEMU fixture map, not
a validated real-AZ connector mapping. No hardware activation was attempted.

The regular Linaro build still has executable SHA256
`86f264c27644b399e07226827874b2e41c9a152925ba03c9479c4eb1aa7ee0ba`, identical
to the published 0.1.8 executable. No manifest version, package, catalog, tag or
release was changed. Experimental binaries remain ignored local build products.

## Acceptance evidence

[Structured emulator acceptance](acceptance/usb-stop-ui-prototype-emulator.json)
records the exact prototype binary hash, running EP147 PID, request log and six
integration checks. The harness verifies the hash through the process's mapped
file, not just the staged file's path. The observed selector and subsequent native
waveform screen were also inspected visually; captures stay local and ignored.

The emulator had one native USB1 stick plus two sticks below a USB2 hub. Checks
covered knob and touch selection, BACK and touch cancellation, exactly one
whole-hub request, withholding all native hub STOP calls, unchanged mounts after
hub actions, and native USB1 short/long forwarding. The direct USB1 long hold
actually exercised stock ejection in the emulator: USB1 unmounted, both hub sticks
remained mounted, and EP147 retained its PID. This is separate from the hub
prototype's no-unmount behavior.

Host source-name, browser-guard, network-identity, source-settings and new gesture
tests passed. Gesture tests also cover delayed ticks, repeated DOWN events and
topology changes during a held button. The regular host CI includes this test;
the emulator harness is an explicit local test, not a CI emulator claim.

Local harness command, after explicitly installing the prototype in the emulator
and providing the above mounted fixture:

```sh
python src/azhub/research/scripts/test-eject-ui-emulator.py \
  --binary src/azhub/linkfake/out-debug/usb-hub-support-eject-ui.so \
  --output docs/acceptance/usb-stop-ui-prototype-emulator.json \
  --shots src/azhub/research/shots
```

`eject-ui-emu.py` and the harness connect only to the local emulator endpoints
127.0.0.1:2222/4446. They do not install automatically or connect to the real AZ.
The emulator's old `hub-libraries` package was moved aside before loading the
stable-ID candidate so their hooks never ran together. During setup, deliberate
rapid service restarts triggered the existing startup quarantine; its counter
was reset only in this disposable local emulator. No crash is inferred from that
setup event. The final acceptance run used one unchanged EP147 process.
After acceptance, the original emulator package and staged executable were restored,
its original executable hash was rechecked, the experimental service environment
was removed, and the emulator was synced and stopped. Prototype builds and evidence
remain local; no experimental installation was left enabled for the next boot.

## Next phase

Connect recorded requests to a worker-owned safe-eject coordinator only after
validating device generations, loaded-source identity, operation admission and
native completion listeners. The current inventory is a UI snapshot of mounted
disks, not a safe-eject transaction identity or complete partition/export inventory.
It is capped at eight rows; devices beyond that and non-mounted storage need a
defined production UI policy. Physical connector mapping, native timing/cancel
behavior and indicator ownership still require separate validation before hardware
use. Real-AZ operations require new action-specific permission.

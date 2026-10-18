# Safe ejection: first emulator implementation

This records the first accepted emulator phase. Current candidate corrections,
exFAT support and the explicit AZ candidate build supersede the original limits
below; see [active release preparation](SAFE-EJECT-RELEASE-CANDIDATE.md).

The explicit `make safe-eject-emulator` target connects USB STOP to actual safe
ejection in the local emulator. It is excluded from the regular build, requires
`LF_SAFE_EJECT` and `LF_EJECT_UI_PROTOTYPE`, and refuses activation outside the
`6.6.138-dirty` emulator kernel. This is not a hardware-ready release. Firmware
addresses remain specific to `ep147-9e9454e357389ece`; compatibility has not expanded.

## Interaction

- Short USB STOP on a hub opens a touch/knob selector with volume labels and hub
  locations. BACK cancels. Selecting a stick closes the selector and starts work
  in the background; native controls remain available.
- A 1500 ms hold submits all currently observed devices on that hub once.
- Loaded or loading media refuses ejection, including paused tracks. Unload the
  track through the native interface before retrying.
- Completion displays a banner and a persistent "safe to remove" row. A failed
  unmount displays an error and allows retry; it never reports success.
- Direct native USB STOP remains routed to the original handler.

## Safety boundaries

The worker identifies a physical USB connection using its resolved sysfs device,
inode and device number. Partitions and LUNs under that connection are grouped;
labels and `sdX` names are not transaction identities. The connector roots must be
explicitly supplied for the emulator fixture. An overflowing whole-hub inventory
or a newly attached child prevents a whole-hub success claim.

Reservation blocks new native track-load admission, library request dispatch,
source announcements, configuration reuse and fallback mounting. Dispatch already
admitted is drained before native unmount tasks are queued. Track ownership uses
the full internal media identity, completion hooks and native per-deck state; a
track title or content ID alone never identifies a device.

Each source closes its own MusicLibraryServer and information server. Both native
completion callbacks must run before its alias is removed. The worker then flushes
filesystems with `syncfs`, closes its directory handle, uses ordinary `umount`,
and verifies no target mounts remain. Forced/lazy unmount and USB power switching
are not used. Raw block-device users and unsupported FUSE filesystems refuse
before withdrawal; ordinary open files can produce a real `EBUSY` after teardown.
Such a failure retains inhibition and permits retry after the external user exits.

An atomic journal in `/run/azhub-eject-state` preserves inhibition, labels and
completion across an EP147 restart. A different physical connection generation
restores discovery. This journal does not survive a guest reboot.

Exact-firmware reconstruction points used by the experiment:

| Address | Role |
|---|---|
| `0x21aeb50` | Native USB STOP handler; forwards non-hub gestures |
| `0xa7f688` | LoadController track-load admission |
| `0xa77ae4` | Load completion lambda prologue after its entry guard; preserves live argument registers |
| `0xa75bd0` | Native unload completion |
| `0xbc5ed0` | Storage controller per-deck loaded-media query |
| `0x1373928`, `0x1975468` | Per-source Music and information server asynchronous unmount |
| `0x2bb1268`, `0x2cafeb8` | Native server vtables; per-source overrides gate request dispatch |

The completion callback slot is `+0x18`. Callback storage remains alive until
process exit because queued native tasks can finish after a timeout. Reconstruction
uses the existing local ELF/Ghidra helpers; raw firmware output is not stored here.

## Reproduction and evidence

The local two-stick fixture uses xHCI hub `2-1`, with `usblib.img` and
`hublib2.img` at ports 1/2, plus a separate native USB1 stick. The guest renotifier
must exclude nested hub storage from native USB announcements; otherwise it
incorrectly creates an extra native library handle. The reconstruction helper is
`src/azhub/research/scripts/patch-renotify-hub-fixture.py`. Preserve and restore the
original guest script after testing. This is fixture setup, not a production fix.

Install the candidate explicitly in the emulator with its matching approved hash
and enable these service environment variables:

```text
AZHUB_EJECT_UI_PROTOTYPE=1
AZHUB_EJECT_PORT2_ROOT=/sys/bus/usb/devices/2-1
AZHUB_EJECT_STATUS=1
AZHUB_EJECT_TEST_PAUSE_MS=3000
```

The last variable is an optional bounded race-test delay, not a production delay.
Telemetry is an opt-in worker-written snapshot at `/run/azhub-eject-status`.
With empty decks and both hub sticks already safely unmounted, run:

```sh
python src/azhub/research/scripts/test-safe-eject-emulator.py \
  --binary src/azhub/linkfake/out-debug/usb-hub-support-safe-eject.so \
  --output docs/acceptance/safe-eject-emulator.json \
  --shots src/azhub/research/shots
```

The harness verifies both the staged and actually mapped binary hashes. It drives
native load/play/unload controls, ejects one source while the other advances,
tests whole-hub refusal/completion, open-file failure/retry, physical reconnection
and restart inhibition. Endpoints are restricted to localhost SSH 2222 and QMP
4446. It does not install a package or access the real AZ. Screenshots stay ignored;
audio output is not captured. The structured acceptance record identifies the
exact tested executable and observations. Ordinary builds retain the published
executable SHA256 `86f264c27644b399e07226827874b2e41c9a152925ba03c9479c4eb1aa7ee0ba`.

`test-safe-eject-boundaries.py` is a separate follow-up on the same fixture. It
checks refusal before withdrawal with a raw block-device handle, success after
that handle closes, and native USB1 short/long STOP forwarding. It restores native
USB1 for fixture cleanup. Its `--binary` and `--output` arguments identify the
candidate and structured boundary evidence independently of the main suite.

The accepted executable SHA256 for this phase is
`84d1445635707eba1447738c7725f04d10cce3a3eedbf0a93c0e9169f7b1a862`.
The main suite passed eleven checks; the separate
[boundary record](acceptance/safe-eject-boundaries-emulator.json) passed touch
submission, raw-device refusal/retry and native USB1 forwarding/ejection. The
original emulator package and staged executable were restored and both verified
as `6f03e2b5c9a36853f538019d7e4c1dc4123566c7be7cfab47da50e739bae7f98`.
The original renotifier was restored byte-for-byte, the experiment environment and
owner entry were removed, and the guest was synced and stopped. No experimental
mod is left enabled for the next emulator boot.

## Remaining limits

Physical connector mapping, hardware button timing, indicators, supported hardware
filesystems and real-AZ lifecycle behavior still require separate research and
action-specific hardware permission. FUSE teardown is deliberately unsupported.
Multiple partitions/LUNs and more than two simultaneous eject targets have not
been accepted end to end. Unexpected physical removal is an error, not safe eject.
External PRO DJ LINK remains incompatible.

The SDK can retain loaded metadata after a native missing-file load failure. If
that conflicts with native empty-deck state, the implementation conservatively
refuses eject with a changing-track message; a successful load followed by native
unload clears the ambiguity. Resolving this without weakening ownership checks is
follow-up work. The Mod Menu must be closed before the SDK permits selector focus.
On the native SOURCE screen, returning knob focus can also select the core's Mod
Menu row; a knob-click selection in that context opened the menu during a boundary
test and blocked the next selector until BACK. Touch selection is tested separately.
This SDK focus interaction needs a regression fix before production promotion.
Additional fault injection for load/dispatch races remains necessary before
promoting this experiment into the normal build.

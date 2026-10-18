# USB Hub Support 0.2.0

Adds safe ejection for USB sticks connected through a hub.

- Short-press the hub port's USB STOP button to choose one stick, using touch or the browse knob. BACK cancels.
- Hold USB STOP for 1.5 seconds to request ejection of the whole hub.
- Ejection runs in the background. Wait for the final safe-to-remove confirmation before unplugging; “Preparing USB for removal” is not permission to unplug.
- Native USB ports retain their normal behavior. This mod does not control their LEDs or cut USB power.

## Important limitations

Unload tracks from the selected USB before ejecting it. Loaded, paused, playing
or loading tracks prevent ejection in this version. Playback from cached tracks
after ejection is not implemented. A hardware trial kept the loaded source
mounted and usable, but the user saw only the preparation message rather than a
clear rejection. If no final safe-to-remove confirmation appears, leave the USB
connected. Support for ejecting a source with loaded tracks is follow-up work.

**PRO DJ LINK:** External-player communication remains blocked while this mod is
enabled. Disable the mod and restart the AZ before using external PRO DJ LINK.
Turning Link on does not disable the mod. Hub sources are not shared with CDJs.

Requires the exact firmware 2.00 tag `ep147-9e9454e357389ece`, kernel 4.4.194
and Community Mods core 1.2.3 or newer. Compatibility is not broadened. Eight hub
sources are implemented; eight-source hardware acceptance is not claimed.

## Validation and stability

The user tested this exact executable on their AZ with two hub sticks plus native
USB1. Short-press selection/BACK, individual eject, browsing/loading from the
remaining stick, reconnection, and whole-hub long press passed. Read-only SSH
confirmed both hub filesystems and library aliases unmounted after whole-hub eject,
with native USB1 still mounted. These are user-operated tests with SSH state
verification, not exhaustive hardware acceptance across views, themes or long sessions.

Local emulator scenarios cover source isolation, load races, failed loads,
partitions, exFAT, journal failures, mount timeouts and initialization failure.
Twelve reconnect/eject cycles retained endpoint descriptor/thread and VmData
counts; warm-to-final RSS decreased 16 KiB. Host components and production UI
passed plain, ASan/UBSan and TSan checks. Proprietary firmware internals remain
outside sanitizer coverage; this does not prove the entire mod leak-free.

Completion adapters are now bounded, per-source caches remain independent,
resources are prepared before hook claims, surface text is clipped, and workers,
helper execution and logging use SDK services. A helper timeout or uncertain
helper state prevents ejection across application reloads and asks for an AZ
restart. Unmounts are never forced. Temporary coordinator state is stored in RAM.

ELF SHA256: `09b8ffa89b73ff9c5c402d81955bd6c567c2b520edb5c3f38fd00adcf865bd5b`.
Package SHA256: `1c24d4ea3e73dbfc7650e4ac9c47ee310a10ee883c1efbea41455458e6f41f09` (350722 bytes).

See the [hardware record](../acceptance/usb-hub-support-0.2.0-az-stability-functional.json)
and [local evidence ledger](../acceptance/usb-hub-support-0.2.0-stability-summary.json).
The ledger records the local fixture snapshot correction and historical failures.
Previous release assets and tags remain unchanged.

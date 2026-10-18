# USB Hub Support 0.2.1

Safe eject now works with paused or playing tracks loaded. Short-press USB STOP
to choose one hub stick, or hold it to eject the whole hub. The mod asks the
native disconnect service to prepare playback caches, waits for completion and
then performs ordinary filesystem unmount. Pending work survives timeout/retry
without duplicating the native disconnect request.

Wait for the final **safe-to-remove** confirmation before unplugging. A preparation
banner alone is insufficient. On failure, leave the USB connected. Cached playback
uses the firmware's capacity and function limits; full-track retention for every
format, duration or workload is not guaranteed.

**PRO DJ LINK:** External-player communication remains blocked while this mod is
enabled. Disable the mod and restart the AZ before using external PRO DJ LINK.
Turning Link on does not disable the mod. Hub sources are local and are not shared
with external players.

## Validation

- Real AZ: the user confirmed individual loaded-track eject from NACHO PLUGS and
  complete playback of that one track afterward. Read-only SSH confirmed ordinary
  filesystem/library-alias removal with the physical device present, retained
  exact-media cache proof, unchanged application PID and mounted CAMI/native USB1.
  Audio is a manual user observation, not an agent recording.
- Exact-candidate QEMU: individual and two-playing-deck whole-hub eject, cached
  seek/loop, replacement media at the same port, failed-load recovery and reserved
  LOAD denial. A sample-exact capture confirmed deck audio after all hub backings
  were unmounted, with zero dropped frames.
- Thirteen portable suites passed plain, ASan+UBSan and TSan modes (39 runs), plus
  production SDK surface checks. Both compiler builds and packages validated.
  A separate guarded fault overlay tested delayed native completion and retry;
  that overlay is excluded from the release package.

## Known limits and compatibility

Twelve local loaded/eject/unload cycles kept owned references, pending requests,
threads and descriptors bounded, but native RSS grew by 244580 KiB from cycle 2
to 12. Long-session memory behavior remains unvalidated. QEMU also encountered
core input-filter budget failures in separately recorded attempts; successful
intervals and failed controls are distinguished in the evidence. Native cache
capacity, long-session hardware timing and expanded hardware cases need follow-up.
Two-loaded-source whole-hub eject is emulator-tested, not hardware accepted.

Compatibility remains firmware 2.00, exact tag `ep147-9e9454e357389ece`, kernel
4.4.194 and Community Mods core >=1.2.3. Eight hub sources are implemented;
eight-source hardware acceptance is not claimed. Sources retain independent
servers and caches without borrowing native USB library slots.

See [hardware acceptance](../acceptance/usb-hub-support-0.2.1-az-loaded-eject.json),
[local evidence](../acceptance/usb-hub-support-0.2.1-loaded-summary.json) and
[technical research](../LOADED-TRACK-EJECT.md).

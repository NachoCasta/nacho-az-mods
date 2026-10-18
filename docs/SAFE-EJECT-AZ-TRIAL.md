# Deferred real-AZ loaded-track trial

Status: published 0.2.1; the initial one-source trial is accepted. The user authorized preflight, exact selected
installation/hash approval, application restart and a bounded one-hub-stick
loaded-track eject trial. Activation and the initial loaded-source trial succeeded. The user reported
full-track playback after individual NACHO PLUGS eject; SSH confirmed absent
backing/alias mounts with the physical device present, a completed exact-media
cache proof, retained deck state and mounted native USB1/CAMI. See
acceptance/usb-hub-support-0.2.1-az-loaded-eject.json. This covers one track/source;
audio is user-reported, not instrumented.
Additional two-stick, replacement, fault, long-session or further restart actions
need their own authorization. Requirements below describe the original plan,
fulfilled only for the bounded initial scope.
Local builds and emulator evidence do not authorize installation, approval,
reload, restart or tests on the real AZ. Public release remains 0.2.0.

## Candidate identity

- Executable SHA256: `9feec49c06f10dd01e191ed5094f68eb1e7e7b35cd9ed76d757a8477a37a945b`.
- Package SHA256: `6a33b8dd496bb2130289cf7e49299b3cf24a29e4895afcf4c460184d37bd265f` (355839 bytes).
- Source snapshot and package validation: `acceptance/usb-hub-support-0.2.1-loaded-build.json` and `acceptance/usb-hub-support-0.2.1-loaded-packages.json`.
- No 0.2.1 hardware acceptance. The last installed hardware executable is public 0.2.0, SHA256 `09b8ffa89b73ff9c5c402d81955bd6c567c2b520edb5c3f38fd00adcf865bd5b`.

## Proposed authorized sequence

1. Obtain read-only preflight authorization. Verify exact firmware tag
   `ep147-9e9454e357389ece`, kernel 4.4.194, core >=1.2.3, existing executable,
   mounts and fresh deck state. Keep external PRO DJ LINK disconnected/disabled.
2. After local gates pass, obtain authorization to install and approve the exact
   candidate, restart the application and perform the agreed bounded trial.
   Preserve the current package/approval for rollback. Use the normal SDK flow.
3. With one hub stick and native USB1, check short STOP selector, BACK and target
   selection. Load a known-present track from that physical hub source, pause it,
   request individual safe eject and wait for the final safe-to-remove result.
   Verify the filesystem and library alias are unmounted and native USB1 remains.
4. User checks actual audio after playback resumes, seeks and loops. Repeat with
   a playing track only within the agreed scope. SDK flags alone do not establish
   audible continuation. Cache capacity and playback functions may be limited.
5. If separately included in authorization, connect two hub sticks, load one track
   from each and hold STOP to eject both while playing. Verify both sources
   unmount, user-observed audio continues and native USB1 stays available.
6. If included, replace a stick at the same port while its old track remains
   cached; load and eject the replacement independently. Manual unload afterward
   must recover. Any failure result means leave the USB connected.
7. Observe input responsiveness, descriptors, memory and threads. Keep fault
   injection, unplug-during-write and simulated timeouts in the emulator.

Record user observations separately from SSH measurements. Do not broaden firmware
compatibility or claim eight-source hardware acceptance from this trial.

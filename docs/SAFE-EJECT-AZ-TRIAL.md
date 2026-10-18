# Deferred real-AZ trial

Status: pending user availability and action-specific authorization. Do not run
this plan while the user is unavailable. Local builds and emulator evidence are
not authorization to install, reload, restart or test the real device.

## Candidate identity

Use the current approved-for-trial record after local gates pass, not a historical
0.2.0 package with the same version. The latest built local candidate is recorded
in `acceptance/usb-hub-support-0.2.0-stability-timeout-build.json`; matching runtime
checks are still underway. Public 0.1.8 is unchanged. The last real-AZ deployment
is the separately approved passive input diagnostic, whose hash is recorded in
CURRENT-STATE.md. No later fixes were deployed.

## Proposed later sequence

1. Obtain authorization for a read-only preflight. Confirm the exact firmware tag,
   kernel, core version, running executable, current mounts and fresh deck state.
   Confirm `/run` and `/tmp` backing filesystems for volatile coordinator state.
   Keep external PRO DJ LINK disconnected/disabled for this mod trial.
2. Review the exact selected package/ELF hashes and the existing backups. Obtain
   installation authorization separately from activation and functional testing.
   Use normal SDK approval and reload mechanisms only within the approved scope.
3. With a hub stick and native USB1 connected, short-press the hub port STOP while
   the panel has been idle. Verify selector opening, BACK/cancel, touch selection
   and knob capture without moving the underlying native browser selection.
4. Load a known-present track from the hub source. Verify refusal while loaded,
   paused, playing and during load; compare the physical source identity rather
   than matching only its title/content ID. Speaker output is user-observed.
5. After native unload, eject one hub stick. Require a completed success banner,
   ordinary filesystem unmount and continued operation of the other source.
   Add a second hub stick only as a separately agreed hardware test scope.
6. Verify whole-hub hold behavior, reinsert handling and persistence across an
   explicitly authorized reload. Confirm native USB1 controls and independent
   sources remain correct. Defer all destructive media fault simulation to the
   emulator; a failure banner always means leave the device connected.
7. Observe long-session input-filter health, UI responsiveness, memory, descriptors
   and threads on hardware. QEMU budget overruns do not validate hardware timing.
   Expand views/decks/themes coverage before publication acceptance.

If temporary logging is authorized, preserve its previous state and restore it
when the capture finishes. A rollback to a preserved stable package also requires
its own authorization; never switch or restart the real AZ as a build side effect.
Record actual observations, user reports and untested cases separately.

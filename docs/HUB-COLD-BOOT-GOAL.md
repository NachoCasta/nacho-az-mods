# Automatic hub discovery after cold boot

Status: active. The owner reports that a hub connected before power-on only
works after unplugging and reconnecting it. Release0.4.6 is the baseline.

## Required outcome

- Identify whether enumeration, mounting or library publication fails.
- Reproduce the condition in the emulator, explicitly modeling the hardware
  boundary when its permissive kernel does not reproduce the AZ restriction.
- Recover automatically after the guarded kernel patch is ready, without
  requiring physical reconnection or blocking the UI.
- Preserve native USBs, per-source server/cache independence, source numbering
  and metadata, analyzed waveforms, safe eject/reconnect and PRO DJ LINK.
- Add regression coverage for ordering, bounded retries, already-bound/root/
  non-hub devices and helper failures/timeouts. Keep exact binary evidence.
- Validate at least two physical cold boots with the hub and sticks connected
  beforehand, including source discovery, browsing, loading and user-confirmed
  playback. Separate instrumented state from manual audio/UI acceptance.
- Preserve the stable mod ID and exact firmware/kernel/core compatibility.
  Use the authorized USB/framework workflow; no manual internal AZ edits.
  Publication requires a separate owner decision.

## Confirmed boundary

The physical boot log rejects hub2-1:1.0 at0.758s with error-7, before azhub
loads at10.237s. The later physical reconnection succeeds. The existing mount
worker sees only block devices and cannot recover an unbound hub interface.
The previous emulator HFS+ cold-start check does not cover this4.4 kernel gate.
See [progress evidence](acceptance/usb-hub-support-cold-boot-progress.json).

The emulator model must start with the owned external hub present but its driver
unbound and children absent, then prove baseline failure and candidate recovery
without a QMP hotplug. This models the observed post-failure state; it does not
claim that the6.6 emulator kernel executed the AZ depth restriction.

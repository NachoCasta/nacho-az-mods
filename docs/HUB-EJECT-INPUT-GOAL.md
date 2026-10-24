# Reliable hub eject input during prolonged sessions

Status: active. Baseline release0.4.7 remains published and unchanged.

## Required outcome

- Diagnose the intermittent selector-unavailable banner without assuming that
  the owner's physical incident has the same cause as an emulator failure.
- Reproduce the failure deterministically and retain a baseline negative control.
- Open individual-source selection reliably through short USB STOP during
  prolonged normal operation, without restarting the mod or application.
- Retain safe control ownership and refuse unsafe input when ownership cannot
  be obtained. Do not disable core timing safeguards or manually modify core,
  firmware, internal settings or process memory.
- Preserve native controls, individual/whole-hub loaded eject, cached playback,
  waveforms, source metadata/numbering, independent libraries, startup recovery
  and PRO DJ LINK.
- Add fault-injection and extended-session coverage, then validate in the
  emulator before preparing an exact-hash physical candidate. Do not reload
  while decks are loaded or playing. Publication requires another owner decision.

## Current evidence

The owner supplied a photo of the unavailable banner after an extended playing
session; hub sources and ordinary operation reportedly remained functional.
The incident's original process logs have not been recovered. Read-only SSH
reached the current0.4.7 process but logging does not contain that incident.

The existing owned emulator process626 independently reproduces the exact
banner after a short USB2 STOP press. Its log contains two SDK filter shutdown
messages, each after eight calls exceeded200us. Both raw and edited serials are
stale. The process still has two tracks loaded. This proves one cause of the
symptom, not the cause of the owner's historical incident.

Source inspection shows that core filter strikes are cumulative, remain set
until application restart and use elapsed architectural-counter time around the
callback. A scheduling interruption can therefore contribute to the measured
interval; no trace yet proves which event caused the observed overruns.
Registration is init-only, so re-registering from tick is not supported.

The mod's current health detector also permanently latches a fault on a valid
panel read whose raw or edited serial did not advance. A later healthy stream
cannot clear it. Invalid-CRC frames and idle periods are excluded. This second
failure path needs a separate test and must not be conflated with SDK shutdown.

## Design investigation

Prefer the SDK surface's supported input/focus contract over continuous custom
frame filtering if its held-button and encoder-release behavior can be proven
safe. Removing the health gate alone is not a fix: the existing editing filter
also restores the physical encoder counter and suppresses held keys on release.
Any replacement must validate native SOURCE/BROWSE position, click/BACK release,
counter wrap, competing dialogs and true loss of control ownership. Simply
clearing a fault or raising a timing budget does not satisfy this goal.

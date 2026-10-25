# Reliable hub eject input during prolonged sessions

Status: complete for emulator-first candidate preparation. Baseline release0.4.7
remains published and unchanged;0.4.8 physical acceptance is not claimed.

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

## First replacement experiment

A private SDK-only prototype, hash
`50a64ac4e56347b42615a600200e144d5c4f5c5a36996dadf120174bb6db354d`,
opens the selector and moves its selection using SDK knob events. It is not an
accepted fix: native BACK changes the underlying SOURCE view after immediate
focus release. The SDK focus contract provides press events but no physical
knob/BACK release state; its touch UP events do not prove button release.
Existing production code remains unchanged. Do not promote this prototype or
replace its safety gate with an unconditional open. Preserve this negative
control when evaluating release ownership and encoder restoration.

## Integrated candidate and completed emulator gates

The SDK-only experiment above remains rejected. Unpublished candidate0.4.8
instead keeps the normal encoder modal and its release behavior when filters
are healthy. After an observation fault, short STOP opens a separate touch
surface without requesting physical focus. Encoder and BACK retain their native
functions; native view changes close the touch panel. This interaction is a
proposed fallback, not an assertion of owner UI approval or full encoder support
after SDK quarantine. Core timing limits and init-only registration are unchanged.

Both forced after-open quarantine and a subsequent unforced quarantine on the
normal exact emulator executable allow touch selection without process restart.
Transient health faults separately recover only after eight consecutive valid
healthy observations. Missing panel/surface resources, competing focus and
held-close state retain explicit refusal paths. Late and foreign-surface events
cannot submit another removal request.

The source implementation is committed locally as `e574561`. Both runtime
builds reproduce the recorded hashes. All106 portable components pass in each
plain/ASan+UBSan/TSan mode; actual selector-open coordinator controls and both
SDK UI presentations also pass. Package validation and diagnostic exclusion pass.
Mixed FAT/HFS+ waveforms, loaded whole-hub retained cache/advancing engines,
native Link Off/On and fresh HFS+ reconnect/individual removal have emulator
evidence. The individual22second sample reached its end before advancement
observation, so that individual measurement is not a playback pass.

The terminal duration control passes61 open/cancel cycles over1801.65 seconds in
the same normal process after unforced SDK quarantine, with zero new removal
requests. A subsequent cold startup verifies the exact runtime and automatic
FAT/HFS+ recovery in the explicit unbound-hub QEMU model, without hotplug.
Native USB fresh loading advances and renders both graphs. A final longer FAT
individual loaded-source eject confirms removed backing/alias mounts, retained
native/HFS+ neighbor, phase2 cache, advancing engine and both graphs. Final
native unloading reaches EEEE. The22second sample's earlier failed advancement
measurement remains recorded rather than being relabeled a pass.

The [requirement audit](acceptance/usb-hub-support-eject-input-completion.json)
and [exact package](acceptance/usb-hub-support-eject-input-package.json) close
the preparation deliverable. Physical installation and publication were outside
that completed milestone. The exact candidate was subsequently installed and
the owner reported general success and requested release; see the
[physical](acceptance/usb-hub-support-eject-input-physical.json) and
[publication](acceptance/usb-hub-support-0.4.8-publication.json) records. Physical
quarantine and prolonged-session acceptance remain unclaimed.

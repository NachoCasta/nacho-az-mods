# Safe-eject release preparation

## 0.2.0 publication decision

The user explicitly requested committing and publishing the current progress after
successful two-stick hardware trials. The exact accepted package is retained.
Loaded-track ejection remains unsupported; the missing refusal banner is disclosed
in the release notes and deferred with that follow-up. Broader hardware coverage
is not claimed. Earlier no-authorization/pending-hardware statements below are
historical; see the current hardware record and release notes.


Status: **local preparation complete; ready for a separately authorized AZ trial, not publication**.
Public 0.1.8 is unchanged. Earlier 0.2.0 candidates were explicitly installed
and activated on the real AZ; the first selector trial refused to open. The last
real-AZ artifact is the separately approved passive input diagnostic, not the
latest local stability revision. Further hardware actions require a new,
action-specific authorization.

Read [the publication audit](SAFE-EJECT-PUBLICATION-AUDIT.md) and
[CURRENT-STATE.md](CURRENT-STATE.md) for active hashes and remaining gates.
`acceptance/usb-hub-support-0.2.0-candidate.json` and the chronology below describe
the historical 057d45 executable. They do not validate later source changes.
The stability revision now includes event-driven panel readiness, bounded
completion adapters, direct alias mount syscalls, SDK-managed workers/helper
execution/logging, preallocated private tagged caches and clipped surface text.
Matching runtime checks and the bounded resource review passed. See the
[final evidence ledger](acceptance/usb-hub-support-0.2.0-stability-summary.json).
Hardware acceptance, a source commit, remote CI and publication approval remain deferred.

## Historical candidate investigation

## Implemented corrections

An earlier input investigation, preceding the current correction below, preserved both
SOURCE and BROWSE view IDs through capture and release, but its load requested
CID 2360 instead of expected playable CID 495. The waveform showed track number
135 and E-8306 NO FILE. The selected row immediately before capture was not
independently measured, so that run could not distinguish navigation setup from
capture drift. The regression now requires a successful CID 495 load from the
physical A filesystem before its BROWSE capture assertion.

That new precondition passed on the same binary: CID 495 loaded from physical A
with native completion result 0. The run then failed in its unload helper before
BROWSE capture. Inspection showed the helper's chevron coordinates did not match
the retained waveform layout. The helper now explicitly selects 2 DECK and uses
header coordinates `(310,188)` / `(310,596)` in the emulator's 2560x1600 input
space. A separate emulator interaction verified the deck-2 unload confirmation
and native unload-completed event; all four SDK deck states were then empty.
This is fixture repair, not a passing full input regression. The earlier NO FILE
attempt had been recovered by an explicit localhost-only EP147 restart after
confirming result 1 and no playing deck. No physical AZ operation was performed.

Firmware RTTI identifies `0x22d9fc8` as
`device_adapter::BrowseRotaryEncoderAccelerator`; the separate
`meow::HuiEncoderDefaultAdapter` dispatch at `0x23647b0` returns its input unchanged.
Those RTTI facts alone did not establish which path handled the tested browse
turns. The later instrumented observations and replacement hook are described
below. Raw research stays local.

The newest input correction is local binary
`057d45d1758b4aa573d8ca08a997d09725c07ecbc4f970ae40677efbed0ac112`,
compiled successfully as normal and explicit candidate targets. The preceding
instrumented binary `8b4c8a5d0ed1e8208c5f142144e2861082198e1ba402211cc9b1d7a9f5ced50f`
received SDK turns but recorded zero nonzero-input accelerator hook calls in both
SOURCE and BROWSE. A localhost-only, forwarding observer probe then observed
delta 1 at `0x2360f80` for EasyID `0x01d690928446227a` while only the browse knob
was injected. The new correction replaces the accelerator hook with that shared
notification hook, scopes suppression to this exact ID, copies its 12-byte
payload and clears only its turn field. It verifies the first instruction and
the known encoder-component vtable slot before claiming the hook. Other encoder
IDs and push-state fields pass unchanged. The full input regression passed on
this exact binary: 100 captured turns preserve SOURCE; another 100 preserve the
BROWSE selection; CID 495 loads from physical A both before and after capture;
the loaded track prevents ejection; native unload completes; all decks finish
empty without filter disable or process restart. Evidence is
`acceptance/usb-hub-support-0.2.0-input.json`. Previous failed input records remain
historical evidence. The filesystem/topology suites and package validation
still need acceptance on this exact executable before a hardware trial.
The temporary probe hook has been removed from source; its emulator-only service
drop-in remains scheduled for removal by the local restore script.

The selector requests SDK surface focus and receives knob/click/BACK through its
input callback. Raw and editing filters observe physical key releases, restore
the physical counter byte after the core's focus freeze, and report activity to
the UI thread. The shared encoder notification hook at `0x2360f80` suppresses
the identified browse encoder's turns during capture. A guarded
ListBox hook at `0x2355270` suppresses the core's synthetic Mod Menu selection
on focus release. Captured keys remain swallowed through release; closing has a
bounded fallback if input stops. The input regression above passed on the new
notification hook; earlier failing SOURCE runs used the accelerator hook.
Earlier filesystem acceptance does not establish filesystem correctness for
this revised executable.

A completed native load failure is tracked per controller/deck and native request
record. When the native loaded-media query says empty, matching residual SDK
metadata no longer prevents ejection. A new load invalidates that exception.
The native map entry, rather than smart-pointer storage addresses or a possibly
cleared pending field, correlates the completion with the admitted request.
Physical ownership uses the complete outer member identity and USB/SD type and
instance. Native browser and server-registry representations can differ in the
nested LocalMediaID member byte; track-validity flags also do not identify storage.
`eject-source.h` normalizes those representations while preserving all outer-member
bits and device coordinates. Host cases verify member, kind and slot isolation;
new emulator acceptance is required for this correction.

The native TrackLoader execute entries at `0xab2ab8` and `0xab35b8` reject
reserved-source loads before early metadata preparation and queuing. Common
processing at `0xa7d0f8` and the final `doLoadTrack` boundary remain guarded as
well. A native trace showed why common processing alone was too late. Actual
LOAD rejection and subsequent valid-load recovery passed on the intermediate
executable recorded in `acceptance/safe-eject-load-race-emulator.json`; this
requires repetition on the final candidate.

The raw frame observers perform only bounded arithmetic/atomic operations. Input
diagnostic logging is drained on the UI tick, because logging/timing in a bus
callback contributed to the SDK disabling that observer during a prolonged run.
The emulator helper now repeats physical LOAD frames until release; a single
injected frame could be missed by native input sampling under load.

Stock exFAT-FUSE is recognized by the daemon's executable, USB device argument,
mounted filesystem and live `/dev/fuse` descriptor. Its own raw handle is expected;
other raw users still prevent ejection. Normal unmount and daemon closure replace
the earlier blanket FUSE refusal. Both a plain exFAT volume and an exFAT Rekordbox
library with its native bind alias passed busy-file refusal/retry and actual
unmount tests. Both independent library-server callbacks ran before alias closure.
Another FUSE filesystem remains unsupported and refuses before withdrawal.

Intermediate evidence is in `acceptance/safe-eject-regressions-emulator.json`,
`acceptance/safe-eject-exfat-emulator.json` and
`acceptance/safe-eject-exfat-library-emulator.json`. These identify their own
tested executable hashes; they do not establish acceptance of a later rebuild.

## Build and packaging boundaries

`make az-candidate` explicitly builds `usb-hub-support-az-candidate.so` with the
eject flow enabled for the existing exact firmware and kernels 4.4.194 or the
local emulator's 6.6.138-dirty. The regular build now uses the same flags and
produces the same executable as the explicit candidate target. Source version
0.2.0 remains unpublished while validation is underway. No build performs
installation or deployment.

The candidate defaults the two front connector roots to
`/sys/bus/usb/devices/1-1` and `2-1`, matching the stock `device-mount.sh` constants
`USB1_BUS_NAME` and `USB2_BUS_NAME`. Resolution happens on the inventory worker,
so an absent connector at initialization can appear later. Explicit root overrides
are emulator fixtures. Real-AZ read-only preflight must confirm this mapping.
The firmware tag and instruction-checked GPL kernel asset are unchanged.

`src/azhub/research/scripts/prepare-eject-candidate.py` stages only the selected
ELF, manifest, icon, licenses and three existing assets under ignored `local/`.
It uses the pinned SDK packer to validate the directory and package and writes
hashes to `candidate.json`. It does not modify the source manifest, root catalog,
release tags or public assets. The SDK settings-partition warning remains the
existing optional legacy-config read; ejection state is written under `/run`.

## Local acceptance ledger

The latest candidate and exact input evidence are identified above. The first
sequential run stopped at native USB1 STOP after completing the main FAT suite.
Subsequent boundary repetitions and the final normal-timing main/STOP sequence
passed. Every accepted record finished successfully with the selected executable
hash. The original failure is retained below; later success does not establish
its precise cause.

The load/reservation race and failed-load/SOURCE regressions now passed on the
same executable; their records are `acceptance/usb-hub-support-0.2.0-load-race.json`
and `acceptance/usb-hub-support-0.2.0-regressions.json`. A reserved source rejects
native LOAD before queuing; a later successful load from physical B still blocks
its ejection. A completed native file-load failure permits actual unmount despite
matching stale SDK metadata. A prior focus-only attempt that generated no LOAD,
and a wrong-source setup rejected by physical identity, remain documented in
`acceptance/safe-eject-load-race-focus-investigation.json`.

All six host behavior suites and the pinned ARM64 SDK build/package validation
passed. See `acceptance/usb-hub-support-0.2.0-build-checks.json` for exact compiler,
image and output hashes and reviewed warnings. The SDK output is a separate
build-only artifact, not the selected Linaro runtime candidate. The initial local
ARM64 execution failure was resolved by registering Docker's ARM64 interpreter;
the pinned Dockerfile and toolchain were unchanged. The final run without the
artificial eject pause passed, with diagnostic status/load tracing retained.
It also verified physical knob navigation immediately after capture. Records:
`acceptance/usb-hub-support-0.2.0-normal-emulator.json`,
`acceptance/usb-hub-support-0.2.0-normal-boundaries.json`, and the updated input
record. `acceptance/usb-hub-support-0.2.0-startup-remount.json` separately proves
startup revocation and a real retry on the same executable.

The main FAT suite has now passed all 11 end-to-end checks on executable
`057d45d1758b4aa573d8ca08a997d09725c07ecbc4f970ae40677efbed0ac112`.
Its evidence is `acceptance/usb-hub-support-0.2.0-emulator.json`. This includes
independent server closure, ejecting B while A playback advances, paused/loaded
and whole-hub refusal, real EBUSY and retry, whole-hub completion, reconnection
and persisted mount inhibition after an intentional EP147 restart. Native USB1
remained mounted. Boundary, exFAT, topology and partition suites are subsequent
gates, not implied by this main-suite result.

The same executable now also passed all six topology checks and the three
two-partition checks. See `acceptance/usb-hub-support-0.2.0-topology.json` and
`acceptance/usb-hub-support-0.2.0-partitions.json`. The partition restart fixture
initially missed its first STOP DOWN even after the core's loaded marker; only
UP reached the hook. An initialized-session retry and a separate cold restart
with a ten-second input-settling period both completed the remaining unmount.
The full partition test then passed with that explicit startup preparation.
This changes the emulator fixture, not the ELF. The earlier failures remain in
`acceptance/safe-eject-partition-startup-investigation.json`; the precise native
startup timing has not been established by these observations.

The boundary run completed touch submission, raw-device refusal and retry, then
failed because native USB1 stayed mounted after its forwarded long STOP. A
separate 5-second hold also left it mounted. With community mods disabled in an
explicit fresh emulator EP147 session, the same 5-second gesture unmounted USB1.
This comparison changes both enablement and session state. Restoring the same
candidate in another fresh EP147 session also allowed native USB1 to unmount
with a 5-second hold. A general forwarding defect is therefore not established;
the boundary sequence is being repeated to isolate session history or timing.
Both subsequent complete boundary runs passed on the same executable; the second
ran without an EP147 restart. They are recorded in
`acceptance/usb-hub-support-0.2.0-eject-boundaries.json` and
`acceptance/usb-hub-support-0.2.0-eject-boundaries-repeat.json`. Both use the
original 2.3-second long hold and restore native USB1 at completion. This does
not explain the first failed session. The local continuation now proceeds with
exFAT, topology and partitions; final readiness must retain this investigation
and verify the complete sequential path rather than erasing the failure.
See `acceptance/safe-eject-native-stop-investigation.json`. Later suites were
not reached by this run and their older records do not validate this executable.

### Historical failed runs and corrections

The latest sequential final run stopped at the second short STOP request: native
events classified it as a hold and refused whole-hub ejection while a track was
loaded. The SDK also disabled a frame filter after eight over-budget callbacks.
This run did not complete the main suite or the later boundary cases.

A focused test then proved a stale-safe defect: independently remounting a still
connected, successfully ejected stick left its row in state 2. The correction
rechecks completed results on the inventory worker and before request preflight,
revokes safe state to retry-required state 3, retains mount inhibition and reports
the change. Its new executable requires fresh acceptance; previous hashes do not
cover the correction. Focused acceptance of executable
`907e9b2c9df9adda520090caf3d4c6b4c1738553d638601efe4bfdcdc3d509ff`
is recorded in `acceptance/usb-hub-support-0.2.0-remount.json`: startup and live
inventory revoke stale safe state, and two retries actually unmount the reopened
filesystem. This is a narrow emulator regression, not full candidate acceptance.
The local package was regenerated and passed pinned-SDK validation; final
sequential acceptance is still required.

The emulator STOP helper now times pressed frames and 300 ms of release inside
one guest shell. On Windows, the upstream sender uses SSH for each frame while
EP147 runs; waiting for that connection can extend a requested short press past
the native hold threshold. A lone release frame could also be missed by native
sampling. This fixture correction does not establish the cause of SDK
frame-filter time-budget failures; those remain under investigation.

The first guest-side helper counted iterations, which still added process-spawn
overhead under TCG and reproduced the false hold. The corrected helper bounds
the press by elapsed `/proc/uptime` time instead. Three consecutive requested
500 ms presses then opened the selector while another USB track played; native
BACK cancelled all three. The sequential acceptance run must be repeated with
this corrected transport; its earlier failed run remains a failed run.

A later run loaded the intended title from the wrong stick: its browser capture
showed USB 2, and native request identity plus the bind mount proved it was B.
Ejecting A correctly completed while B remained loaded; the run failed its
loaded-A precondition. The two fixture libraries have identical titles/content
IDs, so these cannot establish source ownership. Native touch injection now
holds for 300 ms, and load checks decode the traced native member/media instance
and compare the alias's filesystem device with the intended backing mount before
requesting ejection. Knob frames now use one guest connection with the same
counter/strobe sequence and 120 ms detent spacing, instead of a separate SSH
connection per detent. The control device requires complete-frame writes;
encoder packets are materialized in a temporary file and sent by cat because
shell printf can split its writes. These are local fixture corrections, not
changes to the packaged executable.

Further captures showed that A was missing from native SOURCE while its backing
mount and owned bind alias were present and both sources had answered media
queries. The first source had been announced before the owned servers were
constructed. Repeating the mount announcement alone did not restore its row;
removing and reintroducing that native media announcement did, without a physical
disconnect. The 300 ms touch change is therefore not a complete explanation or
fix. First-member packet ordering and initial source registration are being
measured on the unchanged candidate before choosing a runtime correction.

Packet timing then proved that the late first-source announcement preceded its
member's first loopback keepalive by 504 ms. Waiting for owned server registration
was tried locally but blocked bootstrap: initial media requests create those
servers. That approach is not the current source. The current correction instead
waits for a native status packet addressed to that internal member on port 50002
before announcing its first media. Additional slots share the established member
transport. Dispenser-thread construction and independent caches remain unchanged.
On executable `8f1d3c7b93f28841a5fe0aa5c0cc516a469e317a1bb900eb24cec3353ad676df`,
initial visual interpretation suggested that adding B removed A's SOURCE row.
Later inspection did not establish that interpretation or a B-triggered removal.
This historical run is not passing initial-source acceptance. A temporary,
separate emulator diagnostic traced native row removal; it is disabled and is
not part of the candidate package.
The package was regenerated for this executable and passed SDK validation, but
is not ready for hardware or publication.

Another cold run showed A missing even before B connected, without a native
row-removal callback. Thus a distinct B-triggered removal is not established.
Investigation also found that a newly constructed owned server can receive its
first library request before the injector's delayed mount task is queued. The
current source queues each pair's mount tasks before dispatching its first
request, under the same admission accounting used by ejection. Mount producers
serialize with source reconciliation; the mounted flag is published only after
both tasks are queued. This ordering correction is included in the current
package. Earlier main FAT acceptance covered executable
`5fd231dc500e956f972d17096abef6a3475594911a75d9e470c10f5437aa31c1`;
the later acceptance results for the current executable are recorded above.

Focused host behavior checks passed. The normal and explicit candidate builds
match, source CI includes candidate compilation and the new host cases, and
offline package-member verification passed. The preserved SDK Dockerfile matches
the pinned upstream Git blob `072f5ac77e361c81029861500d48aeb574d6b1e4`.
There is no remote CI result for the uncommitted development source. The later
successful local SDK-container compilation is recorded above.

### Local closure and remaining gates

- Normal-timing main/STOP and input checks passed; original failures and startup
  observations remain beside the successful evidence.
- Required runtime records, package membership and 21 build inputs were
  correlated with the selected executable. The SDK output remains separate.
- Source, compatibility, release notes and reviewed warnings were checked;
  untested hardware/formats/LUN layouts remain explicitly unvalidated.
- Original emulator packages and renotifier were restored and hash-verified.
  Test filesystems were ordinarily unmounted and QMP quit completed; ports 2222
  and 4446 closed. See `acceptance/usb-hub-support-0.2.0-emulator-cleanup.json`.
- Rollback 0.1.8 package SHA256 was reverified as
  `320949f7178084d98423470bc9888452cdf415c07257d2ec6b7b00d4a623d44f`.
- Remaining: specific permission for real-AZ installation/activation/trial, final release
  source commit/tag, and the user's publishing decision. Source version is
  0.2.0; public catalog and immutable published assets remain at 0.1.8.

Optional emulator diagnostics are `AZHUB_EJECT_STATUS`,
`AZHUB_EJECT_TRACE_LOAD` and a bounded `AZHUB_EJECT_TEST_PAUSE_MS`. They are not
hardware installation requirements; test delays are absent by default.

## Real-AZ trial and publication gates

**Current hardware blocker:** the first user-operated short STOP attempt showed
the selector input-unavailable warning. Both raw/edit input serials and their
observed timestamps were zero in read-only inspection, despite eject support
being enabled. The cause is not established; do not publish or treat emulator
input acceptance as hardware proof. See
`acceptance/usb-hub-support-0.2.0-az-selector-failure.json`.

Read-only preflight was authorized and performed on 2026-10-18. Kernel 4.4.194,
core deck 1.2.3 and running tag `ep147-9e9454e357389ece` match the candidate's
requirements. Installed 0.1.7 has the known stable executable hash. The user then
connected one hub stick: hub `2-1`, storage `2-1.4`, FAT32 backing mount and
read-only library alias were observed while native USB1 remained mounted.
Existing deck telemetry was fresh and reported `EEEE`. These observations do
not validate candidate ejection. The loader's `.deck-info` reports `2.01`; no
compatibility expansion is inferred. The user confirmed that `2.01` denotes firmware 2.00
with the AZ-MODS patch, resolving the version-label question by user report.
The user subsequently authorized installation: all seven candidate members were
verified on the mods USB, with the original 0.1.7 directory preserved in
MODS/backups. Subsequent user-authorized apply/reload and hash-specific SSH
approval activated 0.2.0 with 34 hooks. Native USB1 and the hub source remained
mounted. Functional trials remain unauthorized. See
`acceptance/usb-hub-support-0.2.0-az-install.json` and
`acceptance/usb-hub-support-0.2.0-az-activation.json`.
Evidence: `acceptance/usb-hub-support-0.2.0-az-preflight.json`.

Present the concrete candidate package, hashes, compatibility, results and this
trial plan to the user before requesting any real-AZ action:

1. Ask for read-only SSH preflight. Verify exact firmware/kernel/core, current
   mod, connector sysfs paths, filesystem types and that decks are unloaded.
   Read current state without changing the user's Link setting.
2. Prepare the immutable 0.1.8 rollback package and verify its known hash. Ask
   before copying/installing the candidate. Keep the stable ID and avoid enabling
   legacy `hub-libraries` alongside it.
3. Ask separately before enabling/reloading or restarting the real AZ. Begin
   with two hub sticks and an independent native stick, matching the accepted
   physical setup; do not start with eight-source claims.
4. With permission for the stated test sequence, verify paused/playing refusal,
   single-stick eject while another plays, whole-hub eject after native unload,
   safe reconnection and unchanged native STOP behavior. Use the screen's result;
   the coordinator does not currently drive the native port LEDs.
5. Record actual observations, package/ELF hashes and failures separately from
   emulator evidence. Public publication requires the user's publishing decision
   and the release checks in RELEASING.md. Do not replace 0.1.8 assets or tags.

External PRO DJ LINK remains incompatible and its prominent warning stays in
the candidate manifest. Rear USB-C hosting is a separate, unresolved feature.

The local rollback package is prepared at
`local/rollback/usb-hub-support-0.1.8.azmod`. Its SHA256 was verified as
`320949f7178084d98423470bc9888452cdf415c07257d2ec6b7b00d4a623d44f`
and SDK package validation passed. This preparation did not deploy it anywhere.
## Historical rejected input candidate (2026-10-17)

The local Linaro candidate `d219d04541e2a5cb6dc70e07abcd5c35d44fbf0a49495012700ce1f4c11f9159`
publishes raw/edit input heartbeat counters on every valid mixer frame rather
than once per 64 frames. The opening refusal now logs both observed counters
and their ages. Both normal and explicit candidate targets compiled successfully.
This binary has not been packaged, published or tried on hardware.

The localhost input regression opened and cancelled the selector successfully,
including one logged suppression of the core's synthetic SOURCE focus pin.
However, the subsequent 100-turn SOURCE capture test failed: after BACK,
telemetry reported native view 5 (Mod Menu) rather than expected view 1 (SOURCE).
The process remained alive, both hub devices remained mounted and all decks
were empty. No filter-disable message appeared in this run's log at the failure.
Input ownership is therefore still unresolved; this candidate is not accepted.
The next investigation must distinguish core menu activation during capture
from activation on focus release instead of treating the initial suppression
log as sufficient proof of isolation.


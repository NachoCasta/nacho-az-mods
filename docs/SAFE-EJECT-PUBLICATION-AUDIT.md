# Safe-eject publication and stability audit

Publication decision: the user explicitly requested publishing the accepted 0.2.1
progress. Exact candidate bytes are retained. Native long-session memory/input
behavior and expanded hardware cases remain follow-up work, disclosed in the
[release notes](releases/usb-hub-support-0.2.1.md). Earlier deferred/unauthorized
statements below preserve the research and acceptance chronology.


## Unpublished 0.2.1 loaded-track candidate

Public release is 0.2.0. Current loaded-track source, exact candidate hashes and
local acceptance are in [the active research/evidence ledger](LOADED-TRACK-EJECT.md).
The new portable detach model raises the host gate to 13 suites / 39 runs;
production SDK checks remain 10/8/8. Both candidate packages passed validation.
No new real-AZ access or 0.2.1 publication is authorized. Native long-session
memory behavior remains unvalidated; a local QEMU regression also encountered
core input-filter budget enforcement, with a fresh-process repeat recorded.

The 0.2.0 status, counts and hashes below preserve its publication chronology;
they do not validate the new 0.2.1 executable. Follow the updated
[deferred loaded-track AZ trial](SAFE-EJECT-AZ-TRIAL.md) only after authorization.


## 0.2.0 publication decision

The user explicitly requested committing and publishing the current progress after
successful two-stick hardware trials. The exact accepted package is retained.
Loaded-track ejection remains unsupported; the missing refusal banner is disclosed
in the release notes and deferred with that follow-up. Broader hardware coverage
is not claimed. Earlier no-authorization/pending-hardware statements below are
historical; see the current hardware record and release notes.


Status: **local preparation complete; ready for a separately authorized AZ trial. Publication is not cleared.**
Public 0.1.8 is unchanged. The first 0.2.0 hardware selector attempt failed;
the selected revision has not been deployed or hardware accepted.

The [final evidence ledger](acceptance/usb-hub-support-0.2.0-stability-summary.json)
identifies the selected executable, packages and matching test records.
The audit uses the [official guidelines and checklist](https://kyle-hosman.github.io/az-mods-sdk/GUIDELINES/#checklist-before-you-publish)
and SDK v1.2.3 (`62a156abf3a4789e9e3cdd610fe106227a3a8967`).

| Requirement | Final local result / remaining gate |
|---|---|
| Build, package validation and warning review | Selected Linaro and independent SDK builds/packages pass. All seven members checked against source. Normal and explicit candidate outputs match. Optional legacy settings-read warning reviewed; no new persistent settings writes. |
| Plain / ASan+UBSan / TSan | Twelve portable suites pass all three modes (36 runs). Production surface SDK checks pass 10/8/8; no-I/O/no-allocation measured in plain mode. Official planted controls detected with Clang. Native firmware adapters are outside sanitizer coverage. |
| Bounded callbacks | Filters use bounded field/atomic operations; UI drawing clipped. Filesystem/inventory/helper work runs on SDK workers outside UI/load locks. SDK filter timing enforcement remains enabled. Real hardware timing remains a gate. |
| Constructors / exports / SDK services | Only toolchain frame_dummy init-array entry. One mod API export plus normal toolchain symbols. No direct system/popen/fork/vfork/clone/pthread_create/posix_spawn imports. Workers and helper processes use SDK services. |
| Ownership and initialization | Sixteen fixed completion slots replace unbounded retention. Private per-source caches and registration resources prepared before claims. Final-claim refusal test passes worker exit, absent aliases and responsive native USB UI. See [lifetime review](SAFE-EJECT-LIFETIMES.md). |
| Functional safety | Matching main eject, input capture, native STOP, topology, partitions, reserved/failed loads, exFAT and journal-failure tests pass. Mount timeout or unknown helper state inhibits eject and remains inhibited across application reload. No false safe result. |
| Resource stability | Twelve reconnect/eject cycles: endpoint FDs 58, threads 279 and VmData unchanged; warm-to-final RSS decreased 16 KiB. Selector/knob/BACK still worked afterward. This bounded observation is not proof of a leak-free proprietary runtime. |
| Firmware / identity / disclosure | Exact firmware unchanged. Independent servers/caches retained; unknown media refuses eject. Description discloses STOP changes, loopback, state writes, GPL module and external Link restriction. |
| File writes | Bounded volatile journal (16 records), empty mount-fault marker under /run and bounded /tmp configuration are deliberate RAM-state exceptions to the data_dir good habit: safety state must outlive removal of the mod's USB. Emulator /run and /tmp are tmpfs; verify their backing during the later authorized AZ preflight. No music writes intended. |
| Public source / license / hygiene | License and source prepared; current changes remain uncommitted. No firmware, keys, music, databases, screenshots or raw analysis should be committed. Source commit and remote CI remain publication gates. |
| Hardware / publication decision | Deferred: views, decks, themes, physical removal/reinsert, timing and long session. Follow [AZ trial plan](SAFE-EJECT-AZ-TRIAL.md). Publishing still needs the user's decision and [release procedure](RELEASING.md). |

## Fixture correction and limits

Final inspection found that QMP hotplug reopened the original local A/B emulator
images as writable raw nodes; CLI snapshot protection did not cover those nodes.
No baseline hashes exist, so original image immutability cannot be established.
No write to those originals was confirmed. Earlier blanket snapshot-only claims
are superseded. This concerns local fixtures, not the physical AZ.

The helper now refuses writable raw backing and retains a named qcow2 overlay
across hotplug. A separate paused QEMU with a new disposable 16 MiB image passed
write/read persistence, unchanged base SHA256 and unsafe-node refusal. See
[storage protection evidence](acceptance/usb-hub-support-0.2.0-storage-protection.json).
The firmware emulator and disposable test VM were stopped after cleanup.

Earlier failures remain recorded: idle-heartbeat hardware refusal, SDK text-area
overflow, listener retention, duplicate SDK launch argument and SDK filter budget
refusal after an older stress run. The final selected process passed its post-stress
selector check without bypassing budget enforcement. GCC sanitizer-control and
Docker seccomp limitations were resolved by the recorded Clang/unconfined host
backend, not counted as passes on the failing backend.

## Historical investigation log

The following entries preserve intermediate observations. Pending-work statements
below are historical; the ledger above and final summary define current status.

# Safe-eject publication and stability audit

Status: **in progress; publication is not approved or technically cleared**.
This audit follows the user's 2026-10-18 request to complete the eject flow and
check every non-hardware requirement, including resource stability. Public 0.1.8
remains unchanged. The first 0.2.0 hardware selector attempt failed; emulator
acceptance does not override that observation.

References:

- [Official guidelines and publication checklist](https://kyle-hosman.github.io/az-mods-sdk/GUIDELINES/#checklist-before-you-publish), retrieved 2026-10-18.
- Pinned SDK v1.2.3, commit `62a156abf3a4789e9e3cdd610fe106227a3a8967`.
- [Repository release procedure](RELEASING.md).
- [Candidate gates and evidence](SAFE-EJECT-RELEASE-CANDIDATE.md).

## Checklist ledger

| Requirement | Current evidence / remaining work |
|---|---|
| Package validation and warning review | Selected candidate and separate SDK build validated previously. Repeat on final changed artifact; retain warning review. |
| Plain, ASan + UBSan, TSan runs through `make test` | Final `make test` passes twelve portable suites in all three modes and the actual production surface callbacks through the SDK harness (10/8/8 checks). All official planted controls were detected with Clang in one final run. Native firmware adapters remain outside host sanitizer coverage. See `acceptance/usb-hub-support-0.2.0-stability-host.json`. |
| Setup in init, no active constructor | Final selected ELF has only the toolchain `frame_dummy` init-array entry. Community setup runs from `community_init`; standalone research remains a separate profile. |
| Busy callbacks remain bounded, no direct file/network work | Raw/edit filters use bounded field/atomic operations. Production surface callbacks passed SDK no-I/O/no-allocation checks in plain mode. Inventory/journal/mount work runs on workers, outside UI/load-admission locks. Native library construction/enqueue adapters remain outside host sanitizer coverage. SDK filter budget enforcement stays enabled. |
| Program execution uses SDK services | Audit found shipped `system`, direct `pthread_create` and `posix_spawn` paths. Local changes replace alias shells with mount syscalls, use SDK transport threads, and SDK launch for fallback helpers. Final Linaro/SDK builds and import review passed; matching fallback-helper integration is still pending. |
| Unknown deck/identity handling | Existing source/deck tests are partial evidence. Audit retained pointers and every fallback, including allocation failure and unrecognized loaded media. |
| Exact firmware declaration and hook ownership | Manifest and descriptor agree on `ep147-9e9454e357389ece`. Local initialization now prepares resources and workers before installing claims; workers wait on atomic readiness. A targeted rejected-final-claim fixture passed with workers stopped, no hub aliases and responsive native USB UI. |
| Public source, license, content hygiene | Public repository exists; final 0.2.0 source is still uncommitted/unpublished. Final source/tag and tracked-content review remain required. No firmware or raw analysis may enter the release. |
| Description discloses behavior | Source description now discloses loopback servers/caches, STOP changes, temporary state writes, the checked GPL kernel module and external Link restriction. Exact firmware and minimum core remain unchanged. |
| Hardware views/decks/themes/long session | **Explicit hardware gate, not waived as passed.** User requested preparation excluding tests that require the real AZ. Record remaining cases for the later authorized trial. |

## Confirmed resource defect and local fix

`safe_new_listener()` allocates completion adapters for server withdrawal and
retains them until process exit. Each source has two adapters per newly dispatched
attempt; reconnects and incomplete retries can accumulate allocations. This is
unbounded retention, not an accepted fixed cache. The local revision now uses
sixteen fixed slots and resumes pending withdrawals on retry. Exact native
completion/destructor control flow supports reuse only after completion; see
[lifetime evidence](SAFE-EJECT-LIFETIMES.md). Component tests pass all three
sanitizer modes, including delayed completion and 100,000 cycles. Native integration and resource observation on the final selected ELF are still being completed; earlier component and runtime evidence remains separately identified.

## Required resource review

- Attribute ownership and bounds to every server, SQL/tagged cache, listener,
  registration node, descriptor, surface and thread.
- Distinguish deliberate bounded process-lifetime state from growth per request,
  track, eject attempt or reconnect. Verify cache eviction and teardown behavior.
- Check every allocation failure; preserve independent sources and avoid falling
  back to a native USB library/cache.
- Audit shared fields and lock ordering under TSan-compatible host scenarios.
  Include actual worker/UI interleavings rather than only single-thread tests.
- Check close-on-exec on files and sockets and closure on all error paths.
- Verify quiescence and bounded redraw/log rates when idle.
- Record sanitizer instrumentation, controls, coverage and exclusions. A green
  helper test does not prove the firmware adapters or the whole mod leak-free.
- Repeat final exact-binary emulator regressions and a sustained churn/resource
  observation after fixes. Retain failures as well as passing evidence.

## Hardware input investigation

Existing read-only observations establish that STOP routing works, the two SDK
filters are registered and enabled with zero budget strikes, the mixer fd is 14,
the core is active, and the mod delegates reads to the installed core. Both input
heartbeat counters remain zero. Passive diagnostic reads were also zero while
idle. Vendor GPL driver `drivers/spi/subucom_spi.c`,
`subucom_spi_check_rxbuf`, signals `rx_detected` only when input changes; its
poll/read path clears that event after reading. A 500 ms heartbeat requirement
is therefore invalid for an idle panel. The local fix checks callback progress
per valid existing read and permits idle periods without losing ownership.
This diagnosis and the revised path still need exact-binary emulator validation.
See `acceptance/usb-hub-support-0.2.0-az-selector-failure.json`.

A separate local diagnostic overlay observes the existing read result and a
bounded frame snapshot without modifying it. It is not a publication candidate.
The user explicitly authorized its installation, approval and stock reload.
Its ELF SHA256 is
`ae005285d2d131f1e2dfa7c9fa50723cd8aa4540e428758676cd7e8992a372de`.
The host observer test checked unrelated descriptors, negative reads, preserved
short-read lengths, unchanged input and bounded snapshot cadence. These checks
do not establish runtime performance or hardware acceptance.

## Latest local changes and emulator fixture caveat

After the recorded host run, tagged-cache preparation was moved out of request
callbacks into initialization. Eight private receiver blocks are allocated as a
batch before any native constructors or hook claims. Allocation failure frees
only unconstructed blocks and aborts init; recognized hub requests cannot fall
back to a native USB cache. This revision still needs its own build/runtime
verification. Mount-worker path buffers were also sized for complete directory
entry names to remove SDK compiler truncation warnings.

The cold snapshot emulator currently mounts native USB1 but does not advertise
it in SOURCE. Consequently hub A is the first visible row, rather than the
second row used by the historical UI script. Two runs stopped on the unchanged
physical-device load assertion when the old coordinates selected B. A local
runner now selects the observed first row while retaining physical-device
identity verification. Native USB1 browsing/STOP coexistence is still an open
fixture/integration gate; preserving its backing mount alone does not validate
that functionality. No corresponding real-AZ action was performed.

The SDK custom-output build compiled once, then its automatic validator refused
the source directory because it only searches `out/` and the mod root. This is
not a successful final SDK validation. A subsequent SDK-container run failed to
execute ARM tools (`Exec format error`), reproduced with `/bin/sh` alone.
Linaro builds succeed; SDK container execution must be repaired or repeated on a
working ARM environment before clearing that gate. No Docker daemon restart was
performed. Native USB1 subsequently appeared after an explicit filtered emulator
re-announcement, invalidating the transient first-row UI coordinates; those runs
must not be treated as acceptance.

The missing local ARM interpreter registration was restored using the already
installed `tonistiigi/binfmt` image (`--install arm64`); no daemon restart or
hardware access was needed. A forced SDK rebuild is now being repeated. Native
USB1 was verified visible alongside hub A/B before the current exact-binary run.
The temporary first-row fixture runner is superseded by the original coordinates.

The forced SDK rebuild now passes without compiler warnings, and its separately
staged package validates. See `acceptance/usb-hub-support-0.2.0-stability-build.json`.
Its ELF differs from the selected Linaro runtime artifact and is not assigned the
latter's emulator acceptance. The selected ELF import inspection found no
`system`, `popen`, `fork`, `posix_spawn` or direct `pthread_create` imports.

A further resource review found unconditional legacy `/tmp/linkfake.log` writes
and flushes in the community build. The latest local revision routes bounded
formatted messages through the SDK logger instead, respecting its logging policy.
Only the standalone research profile retains the private logfile. This creates
a new selected ELF (`43d4e45e...`), so preceding c261f636 integration results remain
separate. Both current Linaro and SDK packages validate; matching runtime tests
are in progress. The current SDK build again completed without warnings.

The first input run after the boundary suite correctly stopped on a physical
identity mismatch: B had been the first discovered source after an empty startup,
so its retained member/slot preceded A. Restarting the emulator process with both
mounted restores the explicitly required original A/B source order for that test.
This is a test precondition, not a reason to renumber live sources in production.

## Partial-initialization review

The local GPL core loader retains the mapping and already installed hooks when
`init()` returns an error; it does not roll them back. Existing worker readiness
and native UI/load guards therefore need matching transport/factory guards.
Source now forwards read/write/send/recv/connect/UDP/mailbox calls unchanged
while readiness is false, and prevents factory/holder adapters from creating
hub servers before readiness. These edits follow the running 43d4e45e resource
observation and are not represented by that executable. Do not overwrite or
reuse its evidence for the next build. A targeted init-refusal fixture and new
matching runtime regressions remain required.

Failure-state handling now publishes a terminal negative readiness state, causes
prepared workers to exit, and closes unpublished loopback sockets. Readiness
checks require exactly the successful state; a negative failure value cannot
activate routing. Native encoder observation also forwards unchanged until ready.
A separate ignored compile overlay refuses the last selector hook claim to test
the real core's partial-init path in the local emulator. It is not a deployment
or release candidate. Constructor-initialized native cache storage remains bounded
process-lifetime storage; its destruction must not be guessed on init failure.

## Sustained-run filter budget observation

The 43d4e45e process completed twelve physical reconnect/eject cycles with the
same endpoint FD count (58) and thread count (279), and lower endpoint RSS than
its initial sample. These observations are recorded separately and do not prove
absence of native leaks. The immediately following main suite stopped before
loading: the SDK had disabled the edited filter after eight calls exceeded its
200 us budget. Logs confirm the input-health path refused selector ownership.
Do not count that main run as acceptance. Concurrent local compiler/container
work may affect QEMU timing, but causation is not established. Repeat from a fresh
process without concurrent builds; do not bypass or increase the production
filter limit. Actual hardware timing remains a deferred hardware gate.

The targeted final-claim refusal fixture passed: the core reported init error -80
with already patched code, prepared workers exited, hub aliases were absent,
and native USB1 remained visible and responded to touch/settings navigation.
The normal ab12dcc6 candidate was restored afterward. Screenshots were inspected
locally; no audio-load acceptance is inferred. See
`acceptance/usb-hub-support-0.2.0-init-failure.json`.

## SDK helper argument regression

The ab12dcc6 media run passed real exFAT-FUSE closure/busy checks and initial
topology faults, then stopped because sdg never mounted. SDK launch accepted the
program, but the caller supplied the executable as an argument even though the
SDK already prepends it. The single-argument helper consequently exited early.
This was a regression in the local SDK migration, not an accepted fixture error.

The latest source passes only the device name. The production helper-launch
routine is now shared with host tests that enforce argument shape, launch
refusal, normal waiting, bounded timeout/stop and missing-service handling.
All portable suites and production UI sanitizer checks pass after the correction.
The failed topology fixture was safely ejected and extra virtual devices removed
before further work. A new binary and matching extended/runtime regressions are
required; ab12dcc6 evidence does not clear this revised source.

## Mount-helper timeout safety

Review of SDK 1.2.3 shows `launch_stop()` schedules the final kill after its grace
period and returns immediately. Releasing mount admission at that point cannot
establish that mount work has finished. Source now latches a timeout fault before
requesting stop, disables further fallback launches and refuses subsequent hub
eject requests. A fixed empty `/run/azhub-mount-fault` marker preserves this fault
across application reloads; the UI asks the user to leave the hub connected and
restart the AZ. It clears with an OS restart, not a mod reload. Plain/sanitized
and actual-core fault validation are pending for this newest revision; the
running 04d5d9a1 tests do not cover it.

## Volatile coordination files

The SDK's USB-data placement guidance is the default for persistent mod files.
This coordinator deliberately keeps bounded process/reload coordination in `/run`
and discovery/status scratch state in `/tmp`: it must still work when the mods
USB is safely ejected, and it must not consume the settings partition. The
journal has sixteen records, the timeout marker is one empty file, and discovery
output has at most eight source rows. Diagnostic status output is opt-in. Logs
now use the SDK's cap and enable policy, not a private growing file. These are
intentional volatile-state exceptions to the general `data_dir()` habit, not
persistent user-data stores. Confirm their backing mounts in the later hardware
preflight; no new settings-partition writes are introduced.

The selected 09b8ffa8 fault fixture passed with the actual SDK/core: a controlled
exec-sleep helper timed out, its process exited under SDK stop, eject was refused,
and the restored real helper stayed blocked across an EP147 reload. The original
helper bytes were hash-verified after restoration. Empty FAT fixtures were
ordinarily unmounted, then explicitly removed; only afterward was the injected
marker cleared for emulator recovery. This is not a real-AZ recovery procedure.
See `acceptance/usb-hub-support-0.2.0-mount-timeout.json`.

The selected executable also passed topology/SDK fallback. Repeated partition
setup initially stopped on a duplicate QMP backend name; the script now reuses
an existing backend only after checking that its resolved path is the same
disposable image. The repeated partition suite then passed. This changes only
the test fixture; package bytes are unchanged.

Final selected normal/explicit candidate targets were rebuilt and verified
byte-identical. Both compiler packages contain exactly the seven expected members,
with source manifest/asset/icon/license bytes checked and ELF hashes matching
their own records. The public catalog remains 0.1.8. See
`acceptance/usb-hub-support-0.2.0-stability-timeout-package.json`.

All eight selected-binary runtime suites now pass: topology, partitions,
reserved-load race, failed-load recovery, exFAT, SOURCE/BROWSE input capture,
main eject and native USB1 STOP. Journal write-failure injection and final
same-binary resource observation are the remaining running checks.

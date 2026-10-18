# Ejecting hub USBs with loaded tracks

Publication: [USB Hub Support 0.2.1](https://github.com/NachoCasta/nacho-az-mods/releases/tag/usb-hub-support-0.2.1)
is published with the exact accepted package. Source CI and anonymous catalog,
package, icon and pinned-source checks passed. The user explicitly authorized
publishing the accepted functional progress with native long-session memory/input
and expanded hardware limits disclosed in the [release notes](releases/usb-hub-support-0.2.1.md).
See [publication evidence](acceptance/usb-hub-support-0.2.1-publication.json).
Earlier deferred/unauthorized statements below preserve the research chronology.


Status: functional loaded-track eject accepted on the real AZ and published in
0.2.1. The user authorized preflight, installation/hash approval, application
restart and the bounded one-source trial. NACHO PLUGS unmounted with a loaded
track; the user confirmed full-track playback afterward. Read-only SSH verified
the retained exact-media cache proof, absent backing/alias mounts, present physical
device, unchanged PID and mounted CAMI/native USB1. See [hardware evidence](acceptance/usb-hub-support-0.2.1-az-loaded-eject.json).

Hardware audio is a manual user observation, not an agent audio recording. This
acceptance covers one source and one track. Expanded hardware, long-session native
memory/input timing and publication decisions remain open. Public 0.2.0 is unchanged.
The local-only and pending statements below describe earlier preparation.

## Current candidate and evidence

Selected executable: `9feec49c06f10dd01e191ed5094f68eb1e7e7b35cd9ed76d757a8477a37a945b`.
The source snapshot, compiler identities and two independently validated packages
are recorded in [build evidence](acceptance/usb-hub-support-0.2.1-loaded-build.json)
and [package evidence](acceptance/usb-hub-support-0.2.1-loaded-packages.json).

- [Exact-candidate emulator acceptance](acceptance/usb-hub-support-0.2.1-loaded-eject-emulator.json): paused/playing individual eject, seek/loop, independent remaining source, two playing decks with whole-hub eject and manual unload afterward.
- [Replacement and audio acceptance](acceptance/usb-hub-support-0.2.1-loaded-replacement.json): a new stick at the same port has a different opaque media identity; old cached playback survives, the new stick loads and ejects independently. With all hub backings unmounted, 6.18 seconds of sample-exact deck audio had zero dropped frames. Raw audio was deleted locally.
- [Delayed-completion fault experiment](acceptance/usb-hub-support-0.2.1-native-timeout.json): a guarded emulator-only overlay held an actual forwarded native receipt. Repeated timeout did not unmount or enqueue a duplicate; releasing the receipt allowed retry. This is fault-path evidence, not normal-executable acceptance.

The selected build has 35 exact-firmware hooks. Native completion is forwarded
before acknowledging the ticket; the ticket matches owner, full opaque MediaID
and a task serial after the enqueue fence. Sixteen static tickets and four cached
deck proofs retain pending/cached identity ownership. Physical connection tokens
prevent a newly inserted stick from inheriting an earlier detach proof. Load
epochs prevent an old callback from clearing a newer request. No forced/lazy
unmount or native USB library-slot borrowing is introduced.

[Twelve repeated cycles](acceptance/usb-hub-support-0.2.1-loaded-resources.json)
completed load/eject/manual-unload/replug with zero pending tickets and zero
cached-proof references at every settled sample. Threads stayed at 279; settled
FD counts were 58 after the first-cycle 59. The selector/knob/BACK worked afterward.
Native process RSS increased by 244580 KiB from cycle 2 to cycle 12 (it decreased
slightly in the final cycle); VmData increased by 25600 KiB over that interval.
The proprietary cache/allocator cause and long-session bound are not established.
Do not describe this result as absence of native leaks or long-session acceptance.
[Fresh-process regression acceptance](acceptance/usb-hub-support-0.2.1-loaded-regressions.json)
passed repeated busy-holder refusal with SOURCE focus preserved and safe eject
after a native missing-file load failure with stale SDK metadata. [Reserved-source LOAD race](acceptance/usb-hub-support-0.2.1-loaded-load-race.json)
also passed with an explicit 8000 ms emulator reservation window: an actual
native LOAD was denied, eject completed with the deck empty, then the remaining
source loaded and completed native cached eject normally. This verifies race
admission, not ordinary eject timing. The chronology below preserves earlier failures and prototypes; those binaries do
not substitute for exact-candidate acceptance. Native caching may limit playback
functions and capacity; successful filesystem eject does not promise full-track
retention for all formats or media. Real-AZ acceptance is still required.

Evidence ledger: [0.2.1 local summary](acceptance/usb-hub-support-0.2.1-loaded-summary.json).

## Target behavior

Allow individual or whole-hub safe eject with paused or playing tracks loaded.
Separate successful filesystem removal from the amount of audio retained in
memory. Do not claim full-track continuation from SDK loaded/playing flags alone.
Keep other sources independent, reject new loads on a reserved source, and never
force or lazily unmount. Loading races and unknown identities must fail safely.

## Exact-firmware observations

Target remains firmware 2.00, `ep147-9e9454e357389ece`.

| Entry | Observed role |
|---|---|
| `0xbc89c8` | Native USB STOP processing, resolving media and disconnect requests. |
| `0xbc63b8` | Loads the disconnect dependency at storage controller +0x5d0 and calls virtual slot +0x10 with full MediaID and a flag. |
| `0xa59b60` | DisconnectController async enqueue entry for MediaID and boolean. |
| `0xa59e78` | Secondary-interface thunk adjusts this by -0xd8 before the enqueue entry. Call the validated virtual slot with its own interface pointer. |
| `0xa59438` | Creates the per-media inner disconnect processor with media and flag. |
| `0xa566f8` | Starts dependent deck/preview unload work. |
| `0xa77110` | LoadController dispatches native unload with a mode field. |
| `0xa561c0` | Queues a completion task; not final file/cache completion. |
| `0xa59e80` | Completion task body processes PCM/file-cache work when needed, removes the pending processor and notifies registered listeners. |

Native listener registration at `0xa5b0e8` mutates a vector without an obvious
lock. Dynamically registering a listener during playback therefore needs a
thread-ownership proof. A local prototype instead observes the end of the native
completion task, forwarding it normally first. Raw analysis stays ignored/local.

## Emulator baseline and failures

All four image backings were inspected as qcow2 COW graphs. Hub A/B use persistent
named overlays with explicit read-only raw backing. Firmware/music were not
copied into the repository or uploaded.

The initial native USB1 fixture had no playable audio, so the first browser/load
attempt stopped before eject. A known-present local audio file was then copied
only into USB1's existing COW snapshot and the stock source re-announced.
Native STOP with that file loaded produced unload mode 2 and kept SDK loaded
state after ordinary USB1 unmount. Starting playback advanced reported position,
but the native UI subsequently showed EMERGENCY LOOP and a limited-functions
warning. Eject overlapped the end of loading. This is evidence of limited native
continuation, not full-song cache acceptance or measured audio output. The normal
unload UI helper did not clear that state; the paused emulator application was
stopped/restarted for fixture recovery. The real AZ was not involved.

An initial hub prototype refused before withdrawal because it recognized only
the primary disconnect entry, not the secondary-interface thunk. The next variant
accepts either exact known entry and invokes the validated virtual slot. It also
moves observation from the early completion enqueue to the final task body.

## Required before production integration

- Correlate completion with the exact controller, full media identity, source
  generation and one outstanding request; retries must not accept stale callbacks.
- Preserve unknown-media and in-flight load safety. Validate failure and timeout
  paths before relaxing the shipped loaded-source guard.
- Observe actual ordinary unmount, retained deck state, position and audio/UI
  behavior for paused/playing tracks, seek, cue/loops and long files.
- Verify independent eject while another source plays, whole-hub multiple decks,
  reconnect and different-stick replacement while old cached tracks remain loaded.
- Keep native completion callbacks bounded; filesystem work remains on workers.
- Add host tests for the coordinator state machine, then exact-binary emulator
  checks. Hardware deployment/testing requires fresh action-specific approval.

The research diagnostic `loaded-eject-probe.c` refuses kernels other than the
known local emulator kernel and only forwards/logs native calls. It is not a
release asset or production feature.

## First successful hub prototype

The final local prototype ELF is
`4e59b2050dbf540d61ff7dd99a1b64947fb20b87da90bfe822a96dfe703b9d69`.
Production 0.2.0 source and release assets remain unchanged.

It invoked the validated native disconnect virtual slot, waited until the final
completion task returned, then used existing server acknowledgments and ordinary
unmount. A paused, loaded A track remained in the deck after its USB unmounted;
B and native USB1 remained mounted. Playback subsequently advanced beyond
19 seconds with normal waveform. Physical SEARCH+ moved to 160.285 seconds,
and an explicit four-beat loop cycled around 163–165 seconds after unmount.
Audio output was not captured, so this is UI/position/cache evidence.

A was reinserted while its cached deck kept looping. B then loaded its own
physical file, and whole-hub hold unmounted both USBs with both decks loaded and
playing. B later advanced beyond ten seconds; native USB1 stayed mounted and
there was no process restart. Both decks were paused afterward. See the
[prototype record](acceptance/loaded-track-eject-prototype.json).

This proves feasibility, not production readiness. The prototype deliberately
omits final unknown-media, generation and retry policies. An initial reservation
window also allowed discovery to mark a source absent before source reservation
was published; native disconnect was consequently requested twice. Production
integration must publish the physical record and source reservations atomically
with discovery, and define ownership of native disconnect requests/completions.
The observed completion payload flag initially called “success” in local logs
is not an error/success result: the native task uses it to select PCM/cache
cleanup. Do not infer failure from its value zero.

## Historical model before integration

A fixed sixteen-ticket detach pool now has host tests for exact opaque MediaID,
controller ownership, native task serial fences, timeout/retry retention, cached
deck references, connection conflicts and 100,000 reuse cycles. Plain,
ASan/UBSan and TSan runs passed. At that stage the model was not yet wired into production.
It relies on the native task counter at controller +0x20 and task serial at +0x10;
the final notification payload carries owner at +0x20 and MediaID at +0x28.
Validate those fields on the exact integration build. A serial fence alone does
not identify a newly submitted request among concurrent native events: preserve
the single pending native processor and full identity/connection contract too.

The native callbacks after same-port reinsertion used different opaque byte-5
values (0x0102 versus 0x0202 for A). Source matching intentionally normalizes this
byte, but detach completion must preserve it. Do not construct a detach MediaID
using the fixed server-local 0x0102: collect the full ID of the actually loaded
track. Otherwise an old cached deck and the reinserted current library can be
confused. The successful whole-hub prototype still used that shortcut and is not
an acceptable production implementation.

## First integrated coordinator checks

The full integration bridge collects each actually loaded opaque MediaID and uses
fixed tickets, per-connection tokens and per-deck load epochs. It waits for the
final native callback before server withdrawal. Pending tickets survive timeout
and retry; cached deck proofs retain completed tickets until native unload or another
successful load. Unknown identities and in-flight loads remain guarded.

An initial integrated build refused before native dispatch because a proposed
primary-vtable check was wrong. Bounded emulator memory inspection established
the actual dependency interface vtable `0x29e96f0`, thunk `0xa59e78`, root
adjustment -0xd8 and owner vtable `0x29e9610`. Native task counter +0x20 was zero
before the request; final task serials were 19 and 38, and the counter read 38
afterward. Cached native ILoadState remained loaded with the same opaque ID.

Corrected intermediate ELF
`c21d88337b217d3416edc61b11e8f3767b888a96d4010bce70ca078bfc0daf0c`
then passed individual loaded-A eject through the production bridge, with B and
native USB1 mounted and deck 1 retaining its track. This is intermediate
integration evidence, not acceptance of the later candidate. A first navigation
attempt selected B rather than expected A; physical identity assertion stopped it
before eject. Subsequent selection is calibrated and still independently checked
against native request identity and actual backing/alias device numbers.

Further local corrections preserve a newer loading guard when an old unload
callback arrives: state-zero native completion and exact media must match, and
an in-flight request is never cleared. Preparation is published before work is
released to the worker, preventing a fast final result from being overwritten.
These corrections are included in selected 0.2.1; matching acceptance is linked
at the top of this document. Remaining gates are listed there.

## Subsequent regression input-budget failure

After the twelve-cycle observation ended successfully, the first repeated busy-
holder regression correctly refused eject. On its second selector request the
core reported that the frame filter had exceeded 200 microseconds eight times
and switched it off for the process. The selector helper then timed out. This
attempt is not regression acceptance. It occurred in local QEMU; no hardware
timing conclusion follows. A fresh-process repeat is required. Do not erase this
failure or include it in the successful resource-test interval.

The fresh-process repeat subsequently passed both busy refusals and the native
missing-file/stale-metadata eject regression without a filter-disable event in
its accepted interval. Its record explicitly retains the earlier failed attempt.

## Memory control: ordinary load/unload on one connection

A fresh normal-candidate process completed four load/manual-unload cycles without
native detach or replug during the cycles. The [control record](acceptance/usb-hub-support-0.2.1-load-unload-control.json)
uses the same selected executable and independently verifies source A. Settled
RSS was 765916 KiB at cycle 2 and 765680 KiB at cycle 4; VmData stayed at 4316004
KiB after warmup. Threads stayed at 277 and settled FDs varied between 59 and 60.
All detach pending/reference/proof counts were zero. This short same-connection
control does not explain the earlier repeated eject/replug RSS increase. The
next control unloads before ejecting and reconnecting, so it exercises connection
generations without requesting loaded-track cache detach.

## Partial memory control: unload before eject/replug

The [second control](acceptance/usb-hub-support-0.2.1-unload-first-control.json)
requested four cycles but completed only one. Its first cycle loaded A, manually
unloaded the deck, ejected the empty source and reconnected on the next cycle.
RSS increased from 768596 to 818160 KiB in the completed cycle. No native-detach
request occurred anywhere since that normal process started, including the
preceding same-connection control. Thus some observed RSS growth is reproducible
without the new loaded-track disconnect request; it does not establish a cause,
a long-session bound or behavior of the old published executable.

In the second cycle the track loaded and manually unloaded, but the core disabled
the input filter after eight executions above 200 microseconds. The selector
helper timed out. The four-cycle control is **not accepted**. This is a second
recorded QEMU timing failure, not a new real-AZ observation. Keep input timing and
long-session native memory as open gates. The application was restarted locally
after preserving the evidence; no production source or candidate package changed.

# USB Hub Support startup crash: 2026-11-21 investigation

Status: the immediate null function-pointer call is identified. Static analysis of
the exact core 1.2.9 and shipped 0.4.11 executables confirms a startup
hook-publication window that produces this exact crash signature (see
[Publication window confirmed](#publication-window-confirmed-in-the-exact-binaries)).
The emulator reproduces the field failure deterministically with the unfixed
publication. The corrected publication survives the same forced window (see
[Emulator reproduction](#emulator-reproduction-old-and-fixed-controls)). The
fix ships as 0.4.12 (runtime `d23170c5...`), with an exact-runtime emulator smoke
pass. Physical acceptance is not claimed.

## User report

The owner left the controller with a USB hub connected for an extended period,
possibly overnight, and later saw a warning that USB Hub Support had been
suspended after the deck stopped while the mod was running or starting. The
owner suspected a controller restart and also mentioned starting with another
user who had the mod. The timing and relationship between these events are not
established by the retained logs.

## Observed device state

Read-only SSH inspection took place on November 21, 2026 Pacific time
(November 22 UTC). At the first inspection, the device reported
2026-11-22 02:34:32 UTC and approximately five minutes of uptime.

- Firmware tag: `ep147-9e9454e357389ece`; kernel: `4.4.194`.
- Framework loader reported set `1.2.9`.
- Installed USB Hub Support: `0.4.11`, framework status `5`.
- Exact framework message: `Suspended: the deck stopped while this mod was starting`.
- Persistent `usb-hub-support.crashed` marker contained `froze`.
- `EP147.service` reported `NRestarts=1` and current `MainPID=3622`.
- Only the current boot was available through `journalctl --list-boots`.
- `/sys/fs/pstore` was empty; the current mods USB had no `MODS/logs` directory.

The service restart counter supports an application restart in this boot. It does
not prove that the mod caused an earlier operating-system reboot.

## Retained crash evidence

`/tmp/azmod-crash.log` contained one crash record, 30,688 bytes including mappings:

| Field | Value |
|---|---|
| Signal | `11` (`SIGSEGV`) |
| Recorded epoch | `1790315072` |
| Framework attribution | `community mod: usb-hub-support` |
| Thread | `HuiProcessor`, TID `3539` |
| Fault address / PC | `0x0` / `0x0` |
| LR | `0x7f8fa2edac` |
| x8 | `0x0` |
| Mod executable mapping base | `0x7f8fa21000` |
| Relative return address | `0xddac` |
| First recorded native frame | `0x237d67c` |

Other recorded native frames were `0x235db8c`, `0x2363de0`, `0x24b4840`
and `0x24b497c`. Existing queue research associates `0x237d67c` with
`AsyncTaskBox::executeTasks` calling the queue-pop hook.

The staged executable copied for inspection has SHA256:

```text
02fd149b23a13a1d8a32383694b24b8137ead89ca6d5675edb644b795f9778c2
```

This matches the accepted 0.4.11 runtime in
[the source recovery candidate record](acceptance/usb-hub-support-source-startup-recovery-candidate.json).
The crash mappings name the same staged path, but the crash log itself does not
record a binary hash. Symbolization uses the matching staged release artifact
retrieved during inspection.

## Immediate failure and source correspondence

Inspection of that executable resolves offset `0xddac` to `probe_queue_pop`.
The preceding call at `0xdda8` branches through `x8`; the crash registers show
`x8=0`, PC=0, and the expected return address. The pointer is loaded from the
storage used for `probe_queue_pop_original`. This identifies the immediate
failure as calling a null original-function pointer.

In [link-request-origin.h](../src/azhub/linkfake/link-request-origin.h),
`probe_queue_pop` calls `probe_queue_pop_original` before checking
`lf_runtime_ready()`. Installation uses:

```c
probe_queue_pop_original=make_hook(0x237f900,probe_queue_pop);
```

In [linkfake.c](../src/azhub/linkfake/linkfake.c), the community implementation of
`make_hook` passes a local `original` variable to the SDK's `claim_hook`, then
returns that value. The caller assigns the returned pointer to the storage read
by the callback only afterward.

If `claim_hook` makes the hook callable before that assignment, another thread
can enter `probe_queue_pop` while its original pointer is still zero. This is a
specific candidate race to reproduce. The SDK's publication ordering and the
actual interleaving have not yet been verified. The source pattern alone does
not prove the historical interleaving, and other causes of a zero pointer have
not been excluded.

The retained mod log lists successful hook claims through `0xa7c848`, followed
by the suspension message. Together with the framework startup classification,
this supports a failure during initialization. It does not locate the exact
assignment interleaving: log order and crash-handler timing must be considered.

## Publication window confirmed in the exact binaries

Offline disassembly (local only; no disassembly is committed) of the core and
the crashed executable resolves the SDK ordering question.

SDK contract. The pinned v1.2.3 documentation states that `claim_hook` redirects
the function and stores the original in `*orig`, that the patch is one aligned
4-byte store, and that hooks run on whatever EP147 thread calls the function.
Its example passes the callback's real storage (`(void **)&orig_some_fn`). It does
not document when `*orig` is written relative to the patch.

Core 1.2.9 (`azmod.so`, SHA256
`49921947d275e1f1de7905636929da88b4a1353704855f6427a2ea8ca1d7c294`, the emulator's
cached copy of the set the device reported):

- `claim_hook` (`0x74918`) validates the caller, takes the hook mutex and passes
  the caller's `orig` pointer unchanged to the installer at `0x74380`.
- The installer maps and fills the trampoline, then stores it into the caller's
  `*orig` (`0x74588 str x28,[x24]`), then publishes the branch with a release
  store (`0x7459c stlr w0,[x26]`), then flushes caches.
- Later failures after the branch is live (`0x748dc`) are only logged, and the
  claim still succeeds. Every refusal returns before `*orig` or the code is
  written.
- After the installer returns, `claim_hook` unlocks the mutex and writes the
  `claim_hook 0x... ok` line to the mod log before returning.

Therefore storage passed directly to `claim_hook` is valid before any thread
can enter the hook. Storage the mod fills after `claim_hook` returns is not.

Shipped 0.4.11 (`02fd149b...`): `make_hook` is inlined into `community_init`.
For `0x237f900` the trampoline is written to the stack temporary
`[x29,#192]` (`0x1b8d0`, call at `0x1b8f8`). The global
`probe_queue_pop_original` (`0x45d78+2040`) is stored only at `0x1b940`, during
setup of the following `0xaab598` claim. `probe_queue_pop` loads the same slot
(`0xdd98 ldr x8,[x19,#2040]`) and calls it at `0xdda8`. The return address of that
call is `0xddac`.

From the core's `stlr` until `0x1b940`, an EP147 call to `0x237f900` reaches
`probe_queue_pop` with a null original. That interval includes cache maintenance,
a mutex unlock and a flushed log write. Such a call branches to address zero with
`x8=0` and `LR=base+0xddac`, which matches the crash record exactly. The queue pop
runs on `AsyncTaskBox::executeTasks` (`HuiProcessor`), a busy UI thread.

The retained log shows five more successful claims after `0x237f900`. That fits
a fault in the window: the init thread keeps claiming while the crashing thread's
handler writes its 30 KB record. This remains an inference from ordering; the
exact historical interleaving was not observed.

Every hook installed by the old `x=make_hook(...)` pattern has the same window.
Only hooks whose callbacks EP147 runs on another thread during `init` can fault
there. `0x237f900` is the one observed.

## Drafted fix and host control

`make_hook(target, hook, void **original)` now passes the callback's storage
directly to `claim_hook`. On refusal it writes the native target as before,
because a refusal leaves no live patch. All installers were converted. The
standalone (non-community) branch publishes its trampoline with a release fence
before patching.

`test-hook-publication.py` extracts the production `make_hook` body and calls the
hook from inside a mock `claim_hook`, after `*orig` is published but before return.
The unfixed wrapper observes null (negative control). The production wrapper
calls the native function. This passed under WSL GCC with `-O2` and with
`-O1 -fsanitize=address,undefined` (2026-11-21). `test-hook-registration.py`
passes with the new signature, including its zero/double negative controls.

Compiled-code audit (2026-11-21). This used a non-release WSL
`aarch64-linux-gnu-gcc` build with the production `all` flags, not the pinned
SDK container. The `0x237f900` claim passes `x3=0x404a0+0xa50`, the same slot
`probe_queue_pop` loads (`[x20,#2640]`, `x20=0x404a0`). Of 56 `claim_hook` call
sites, only two pass a stack address. One is the out-of-line `make_hook`, which
uses its local only when the caller passes `NULL` (`csel ... eq`). The other is
the `community.c` EP147 PLT-stub loop, which passes `NULL` by design: those
callbacks use `real_*` libc pointers that `linkfake_init()` resolves through
`dlsym` before any claim (`community.c:223` precedes the claims at line 246).
`eject-ui.h` and `safe-eject.h` already pass globals directly to `claim_hook`.
The release build must repeat this check on the exact container artifact.

Retained research probes under `src/azhub/research/scripts/` (for example
`link-load-queue-probe.h`) still use `x=make_hook(addr,hook)`. They are not part
of the shipped build, and with the new signature they fail to compile instead of
reintroducing the window. Convert them before reusing them.

## Emulator reproduction: old and fixed controls

Date: 2026-11-21 (emulator clock 2026-11-22 UTC). Environment: local XDJ-AZ
emulator (QEMU TCG, 6 CPUs) with firmware `ep147-9e9454e357389ece` and AZ-MODS
set 1.2.9. Its mapped `azmod.so` md5 `750205718f33cb5dd872063e5c033b4c` equals the
cached 1.2.9 core analyzed above. Builds were loaded with the emulator's
`tools/az-push-mod.py`, the emulator's standard approve/restart path. No physical
device was touched.

`prepare-startup-hook-window.py` generated two emulator-only trees in ignored
`local/`. `--unfixed` uses HEAD (0.4.11) sources; the default uses the corrected
working tree. Each adds the same diagnostic: after the `0x237f900` claim succeeds,
`make_hook` holds a 5 s window open and `probe_queue_pop` logs its original pointer
once when entered during that window. In the old tree the window sits after
`claim_hook` returns and before the caller stores the result (the 0.4.11 gap,
widened). In the fixed tree it sits at the same point after a successful claim.
Both were built in the pinned `az-mods-sdk:1` container (GCC 7.5) with production
`all` flags plus `-DLF_PUBLIC_RUNTIME_EMULATOR`. That flag is required by the
diagnostic's `#error` guard and only affects `public-sharing-runtime.h` code
absent from `all`.

| Control | Executable SHA256 | Runs | Callback in window | Outcome |
|---|---|---|---|---|
| Old (negative) | `c22e0f8d2ac57166e387a50e372e4a4ae34b42a3600851f513e72590cd3b81ff` | 2/2 | `original=(nil)` | `5`, `Suspended: the deck stopped while this mod was starting`, marker `froze`, EP147 `NRestarts=1`; no `STARTUP WINDOW closed` line |
| Fixed (positive) | `0a20a81e589c3900169ae42ddb16ca32210f65961b46747c2fd01dec8dc37725` | 2/2 | `original=0xffff95c03000`, `0xffffb9e7f000` | window closed, `loaded 0.4.11 ... (64 hooks)`, status `1`, `NRestarts=0`, no crash marker |

In both old runs EP147 entered `probe_queue_pop` on another thread within the
window and the process ended before the window closed. This reproduces the field
status text, `froze` marker and service restart. The emulator core wrote no crash
record: `/tmp/azmod-crash.log` stayed at 0 bytes and the volatile journal had
rotated at the restart. The emulator therefore does not independently show
`PC=0`/`LR=+0xddac`. That register signature rests on the physical record and the
disassembly above.

Non-diagnostic fixed runtime. The production `all` target in the same container
produced `efb4a0baec5f1cd67a017ca618dbbb09384bd56302f06a2df3ee3bcc8538f1e4`
(highest required symbol `GLIBC_2.17`). Its `0x237f900` claim passes
`x3=0x3a690+0x7f8`, the slot `probe_queue_pop` loads (`[x19,#2040]`,
`x19=0x3a690`). The native target is stored there only on the refusal path. This
build is not yet a release candidate: the manifest still says 0.4.11. It loaded in
the emulator with 64 hooks, status `1` and `NRestarts=0`. Only startup was checked.

Emulator state changes. Before testing, the emulator had legacy `hub-libraries`
0.1.4 enabled alongside `usb-hub-support`. It owned the EP147 PLT stubs, so
`usb-hub-support` failed with `init() returned -3`. The prior stage
`community/` and `azmod-community/` were copied to
`/home/root/crash-repro-backup-20261121` in the emulator, and `enabled` was set to
`usb-hub-support` only. The emulated stick's `MODS/community/usb-hub-support/` now
holds the fixed runtime above. The Docker VM's arm64 binfmt registration had been
lost mid-session. It was restored from the cached `tonistiigi/binfmt` image
(`400a4873b838`, `--install arm64`, `--pull never`), as in earlier records.

## 0.4.12 candidate: emulator smoke pass and scripted regressions

Correction: the release runtime is the `sharing-runtime-candidate` build
`d23170c5...` (see [Shipped 0.4.12](#shipped-0412-runtime-and-package)). The plain
`make all` build `efb4a0ba...` used in this section omits the Ethernet sharing
runtime that 0.4.11 shipped. The package comparison before publication caught
this. Results in this section apply to `efb4a0ba...`. They were repeated on the
shipped runtime below.

`azmod.json` is now 0.4.12. `azmod-pack build` (pinned `az-mods-sdk:1`, plain
`all` target) reproduces executable
`efb4a0baec5f1cd67a017ca618dbbb09384bd56302f06a2df3ee3bcc8538f1e4`, byte-identical
to the audited fixed runtime above.

Emulator fixture (2026-11-21, emulator clock 2026-11-22 UTC). The guest was
powered off cleanly and booted headless with
`AZ_QEMU_EXTRA=-device qemu-xhci,id=ejectxhci; -device usb-hub,bus=ejectxhci.0,port=1`.
`xhci-pci-emulator.ko` was rebuilt with `build-emulator-xhci.py` (ignored) and
loaded with `insmod`. `usblib.img` (HUB_LIB) and `hublib2.img` (HUB_LIB2) were
hot-added at ports 1.1/1.2 as qcow2 overlays over read-only bases with the
suite's `snapshot_drive`. Overlays are in ignored `local/emulator-overlays/`.

At boot the AZ-MODS loader re-staged an older `usb-hub-support` (0.4.11
`2388ac0e...`) from the stick's `MODS/community-mods/`. `az-push-mod.py` only
writes `MODS/community/`. The 0.4.12 manifest and executable were therefore also
written to `MODS/community-mods/usb-hub-support/` (`.new` then rename; assets
unchanged) and re-staged/approved with `az-push-mod.py --no-restart`. Mapped,
staged and stick executables all hash `efb4a0ba...`.

Manual smoke pass (exact candidate, screenshots in ignored
`src/azhub/research/shots/smoke-0412/`):

| Step | Result |
|---|---|
| Startup | `loaded 0.4.12 ... (64 hooks)`, status 1, `NRestarts=0` |
| SOURCE | Native USB 1 (AZ_USB), then HUB USB 1/HUB USB 2 with rekordbox names |
| Browse HUB USB 1 | TRACK list, 2547 entries, preview waveforms |
| Load `Algo más` to deck 1 | Loaded from `H USB1`; detail and overview waveforms render |
| Play | Remaining time advanced 03:22 -> 03:11 in ~11 s; deck MASTER |
| PRO DJ LINK Off -> On -> Off (Utility, confirmed) | Both hub sources retained, deck kept playing; log `mode continuity: retained mask=0x80`, `mode recovery ... retained=0x80` for epochs 2 and 3; setting restored to Off |
| USB STOP selector -> eject HUB_LIB2 (unloaded) | `safe-eject success targets=1`, `Safe to remove HUB_LIB2`; its row removed; HUB_LIB, playing deck and mod unaffected |

The selector opened in its touch variant (`eject-ui touch-selector ... fault=3`)
because the core had switched off the frame filter (`took longer than 200 us 8 times`).
This is the known TCG timing behavior already recorded in CURRENT-STATE. The
frame filter is an SDK service, not a `claim_hook`. The 0.4.8 touch fallback
behaved as designed. The knob path was exercised separately below.

Scripted regressions. These require EP147 environment
`AZHUB_EJECT_STATUS=1 AZHUB_EJECT_TRACE_LOAD=1` and the corrected
(hub-aware renotifier) emulator image. The booted image is the uncorrected
`initramfs-patched.cpio.gz` (`39b9119c...`), whose patch-37 `--until-ack` poller
announced a hub stick as native `USB2` on the first attempt. A volatile
`/run/systemd/system/EP147.service.d/90-crash-repro-telemetry.conf` sets the two
variables. It resets `ExecStartPost` and re-adds only the real
`az-usbload --post-start` line, which drops the emulator poller as the corrected
image does. It disappears at the next guest boot.

`test-loaded-eject-regressions.py` on `efb4a0ba...`:

- PASS: repeated knob-selected requests from SOURCE preserve SOURCE and permit
  reopening (knob selector, busy-refusal path).
- Stopped at the calibration load's `assert_load_disk`. The script assumes the
  historical fixture where `sdb` is a native USB2 stick behind `/B`/`/C`. Here
  `sdb` is hub stick A. The actual request
  `0x1010200000602` resolves to player 7 `usb`, i.e. `/media/player07/usb`, which
  is mounted from `sdb1`: the correct hub source. This is a fixture-layout
  mismatch, not a mod failure, but the remaining checks (failed-load stale
  metadata eject) did not run.

`test-loaded-eject-emulator.py` pins version 0.2.1 and was not run.
`test-safe-eject-emulator.py` and the topology/partition suites need the same
historical native-fixture layout and disposable FAT images.

## Shipped 0.4.12 runtime and package

Published 0.4.11 contains the Ethernet sharing runtime and helper assets
(`usb-hub-support-network-helper.so`, `public-dhcp-event-hook.so`,
`azhub-source.c`). Its runtime matches the `sharing-runtime-candidate` target: a
fresh rebuild of 0.4.11 sources is 241,968 bytes with the sharing strings, against
the published 241,992. The exact 0.4.11 build recipe was not recorded in the
repository; the 24-byte difference is unexplained.

0.4.12 runtime: `make sharing-runtime-candidate` in pinned `az-mods-sdk:1` from
the committed LF sources:
`d23170c5eca4a637a84f031189716942ce317a4f8c5521343ab2a16e40cb2167` (241,968
bytes). The `0x237f900` claim passes `x3=0x45d78+0x7f8`, the slot
`probe_queue_pop` loads at `0xdd98`. None of the 57 claim sites passes a stack
address.

Package. The manifest comes from source. Every asset, `LICENSE` and the icon are
byte-copied from the published 0.4.11 package; their sources are unchanged since
`8405739`. `azmod-pack package --github ... --tag usb-hub-support-0.4.12` gives
`fa559c1847f2316b05e107660c3d1c0358018f640f55a6df03f5296763e3a5a3` (711,704 bytes),
and `validate` reports OK. Compared with 0.4.11, the package has the same 18 members
and modes; only `azmod.json` and `usb-hub-support.so` differ.

Exact-runtime emulator smoke pass. Same fixture as above, with the runtime mapped
as `d23170c5...`:

- Load from HUB USB 1 was admitted as `source=0` (`0x1010200000602`).
- Playback advanced from 9.3 s to 14.0 s.
- With Link On and then Off, both hubs were retained and the deck kept playing.
- The knob-selected eject of HUB_LIB2 reached `safe-eject success` with the
  "Safe to remove" panel.
- The PID stayed the same throughout, with `NRestarts=0`.

On Link changes the sharing helper launches, then reports
`startup timeout`, then retires gracefully. Published 0.4.11 (`02fd149b...`) in the
same fixture shows the identical sequence, so it reflects the emulator's
missing sharing-network fixture rather than this change. Evidence:
[candidate record](acceptance/usb-hub-support-0.4.12-startup-crash-candidate.json).

## Timing and limits

The crash epoch and several file/service timestamps use September 25, while the
current journal and network-adjusted device clock use November 22. Loader logs
also change from `05:44` to `02:30`. Startup occurred with an unadjusted clock;
the crash epoch is not a reliable calendar date for the owner's incident.

No previous-boot journal was available. Consequently, this evidence does not
establish why the earlier prolonged session ended, whether its failure matches
this startup crash, or whether changing users contributed. No memory leak,
USB hardware failure, kernel panic, or prolonged-session defect is established.
The suspension text alone is not proof of the root cause.

## Preserved local evidence

The following files are retained outside this worktree in the original
checkout's ignored `local/crash-20261121/` directory:

- `crash.log`: retrieved crash record.
- `usb-hub-support.so`: retrieved staged mod executable, hash above.
- `context.txt`: framework status, mod/loader logs, recent EP147 journal and uptime.
- `diagnosis.txt`: initial inspection notes.

Machine-local directory:
`C:/Users/icast/Repos/nacho-az-mods/local/crash-20261121/`.
Raw captures and the executable are not Git artifacts. No device files,
approvals, settings, activation state or process memory were changed during
inspection, and no restart was requested.

## Active goal and remaining work

The owner requested an emulator diagnosis and a new version ready for
publication. The goal is active. The cause is reproduced and the fix is
verified against it in the emulator. No new version or release package has been
completed.

1. Done: SDK/core publication ordering verified statically, and the window
   reproduced in the emulator with an unfixed negative control (2/2).
2. Done: every production claim publishes through its callback storage
   (source, WSL cross build and pinned-container artifact `efb4a0ba...`).
3. Done: the corrected control survives the same forced window (2/2). The
   0.4.12 candidate passes the manual emulator smoke pass (source, browse, load,
   waveform, playback, Link Off/On/Off, eject) and the first scripted knob-selector
   regression. Open: scripted suites built for the historical native-fixture
   layout need that fixture (or adaptation) before their remaining checks run.
4. Partly done: the full portable host suite (`test-host.sh`, WSL
   `TEST_CC=clang`, plain/ASan/TSan) passed on the corrected sources
   (2026-11-21, exit 0, 230 PASS lines). The run includes `test-hook-publication.py`. With
   `TEST_CC=gcc`, WSL stops at the C++ unwind step because `g++` is not installed.
   Remaining: SDK whole-mod harness and package checks on the versioned candidate.
5. Done: version 0.4.12, [release notes](releases/usb-hub-support-0.4.12.md)
   and exact-artifact evidence. The owner authorized publication on 2026-11-21.

The active goal does not include physical-controller deployment; physical
acceptance of 0.4.12 is not claimed.

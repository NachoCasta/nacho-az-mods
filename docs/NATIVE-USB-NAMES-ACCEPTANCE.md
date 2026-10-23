# Native USB Names 0.1.0 local candidate acceptance

Status: validated local candidate ready for a separate physical-testing decision. No physical hardware acceptance.

Exact target: firmware 2.00 `ep147-9e9454e357389ece`, hardware kernel 4.4.194. SDK 1.2.3 commit `62a156abf3a4789e9e3cdd610fe106227a3a8967`. The separate emulator executable explicitly accepts only kernel `6.6.138-dirty`. Both use the same production source and instruction checks; only the kernel build gate differs.

## Regressions

All five suites pass in plain, ASan/UBSan and TSan builds: policy/Unicode, real temporary udev/by-label lookup, production worker, native presentation-listener dispatch, and production information-task bridge. Coverage includes mixed case, malformed UTF-8, surrogate encodings, whitespace/controls, bounds, code-point-safe truncation, exhaustive native/foreign identities, blocked old filesystem results, 40 insertion-reuse cycles, neighbor independence, stale events, ownership checks and contention retry. Tests execute owned fake native objects, never firmware on the host.

Command: `SAN="plain asan tsan" TEST_NO_ASLR=1 sh src/native-usb-names/test-host.sh` inside the existing `usb-hub-host-tests:local` image (GCC 13.3). The runner changes into its source directory. The isolated container needs `--security-opt seccomp=unconfined` for per-process `setarch -R` on the TSan pass. The initial TSan address-map failure occurred before tests ran; the successful retry does not change host ASLR or restart services. Executables remain ignored under `local/native-name-tests/`.

## Final hardening delta

The final v9 build clears the output when truncation leaves no visible characters, and the production worker discards partial output whenever label acquisition reports failure. A 31-NBSP prefix followed by visible text and a deliberately dirty failed-read fixture now have explicit regressions. All five suites pass again in plain, ASan/UBSan and TSan; actual stdout is preserved in `local/v9-host-checks.json`. v8 package receipts are marked superseded.

The following v8 rows preserve the original integration controls. Final v9 exact-runtime results are recorded separately below; do not infer their identity from v8 screenshots.

## Emulator evidence

Owned instance: SSH 2232, QMP 4456, private writable overlays, controls and logs under `local/emu/`. All device-control/SSH operations targeted the owned 2232/4456 instance. Early upstream control-state defaults were replaced with encoder/mixer state under this worktree; no shared services were restarted. Only normal framework discovery, Allow, Turn off/on and empty-deck apply/reload were used. No internal approval/settings edits, process-memory writes, OS changes or physical access occurred. Framework staging is persistent internal storage.

An emulator-only getter fault mod supplies unavailable, empty and invalid-control presentation values without rewriting disk properties. It is not part of the candidate archive. The corrupt-database volume fixture alone naturally falls back natively and cannot prove this mod's behavior. The disabled-mod control with the same fault provider shows generic USB1; the enabled exact v8 runtime shows VOL_FALLBK.

| Check | Local evidence | Result |
| --- | --- | --- |
| Exact mapped new-mod runtime, no hub loaded | `local/emu/v8-alone-runtime.json` | SHA256 verified against the built v8 executable; fault helper present |
| Missing name, usable label | `local/emu/v8-controlled-volume.png` | VOL_FALLBK; USB2 retains Nacho Chips |
| Empty name | `local/emu/empty-choice.png`, `local/emu/v8-empty-volume.png` | Empty control selected; VOL_FALLBK |
| Invalid control name | `local/emu/invalid-choice.png`, `local/emu/v8-invalid-volume-settled.png` | Invalid selected; VOL_FALLBK |
| Both missing after source reuse | `local/emu/v8-both-missing-reused.png` | USB1, no stale VOL_FALLBK; USB2 unchanged |
| Native icons/colors and neighbor | `local/emu/v8-presentation-pixels.json` | Exact crop equality against failed refresh control |
| Browse previews | `local/emu/v8-track-list.png` | Actual analyzed previews visible |
| Native load and real waveform | `local/emu/v8-usb2-waveform.png` | Algo más; detail and overview visible |
| Detector positive/negative | `local/emu/v8-waveform-positive-negative.json` | Loaded deck passes; empty deck fails as expected |
| Native playback engine | `local/emu/v8-play-a.png`, `local/emu/v8-play-b.png` | Visible position advances 1.522 to 2.868 seconds; no audible or hardware acceptance |
| Supported rollback/reactivation | `local/emu/v8-disabled-settled.png`, `local/emu/v8-reenabled-settled.png` | USB1 generic when off, VOL_FALLBK when on; native neighbor preserved |
| Exact coexistence runtimes | `local/emu/v8-coexistence-runtime.json` | Both mapped hashes verified; hub 0.4.5, one hub source, Link off, USB1 fault helper active |
| Distinct native and hub sources | `local/emu/v8-coexistence-source.png` | Native fallback, native configured name and independent HUB row |
| Concurrent native/hub waveforms | `local/emu/v8-coex-both-waveforms.png` | Native deck 1 and hub deck 2 both retain real detail/overview waveforms |

All screenshots remain local and ignored. Early captures during native restart/insertion are not acceptance evidence. The superseded v4/v5 refresh failures remain described in the research notes. Do not use their packages for physical testing.

## Final v9 emulator results

The corrected emulator executable is `8888153fcb3e6f5c736313f241d9c9eefd02b23be62bd8f9f81a8d20e1130d25`. Its mapped file identity was verified, including the absence of other running mods in the standalone pass. The exact hardware executable is separately built with the hardware-only kernel gate; emulator results do not establish hardware acceptance.

| Check | Local evidence | Result |
| --- | --- | --- |
| Standalone exact mapping | `local/emu/v9-standalone-runtime.json` | Only Native USB Names active; helper and hub off |
| Configured name priority and native presentation | `local/emu/v9-standalone-source-settled.png` | Nacho Chips preserved over actual HUB_LIB volume label |
| Native Browse and load | `local/emu/v9-standalone-selected.png` | Analyzed previews and selected Algo más |
| Real standalone waveforms | `local/emu/v9-standalone-waveform.png` | Detail 18,517 colored pixels; overview 8,847; both visible |
| Standalone playback engine | `local/emu/v9-play-a.png`, `local/emu/v9-play-b.png` | Position advances 0.526 to 2.474 seconds; no audible acceptance |
| Fault-scope exact mappings | `local/emu/v9-faults-runtime.json` | Exact new mod and excluded both-port helper; hub inactive |
| Independent native fallbacks | `local/emu/v9-both-fallbacks.png` | USB1 VOL_FALLBK, USB2 HUB_LIB |
| Empty name | `local/emu/v9-empty-choice.png`, `local/emu/nusb-v9-empty.png` | Empty selected; fresh epoch 5 resolves VOL_FALLBK |
| Invalid control name | `local/emu/v9-invalid-choice.png`, `local/emu/nusb-v9-invalid.png` | Invalid selected; fresh epoch 7 resolves VOL_FALLBK |
| Both unavailable | `local/emu/v9-unavailable-choice.png`, `local/emu/nusb-v9-noname.png` | Unavailable selected; epoch 9 keeps USB1 generic, no prior label |
| Final icon/color comparison | `local/emu/v9-presentation-pixels.json` | All six native icon/color crops match the standalone baseline exactly |
| Final waveform controls | `local/emu/v9-waveform-controls.json` | Native loaded positive, empty-deck negative, and both coexistence decks pass their expected visibility checks |
| Exact final coexistence | `local/emu/v9-coexistence-runtime.json` | Native v9, excluded both-port helper and hub 0.4.5 mapped hashes verified again with decks LLEE in PID 270655 |
| Final independent source labels | `local/emu/v9-coexistence-source.png` | Native VOL_FALLBK and HUB_LIB remain separate from configured hub Nacho Chips |
| Final real concurrent waveforms | `local/emu/v9-coex-both-waveforms.png` | Native detail/overview 18,517/8,847 pixels; hub 19,000/8,844; native USB2 and HUB USB1 identities retained |
| Reinsert/source reuse | `local/emu/nusb-v9-restored.png`, `local/emu/nusb-v9-*-swap.json` | Epoch 11 restores VOL_FALLBK; neighbor HUB_LIB unchanged; all four swaps keep PID 237088 |

Each swap checks the observed native USB1 controller, current-PID empty decks, native unmount before detaching, and the exact private overlay. No mounted mods USB was force-detached. The helper's settings do not survive reload through its initial test implementation; final tests use explicit both-port startup defaults and live UI mode choices. This is an excluded emulator test control, not candidate settings behavior.

## Package and remaining scope

The authoritative [candidate receipt](acceptance/native-usb-names-0.1.0-candidate.json) records final source digests, executable/archive/member hashes and all scoped evidence. The package is `local/candidate-0.1.0/mod/dist/nachocasta-native-usb-names-0.1.0.azmod` (61,331 bytes). Package SHA256: `96f78e4945002ff3af95172167d846cef169cd49ff68078d9001826ba4a8e1d5`. Hardware executable SHA256: `fe8d50ead1488a7f72211042bd119390827b364a8eee541efe024444ea06d4c4`. SDK directory and archive validation pass without warnings. The archive contains only manifest, icon, license and hardware executable; no fault helper, hub dependency, screenshot, firmware or music.

Installation, rollback and the physical test checklist are in `src/native-usb-names/README.md`. Physical tests require separate user approval for this new mod. Remaining hardware work includes real USB insertion timing, both native ports with configured/unnamed Unicode media, actual waveform visibility, audible playback, removal/reinsert, rollback, and hub coexistence. No phase-sync, actual-CDJ, eight-source or long-session acceptance is claimed. No release, public catalog change, push or physical deployment occurred. All remaining physical work requires a new, explicit approval.

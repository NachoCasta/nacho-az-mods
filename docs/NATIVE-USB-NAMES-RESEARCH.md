# Native USB naming research

Scope: exact firmware 2.00 `ep147-9e9454e357389ece`. Raw analysis and screenshots remain under ignored `local/`.

## Native boundaries

- `0x1c824f8` constructs the Source row title/subtitle pair and is called by model insertion `0x1c373f0` and refresh `0x1c34dc8`. Core already owns model insertion; do not claim it.
- `0x1c7fef0` calls configured-name provider `0x1c7fad8` before applying the native generic fallback. Its consumers include Source and other source-presentation controllers.
- `0x1c7fad8` receives provider in x0, selector in x1, MediaID pointer in x2 and a nontrivial native String return buffer in x8. Native String is one UTF-8 handle; its C++ nontrivial return ABI must not be replaced with an ordinary one-word C return.
- Provider `+8` retains the native storage-manager service. Exact vtable `0x2b7b538`, method `+0xe8` (`0x11199b8`) returns the native root and flags using x8. Its String is released through native ownership operations.
- Native manager notification `0x111d538` (vtable `+0x10`) dispatches information-change listeners at virtual `+0x40`. The ordinary native update path `0x111d6a0` invokes this notification after retaining metadata. A label refresh must notify only; it must not replace metadata or native identity.
- Local mount-complete enqueue `0xbbaad8` receives controller, LocalMediaID, management-system pointer, root, alternate root and PropertyTable in x0 through x5. Invalidating the naming epoch before forwarding prevents publication of an old insertion after source reuse. It does not create a mount.

## Instruction checks

At `0x1c7fad8`: `a9ba7bfd 910003fd a90153f3 aa0203f3`.
At `0xbbaad8`: `a9ac7bfd 910003fd 79400023 aa0503e1`.
At `0x11199b8`: `a9bb7bfd 910003fd a9025bf5 aa0103f6`.

All are exact-firmware checks, not compatibility claims for other firmware. Both hook claims succeeded in the isolated emulator pass-through diagnostic. The v8 presentation bridge has been exercised in the owned emulator; see the acceptance record.

## Observed diagnostic

SHA256 `4bd2994801cb89d46f522366a52e30df0aa5918893a27412dd9e251e05641ef0` loaded through normal USB discovery, UI Allow and framework apply. Native USB1: outer member1, sdid2; native USB2: outer member1, sdid0x22. Both returned their own `/media/usb/sd*1` roots, flags1 and configured `Nacho Chips` unchanged. The diagnostic did not change names. This confirms the ABI, identity and ownership path; it does not validate fallback, waveforms or hardware.

The initial diagnostic claimed only the first two functions. The current presentation adapter additionally claims the native information-change task body `0x1c1ff10`. None of these three addresses is claimed by USB Hub Support 0.4.5's production hook set. Mapped executable hashes and native/hub waveforms were verified together with the exact hub 0.4.5 emulator runtime.

## Environment

An owned emulator uses SSH2232/QMP4456, private writable overlays over inactive local fixtures and its own logs under `local/emu/`. All QMP/SSH device-control operations target the owned SSH2232/QMP4456 instance. Early upstream encoder/mixer defaults were replaced with state under this worktree. No shared service was restarted. Ubuntu WSL later returned `HCS_E_CONNECTION_TIMEOUT`; it was not restarted. Available Docker SDK and host-test images supply an independent compiler/test path. The SDK checkout matches commit `62a156abf3a4789e9e3cdd610fe106227a3a8967`.

## Preserved failed integration control

The first functional emulator executable `d2ff474600e2d0b642d34c50587f1ab9753b51a80655ce1748aa87cbf798846c` loads alone and preserves actual configured `Nacho Chips`. A separate emulator-only fault helper at native name-getter `0x111c1e0` returns unavailable input for native USB1 without rewriting stored metadata. The worker obtains a valid volume label, but the settled Source screenshot still shows generic `USB1`. This is a failed refresh control, not fallback acceptance. The earlier 32MiB corrupt-database fixture alone naturally uses the volume label without exercising this mod; it is not a positive mod control.

A reduced two-native emulator avoids the stock `sd[a-f]` udev rule limit. The first unexpected-detach attempt with five hub fixtures produced `sdh1`, which was not mounted or announced. No OS or rule edits were made. Subsequent normal native STOP safely unmounts USB1 before replacement and keeps USB2 present.

The real production worker passes a blocked-old-read race,40 source-reuse cycles and neighbor preservation in plain, ASan/UBSan and TSan controls. TSan initially failed before execution because of host address randomization; per-process `setarch -R` inside an isolated container passes without changing host settings. These tests do not establish native UI refresh or waveform acceptance.


## Native title refresh bridge

Read-only runtime inspection confirms that `0x111d538` reaches RootSourceDataSet's information listener. That listener invokes Source controller callback `0x1c305a8`, which repaints the list without reloading cached row titles. This explains the failed generic-label result; it is separate from the title callback `0x1c305c8`.

RootSourceDataSet's existing presentation listener array is at `+0x1a8`, with capacity `+0x1b0` and count `+0x1b8`. The Source controller's matching listener has vtable `0x2e0b538`, whose first method is `0x1c305c8`. That method refreshes existing Source model strings through `0x1c34dc8` and invokes the ordinary list refresh. Information listeners and title listeners must not be confused.

Native changed-event task `0x1c1ff10` runs on JUCE's message thread, either directly or through RootSourceDataSet's native task queue. Its live closure holds the dataset pointer at0, a complete12-byte SourceID at8 and MediaID at20. The bridge first forwards the original task unchanged, then dispatches only a pending, generation-matched native USB title through the registered Source presentation listener. It uses the existing SourceID and verifies its storage identity against the event. It never retains a widget pointer, constructs another service, changes stored metadata or starts a mount.

Additional entry checks: task `0x1c1ff10` is `a9ba7bfd 910003fd a90153f3 aa0003f3`; title callback `0x1c305c8` is `a9be7bfd 910003fd f9000bf3 d100a013`. The closure layout has compile-time offset checks. Actual-task tests cover original forwarding, stale/foreign/mismatched identities, wrong ownership and contention. The v8 emulator bridge passed the controlled unavailable/empty/invalid-name Source checks; see the acceptance record.

## Identity and final validation notes

Native root and nested information events can differ in the byte after the native storage selector. The adapter compares the outer member plus native storage byte (first five MediaID bytes), retaining the complete original eight-byte ID for native queries and the complete SourceID for observer dispatch. The per-port insertion epoch and exact native root additionally guard source reuse.

The final hardening rejects non-visible Unicode prefixes produced by truncation and any partial label returned with a failed acquisition result. This changes the portable output contract and worker publication only; the three exact native hooks and their ABI remain unchanged. See the acceptance record for final executable identity and scoped emulator results.

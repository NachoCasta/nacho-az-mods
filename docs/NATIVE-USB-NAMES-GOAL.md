# Native USB Names candidate goal

Status: validated local candidate ready for the user's separate physical-testing decision. No physical acceptance. See [acceptance](NATIVE-USB-NAMES-ACCEPTANCE.md) and the [candidate receipt](acceptance/native-usb-names-0.1.0-candidate.json).

Create an independent presentation-only AZ-MODS mod, stable ID `nachocasta-native-usb-names`, version 0.1.0. Preserve valid rekordbox device names exactly, including case. Only unavailable, empty or invalid names fall back to the filesystem volume label; absent usable labels retain the native generic name. Preserve native USB identity, colors, icons, library routing and caches. Do not mount metadata early, rewrite disks or depend on USB Hub Support.

Compatibility is restricted to firmware 2.00 `ep147-9e9454e357389ece`, kernel 4.4.194 and SDK/core 1.2.3. Build SDK commit: `62a156abf3a4789e9e3cdd610fe106227a3a8967`. Research actual native presentation boundaries and verify exact instructions before declaring hooks. Validate separate emulator target explicitly; do not broaden hardware compatibility.

## Acceptance gates

- Configured mixed-case names remain byte-for-byte equivalent after Unicode decoding; missing/empty/invalid configured values use only a usable volume label.
- Both missing retains native fallback. Test Unicode, surrogate validity, bounds and truncation.
- USB1 and USB2 remain independent. Removal, reinsert and slot reuse never expose stale names.
- Native icons, colors, browse/load/playback state and real rendered waveforms remain intact.
- Emulator tests cover this mod alone and coexistence with USB Hub Support; screenshots remain ignored under this worktree local directory. Preserve failures and evidence boundaries.
- Record exact executable/package SHA256, toolchain, package validation, meaningful regressions, local screenshot paths, installation and rollback instructions, and remaining physical checklist.
- Review and commit source/tests/documentation locally on the feature branch. No generated binaries or proprietary research in Git.

## Boundaries

No physical deployment, reload or approval-list edits. Ask the user before physical testing of this new mod. No publication, catalog changes, public push, firmware/core/OS edits, process-memory edits or shared-service restarts. Framework persistent staging is internal storage, not RAM or USB-only persistence. Emulator results do not establish hardware acceptance.

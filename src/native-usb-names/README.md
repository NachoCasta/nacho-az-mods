# Native USB Names

Standalone AZ-MODS mod, stable ID `nachocasta-native-usb-names`, version 0.1.0.

Keeps a valid rekordbox device name exactly, including its case and Unicode. When the configured name is unavailable, empty or invalid, the native USB source displays a usable filesystem volume label. If both are unavailable it keeps the native generic fallback. Volume labels are bounded to 31 UTF-16 units without splitting Unicode; configured names are not reformatted.

Presentation only: native USB identities, icons, colors, servers, caches and routing remain native. The mod never renames disks, rewrites metadata, mounts libraries, writes its own persistent data or depends on USB Hub Support. Label lookup reads udev metadata on a worker; native UI hooks perform no filesystem I/O. Each native mount completion invalidates the corresponding insertion before any old worker result can be used.

Compatibility: firmware 2.00 `ep147-9e9454e357389ece`, hardware kernel 4.4.194, core>=1.2.3. Build headers and tools are SDK 1.2.3 commit `62a156abf3a4789e9e3cdd610fe106227a3a8967`. Unknown firmware, kernel, instruction words or occupied hooks fail closed. An explicit development emulator build accepts only kernel `6.6.138-dirty`; it is not the hardware package.

## Local build

Use the pinned SDK template compiler container, or an explicitly recorded compatible AArch64 toolchain. The Makefile builds only; it never deploys or reloads.

```sh
make -C src/native-usb-names CXX=aarch64-linux-gnu-g++ \
  AZ_SDK_INCLUDE=/absolute/sdk/include OUT=/absolute/worktree/local/hardware
# Separate emulator executable:
make -C src/native-usb-names CXX=aarch64-linux-gnu-g++ \
  AZ_SDK_INCLUDE=/absolute/sdk/include EXTRA_CPPFLAGS=-DNUSB_EMULATOR \
  OUT=/absolute/worktree/local/emulator
```

Use the pinned `azmod-pack` to validate and package a local staging directory containing the manifest, original reproducible storefront icon, GPL license and exact hardware executable. Record hashes before installing. Source and compatibility research are in [the goal](../../docs/NATIVE-USB-NAMES-GOAL.md) and [native research](../../docs/NATIVE-USB-NAMES-RESEARCH.md). See [local acceptance](../../docs/NATIVE-USB-NAMES-ACCEPTANCE.md) for validation status and evidence. The [candidate receipt](../../docs/acceptance/native-usb-names-0.1.0-candidate.json) identifies the reviewed package and executable hashes. The exact hardware executable was verified as loaded alongside USB Hub Support 0.4.5, and the user reported successful physical operation. Detailed physical waveform, audio and remaining checklist acceptance are not separately claimed. See the [release notes](../../docs/releases/nachocasta-native-usb-names-0.1.0.md) for the published package.

## Physical installation, after separate user approval

1. Obtain explicit approval to test this new mod and identify the reviewed package/executable hashes. USB Hub Support's SSH approval exception does not apply.
2. With empty decks, retain a rollback copy of the current mods USB configuration. Install the candidate through supported AZ-MODS package controls, or unpack the reviewed package into `MODS/community-mods/nachocasta-native-usb-names/` on the mods USB as documented by the SDK.
3. Use the normal framework discovery flow. In Community Mods > Installed > Native USB Names, select Allow for the exact build. If discovery requires a reload, distinguish it from the later activation reload.
4. Confirm approval readiness, then use normal Reload with empty decks. The framework persistently stages executables/settings internally; this is not RAM or USB-only persistence.
5. Run the physical checklist below. Do not infer hardware acceptance from emulator screenshots.

## Rollback

With empty decks, turn Native USB Names off in Community Mods and use normal Reload. To uninstall, use Remove from USB through the framework and reload normally. Removing the USB alone does not unload a running library. Native metadata and disk labels require no restoration because the mod never changes them. USB Hub Support can remain installed; it has a distinct ID and production hook set.

## Remaining physical checklist

- Confirm exact firmware, kernel, core, package hash, executable hash and loaded result.
- Test this mod alone first, then with the reviewed USB Hub Support 0.4.5.
- `Nacho Chips` must retain its mixed case even when its volume label is `NACHO CHIPS`.
- Empty/missing configured names must use the correct per-port label; both missing must keep native USB1/USB2. Check non-ASCII labels and safe truncation where available.
- Exchange distinct media on both ports, safely remove/reinsert and reuse a port; no old name may leak to a new source. The neighbor must remain unchanged.
- Confirm native colors/icons, source selection, browsing and fresh track loads.
- Inspect actual Browse preview, detailed waveform and overview waveform on analyzed tracks, then play and seek. Report visible waveform and audible playback separately.
- Unload, disable/reload and confirm native fallback presentation. Record failures and exact bytes; do not claim long-session or hardware timing acceptance from a short trial.

The user subsequently authorized physical installation, exact-hash approval and publication of 0.1.0. That authorization is recorded in the acceptance follow-up. Future publication or device changes require their applicable authorization.

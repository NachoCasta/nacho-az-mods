# Nacho AZ Mods

**NachoCasta's** collection of XDJ-AZ mods, tools, and research notes.

USB Hub Support0.4.6 adds HFS+ rekordbox libraries through hubs, including
analyzed waveforms, while retaining port-based numbering and configured names/colors.
HFS+ was validated in the emulator; the owner confirmed ordinary USB operation on
the physical AZ. See the [0.4.6 release notes](docs/releases/usb-hub-support-0.4.6.md)
for validation and limits.

Add `NachoCasta/nacho-az-mods` under **Community Mods > Repositories** in the
AZ-MODS app or on the deck, then refresh **Browse** to find USB Hub Support.
Downloads are hosted in this repository's GitHub Releases. The official Mod Index
submission is tracked in [index PR #4](https://github.com/Kyle-Hosman/az-mods-index/pull/4);
see the current-state notes for the latest recorded inspection.

## Start here

- [Current state and next steps](docs/CURRENT-STATE.md).
- [Release guide](docs/RELEASING.md): prepare, validate, publish, and verify a new version.
- [Publishing model and Orca organization](docs/PUBLISHING.md).
- [Research index](docs/RESEARCH-INDEX.md).
- [Local Ghidra, firmware, and build resources](docs/LOCAL-RESOURCES.md).
- [Provenance, licenses, and validated binary](docs/PROVENANCE.md).
- [Agent instructions](AGENTS.md) and [Claude entry point](CLAUDE.md).

Repository documentation, code comments, commit messages, PRs, and release notes are written
in English. Preserve identifiers and observed test data, including track titles, verbatim.

## USB Hub Support

**0.4.4 adds experimental Ethernet sharing of hub USB libraries over PRO DJ LINK.**
One physical hub stick passed independent software-client discovery, full library
browsing and exact complete database/track downloads, including after native Link
Off/On. Native USB access remains available. Actual-CDJ interoperability and Wi-Fi
sharing remain unvalidated. See [release notes](docs/releases/usb-hub-support-0.4.4.md).
The local-only statements below describe earlier releases.

**0.3.1 adds native-style HUB source labels and a clearer eject selector.**
Source/Browse/Waveform reuse the AZ's USB artwork and lettering; additional
numerals3–8 use the closest native font. The user approved the appearance on
the physical AZ. Native artwork remains on the controller and is not packaged.
See [0.3.1 release notes](docs/releases/usb-hub-support-0.3.1.md).

**0.3.0 adds PRO DJ LINK coexistence:** four local decks with Link Off, or the
native two-local-deck mode with Link On and external players. Hub libraries remain
local and are not shared with external CDJs. Real-AZ tests covered a software peer
over Wi-Fi, master/tempo/SYNC, reconnect and a fresh hub-track load. Actual CDJ
interoperability and audible phase alignment remain unvalidated.

**0.3.2 preserves hub sources across PRO DJ LINK On/Off changes.** The owner
confirmed the loaded-track continuity trial works on the physical AZ. Repeated
emulator checks retain source identities without announcement replay, preserve
loaded decks1/2 and permit fresh loads after either transition. Native disabling
of decks3/4 with Link On remains expected. See
[0.3.2 release notes](docs/releases/usb-hub-support-0.3.2.md) for evidence and limits.
Actual-CDJ, phase-sync and long-session validation remain open.
The external-player blocking warning in older release notes applies to0.2.1 and earlier.

Connect multiple USB sticks to your XDJ-AZ through a USB hub. Browse playlists and tracks
from each stick directly on the controller, just as you would with a normally connected
USB stick. Supports up to eight sticks; five were tested together in the emulator.
No computer or external players are required during use.

**0.1.4** was installed over SSH and the user confirmed it works correctly on their AZ.
**0.1.8** adds the PRO DJ LINK warning; its executable and assets are unchanged from 0.1.7.
**0.1.7** updates the package description; its executable and assets are unchanged from 0.1.6.
**0.1.6** adopts the USB Hub Support name and `usb-hub-support` ID. It rebuilds the
same implementation with updated core ownership, asset and thread identifiers. Firmware
compatibility is unchanged: firmware 2.00 (`ep147-9e9454e357389ece`), kernel 4.4.194,
and Community Mods core 1.2.3 or later. The user confirmed installing 0.1.7 from this repository and browsing, loading and playing
tracks with two sticks on a hub plus one on native USB1 (three simultaneous sticks).

**Migration:** uninstall the old `hub-libraries` mod before enabling `usb-hub-support`.
They use the same hooks and must not run together. The app treats the new ID as a new mod,
not an in-place update. See the [release notes](docs/releases/usb-hub-support-0.1.8.md).

Mod source: [src/azhub/linkfake](src/azhub/linkfake).
Kernel compatibility module: [src/azhub/azhub.c](src/azhub/azhub.c).
SDK header: [pinned v1.2.3 header](https://github.com/Kyle-Hosman/az-mods-sdk/blob/62a156abf3a4789e9e3cdd610fe106227a3a8967/include/azmod-mod.h).

**0.2.1** allows safe eject with tracks loaded. Short-press USB STOP to select a
stick, or hold it to eject the whole hub. Wait for the final safe-to-remove
confirmation before unplugging. Loaded tracks use the firmware's native playback
cache, whose capacity and available playback functions can vary.

The user confirmed individual loaded-track eject and complete playback of one
track from NACHO PLUGS on the real AZ. SSH confirmed the filesystem and library
alias were unmounted while the device remained connected; CAMI and native USB1
stayed mounted. Whole-hub eject with two playing decks passed in the emulator.
Long-session native memory behavior and QEMU input-filter timing failures remain
under investigation; broader hardware/cache-capacity coverage is pending.
See [0.2.1 release notes](docs/releases/usb-hub-support-0.2.1.md).

**0.2.0** introduced the short/long USB STOP safe-eject controls and passed
user-operated two-stick hardware trials after unloading tracks. Loaded-track
eject is introduced in 0.2.1.

The imported layout preserves references from the original research. Future mods can use
`mods/<id>/`, each with its own manifest, build, license, and independent version history.

## Local references

Screenshots remain in `src/azhub/research/shots/`, ignored by Git. They are neither uploaded
nor backed up, as requested by the user. USB images, music, firmware, full Ghidra projects,
complete disassemblies, and SSH keys are also kept outside Git. Research indexes and notes
are versioned. Read historical scripts before running them: some modify emulator or deck state.

## Build and package

Pinned SDK: `Kyle-Hosman/az-mods-sdk`, `v1.2.3`, commit
`62a156abf3a4789e9e3cdd610fe106227a3a8967`.

```sh
git clone --branch v1.2.3 --depth 1 https://github.com/Kyle-Hosman/az-mods-sdk .azmod-sdk
python3 .azmod-sdk/tools/azmod-pack build src/azhub/linkfake
python3 .azmod-sdk/tools/azmod-pack package src/azhub/linkfake
```

On Windows, build through WSL/Docker. Validation and packaging also work with Python on
Windows. `src/azhub/linkfake/build.sh` retains the Linaro build method used for 0.1.4;
`AZHUB_TOOLCHAIN_PREFIX` selects a different toolchain.

The mod and SDK use GPL-3.0-only; `azhub.c` and its kernel module use GPL-2.0-only.
See [provenance](docs/PROVENANCE.md). This is not an official AlphaTheta product.

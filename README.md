# Nacho AZ Mods

**NachoCasta's** collection of XDJ-AZ mods, tools, and research notes.
Add `NachoCasta/nacho-az-mods` under **Community Mods > Repositories** in the
AZ-MODS app or on the deck, then refresh **Browse** to find USB Hub Support.
Downloads are hosted in this repository's GitHub Releases. The optional Mod Index
listing has not been requested.

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

**PRO DJ LINK:** While this mod is enabled, communication with external players is blocked.
Disable the mod and restart the AZ before using external PRO DJ LINK. Turning Link on
does not disable the mod. Hub USB sources are not shared with external CDJs.
For standalone hub use, leave the PRO DJ LINK setting off.

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
SDK header: [src/azmod/azmod-mod.h](src/azmod/azmod-mod.h).

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

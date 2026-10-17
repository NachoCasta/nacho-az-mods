# Nacho AZ Mods

**NachoCasta's** collection of XDJ-AZ mods, tools, and research notes.
Add `NachoCasta/nacho-az-mods` under **Community Mods > Repositories** in the
AZ-MODS app or on the deck, then refresh **Browse** to find Hub USB Libraries.
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

## Hub USB Libraries

Browse rekordbox libraries on USB sticks connected through a hub. Each source has its own
servers and caches inside the AZ; no computer or external players are required during use.
The implementation currently supports up to eight hub sources. Five were tested simultaneously
in the emulator; eight simultaneous sources have not been validated on hardware.

**0.1.4** was installed over SSH and the user confirmed it works correctly on their AZ.
**0.1.5** is the first standard `.azmod` release: manifest metadata, an original icon,
and catalog packaging. Its executable and kernel asset match the accepted 0.1.4 build;
0.1.5 has not had an additional hardware test. Requires firmware 2.00
(`ep147-9e9454e357389ece`), kernel 4.4.194 and Community Mods core 1.2.3 or later.
See the [release notes](docs/releases/hub-libraries-0.1.5.md) for limits.

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

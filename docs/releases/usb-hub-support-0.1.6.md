# USB Hub Support 0.1.6

Renames Hub USB Libraries to **USB Hub Support**, with mod ID `usb-hub-support`.
The descriptor, hook ownership, asset lookups, logging, worker ownership and build filenames
now use the new ID. Library routing, mounting and waveform behavior are unchanged.

## Install and migrate

Add `NachoCasta/nacho-az-mods` under Community Mods > Repositories, refresh Browse,
and install USB Hub Support. The catalog no longer lists the legacy `hub-libraries` ID.

**Uninstall any old `hub-libraries` copy before enabling this mod.** A different ID is a
new installation, not an in-place update. Both mods claim the same hooks and must not run
together. The user's earlier development installation has not been changed remotely.
The 0.1.5 release remains as historical provenance and is superseded by this release.

## Compatibility and validation

Requires XDJ-AZ firmware 2.00, exact tag `ep147-9e9454e357389ece`, kernel 4.4.194,
and Community Mods core 1.2.3 or later. Firmware compatibility has not been broadened.
Rebuilt with the original Linaro 6.3.1 toolchain. ELF descriptor ID and removal of legacy
ownership strings were checked, focused behavior tests passed, and SDK v1.2.3 validated
the package. The bundled kernel asset is unchanged. The renamed build has not had a
new emulator playback session or real-AZ test; hardware success refers to the prior 0.1.4.

Five simultaneous sources were tested in the emulator on the earlier implementation;
eight are implemented but unverified together. Slot 8 has a blank native icon/caption;
capacity/track-count announcements contain stubs. Identity selection needs an UP IPv4
subnet with a /29 or shorter prefix. Prolonged hardware sessions, removal of a playing
source and same-path replacement without observed absence remain unverified.
Disable the mod and reload through the normal Mod Menu to roll back.

## Artifacts

Executable SHA256: `86f264c27644b399e07226827874b2e41c9a152925ba03c9479c4eb1aa7ee0ba`.
Package SHA256: `f01aa99d1a0e131c27f2ac45fa20305b6000e2d29359787a0240cf8dedbae996` (308586 bytes).
The legacy `/home/root/settings/azhub.conf` read remains the known SDK validator warning.

# Hub USB Libraries 0.1.5

First standard AZ-MODS catalog release. Browse rekordbox playlists and tracks from
USB sticks on a hub, using independent library servers inside the AZ. No external
computer or Pro DJ Link players are needed during use.

## Install

Add `NachoCasta/nacho-az-mods` under Community Mods > Repositories in AZ-MODS,
refresh Browse, and install Hub USB Libraries. Approve it through the deck's Mod Menu.
Do not install a second development copy alongside it: the mod ID remains `hub-libraries`.

Requires XDJ-AZ firmware 2.00, exact tag `ep147-9e9454e357389ece`, kernel 4.4.194,
and Community Mods core 1.2.3 or later. Other firmware is unsupported.

## Changes and validation

- Standard `.azmod` packaging, publication metadata, original icon and catalog entry.
- Runtime executable and bundled GPL kernel asset are unchanged from the user-confirmed
  0.1.4 build, including volume labels and per-stick waveform preferences.
- Five simultaneous sources tested in the emulator; the user confirmed 0.1.4 on hardware.
  No additional 0.1.5 hardware test is claimed.
- Focused behavior tests and SDK build/package validation passed in CI. The shipped
  executable is the accepted Linaro build, not the separate SDK-container build.
- Package validated with SDK v1.2.3; optional legacy configuration is read from
  `/home/root/settings/azhub.conf` (known validator warning, no settings write).

## Known limits

Up to eight sources are implemented; eight simultaneous sources and prolonged hardware
sessions remain unverified. Slot 8 has a blank native icon/caption. Capacity/track-count
announcements contain stubs. Automatic identity selection needs an UP IPv4 subnet
with a /29 or shorter prefix. Removing a playing source and same-path media replacement
without observed absence remain unverified. Disabling the mod and reloading through the
normal Mod Menu is the rollback path.

## Artifact

Package SHA256: `c93e6b35b5da0a0d0dffe45e3d9834287010ff41836b590095dea8393a2ba131` (308564 bytes).
Executable SHA256: `6f03e2b5c9a36853f538019d7e4c1dc4123566c7be7cfab47da50e739bae7f98`.
Source, GPL licenses and research are included in the repository; firmware and music are not.

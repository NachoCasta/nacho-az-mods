# Current state â€” 2026-10-17

## Verified stable build

Hub USB Libraries0.1.4: firmware2.00, EP147 tag `ep147-9e9454e357389ece`, kernel4.4.194,
Community Mods core1.2.3. Binary SHA256:
`6f03e2b5c9a36853f538019d7e4c1dc4123566c7be7cfab47da50e739bae7f98`.
The exact dev ZIP SHA256 is
`c7c7f141b5f406e388bb75bf421ed8ea22a8d872bb185216c00e79819bd39f7c`.

- Emulator acceptance is recorded in `src/azhub/research/community-014-acceptance.json`.
- SSH copy/verification is recorded in `src/azhub/research/az-014-usb-install.json`.
- After installation the user reported that it worked perfectly. This is hardware user acceptance,
  not a new instrumented hardware run; we did not independently inspect the process after their reload.
- Earlier JSON fields saying "awaiting reload" describe the time of the copy, not the latest user report.

## Architecture and fixes

Each source owns MusicLibraryServer, PersistentStorageServer, SQL factory cache and tagged-track
cache. Requests use private codes e0..e7. Two internal member identities3/4 have four media slots
apiece (3,7,8,2); total8 hub sources. These identities stay inside the AZ with loopback transport;
physical Pro DJ Link packets are filtered. The persisted Pro DJ Link mode can remain off.

The kernel module reversibly raises the hub-depth check for exactly the known firmware kernel.
Automatic discovery finds mounted hub libraries; a worker mounts devices beyond stock sda..sdf
rules and releases missing mounts after source handles and aliases clear.

0.1.3 fixed synthetic member labels on real subnets.0.1.4 fixed volume labels/Unicode and waveform
preferences: each source publishes its own validated DEVSETTING.DAT; alternating local-deck
status templates no longer overwrite color/position. Legitimate preference changes still repaint.

## Evidence and limits

Five sources were tested simultaneously in the emulator, with284-track playlists and2547 total
DB tracks. Only17 audio files exist in the images: use confirmed-present Algo mÃ¡s for playback.
0.1.4 tests include separate blue/center vs RGB/bottom preferences,200 alternating status templates
in20s,30 native-state samples during stress, restoration/live repaint and fifth-source load.
Audio output was not captured. A played03:22->01:39 then paused; fifth source loaded deck3.

Eight simultaneous sources, prolonged hardware sessions, removal of a playing source,
same-path replacement without observed absence, and different-content fixture isolation remain
unverified. Slot8 has a blank native icon/caption. Capacity/track-count announcement fields include
stubs. Automatic network identity selection needs an UP IPv4 subnet of /29 or shorter.

Read-only USB sysfs on the real AZ also confirmed the two connected Type-A buses use EHCI480Mb/s;
the presence of another xHCI5000Mb/s controller does not prove the Type-A ports are SuperSpeed.

## Public release 0.1.5

The user authorized public distribution after review. Personal Git attribution and historical
local paths/network references are retained by agreement. Current device/workstation configuration
is in ignored `local/`; screenshots and external firmware/analysis inputs remain excluded.

Release: https://github.com/NachoCasta/nacho-az-mods/releases/tag/hub-libraries-0.1.5
Catalog: `azmods-repo.json`; app repository identifier: `NachoCasta/nacho-az-mods`.
Anonymous catalog, package size/SHA256, icon and public source access were verified after
publication; the official SDK `repo check --download` also passed.
Source tag commit: `36a32fbf0cbd3a1c96a4197bee3d48a92cc50bc4`.
No Mod Index submission or new AZ deployment/test is part of publication.

The release contains the accepted Linaro executable, SHA256
`6f03e2b5c9a36853f538019d7e4c1dc4123566c7be7cfab47da50e739bae7f98`, and the unchanged accepted kernel asset.
The only source portability change is the SDK include path; runtime implementation is unchanged.
The SDK header is text-identical to v1.2.3. CI host behavior tests and SDK-container build/package
validation passed before publication; the container build uses a different compiler and is not
the shipped executable. The release package was validated separately with pinned SDK v1.2.3.

Package: 308564 bytes; SHA256 `c93e6b35b5da0a0d0dffe45e3d9834287010ff41836b590095dea8393a2ba131`.
The official packer regenerated the standard archive from current source assets, including
correct license-file line endings. This packaging change does not alter the executable or kernel.
0.1.5 has not been installed or additionally tested on the real AZ.

The validator warns about /home/root/settings: the mod READS the optional legacy azhub.conf
override there; it does not write that configuration file. This existing reference remains
unchanged to preserve the tested implementation. SDK-container CI uses another compiler; its
output hash/compatibility must not be confused with the bit-identical Linaro acceptance above.

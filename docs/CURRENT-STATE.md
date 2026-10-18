# Current state â€” 2026-10-18

## Published release: USB Hub Support 0.2.0

Release: https://github.com/NachoCasta/nacho-az-mods/releases/tag/usb-hub-support-0.2.0
Source/tag commit: `1c75601c527fbdd821b5872a334f345aecb60396`.
Source CI: https://github.com/NachoCasta/nacho-az-mods/actions/runs/39158328964
Both behavior-tests and sdk-build passed. All 38 accepted source blobs match the
release commit. Package SHA256:
`1c24d4ea3e73dbfc7650e4ac9c47ee310a10ee883c1efbea41455458e6f41f09`
(350722 bytes); ELF SHA256:
`09b8ffa89b73ff9c5c402d81955bd6c567c2b520edb5c3f38fd00adcf865bd5b`.
The published package is byte-identical to the locally accepted and hardware-tested
candidate. Two-stick hardware observations are recorded below. Loaded-track
source eject remains unsupported; its refusal-banner visibility issue is disclosed.
The user explicitly chose publication of this progress and deferred loaded-track
support. Earlier draft/no-publication statements below preserve historical state.

Next development: research native cached playback and safe eject with loaded
tracks locally first; real-AZ changes/tests still require action-specific approval.


## 0.2.0 publication decision

The user explicitly requested committing and publishing the current progress after
successful two-stick hardware trials. The exact accepted package is retained.
Loaded-track ejection remains unsupported; the missing refusal banner is disclosed
in the release notes and deferred with that follow-up. Broader hardware coverage
is not claimed. Earlier no-authorization/pending-hardware statements below are
historical; see the current hardware record and release notes.


## Latest real-AZ activation

The user explicitly authorized installing, approving and activating the selected
09b8ffa8 stability executable. All seven USB package members were verified; the
previous diagnostic and approval file were backed up. Stock apply restarted the
application from PID 94558 to 1320281. A fresh snapshot showed EEEE and exact
firmware tag; the settings executable hash and loaded-mod log match the candidate.
User confirmed short-press selector open/BACK and individual safe eject after
unloading the track. Read-only SSH confirmed the hub filesystem and library
alias were unmounted, native USB1 stayed mounted, and the journal recorded state 2.
A loaded-track attempt kept the source available but no rejection message was
observed. User also confirmed whole-hub hold with one stick; a second SSH observation
confirmed hub backing/aliases absent, native USB1 mounted and journal state 2.
Two-stick hardware trial then passed: user confirmed browsing/loading CAMI
after individually ejecting NACHO PLUGS. After reconnecting both, whole-hub hold
removed both sources. SSH confirmed both hub backings/aliases absent, both
journal entries state 2 and native USB1 still mounted (same PID 1320281).
Loaded-track refusal-message visibility and broader acceptance remain open. See
[functional observations](acceptance/usb-hub-support-0.2.0-az-stability-functional.json) and
[activation record](acceptance/usb-hub-support-0.2.0-az-stability-activation.json).
The historical development handoff below predates this activation.

## Latest development status

Final local evidence: [stability summary](acceptance/usb-hub-support-0.2.0-stability-summary.json).
A/B emulator hotplug bypassed CLI snapshots; original fixture image byte identity
is unknown. The helper is corrected and independently tested with a disposable image.
No real AZ operation was performed during this autonomous continuation.

Active 2026-10-18 stability work follows `SAFE-EJECT-PUBLICATION-AUDIT.md`.
The last authorized real-AZ deployment was a separate diagnostic ELF:
`ae005285d2d131f1e2dfa7c9fa50723cd8aa4540e428758676cd7e8992a372de`.
No functional eject test was performed with it. The user is unavailable and
asked for autonomous local progress, deferring requests for participation or
additional hardware actions. Do not deploy further local changes implicitly.

Local stability preparation is complete for a separately authorized AZ trial. Event-driven panel readiness replaces
an invalid idle heartbeat deadline; completion adapters now use sixteen fixed
slots. Alias mounts use direct syscalls, workers/helper execution/logging use SDK
services, streams request close-on-exec, resources precede claims, and partial
initialization leaves adapters inactive with prepared workers stopped. Private
tagged caches are prepared before publication, without a native-cache fallback
on allocation failure. Production surface text is clipped to its area.

Latest selected local ELF:
`09b8ffa89b73ff9c5c402d81955bd6c567c2b520edb5c3f38fd00adcf865bd5b`.
Validated package: 350722 bytes, SHA256
`1c24d4ea3e73dbfc7650e4ac9c47ee310a10ee883c1efbea41455458e6f41f09`.
The independent SDK build/package also passes without compiler warnings; it is
not assigned this executable's runtime acceptance. See
`acceptance/usb-hub-support-0.2.0-stability-timeout-build.json`.

This revision latches a helper timeout or unknown helper state, inhibits new
fallback launches and refuses eject while completion is uncertain. An empty
`/run/azhub-mount-fault` marker preserves inhibition across application reloads;
the UI asks users to leave the hub connected and restart the AZ. All twelve
portable suites passed plain, ASan/UBSan and TSan again, including argument,
timeout, unknown-state and marker-restoration cases. Production UI passed SDK
10/8/8 checks. Native helper-timeout validation passed, including actual stop, refusal and
persistence across application reload. Selected-binary topology, partition,
reserved-load, failed-load and exFAT checks also passed; normal UI/main/STOP, journal failure and final resource observation also passed.
These tests do not instrument the proprietary firmware adapters or prove the
entire mod leak-free.

The preceding 04d5d9a1 executable passed matching SOURCE/BROWSE capture, eleven
main eject checks, native USB1 STOP, reserved LOAD/failed-load recovery, exFAT,
topology/SDK fallback and two-partition suites. Its helper argument regression
was fixed and verified by actual sdg mounting. The selected timeout revision also passed corresponding normal-path and
final resource checks; see the final summary. Historical
057d45, ab12dcc6 and other candidate records validate only their stated hashes.

A separate rejected-final-claim fixture passed worker shutdown, absent hub
aliases and responsive native USB UI. An older 43d4e45e process completed twelve
reconnect/eject cycles with unchanged endpoint FDs/threads and lower endpoint
RSS. Its following selector run correctly refused ownership after SDK filter
budget disablement; do not count that failed run as acceptance. Production
budget enforcement is unchanged; actual hardware timing is a later gate.

The first hardware short-STOP trial refused to open the selector. Temporary
logging subsequently confirmed STOP capture while panel-filter counters stayed
zero. Vendor GPL driver research shows that panel reads are change-triggered,
so an idle read deadline was invalid. The revised path still requires a future
authorized hardware trial. The separately authorized diagnostic ELF above
remains the last deployed real-AZ artifact; subsequent local fixes were not
installed there. Temporary SDK logging flags were removed after the earlier authorized capture.
The old diagnostic may still use its legacy private log; this is not a claim that
all diagnostic logging is disabled.
See `acceptance/usb-hub-support-0.2.0-az-selector-failure.json` and
`acceptance/usb-hub-support-0.2.0-az-input-diagnostic.json`.

Read-only preflight confirmed core 1.2.3, kernel 4.4.194 and the exact supported
firmware tag. The user identifies display version 2.01 as 2.00 with the AZ-MODS
patch; this does not broaden compatibility. Original 0.1.7 files and approval
backups were preserved during the authorized candidate installations.

Public release 0.1.8 remains unchanged. Hardware acceptance remains the user's
0.1.7 report with two hub sticks and native USB1. Source version 0.2.0 is an
unpublished development candidate, not hardware accepted or cleared for release.
The sections below retain historical research/release chronology.

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

## Previous release: USB Hub Support 0.1.6

The user explicitly authorized renaming the mod and ID after the first publication.
Current name: **USB Hub Support**; ID: `usb-hub-support`; release tag: `usb-hub-support-0.1.6`.
The 0.1.5 records above remain historical, not the current catalog entry.
Legacy `hub-libraries` must be uninstalled before enabling the new ID; no remote device
migration or AZ test was performed. Runtime changes are limited to ID strings used for
core hook/thread ownership, assets and logging. Current source remains in `src/azhub/linkfake`.
The rebuilt executable is no longer byte-identical to 0.1.4 because the ID changed.
Focused tests, direct ELF descriptor/legacy-string checks and SDK package validation passed.
No new emulator playback session or hardware acceptance is claimed for the renamed build.
See [release notes](releases/usb-hub-support-0.1.6.md) for hashes and limitations.

The renamed release is published at https://github.com/NachoCasta/nacho-az-mods/releases/tag/usb-hub-support-0.1.6.
Both CI jobs passed on source commit `6c6e82106de18843b4e242ba49706c61695fe329`. The public catalog lists only
`usb-hub-support`; anonymous catalog, package size/SHA256, icon and source checks passed,
as did the official SDK `repo check --download`. Legacy release assets were not replaced.

## Previous release: USB Hub Support 0.1.7

User-authorized metadata-only patch: the manifest now matches the simplified catalog
summary and description, without internal implementation details or the emulator test count.
Only version, summary and description changed in the manifest. All other package members,
including the executable and kernel asset, were verified byte-identical to 0.1.6.
SDK v1.2.3 package validation passed. At packaging time, no additional runtime test had been performed; subsequent user hardware acceptance is recorded below.
See [release notes](releases/usb-hub-support-0.1.7.md) for hashes and compatibility.

Release published: https://github.com/NachoCasta/nacho-az-mods/releases/tag/usb-hub-support-0.1.7.
Source commit: `20ce50addf25a2f92924ff6d4720e613973d5a22`; both CI jobs passed. Anonymous catalog version,
package size/SHA256, icon and source access passed after publication. No AZ changes were made.

## Subsequent user hardware acceptance: 0.1.7

The user confirmed installing 0.1.7 from our repository through the AZ-MODS app and
successfully browsing playlists, loading tracks and playing music on their XDJ-AZ.
Two USB sticks were connected through the hub and a third through native USB1,
for three simultaneous sticks. This confirms two hub sources alongside a native source;
it does not establish three hub sources or eight simultaneous hardware sources.
This is user-reported manual acceptance, not an agent-instrumented test. No new AZ
connection, deployment or mutation was performed by the agent. Firmware was previously
identified as 2.00 / `ep147-9e9454e357389ece`; it was not rechecked for this report.
Theme and test duration were not reported. No video or screenshot was requested.
Structured record: [hardware acceptance](acceptance/usb-hub-support-0.1.7-hardware.json).

## Original core workspace cleanup

The original Kyle-Hosman core checkout no longer contains `src/azhub`. A local cleanup
commit removes the 30 tracked research files after migration. Its resulting Git tree
matches `origin/main` exactly; no cleanup commit was pushed to Kyle-Hosman's repository.
All 573 files in the original hub workspace were verified by SHA256 after preservation:
331 screenshot duplicates remain in this checkout's ignored research shots directory;
the other original files and tooling are in ignored `local/core-workspace-cleanup/src/azhub`.
The ignored `local/core-workspace-cleanup/inventory.json` records every original file,
size, hash and retained location. This includes excluded raw research and generated tools;
those files remain local and must not be uploaded. The preserved SDK tool is under
`local/core-workspace-cleanup/src/azhub/publishing/sdk-reference/tools/azmod-pack`.
Active mod development and public documentation continue in this repository.

## Current release: USB Hub Support 0.1.8

Metadata-only patch adds a prominent external PRO DJ LINK incompatibility warning to the
manifest and catalog. While enabled, the mod filters physical Link packets; turning the
user's Link setting on does not disable it. Disable the mod and restart the AZ before
using external players over Link. Hub sources are not shared with external players.
The user reports hub use also worked with the Link setting on; external-player coexistence
was not tested or implemented. Automatic disabling on the user setting is not implemented.
All non-manifest package members are byte-identical to 0.1.7; only version and description
changed. SDK package validation passed. No new AZ operation or runtime test was performed.
See [0.1.8 release notes](releases/usb-hub-support-0.1.8.md).

0.1.8 is published. Both CI jobs passed on source commit `b8f4aba3807cb000da72e70d38de278be55e9c5b`.
The official SDK anonymous catalog/download check passed for 0.1.8. The index PR #4
was updated to disclose the external Link incompatibility and metadata-only release.

## Follow-up inspection: 2026-10-17

Official index PR #4 remains open. GitHub returned no comments, reviews or associated
status checks; this does not establish passing index CI. No PR mutation was performed.
Automatic handling still requires independent user-setting detection and lifecycle
research; see [PRO DJ LINK research](PRO-DJ-LINK.md). No runtime code, package, catalog
or hardware state changed during this follow-up.

The user subsequently requested parallel research into rear USB-C hosting, safe
ejection and PRO DJ LINK compatibility. Findings and proposed validation are in
[development options](DEVELOPMENT-OPTIONS.md). Rear-port host capability remains
unestablished; safe ejection and external-player coexistence are not implemented.
This research did not access the real AZ or change runtime/package artifacts.

## Emulator USB STOP interface prototype

The user authorized an emulator prototype for short-press hub selection and
long-press whole-hub requests. This interaction phase is now implemented and
validated locally; see [prototype notes](USB-STOP-UI-PROTOTYPE.md) and
[acceptance evidence](acceptance/usb-stop-ui-prototype-emulator.json).
Touch/knob selection, cancellation, single-action holds, intact native USB1 STOP
behavior and continued native UI response passed. Hub requests do not unmount
anything. The feature is excluded from normal builds and additionally gated to
the emulator. The regular build remains byte-identical to the published executable.
This historical UI phase does not establish safe ejection or hardware acceptance. No real AZ was
accessed during this implementation or validation.

## Emulator safe-eject implementation

The explicit `safe-eject-emulator` target now performs real background ejection:
connection-generation reservation, native load and library admission control,
independent server completion callbacks, alias closure, filesystem flush and
ordinary unmount. Short USB STOP selects one stick; a hold selects the whole hub.
Loaded/paused sources refuse before withdrawal. Failed busy unmounts retain
inhibition and allow retry. Completed states survive EP147 restarts and physical
reconnection restores availability.

See [implementation and limits](SAFE-EJECT-EMULATOR.md) and the exact tested binary
in [emulator acceptance](acceptance/safe-eject-emulator.json). Acceptance includes
one hub stick ejecting while another keeps playing, whole-hub behavior and actual
open-file failure/retry. This supersedes the earlier research/UI-only statements
for this explicitly gated emulator target. The normal executable remains identical
to the published version; no release, catalog change or real-AZ test is included.
Hardware mapping, FUSE support, failure-state metadata and additional race/fault
validation remain production gates. Two-stick emulator acceptance does not imply
eight-source or hardware acceptance.

## Safe-eject release preparation (in progress)

The user requested an eject flow ready for publication and a real-AZ trial.
SOURCE focus and stale failed-load metadata corrections have intermediate
emulator evidence. Stock exFAT-FUSE closure now has plain-volume and Rekordbox
library/bind-alias evidence, including busy failure/retry. Optional diagnostics
are helping validate late native LOAD admission and completion correlation.
The explicit `az-candidate` target enables the candidate for the already supported
AZ kernel as well as the emulator. Default builds now include the same candidate
flow; the public 0.1.8 catalog remains unchanged. A local 0.2.0 staging/package workflow passes SDK validation,
but its bytes must be regenerated after final code acceptance. No candidate is
yet declared ready for hardware or publication. See
[candidate preparation](SAFE-EJECT-RELEASE-CANDIDATE.md) for remaining tests,
package boundaries, and action-specific hardware permission gates.

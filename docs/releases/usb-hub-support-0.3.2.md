# USB Hub Support 0.3.2

Hub sources now stay available when PRO DJ LINK is enabled or disabled.
The mod preserves its owned private members through the native mode reset,
instead of withdrawing sources and replaying announcements afterward. Loaded
local decks1/2 keep their source identities; fresh browsing/loading remains
available after either transition. Native four-local-deck operation Off and
two-local-deck operation On are preserved; disabling decks3/4 On is expected.

## Validation

The owner installed this exact candidate on the physical AZ and reported
"works perfectly" after the requested loaded hub-track On/Off trial: continued
playback, no new warning triangle and fresh loading afterward. This is manual
user-reported acceptance, without agent audio capture or supplied cycle/source
counts. SSH verified matching USB/staged executable hashes, exact approval and
normal framework activation with59 hooks.

Exact-artifact emulator checks cover empty, paused and playing transitions,
repeated native-plus-hub cycles, both private members7/8, five hub sources,
fresh post-transition loads, four/two local decks and seven independent library
DB descriptors. Isolated software-peer checks cover selective external-member
removal, master/SYNC70BPM, loss and same-player reconnect72BPM. Individual loaded
eject and whole-hub eject with five live sources pass;2165 complete outgoing
status messages pass known-field privacy checks in the latter scope.

All31 portable checks pass plain, ASan/UBSan and TSan; SDK UI passes10/8/8.
The actual C adapter releases removed native handles through normal and throwing
C++ callbacks; a negative build without exception cleanup fails as expected.
Native firmware code is outside sanitizer instrumentation. See the
[readiness audit](../acceptance/usb-hub-support-link-continuity-readiness-audit.json)
and [physical delivery/acceptance](../acceptance/usb-hub-support-link-continuity-az-delivery.json).

## Compatibility and limits

Firmware2.00, exact `ep147-9e9454e357389ece`, kernel4.4.194 and Community Mods
core>=1.2.3 only. Displayed2.01 on the tested unit is AZ-MODS-patched2.00;
compatibility has not broadened. The checked GPL kernel module is unchanged.
Hub sources retain independent servers/caches and remain local; they are not
shared with external CDJs. Native USB library slots are not borrowed.

Actual-CDJ interoperability, audible phase alignment, eight-source hardware,
long-session input timing and cache capacity remain unvalidated. The existing
core cumulative input-filter timing guard can refuse selector input; a normal
reload restored it in an emulator trial. One same-process post-eject/replug
capture showed four rows despite seven mounts/DBs; normal framework reload
restored all five. These remain separate observations, not mode-change failures.
Intentional safe eject may show native source-warning triangles while cached
tracks continue; wait for the final safe-to-remove confirmation.

AZ-MODS stages files persistently internally; removing USB does not unload a
running mod. This release preserves the exact physically tested package and
its original candidate manifest wording. Catalog text and these notes describe
subsequent acceptance. No package member changed after testing.

Package SHA256: `d461a7791ea0977eda3daf776933567197f19461b6abc73e4de370646b9caa81`.
Executable SHA256: `a3d6c7ba438494008edf66fdda43d4be5ae702ed3afa4cdcb57791b9964d1add`.

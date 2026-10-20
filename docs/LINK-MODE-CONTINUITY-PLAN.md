# Hub source continuity across PRO DJ LINK mode changes

## Goal

Keep hub sources available when the user enables or disables PRO DJ LINK.
Preserve source identities, labels, independent servers and caches, and actual
track access. Loaded decks 1/2 must retain playback without newly induced
source-unavailable warnings. Native disabling of decks 3/4 in Link mode remains
expected. Network sharing of hub libraries and broader firmware support are
outside this work.

## Baseline and investigation

The published 0.3.1 adapter forwards native mode transitions, suspends its
internal library-cache override until deferred completion, then invalidates
announcement state for every present, non-reserved hub source. The injector
waits two seconds and replays announcements serially. This restores sources
after withdrawal; it does not establish continuity.

Locate the native withdrawal boundary before changing this recovery path.
Distinguish member removal, media removal, transport changes, alias mounts and
owned library-server lifetime. Preserve external-player mode transitions rather
than suppressing the entire native operation. The internal cache byte is not
the user's persisted setting.

## Evidence gates

1. Reproduce both directions on the exact published baseline in the emulator.
   Record native PID, executable hash, mode, mounts, source visibility and
   withdrawal/reannouncement timing. Separate observations from hypotheses.
2. Trace the native lifecycle and implement a narrowly scoped continuity path
   for owned sources. Do not treat hidden warnings or faster replay as success.
3. Validate empty, paused and playing decks, simultaneous native and hub tracks,
   repeated transitions, fresh browsing/loading and a software Link peer.
4. Recheck independent caches/servers, native four-deck/two-deck behavior,
   individual eject and whole-hub eject. Ejected sources must stay withdrawn.
5. Build and document an exact candidate with hashes, test receipts and explicit
   remaining physical/audio/actual-CDJ limits, ready for a separate AZ trial.

If native constraints prevent continuity, document concrete evidence and obtain
agreement before accepting a reduced result. Builds do not deploy. Publication
requires a later user decision.

## Initial checkpoint

Read-only emulator inspection found PID 733085, empty decks (`EEEE`) and a
completed framework apply (serial 21). The mod log retains acknowledgement of
player 7 slots 3/7. These observations establish a live starting point, not a
new transition trial or continuity acceptance. No physical AZ operation was
performed.

## Baseline reproduction and static leads

The running mapped executable hashes to
`60be7a089fe6cc71dcb404cfd80b96e2218da5f7a2ec63d36b8b70741a8bf462`,
matching published 0.3.1. With decks empty, native UI Off-to-On and On-to-Off
changes both remove the two hub rows from Source while native USB1/2 remain.
Later screenshots show recovery after announcement replay. Native PID remains
733085, and completion epochs advance to 5 (On) and 6 (Off). This is a visual
reproduction, not a precise disconnect-duration measurement or playback trial.
Screenshots remain ignored under `local/diagnostics/link-mode-continuity/`.

Read-only exact-firmware analysis identifies these boundaries for instrumentation:

- Network mode setter `0x013fa7a8` has an Off path that enumerates members,
  calls `0x013e0448` for each selected member and notifies another interface at
  virtual offset `0x60`. Error strings identify `0x013e0448` as
  `prodjlink::LinkDeviceManager::disconnect(LinkMemberNo const&)`.
- LinkDeviceManager mode submission `0x013de6e8` queues a task with vtable
  `0x02bbf550`; executor `0x013e49b0` rebuilds local member descriptors through
  `0x013e4498` and reapplies saved addressing through `0x013dec28`.
- These are static leads, not proof of the On-direction withdrawal mechanism.
  A diagnostic must correlate member/slot and call origin with actual removal
  before deciding which native operations can preserve owned hub identities.

Do not globally bypass disconnect: actual peer removal and safe eject must
remain effective. Do not bypass native mode submission: local deck topology and
external transport still need their native transitions.

## Forwarding-only runtime traces

Two isolated diagnostics were built with the pinned GCC 7.5 SDK and activated
through the emulator's normal mods framework with empty decks. Production code
and the published package remain unchanged. The second diagnostic adds four
forwarding hooks (61 total), including the native member-table clear at
`0x013e18d8`. Its ELF SHA256 is
`714db1e2279d1d2cd971a56e698c7ae88ae71d046986f09401a38a434d2a43c4`;
package SHA256 is
`9fa47cae67b34bc2198c7721e66de856219f7d8fabfd713028fc8530c0b08527`.
PID 804799 completes framework apply serial 23 with that exact staged ELF.

On transition in this process:

- At monotonic time 33446.507, the native member-mode task requests On.
- At 33447.009, the network state machine enters state 0.
- At 33447.010, member-table clear runs from caller `0x013f0770`; its snapshot
  contains the active private member 7. It forwards normally and ends at
  33447.012. Native completion and the existing announcement replay follow.

Off transition:

- At 33506.725, native disconnect receives member 7 from `0x013fafec`.
- At 33516.726, the network enters state 0 and clears an already empty member
  table. This agrees with the disconnect path marking a member for expiry and
  the network mode setter choosing a 10000 ms delay with connected peers.

Read-only listener inspection and static tracing connect state-0 notification
through `0x013ef6c8` to `NetworkManager::onLinkInitialized` at `0x01405da8`,
then `0x013f0658` to the table clear. The clear copies native 0x50-byte records,
empties the protected table, and notifies listeners at virtual offset `0x28`.
It affects actual member lifetime, beyond the visual source list.

Reconstruct the forwarding diagnostic with
`python src/azhub/research/scripts/prepare-link-mode-continuity-trace.py --source PUBLISHED_0_3_1_SOURCE --out LOCAL_DIRECTORY`.
It only prepares source; it never builds, deploys or activates. Reconstruction
matches the tested diagnostic header byte for byte. Raw logs and listener
observations remain ignored in `local/diagnostics/link-mode-continuity/`.
The generator refuses the integrated adapter, which already claims two of the
diagnostic hook addresses; select the historical source explicitly.

A separate private-only preservation probe is now being evaluated. It is a
hypothesis experiment, not a candidate: it cannot establish coexistence with
external peers or safe handling of mixed member tables. The full goal retains
those requirements.

## Private-only hypothesis result: incomplete, not accepted

Probe v3 retains the whole table only when every current member is an active
owned hub member during the native initialization callback, suppresses the
mode-specific disconnect for that member, and avoids replay after a retained
clear. It is isolated under ignored local integration files. ELF SHA256:
`37aa14abf244deeca66e6ddd11f94ef334a8764aec12ffcc60ae4ee500e2e4b1`;
package SHA256:
`faf2c98048adf2a079d4d4db6c5199d24e884c9cfc0be2d680ebf7a85ef4430a`.
Normal emulator activation completes as PID 813211, serial 24, 61 hooks.

Off-to-On with empty decks retains member 7 at 33709.509 and skips announcement
replay. The subsequent Source screenshot contains both hub rows. Fresh hub
browsing and deck1 load of the selected fixture complete, with a populated
waveform and native load-completed result 0. This supports the preservation
hypothesis, but sampled screenshots do not prove absence of every transient.

The reverse trial loads the same fixture from native USB1 on deck2 and leaves
both decks paused. Mode-specific disconnect for private member 7 is suppressed
at 33818.161, but the clear at 33828.162 sees an empty table anyway. The existing
recovery path runs; the settled Waveform screenshot has a source warning on
hub deck1 while native deck2 has none. Both source rows later reappear. Thus v3
fails the objective and must not be promoted as the fix.

Next investigate the native expiry/removal path during the ten-second Off
interval and the suspended internal library-cache override. Do not assume the
explicit disconnect hook covers all removal paths. Cache changes must retain
native network transitions and external peer behavior. The emulator currently
has v3 loaded, Link Off, with deck1 hub and deck2 native tracks loaded (`LLEE`);
unload through native UI before any further framework apply.

## Cache-lifetime probe v4: positive private-only result

V4 additionally restores the internal library gate immediately after forwarding
the native network setter, retaining the shadow of the user's native request.
The ten-second native Off timer still runs. This prevents the private member
from disappearing before the later table clear in the observed private-only
trials. Exact hashes, event times and local evidence hashes are recorded in
[the v4 probe record](acceptance/usb-hub-support-link-continuity-cache-probe-v4.json).

Empty Off/On/Off completes with member7 retained and no announcement replay.
Fresh hub browsing/loading works after those changes. A subsequent trial loads
hub USB1 on deck1 and native USB1 on deck2, starts both with native PLAY controls,
and changes On then Off. Positions/waveforms advance and sampled screenshots
show neither deck receiving a warning. A fresh hub USB2 selection/load after
Off completes with source1 admission and native completion result0. This is
visual/telemetry feasibility evidence, not captured audio or proof of visibility
at every instant. It does not finish the goal.

Read-only observation of the retained record places its private IPv4 bytes at
record +8 (`127.77.0.7`), separately from the number at +1. This gives a stronger
retention discriminator than number alone for the next adapter. Exact-firmware
record size is 0x50; native vector copy/assignment helper `0x013e5810` takes
destination in x0 and source in x1, not an indirect x8 result. Strings at +0x30
and +0x40 have pointer-sized reference-counted storage; native copy helper
`0x0247c9d0` increments the reference count and destructor `0x02469aa0` releases
it. Any selective clear must preserve ownership and callback lifetime; do not
shallow-copy records and then destroy both copies.

Next implement and test a selective clear for mixed private/external member
tables. Retain only present owned private addresses, preserve native callbacks
for removed external records, and keep callbacks outside the native table lock.
Cover physical source disappearance, ejection, fallback, callback cleanup and
concurrency before promoting the prototype. Native deck topology, software peer,
safe eject and the full final artifact matrix still require validation.

Current emulator state supersedes the v3 checkpoint: PID825122 runs v4,
framework serial25, Link Off, two loaded hub tracks (`LLEE`). Production mod
source and published packages are unchanged; no physical AZ operation occurred.

## Integrated candidate v5: local verification complete, runtime matrix open

The tracked 0.3.2 candidate now includes the cache-lifetime change and selective
retention in `link-member-continuity.h` / `link-member-retention.h`.
It snapshots active source membership before taking the native table lock,
matches private member number and address, moves retained/removed records into
separate live ranges, updates the protected native end pointer and dispatches
only removed records through native listeners after unlocking. Normal peer
timeouts and unrecognized call sites forward unchanged. Retained members keep
their current announcement state; fallback replay affects only other members.

Native reference-counted string handles transfer ownership with each record;
the discarded table tail is dead storage. Removed-record cleanup runs on normal
return and C++ listener unwind. The Makefile enables `-fexceptions` for this C
frame. A deliberate build without that flag fails the cleanup test as expected.
The real adapter test covers 100 normal/throwing cycles and verifies callbacks
run outside native/source/mode locks. Partition tests cover both member numbers,
private-address mismatch, stable order, all sizes through capacity plus refusal,
inactive masks, overlapping buffers and exactly-once release of both handles.

All31 checks pass in each portable mode (plain, ASan/UBSan, TSan), SDK UI10/8/8
passes, installer controls pass and the pinned SDK builds/packages the artifact.
See [the v5 build record](acceptance/usb-hub-support-link-continuity-integrated-v5-build.json).
The package is local only: SHA256
`d461a7791ea0977eda3daf776933567197f19461b6abc73e4de370646b9caa81`;
ELF SHA256
`a3d6c7ba438494008edf66fdda43d4be5ae702ed3afa4cdcb57791b9964d1add`.

Normal emulator activation completes as PID854179, framework serial26, with59
hooks. The mapped module path and staged hash agree with the reviewed candidate.
Empty On/Off transitions retain member7 (`mask=0x80`, `replay=0`) and screenshots
after each transition retain both hub rows and both native USB rows. This is a
first exact-integrated-artifact check, not the full runtime acceptance matrix.

Current emulator: v5, Link Off, `EEEE`. Remaining work includes a mixed software
peer, exact-artifact loaded/paused/playing and repeated transitions, member8,
native topology, per-source cache/server observations, safe eject and the final
physical trial preparation. The previous private-only probe's loaded results
are feasibility evidence and do not substitute for this artifact's tests.

The prior isolated peer namespace has gone away; QEMU's localhost mirror and
injection channels15101/15102 remain disconnected, and no NAT redirect guard is
installed. Recreate only the isolated azhub-link namespace for subsequent peer
tests. The existing Java peer classes and WSL Java/TAP device are available.
Published0.3.1 and the real AZ remain unchanged.

## Exact integrated runtime: mixed peer, playing and paused cycles

The bounded isolated Java status-peer scope passes with the same v5 ELF and
PID854179. On epoch4, the selective clear retains member7 and removes one
external member, exercising the mixed-table callback and native string lifetime
path. The peer receives native1/2 statuses after On and loses native3/4. Initial
Off operation also sees native3/4 and generates player3 defense messages; that
conflict is not a valid steady four-local-deck/external-player configuration.
Fresh hub USB1 loading succeeds, and the peer receives68 nonempty-track statuses
from deck1 plus four beat events. A known-field privacy audit passes1500 complete
1152-byte outgoing statuses. This is UDP discovery/status/beat evidence, not
master/SYNC, TCP library querying, phase synchronization or actual-CDJ acceptance.
Transport, peer and observer exit0; the owned namespace and QMP NAT guard/channel
are removed, and SSH remains usable. See
[the peer record](acceptance/usb-hub-support-link-continuity-integrated-v5-peer.json).

Two subsequent loaded On/Off cycles use hub USB1 on deck1 and native USB1 on
deck2. Native CUE/PLAY restarts both tracks before the bounded sequence; reviewed
samples show both remaining-time counters advancing through all four transitions
without reaching the end or displaying a warning. Member7 remains retained with
`replay=0` throughout epochs8..11. See
[the playing record](acceptance/usb-hub-support-link-continuity-integrated-v5-playing.json).

A paused On/Off cycle preserves both positions at04:25.514 with no warnings,
epochs12/13 retained and no replay. Four distinct `exportLibrary.db` descriptors
(native USB1/2 and hub USB1/2) keep the same numbers and targets across the cycle.
This confirms distinct live library files, not a complete runtime SQL/tag cache
content audit. See
[the paused record](acceptance/usb-hub-support-link-continuity-integrated-v5-paused.json).
All audio statements remain visual/telemetry observations; no PCM was captured.

The topology trial loads hub USB2 successfully on deck3 while Off. On unloads
that deck and shows native LINK PLAYER on3/4 while hub/native1/2 remain loaded.
Two LOAD4 injections did not produce a deck4 admission; only `LLLE` was observed
before On. Do not claim complete four-deck loading yet. Retry with a fresh
browser selection and verify actual input delivery before classifying a mod
regression. An earlier slow manual loaded cycle reached the hub track's natural
end; use the later bounded playing record instead for repeated-play evidence.

## Eject gate: input filter disabled after the long session

Short USB2 STOP now refuses the selector with its input-unavailable banner.
No eject request was submitted. The core logs eight calls above its200us frame
filter budget and switches the filter off for this native-process session;
raw/edit serial health then fails closed. The same warning exists in preserved
pre-integration logs. Raw/edit callback source is unchanged by continuity work
and uses short field/atomic operations. Inspected core code measures the virtual
counter interval and accumulates strikes until restart; the exact scheduling
cause of these overruns is not established.

Preserve this safety guard. Individual/whole-hub eject acceptance remains open.
Next unload both tracks through native UI, perform one normal emulator framework
apply of the same exact candidate, verify fresh input health, and run eject
before another long stress sequence. Do not modify core settings or memory.
See [the input-gate record](acceptance/usb-hub-support-link-continuity-integrated-v5-input-gate.json).

Current state: PID854179, v5, Link Off, `LLEE`, both tracks paused at their cue
points; input-filter health is failed for this session. No peer/transport/scope
helper remains live. Remaining gates include fresh-session eject, member8,
complete four-local-deck delivery, peer master/SYNC/reconnect, source lifetime
checks and final physical trial preparation. The goal remains active.


### Fresh-session follow-up: both private members and eject

The exact v5 ELF remains unchanged. Fresh normal framework apply serial27,
PID891874 restores selector input under the existing core timing guard. Individual
loaded-source eject succeeds, followed by long-STOP eject of the remaining source;
native roots survive and cached track positions advance. These checks do not
establish a multi-live-source whole-hub batch or long-session timing acceptance.

Five virtual hub devices are reconnected. Legacy C/E QEMU backend handles require
new child COWs of verified read-only fixture parents; native devices are untouched.
Five rows, seven library descriptors, fresh post-transition loads and four loaded
local decks in Link Off are observed. A separate bounded playing cycle places
member7 and member8 tracks on decks1/2: epochs6/7 retain mask0x180 with replay0,
seven DB descriptors survive, and sampled source labels have no warning icons.
The acceptance records contain exact screenshot/sample hashes. No audio capture
or physical AZ acceptance is inferred.

Next bounded scope is prepared locally in
`local/diagnostics/link-mode-continuity/run-peer-master-eject-scope.py` but has not
been launched. It uses isolated emulator networking, a 70BPM software-master
handoff, peer loss and same-player reconnect at72BPM, with bounded cleanup.
Native SYNC and five-live-source long-STOP eject still require runtime evidence.
Final artifact/source audit and physical-trial instructions remain open.


### Bounded external-master and multi-source eject result

The prepared isolated scope completes on retry using localhost15125; the first
attempt cannot bind15120 and cleans its namespace before any peer runs. Peer3
negotiates master from native1 at70BPM, and both private-member tracks follow with
SYNC. Long STOP2 removes all five live sources (mask0x1f); only native USB mounts
remain. Cached tracks continue sending synced status. After loss and a12-second
gap, peer3 returns at72BPM and negotiates master from native2; both decks follow.
All processes exit0, namespace cleanup completes and emulator SSH stays healthy.
The exact acceptance record hashes full wire captures, logs and reviewed images.
Known-field privacy passes for both decks. Intentional eject warning triangles
are expected and are not evidence of mode-transition failure. Audio/phase/actual
CDJ acceptance remains separate. Final source/artifact/evidence audit and fixture
restoration are still pending; no physical AZ or publication action is taken.


### Final audit and restoration observation

The94 frozen source files match the current checkout byte-for-byte;79 referenced
local evidence files match their recorded hashes. Package manifest and ELF match
the exact reviewed candidate. The readiness audit maps each goal requirement to
its authoritative record and separates physical audio/phase validation.

After five-source eject and virtual reconnect, one capture shows four source rows
although all seven partitions and library DBs are present. Native cached tracks
are then explicitly unloaded (EEEE) before a normal framework apply. This is an
observed same-process post-eject/replug limitation, not a successful five-row
restoration or an unsolicited mode-transition withdrawal. Keep this distinction
in the final candidate limits and record the framework restoration result.


### Readiness decision

Normal framework apply serial28 finishes with exact v5 in PID967182. Decks are
EEEE, Link is Off, seven library DB descriptors are open and the settled Source
capture shows all five hub sources. The previous four-row replug observation
remains recorded rather than erased by this recovery. Namespace and temporary
QMP guard/channel cleanup are verified. No auxiliary peer remains running.

Local candidate-readiness gates are complete; consult the requirement map in
`acceptance/usb-hub-support-link-continuity-readiness-audit.json`. Exact package
and physical trial steps are in `LINK-MODE-CONTINUITY-AZ-TRIAL.md`. This decision
is not physical audio/phase/actual-CDJ acceptance or permission to publish. No
physical AZ operation, push, catalog change, release or tag change is performed.

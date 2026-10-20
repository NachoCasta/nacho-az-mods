# Current state â€” 2026-10-18

## Release preparation: USB Hub Support0.3.0

The user authorized publication of the exact tested package. Pinned SDK packaging
reproduces package SHA256
`38d70598ba8103d970f1b87a651c1e9268affbf4784e221bfb50ae87b0802eee`
and ELF SHA256
`5e8ecd3ee0426bec83d5fc0f73d7f7327aa982c89206a67c97d6fc5634be26bc`.
The immutable package retains experimental manifest wording; updated catalog and
release notes describe hardware results and the unresolved loaded-mode disconnect
and warning triangle. Publication/anonymous download results follow CI below.
The chronological preparation statements below preserve their original scope.

## Current authorization clarification

Fresh CAMI load with the external Wi-Fi peer active now has user confirmation.
After selecting native1 MASTER, the repeated player3 handoff succeeds at128BPM;
the user loads/plays another CAMI track on deck2 and reports success. The60-second
window records799 statuses and133 beat callbacks, with both native decks reporting
synced128BPM. Same native1356103 and CAMI remain afterward. Track selection/audio
are user-reported; protocol corroborates status/tempo, not content identity or
audible phase alignment. First attempt refused handoff without an observed native
master and is retained as a setup negative. See
[fresh hub-load evidence](acceptance/usb-hub-support-0.3.0-wifi-fresh-load-hardware.json).

Same-player Wi-Fi software-peer reconnect now passes bounded protocol checks:
after the prior peer exits, the new player3 observes native1 as master, negotiates
ownership and announces128BPM; native1 reports SYNC at128BPM. The60-second window
receives603 status callbacks but0 beat callbacks. CAMI/native1356103 persist; new
deck2 load and uninterrupted audio await user confirmation and are not implied by
loaded/SYNC status. See [reconnect evidence](acceptance/usb-hub-support-0.3.0-wifi-reconnect-hardware.json).

The real Wi-Fi master/SYNC trial now passes bounded controls: software player3
requests master from native1, receives confirmation in52.952ms, then announces
126BPM. The user enables SYNC and reports success/continued CAMI playback; native1
reports synced126BPM. The60-second window receives714 status and128 beat callbacks,
then the peer stops normally. CAMI and native1356103 remain during/after the trial.
This is protocol plus user-reported audio, not captured phase/latency or actual-CDJ
acceptance. Peer-loss/reconnect and the mode-transition warning remain open.
See [Wi-Fi master/SYNC evidence](acceptance/usb-hub-support-0.3.0-wifi-master-sync-hardware.json).

The user refined the physical mode-change result: switching Link in either
direction briefly disconnects/reannounces CAMI and leaves its already-loaded track
with the source-unavailable triangle; the native USB source/track remains connected
in the same comparison. Do not describe the earlier basic confirmations as seamless
loaded-source continuity. This matches the emulator warning limitation but remains
an unresolved regression/UX issue, not an unavoidable accepted behavior. The user
asked to record it and continue testing; Link is now On with CAMI playing.

Physical Wi-Fi peer discovery/status now passes a bounded control after the user
closed rekordbox:20seconds passive observation receives30 discovery-port packets
and67 beat-port packets; explicit physicalLAN Beat Link player3 status trial
receives200 status updates from native players1/2, with player1 master. No master
request or SYNC change was sent. Native1356103 and CAMI remain after cleanup.
Beat callbacks were0; no audio/playback/sync acceptance follows. IPv4 Java stack
and host harness guard correction were needed; supported mod bytes are unchanged.
See [physical Wi-Fi status evidence](acceptance/usb-hub-support-0.3.0-wifi-peer-status-hardware.json).

The user authorized exploratory physical peer testing through the existing Wi-Fi
network based on prior successful use. Read-only preflight finds the AZ on Wi-Fi
only and LAN carrier0. The new receive-only LAN observer could not bind: Windows
rekordbox PID47564 owns UDP50000/50001/50002. No peer packets were sent, no firewall
or interface changed, and rekordbox was not terminated. Await user closure before
retrying; this refusal does not establish a Wi-Fi or mod failure.

The user now confirms manual hub browsing/loading/playback with native PRO DJ LINK
Off, then confirms continued local hub operation after enabling Link and checking
retained playback/source availability/new load. Record:
[0.3.0 local-mode hardware acceptance](acceptance/usb-hub-support-0.3.0-link-modes-hardware.json).
These are user reports, not instrumented audio or external-player acceptance.
Physical external-player communication and long-session coverage remain pending.

The user explicitly authorized a narrow manual approval-list exception for the
exact tested mod. It is recorded in CLAUDE.md, AGENTS.md and the testing policy.
USB and staged ELF hashes were verified, the prior approval list was backed up
on USB, and only the usb-hub-support entry was replaced under the framework lock
with an atomic rename. Unrelated entries were verified unchanged. With fresh EEEE,
normal framework apply restarted1312330 ->1356103. The real AZ now reports
`mod=usb-hub-support|1|0.3.0|` and successful initialization with54 hooks, exact v5
ELF on kernel4.4.194. This establishes activation only, not browsing/audio/Link
coexistence hardware acceptance. Local records: `az-link-v5-preapprove-result.txt`
and `az-link-v5-after-authorized-approval.txt` under `local/diagnostics/`.

The user subsequently unloaded all decks (fresh EEEE) and authorized reload.
Supported `apply` IPC completed native restart1876957 ->1312330. Framework staging
matches the exact v5 ELF. The core now reports0.3.0, state6,
`New mod found: allow?`; the candidate is not active yet. Await the user's normal
Allow action, followed by supported reload if requested. No approval file edits.
Local snapshot: `local/diagnostics/az-link-v5-awaiting-allow.txt`.

After the user's installation authorization, exact supported v5 was copied to
the real mods USB at `/media/usb/sda1/MODS/community-mods/usb-hub-support`.
All seven members passed SHA256 verification; ELF remains `5e8ecd3ee0426bec83d5fc0f73d7f7327aa982c89206a67c97d6fc5634be26bc`.
The previous0.2.1 folder is preserved on that USB under
`MODS/backups/usb-hub-support-0.2.1-before-link-v5`. No approval/internal files
were manually edited and no apply/reload was requested. Native PID1876957 still
reports loaded0.2.1 and deck state LELE; activation awaits unloaded decks and
supported framework approval/reload. Copying is not candidate hardware acceptance.
Local receipt: `local/diagnostics/az-link-v5-usb-copy.json`.

The user explicitly removed mandatory remove-USB/full-restart recovery verification.
Use the normal AZ-MODS workflow for the emulator-validated candidate: USB delivery,
supported installation/approval/activation, bounded checks and standard restarts.
Manual internal controller edits remain forbidden. Recovery is unverified, but is
no longer an activation gate. This supersedes recovery-gate statements in the
chronological checkpoints below. See [current policy](AZ-TESTING-POLICY.md).

Read-only physical preflight identified running0.2.1, exact firmware tag and
kernel4.4.194, the expected cached/USB loader hashes and loaded decks1/3. No package
copy, activation, restart or recovery test was performed during that inspection.

## Final local preparation checkpoint — 2026-10-19

The supported v5 candidate and bounded emulator evidence are ready for final user
validation. Published **0.2.1** remains unchanged; no release, tag or catalog update
was performed. Physical recovery and activation remain deferred. This checkpoint
supersedes pending-task statements in the chronological notes below; their original
artifact-specific failures and limitations remain evidence, not current passes.

The [final preparation audit](acceptance/usb-hub-support-link-final-preparation-audit-v5.json)
verifies the package/ELF, all 65 frozen production files in both working source and
build export, and 324 local raw receipts across 17 v5 records. The
[requirement ledger](acceptance/usb-hub-support-link-goal-evidence.json) describes
the bounded matrix and the [final trial](PRO-DJ-LINK-CANDIDATE-TRIAL.md) separates
completed emulator work from physical prerequisites.

The final partial-eject control now passes: loaded source3 eject completes before
Off/On, four remaining rows survive both settled modes without resurrecting the
ejected source, and physical virtual-USB replug restores five rows and seven healthy
partitions. Same native PID860, On/Off/On epochs1/2/3; final decks EEEE. This is not
a simultaneous eject/mode race, forced partial-failure test or audio acceptance.
See [partial-eject evidence](acceptance/usb-hub-support-link-partial-eject-modes-v5.json).

## Current hardware workflow authorization

The user permits emulator-first USB-only candidate delivery, normal mods-framework
activation/checks and standard restarts for USB Hub Support / PRO DJ LINK work,
subject to recovery by removing the mods USB and fully restarting. Manual internal
AZ file/settings/approval/preload edits are forbidden. Framework-managed staging
is persistent internal storage, not RAM; a one-restart/no-mod recovery result has
not yet been established for the deployed loader. Until recovery is verified,
continue local/emulator preparation. See [testing policy](AZ-TESTING-POLICY.md).
Earlier action-specific authorization statements below preserve historical scope.

Read-only copies now establish the emulator's actual firmware-to-cached-loader
chain. The effective cached and USB loaders are byte-identical and do not retry a
refused initializer when the selected populated cache is unchanged. This supports
the startup-order concern but does not prove a cold-boot outcome or the real AZ's
loader behavior. See the [emulator loader audit](acceptance/usb-hub-support-link-recovery-emulator-loader-audit.json).


## Active coexistence work: experimental candidate, runtime validation pending

The published release remains **0.2.1**, with external PRO DJ LINK blocked and
its existing warning unchanged. Earlier coexistence trials below use opt-in
research overlays. A local **0.3.0** candidate now uses the supported framework
build and existing exact kernel paths; it is not published or hardware-ready.

The latest supported local candidate is under
`local/integration/link-coexistence-v5/`. Package SHA256:
`38d70598ba8103d970f1b87a651c1e9268affbf4784e221bfb50ae87b0802eee`;
ELF SHA256:
`5e8ecd3ee0426bec83d5fc0f73d7f7327aa982c89206a67c97d6fc5634be26bc`.
It retains private media-state ownership and removes a historical USB1-only
mode-replay restriction. The expanded portable mode test covers all eight
implemented slot assignments, absent/reserved exclusions, stale epochs, eject
deferral and 20000 transition/write races. All 22 plain/22 ASan+UBSan components
and SDK UI 10 plain/8 ASan checks pass; hook claims remain the same 54. Runtime
activation and both-direction recovery without duplicate rows now pass with
empty decks in PID859956. Loaded On/Off/On now restores all five rows,
all five sources load successfully, and four local decks advance in Off; On
retains two loaded decks. The final matrix remains incomplete. See
the [all-slot replay build](acceptance/usb-hub-support-0.3.0-all-slot-replay-candidate-build.json).

Exact v5 now also passes [loaded mode transitions](acceptance/usb-hub-support-link-loaded-modes-v5.json)
and [cold On startup](acceptance/usb-hub-support-link-cold-on-v5.json). The cold
fixture maps USB STOP2 to the virtual hub through one offline service environment
override; package bytes and real-hardware default roots are unchanged. See the
[image provenance](acceptance/usb-hub-support-link-emulator-eject-root-image.json).
In first native PID997, individual loaded-source eject and long-press removal of
the remaining four hub sources pass while both tracks remain loaded and their UI
clocks advance. Both native USB mounts survive. The isolated player3 obtains
master and deck2 follows126 BPM; 570 full1152-byte statuses pass the classified
private-source checks. A later On-to-Off change does not resurrect ejected rows.
See [eject/peer evidence](acceptance/usb-hub-support-link-loaded-eject-peer-v5.json).
These are bounded emulator UI/protocol results, not audio or real-CDJ acceptance.
Native yellow source-unavailable warnings remain visible after mode changes and
eject. [Cold Off](acceptance/usb-hub-support-link-cold-off-v5.json) now passes
in first native PID851 without restart, with five private rows and both native
mounts. [Exact-v5 USB-absent recovery](acceptance/usb-hub-support-link-normal-recovery-v5.json)
also passes on the emulator: cached init refuses with-40 in first/settled PID759,
no private workers/aliases start, and native USB1 browses/loads content1944 with
advancing UI playback. The framework remains mapped on that first recovery boot;
its normal loader writes a stock environment for the next boot without restarting
the application. Deterministic stale queued-stage controls, final adversarial transport coverage
and physical4.4 recovery still require final evidence.

A fresh exact-v5 lifecycle boot now also passes documented held-SYNC Instant
Doubles from native USB1 and private hub media, negotiated peer master transfer,
native peer removal and same-player reconnect, and a new hub load while following
128 BPM. A private-to-native content1944 replacement exposes the native standard
and auxiliary identities in the same first native packet, then remains visible
in189 packets while the other private deck stays masked. Earlier predecessor
visibility timing remains historical. See [ID/peer/native controls](acceptance/usb-hub-support-link-id-peer-native-v5.json).

Physical virtual-USB removal/reconnection on the same port/key changes devnum3
to8 and permits fresh normal load and held-SYNC retry. Another349 full statuses
pass classified private identity and selected-marker checks after reconnection.
See [replug/retry controls](acceptance/usb-hub-support-link-replug-retry-v5.json).
The old track had reached its end before unplug and was restarted from cache
while absent; this is not uninterrupted playback across removal. Manual stale
LOAD/Instant Doubles attempts did not establish queued provenance or execution,
so deterministic queued-stage cancellation remains an open final validation gate.

An offline [controlled request-delay variant](acceptance/usb-hub-support-link-request-delay-v5-build.json)
is now built for the remaining queued-stage test. The reconstruction verifies
all65 frozen production inputs and changes only `link-request-origin.h`: a
USB-resident one-shot marker delays a captured private common-stage request for8s
on emulator kernel6.6 before the unchanged stale check. Its package changes only
the ELF; hook claims/parser registration remain identical. The
[controlled runtime](acceptance/usb-hub-support-link-request-delay-v5-runtime.json)
now passes normal type1 pending-source removal and documented held-SYNC type5
replacement at this common-stage boundary. The normal request captures
generation23907/devnum8; targetdeck2 remains unloaded after removal and a later
fresh load succeeds. The ID request captures generation24331/devnum9; same-port
remove/add completes in0.118s during the pause, old target position remains
unchanged, and fresh normal+ID retry succeeds after devnum10. All seven partitions
remain healthy. These are derived diagnostic results; they do not establish all
native async task/destructor/final-admission interleavings on the supported ELF.
Tracks were unloaded through native UI, then the supported-v5 package was restored
through USB copying and normal framework Allow/Reload. Exact supported ELF5e8ecd
is active in native PID69363,54 hooks,EEEE, with five private rows and both native
USB rows restored. See [restoration evidence](acceptance/usb-hub-support-link-restored-after-delay-v5.json). The real AZ and published release remain untouched.

Exact supported v5 now passes [native USB mode controls](acceptance/usb-hub-support-link-native-usb-modes-v5.json):
USB1/USB2 browse and load with Link Off, both tracks continue through native On,
and explicit native track unload settles EEEE in each mode. Native screenshots
show no source warning triangles in this control, unlike carried private tracks.
All five private sources and both native backings remain available.

[Bounded adversarial controls](acceptance/usb-hub-support-link-adversarial-native-v5.json)
reject six external member7/8 UDP claims and repeated reserved-requester5/6 TCP
sessions, while requester3 receives a native response. Former private test codes
0xe0/e7 receive native errors rather than hub data. Private7/3 and8/7 still load
following these trials;265 complete private statuses pass the classified checks.
Actual fd-number reuse remains host-test scope, not a claim about these TCP runs.

The previous primary-interface mirror transport was UDP-capable but failed TCP:
slirp injected resets when it saw the mirrored guest's SYN-ACK. A queue-all capture
identifies the reset's NAT MAC. The new tracked
`link-primary-tap-transport.py` separates NAT management from the TAP-owned peer
address; six packet/namespace tests and a60-second canonical run pass, including
SSH during and after cleanup. See [transport evidence](acceptance/usb-hub-support-link-primary-tcp-transport-v5.json)
and the [peer guide](../src/azhub/research/link-peer/README.md).
No guest/core/mod changes were required for this transport correction.

The [native metadata baseline](acceptance/usb-hub-support-link-native-visibility-baseline-v5.json)
now reproduces the initial50 zero standard-source packets per deck with the core
active and USB Hub Support refused at init-40. Native auxiliary locations remain
valid in all50 prefix packets in both runs, then standard native tuples remain
visible. Candidate cadence spans~10s for the prefix versus baseline~6.7s; packet
count equivalence does not prove equal latency or the native internal cause.
The baseline's explicit TCPquery occurs~18.7s after first native identity, ruling
out that query as its trigger. No immediate metadata-availability guarantee is
made. The earlier no-core attempt did not load tracks and is retained solely as
a non-comparable preparation. Physical4.4 recovery remains deferred to user presence.

The supported candidate was restored again through a fresh positive cold boot
following the baseline: first/settled native PID860,54 hooks,NRestarts0,Link On,
EEEE, five private/two native rows and seven healthy mounts. See
[final cold restoration](acceptance/usb-hub-support-link-final-cold-restoration-v5.json).
No production source or package bytes changed during these controls. The final
requirement audit remains open; the real AZ has not been accessed.

The preserved media-state predecessor is under
`local/integration/link-coexistence-v4/`. Package SHA256:
`2edbd629de26a76521391b3c9a3f807c29491e393673099e803f407018f6d780`;
ELF SHA256:
`efff2ac033d7362846b11a9384ac75700a2a01591861585b0fc26e48a81a4547`.
It incorporates private media-state ownership after the controlled positive
experiment below. All22 plain and22 ASan/UBSan portable components, SDK UI10
plain/8 ASan checks, unchanged supported hook registrations and strict seven-
member package verification pass. Its bounded functional run is recorded below;
recovery and the full functional matrix are incomplete. See the
[media-state candidate build](acceptance/usb-hub-support-0.3.0-media-state-candidate-build.json).

The preserved parser-corrected predecessor is under
`local/integration/link-coexistence-v3/`. Package SHA256:
`aeb199c064c5ad043e4b8933b7e631119ab5f187e58921b5fcdb3b46c6471031`;
ELF SHA256:
`123703cd5028a3840ef61d13eb640ef7d1ab7c05a2e4741daac622e2a10ae19e`.
It fixes the parser registration omission described below. The pinned SDK build,
22 plain and 22 ASan/UBSan portable tests, SDK UI checks and the new supported-
preprocessor registration audit pass. TSan was not rerun for this correction.
Coordinator review confirms all seven package members and all 65 production/
exported source hashes. Bounded integration and normal-loader recovery have now
been repeated on these bytes; the full functional matrix remains pending. See the
[corrected candidate build record](acceptance/usb-hub-support-0.3.0-parser-candidate-build.json).

The worktree now has private media-state corrections in `link-status.h`,
`linkfake.c`, `test-link-status.c` and subsequent all-slot replay changes in
`link-mode-recovery.h`/`test-link-mode-recovery.c`. The preserved v3 export does
not contain these changes. V4 and v5 builds are separately frozen. The v3 results below continue to
identify the previous artifact, not the changed worktree or experimental build.

The corrected artifact now passes bounded normal-framework integration on the
corrected emulator image: PID 10961 loads 54 hooks, including `0xef05c8`, and
private source A browses its 2547-track library, loads a present track and shows
advancing playback. All five hub backings were connected at power-on; the source
screen shows native USB1 plus five private rows without the extra native USB2
entry. Only A was loaded in this step. The isolated primary-network adapter
passes ARP/ICMP and preserves SSH; Link peer acceptance remains separate. See
[v3 integration evidence](acceptance/usb-hub-support-link-normal-activation-v3.json)
and its [independent audit](acceptance/usb-hub-support-link-normal-activation-v3-independent-review.json).
The audit verifies all ten raw evidence hashes and the exact three image changes,
plus the source/browser/playback/unload screenshots. The earlier same-boot network
smoke test is explicitly separate from the final activation's Link behavior.
Exact-hash cold boots now pass and have an independent review of all 13 frozen
records: positive first PID 729 initializes 54 hooks with all five hub sources
present at power-on; negative first PID 465 refuses with -40 before effects and
remains stable without a restart. A later ordinary native USB loads and shows
advancing playback while the candidate stays inactive. See
[v3 recovery](acceptance/usb-hub-support-link-normal-recovery-v3.json) and
[independent recovery audit](acceptance/usb-hub-support-link-normal-recovery-v3-independent-review.json).
This is the 6.6 emulator path only. Physical 4.4 recovery and the complete
mode/peer/eject/privacy/native-replacement matrix remain pending.

The subsequent exact-ELF functional run (native PID 703) exposes an open source
visibility issue. With the native music/mods USB enumerated as USB2, all five private
servers acknowledge announcements, but the source screen lacks player 7/slot 7
and player 8/slot 7. The three visible private sources load tracks across four
decks with advancing UI clocks in Link Off. A later loaded Off-to-On transition
leaves the expected two local decks, but cannot establish source recovery:
the runtime investigator found an OHCI unrecoverable error at guest uptime
726.75 seconds and all five hub devices disconnected around 727.28 seconds,
before the Off packet window at 922.81 seconds and the subsequent transition.
The absence of private rows therefore does not establish a mode-recovery bug.
The source log confirms all five become absent and their aliases are released;
settled mounts contain only the native USB. Repeat the source-transition matrix
with stable backing devices before changing its recovery logic. Frozen local evidence is under
`local/diagnostics/link-normal-loader/matrix-*`. The Off packet audit covers 408
complete outgoing statuses and passes known private-field and track-marker
checks after unexpected controller loss, rather than proving fresh attached-source
privacy; native replacement and other lifecycle windows remain separate controls.
See [Off status audit](acceptance/usb-hub-support-link-matrix-off-known-status-privacy.json).
The [independent controller-loss review](acceptance/usb-hub-support-link-matrix-controller-loss-review.json)
records the frozen evidence hash and guest-uptime ordering.
The [partial functional matrix](acceptance/usb-hub-support-link-functional-matrix-v3-partial.json)
preserves 71 local evidence files with matching hashes and sizes verified by the
coordinator. It includes the negative source-list result, native restoration and
clean shutdown. A bounded peer handoff/126 BPM follow observation is recorded
[separately](acceptance/usb-hub-support-link-matrix-peer-handoff-review.json);
it occurred after controller loss and does not complete live-hub coexistence.

A normal offline emulator image reconstruction adds the matching xHCI PCI module
and its bounded early boot dependency. The new initrd SHA256 is
`6bf35ea5db8fca4b9ec8d95342d253a33e7fdbc230e2582f3f37a7fab7c725b8`.
The [independent image review](acceptance/usb-hub-support-link-emulator-xhci-image-independent-review.json)
confirms exactly four added entries and no changes to the remaining image,
including firmware, normal loader and USB renotifier. This is an emulator fixture
change only. A [bounded xHCI startup control](acceptance/usb-hub-support-link-xhci-five-no-native-control.json)
now confirms successful module initialization, six mounted disks and five private
rows through 303 seconds in first PID 974, followed by a clean shutdown. Its nine
raw evidence hashes and sizes match. Native music enumerated on `3-1` and had no
native source row, so it does not resolve native USB2 coexistence. A second EHCI
controller is being used to test native `2-1` while xHCI serves the nested hub.
Workload stability and recovery on the new fixture remain pending. The candidate
package and ELF remain unchanged.

The subsequent both-native cold boot now reproduces the slot7 visibility defect
without controller failure: exact candidate PID 762 initializes 54 hooks, all
seven physical USB partitions are mounted, and all five private announcements
are acknowledged. The source screen shows native USB1 and USB2 plus only the
three private slot3/slot8 rows; private player7/slot7 and player8/slot7 are absent.
The fifth nested disk `sdg1` is mounted by the mod's existing fallback helper.
Frozen local inputs are `both-native-init-state.txt` and
`both-native-source-startup.png` under `local/diagnostics/link-normal-loader/`.
This meets the prerequisite for the reviewed diagnostic overlay; it does not
identify the failing predicate or justify changing synthetic status bytes yet.
See the [independent stable-baseline review](acceptance/usb-hub-support-link-slot7-stable-baseline-independent-review.json).
The separate diagnostic ELF `bfe4a141a60bfe60cfdcecf517fdab83d14ca2913ead413e8675e50ad7c9b69a`
now reproduces the missing rows after supported framework activation with 55
hooks. Both missing private MediaIDs reach native mount callback entry, and
their capability calls return true. This narrows the next investigation to the
downstream root/list logic; entry logging does not prove successful completion
or insertion. See [diagnostic runtime evidence](acceptance/usb-hub-support-link-slot7-diagnostic-runtime.json).
These diagnostic bytes are not the supported candidate or final acceptance.

The subsequent 56-hook diagnostic confirms that both missing private slot7 keys
are inserted into the native root tree and source model with mounted state set.
Counts fall before later source additions, and the settled screen lacks those
two rows while all seven backing mounts remain healthy. This changes the next
investigation from initial admission to subsequent removal; the observation does
not identify the removal caller. See the
[closure runtime record](acceptance/usb-hub-support-link-slot7-closure-diagnostic-runtime.json).

A separate 58-hook removal/status diagnostic is now built and packaged locally,
with ASan/UBSan forwarding and consumed-payload/key checks, supported registration
checks and exact seven-member package verification. It observes the native
removal body, exact-key erase and actual outgoing private status fields without
changing packets or suppressing removals. Its package
SHA256 is `27b2903f2b2129b54b4d273ae2a738015a912a1a906b3b7014c59154039305d5`.
The production candidate and previous diagnostics remain unchanged. See the
[removal diagnostic preparation](acceptance/usb-hub-support-link-slot7-removal-diagnostic-build.json).
Normal emulator apply/Allow/Reload now activates those exact bytes in PID776827
with58 hooks. Both missing full keys are removed by native event2, and exact-key
erase reports one removed node for each, called from0x1c1dd90. Seven backing
mounts remain healthy and all decks remain empty. Startup private status records
alternate packet0x45e between0 and4, but their bounded window ends before private
mounts; this is a hypothesis lead, not contemporaneous causal proof. The removal
body's caller0x237d800 is queued dispatch and does not identify the original
consumer. See the [removal runtime record](acceptance/usb-hub-support-link-slot7-removal-diagnostic-runtime.json).

An isolated experiment now generates each private member's USB/SD/USB2/USB3 state
from its own present-source mask. With the same58 observers and unchanged seven
backings, PID792310 retains both native rows and all five private rows, reaches
root count8, and records no native remove/erase calls in its activation segment.
Both formerly missing player7/slot7 and player8/slot7 libraries browse2547 tracks,
load "Algo más" and show advancing playback clocks. Native unload returns EEEE;
all seven mounts remain healthy. This supports the correction but does not prove
full five-source loading, mode transitions, eject or peer coexistence on final
supported bytes. See the [experimental build](acceptance/usb-hub-support-link-slot7-owned-media-experiment-build.json)
and [runtime evidence](acceptance/usb-hub-support-link-slot7-owned-media-experiment-runtime.json).
The same behavior change is now in production source without diagnostic hooks;
supported runtime/regression validation is pending.

The exact v4 supported ELF now passes normal-framework activation in PID820744
with 54 hooks. In Link Off it retains both native USB rows and all five private
rows; all five private sources browse/load, and paired four-deck screenshots
show advancing playback. A live-backing privacy window contains 415 complete
1152-byte statuses with known private fields/selected markers clean. Native
Off-to-On confirmation leaves LLEE and preserves cached playback on 1/2, but only
the two private USB1 rows return. All seven mounts stay healthy. The v4 source
explicitly skips non-slot3 replay based on an older three-fixture experiment;
the resulting missing 7/7, 8/7, 7/8 rows are a recovery failure. Independently of
that failure, isolated peer3 requests native master1 and becomes master, and
deck2 follows 126 BPM for 60 samples; 370 complete On statuses have clean known
private fields. No audio-phase or real-CDJ acceptance is claimed. See the
[v4 partial matrix](acceptance/usb-hub-support-link-functional-matrix-v4-partial.json).
The v5 correction replays every present unreserved source after completion;
both directions must be tested for retained sources and duplicate rows.

Exact v5 normal apply/Allow/Reload initializes 54 hooks in PID859956 under
persisted Link On. Native confirmed On-to-Off-to-On transitions produce epochs
1/2/3, with all five private sources and both native USB rows restored each time.
After the return to On, selecting the first native row and seven encoder steps
reaches final MODMENU; the bottom list view contains the five private rows without
duplicates. All decks stay EEEE and all seven mounts remain healthy. An initial
Utility selection mistake changed Load Lock rather than Link; the native UI
restored Unlock and the actual Link modal/epoch3 are separately preserved. This
record proves empty-deck recovery, not loaded-transition or peer/eject acceptance.
See [v5 activation and empty transitions](acceptance/usb-hub-support-link-normal-activation-v5.json).

The preserved first candidate package SHA256 is
`f6c43ca830bfb2a7935b08720bb211d9955aa95729c3519199f792df615433a7`;
its ELF SHA256 is
`b6462687443b8ae55f9306a3b65988e4d5989bfffad1f2b144bce46571fdf03b`.
**This first packaged candidate is not ready for use:** its normal-framework
functional trial exposes a missing database-parser hook. The only parser claim
is inside the non-community preprocessor branch, so the supported build installs
zero claims at `0xef05c8`; private sources appear but browsing waits without
admission tickets. A corrected source/build check and separately preserved new
artifact are being prepared. Earlier initialization and recovery results below
remain evidence for these original bytes, not for a replacement build.
The missing-hook reproduction is preserved in the
[supported-parser negative record](acceptance/usb-hub-support-link-supported-parser-negative.json).
The source correction now has an independently reviewed, single parser claim in
the actual supported preprocessor output. A new registration check also exercises
missing/duplicate negative controls; rebuilt-artifact runtime acceptance is pending.
SDK build/package validation, all 22 portable components in plain/ASan/TSan
(66 invocations), and the separate SDK UI harness pass. TSan required a scoped
local container seccomp allowance after its first attempt failed before executing
a test. Independent coordinator review confirms all seven package members and
64 exported source hashes match the build record. Normal-loader activation and
bounded emulator cold-boot recovery now pass as detailed below; final functional
regressions remain pending. Research-binary results do not substitute for testing
these exact bytes. See the
[candidate build record](acceptance/usb-hub-support-0.3.0-coexistence-candidate-build.json).
The same 64 source files and final manifest have now been promoted into
`src/azhub/linkfake/` and independently hash-checked against that export. No
generated binary was copied into tracked source, and no commit or push was made.

Normal-framework setup activation now passes on the exact packaged ELF: after
managed removal of legacy `hub-libraries`, supported enable/apply and UI Allow/
Reload, the core reports `loaded 0.3.0 on ep147-9e9454e357389ece (53 hooks)`.
The native process is PID 22891 with HubMount/HubTransport workers and no
`AZHUB_*` environment controls. This is setup activation, not an independent
cold-boot result, functional regression or recovery pass. See the
[activation evidence](acceptance/usb-hub-support-link-normal-activation.json).

Independent positive/negative cold boots from the same populated, enabled cache
now pass on this exact emulator fixture. With the mods USB present, first native
PID 659 initializes 53 hooks and remains stable without a rescue restart. With
no mods USB attached or embedded, first PID 458 refuses initialization with -40
before patching, starts no candidate workers, and remains stable through the
settled capture with zero restarts. A separate later hotplug of an ordinary USB
without mods supports native browsing/loading and advancing playback while the
candidate stays refused. All 12 frozen evidence hashes and the native playback
screenshots were independently reviewed. See the
[recovery record](acceptance/usb-hub-support-link-normal-recovery.json) and
[independent audit](acceptance/usb-hub-support-link-normal-recovery-independent-review.json).
This establishes the 6.6 emulator/application path only: the physical 4.4 kernel
patch and applicable real-loader recovery still require witnessed validation.
Five-hub functional, packet-privacy and native same-ID replacement controls on
the packaged artifact remain pending.

The first five-hub fixture also exposed extra native USB2 announcements. Frozen
scripts and journal evidence attribute those writes to emulator patch37, which
overwrites the real loader's renotifier with a script that maps nested hub paths
to native USB2. Both the pristine firmware script and pinned core exclude those
paths. A workspace-local correction to the normal emulator image build preserves
the real-loader script and omits its conflicting emulator poller; four branch
fixtures pass. The corrected image SHA256 is
`b3c0b20a7a502615d0f45ca7e961099662451812770d0da63c928820e21f7c09`;
the native application, firmware loader and normal loader drop-in are unchanged.
Hub-at-start behavior must still
be retested. See the [renotifier provenance](acceptance/usb-hub-support-link-emulator-renotify-provenance.json).

Emulator evidence covers local hub browsing/loading, five source rows, native
Link mode transitions (four local decks Off; two local decks On), simulated
external-player discovery, master transfer, tempo following and native master
recovery. These results span different diagnostic binaries; they are not a
single integrated candidate acceptance. They do not establish audio-phase sync
or real-CDJ interoperability.

Current implementation priorities are request-origin/cancellation across queued
loads and cached-source privacy during failed or native replacement. Request
origin now survives native asynchronous common-load and allowance tasks in bounded
emulator trials. Reconnecting during either queued closure cancels the stale browser
request without the misleading recording alert; a fresh replacement-source load
succeeds in the same process. A separate final-admission guard now checks the
original physical origin against the complete native media/content key and rejects
missing provenance. Normal loading, a late reconnect rejection and fresh loading
from the replacement pass on one diagnostic binary. A later diagnostic binary
also passes native-API Instant Doubles, queued reconnect cancellation through the
native owner thread, and fresh replacement loading/copying. This is not UI-gesture
or audio-phase acceptance; superseded cleanup and exhausted-registry cases remain
host-modeled. A combined supported integration remains incomplete. See
[Instant Doubles evidence](acceptance/usb-hub-support-link-id-queue-cancel.json),
[browser cancellation evidence](acceptance/usb-hub-support-link-queue-cancel-browser.json)
and [final-admission evidence](acceptance/usb-hub-support-link-queue-final-origin.json).

Full external status capture has exposed another concrete privacy gap: following
abrupt physical disconnect/reconnect, retained hub tracks can report the native
player1/USB3 tuple and content ID. The existing safe-detach ticket classifier does
not cover this path. The fix must preserve committed loaded-source provenance
across physical removal, independently of pending requests and safe-eject tickets.
A repeated content ID at extended packet offset `0x194` is now traced to an
auxiliary TrackLocation row. The native serializer emits ten such rows at
`0x18c + 0x40*i` and another at `0x428`, each containing its own media identity;
their ownership cannot be inferred solely from the sending deck. A narrow mask
for explicitly private media references now passes host tests, together with the
committed-origin classifier for abrupt removal. The combined diagnostic capture
confirms clean auxiliary rows but still exposes standard source tuples before
the first USB STOP interaction. The committed-origin classifier incorrectly
required the unrelated safe-eject controller to have been captured first. Its
correction passes the focused sanitizer regression, which fails when the old
condition is restored. Diagnostic binary `0a0f041d768f8840e7b276384ac0af22280ccaaab5a2512a2a2750cee1adc3fc`
now passes known-field and selected-marker checks across 418 fresh and 421 cached
post-reconnect full statuses, before any USB STOP interaction. Both decks keep
advancing. This closes that bounded diagnostic reproduction, not all extended
packet semantics or final supported-artifact acceptance. Native same-ID replacement
reporting remains a required control. See the
[corrected runtime evidence](acceptance/usb-hub-support-link-known-status-privacy-v4.json),
[owner-zero audit](acceptance/usb-hub-support-link-committed-status-owner-zero.json) and
[location-schema evidence](acceptance/usb-hub-support-link-status-location-schema.json). Clearing standard
media fields alone does not establish complete packet privacy. The preliminary
integrated builds below predate this corrected runtime result. See the
[negative physical-disconnect capture](acceptance/usb-hub-support-link-physical-cache-status-negative.json).

After those fixes, integrate the tested research changes into supported source,
build/package with the exact firmware guard and rerun the mode, network, hub and
eject regressions on that one artifact. Physical activation remains deferred until
the remove-USB/full-restart recovery requirement is verified. The
[implementation plan](PRO-DJ-LINK-IMPLEMENTATION-PLAN.md) records exact experiments
and unresolved limits. The observations below retain their historical order and
must be interpreted with their individual binary hashes and test scope.

An isolated preliminary integration now exists under ignored
`local/integration/link-coexistence/`. Its 26 changed-source hashes match the
snapshot, and its patch applies to the current source tree. The SDK-built ELF
SHA256 is `661a9af8e66668789d1c0d478749e65211be7fbc5bd522837a6b782888192cf4`
(133416 bytes). It retains the exact firmware descriptor and supported kernel
initialization, composes the parser ownership logic into one hook, and adds an
early matching-USB-assets initialization guard. The guard precedes mount recovery,
kernel loading, hooks and workers; the community build disables the standalone
constructor. This is source/build review, not boot recovery evidence. Queue
cancellation is excluded from this snapshot, and no integrated emulator or
hardware acceptance is claimed. Both USB-present and USB-absent normal-loader
cold boots remain necessary because early USB mount timing can also refuse a
legitimately installed package.

The newer isolated `local/integration/link-coexistence-v2/` now includes the
request-origin and Instant Doubles cancellation implementation plus both privacy
fixes, without diagnostic markers, artificial delays or the diagnostic native API
caller. Its 64 recorded source hashes match, the patch applies, and the interim
SDK ELF SHA256 is
`5b9e0da1cfee91e074db2b3bf0fa4f223c52eef650afb43965ed4fe8b1d7ba78`
(143392 bytes; GLIBC 2.17 imports). The standard portable runner passes all 22
components in plain GCC and ASan/UBSan (44 invocations). This snapshot predates
the correction for an uncaptured safe-eject controller. Instant Doubles uses
supported inline hooks and the native cancelled-task
cleanup path; the attempted executable-segment vslot claims were rejected and are
not used. This earlier staged ELF did not run in the emulator. It has been
superseded by the packaged candidate at the top of this document; it must not be
used as that candidate's runtime evidence. Final artifact regressions and
normal-loader cold boots remain pending. Hardware activation remains deferred
until recovery is verified.

## Coexistence experiment chronology

A queued-load reconnect fault now exposes another release blocker: pausing the
common load closure for eight seconds, unplugging/replugging its source and
resuming allowed the old request into the allowance stage. Deck2 still showed
NOW LOADING approximately56 seconds after submission, without final admission
or completion in the captured log. Normal loading passed on the same diagnostic
binary. This is emulator-only evidence; cancellation and request-origin handling
remain unresolved. See [queued reconnect evidence](acceptance/usb-hub-support-link-queued-replug.json).

An identified coexistence blocker: a five-source whole-hub eject after loaded
On/Off/On was refused because the exact loaded-track proof for deck1 could not
be established. All five backing mounts remained present. Playback/mode tests
do not establish safe eject after mode changes. An emulator-only diagnostic
variant now logs native unload state/mode/media/epoch to investigate this;
no fix or hardware candidate is claimed. See
[refusal evidence](acceptance/usb-hub-support-link-mode-eject-refusal.json).

An opt-in load-origin retention experiment now passes a bounded follow-up:
one loaded hub track survives Off/On/Off and whole-hub eject; all five backing
mounts and private aliases disappear, native completion is observed, and cached
playback telemetry advances. The probe preserves mode2 load provenance and
checks the captured physical USB identity before detach. Disconnect/replug
negative coverage, queued-load races and production integration remain open.
See [retention evidence](acceptance/usb-hub-support-link-mode-load-retention.json).

The retention experiment also rejected an old cached load after its USB was
disconnected and reconnected as the same sdX/source slot with a new physical
generation. All five mounts stayed present. Reloading deck2 from the new
connection then allowed whole-hub eject with advancing cached playback. The
extracted origin-check helper passes ASan/UBSan tests; its rebuilt binary is
not yet installed. Queued-load correlation and seamless handling of old cached
tracks across reconnection remain open. See
[origin/reconnect evidence](acceptance/usb-hub-support-link-load-origin.json).

Latest coexistence research: separate private requester identities, pinned TCP
ownership and a partial UDP ingress guard now run in isolated QEMU. Three hub
fixtures enumerate; a loaded hub deck follows a simulated 126 BPM master, and
six external private-identity UDP markers are rejected. This is experimental,
not a hardware candidate: protocol coverage, receive-option/peek semantics,
lifecycle races and complete mode/regression tests remain open. See the
[implementation plan](PRO-DJ-LINK-IMPLEMENTATION-PLAN.md) and
[latest UDP evidence](acceptance/usb-hub-support-link-udp-integrated.json).
The following observations retain the earlier experiment chronology.

Latest peer testing corrected a simulator startup race: the new handoff role
waits for the existing master before requesting ownership. Two sessions of
player 3 now show wire-confirmed master transfer, with the loaded hub deck
following 129 and 127 BPM across disconnect/reconnect. Playback telemetry advanced
through both sessions and the gap. Automatic native-master fallback timing
remains unproven; this is protocol/emulator evidence only. See
[master reconnect evidence](acceptance/usb-hub-support-link-master-reconnect.json).

A loaded comparison without USB Hub Support now reproduces the short post-loss
status window: deck statuses stop about 9.3 seconds after the last peer packet,
while a later screenshot explicitly shows the local MASTER label and continuing
playback. Missing wire assertions alone therefore do not establish failed master
reclamation. This comparison requires the AZ-MODS core's emulator library key
provider; it is not completely stock firmware. Exact fallback timing remains
unmeasured. See [core-assisted baseline](acceptance/usb-hub-support-link-core-fallback.json).

The matching candidate subsequently reclaimed local MASTER without UI input:
the label was absent in the screenshot at nine seconds after the harness stop
event and present at eleven seconds, with advancing playback. The run used
three hub sources, member8/USB1 playback and 145 synced/nonmaster observations
at 129 BPM before disconnection. These are bounded screenshot observations,
not an exact timeout or audio-phase measurement. See
[candidate fallback evidence](acceptance/usb-hub-support-link-ui-fallback.json).

The same candidate now retains five hub source rows through loaded On/Off/On
changes. With Link Off, four loaded decks advanced; with Link On, decks3/4
unloaded while decks1/2 continued, and deck1 followed a simulated external
master at127 BPM. The fifth source (member7/slot8) browsed and loaded, although
its row lacks a native USB icon. All decks used content495, so this does not
establish different-track cache isolation. The loaded-track warning triangle
remains unresolved. See [five-source evidence](acceptance/usb-hub-support-link-five-source-modes.json).

Expanded socket research found native recv/MSG_PEEK usage outside the earlier
recvfrom-only inventory. An opt-in runtime audit then observed all three Link
UDP ports using recvfrom with flags0 and 4096-byte capacity during mode changes,
external traffic and hub playback; no read/recv use on those ports was observed
in these runs. The loaded external-peer session recorded 165 valid deck 1 status
samples. This supports the exercised receive path, not all indirect paths or
generic peek/option semantics. See [runtime audit](acceptance/usb-hub-support-link-udp-read-audit.json)
and [expanded static inventory](acceptance/usb-hub-support-link-socket-read-static.json).

The earlier socket research adds a reproducible static inventory: five direct native
recvfrom sites use flags0, no recvmsg PLT import is present, and reviewed direct
option8 setters use SO_RCVBUF rather than IP_PKTINFO. Indirect/library paths
remain unaudited, so this does not establish general option/peek compatibility.
See [socket inventory](acceptance/usb-hub-support-link-socket-static.json).

Latest eject/mode research: whole-hub loaded eject overlapped a native Off
request and completed with advancing cached playback telemetry. A subsequent
correction distinguishes finalized eject reservations from active withdrawal:
one ejected USB no longer blocks mode recovery of the others. The corrected
candidate passed partial eject plus Off/On/Off, retaining exactly two live
sources while the third stayed unmounted; member8 loading/playback afterward
also passed. See [partial-eject evidence](acceptance/usb-hub-support-link-mode-partial-eject.json)
and [whole-hub overlap evidence](acceptance/usb-hub-support-link-mode-eject.json).
These are distinct binaries and limited emulator checks, not hardware acceptance.

The preceding research candidate also defers an old completion epoch while a
newer transition is active. Its adapter test passes ASan/UBSan and TSan and
fails against the previous header as expected. An emulator On/Off/On sequence
retained three sources, followed by successful hub loading and 308 synced,
nonmaster deck 1 status samples at 126 BPM from the isolated peer. Native
transitions did not overlap in this UI test; pending-request handling remains
unvalidated in firmware. See [epoch/peer evidence](acceptance/usb-hub-support-link-mode-epoch.json).

The preceding mode adapter serializes its internal-cache check/write with native
transition entry and protects source replay with the source mutex. Its actual
adapter passed 20,000 modeled concurrent cycles under ASan/UBSan and TSan.
The exact new binary also passed an unloaded Off/On/Off emulator cycle with
three source rows. Loaded/external-peer and rapid-change coverage remain open
for this binary; see [serialization evidence](acceptance/usb-hub-support-link-mode-serialization.json).

The preceding research-only mode-recovery overlay completes Off/On/Off cycles
with exactly three source rows, first unloaded and then with one loaded,
advancing deck. It separates native network change detection from the internal
library-cache override and republishes withdrawn USB1 rows. A loaded-track
warning icon remains; concurrency, eject overlap, additional-source coverage
and external-peer regressions are still open. This is emulator telemetry, not
audio or hardware acceptance. See [mode-recovery evidence](acceptance/usb-hub-support-link-mode-recovery.json).

The preceding mode regression also confirmed four loaded, advancing local decks
from hub sources after starting with Link Off. However, switching live from
On to Off lost two source rows until application restart. This remains a
functional blocker, not an accepted restart requirement. See
[four-deck/transition evidence](acceptance/usb-hub-support-link-four-decks.json).

Active development is PRO DJ LINK coexistence. The isolated QEMU/WSL Ethernet
peer now establishes native discovery and status exchange with Beat Link 8.0.0:
Link off announces local identities 1..4; Link on announces 1/2, with the peer
assigned 3. Peer beat transmission is observed. No mod coexistence or audio-sync
acceptance is claimed. Beat Link truncates the AZ's 1152-byte statuses to 512
bytes and reports an unknown packet type; this remains a test-harness limit.
See [implementation plan](PRO-DJ-LINK-IMPLEMENTATION-PLAN.md) and
[native baseline](acceptance/usb-hub-support-link-native-baseline.json).

An emulator-only member 7/8 overlay has now enumerated three hub fixtures,
browsed sources 7 and 8, loaded two decks and ejected the whole hub while
retaining advancing cached playback telemetry. Physical Link remains filtered.
This is limited alternative-identity feasibility, not external coexistence,
audio or hardware acceptance. See the [feasibility record](acceptance/usb-hub-support-link-member-feasibility.json).
A separate private-loopback admission experiment is in progress.

The latest isolated-emulator external-traffic experiment exchanged peer status
while hub sources remained available, including member-8 browsing/loading and
playback telemetry. It is not ready for hardware: outgoing statuses expose the
private source identity, loaded master/sync adoption has not passed, and ingress,
handshake ownership and lifetime/race gates remain incomplete. See
[initial coexistence evidence](acceptance/usb-hub-support-link-initial-coexistence.json).

A subsequent idle-role correction and sync-before-play sequence demonstrated
following the simulated external master at 123 BPM, with native sync=true and
master=false. This is protocol/UI tempo evidence, not audio phase or loaded
master-handoff acceptance. See [tempo-follow evidence](acceptance/usb-hub-support-link-tempo-follow.json).

## Published release: USB Hub Support 0.2.1

Release: https://github.com/NachoCasta/nacho-az-mods/releases/tag/usb-hub-support-0.2.1
Source/tag commit: `0c21db0816a6f21bf6eff3aef9ace06b6d7055b7`.
[Source CI](https://github.com/NachoCasta/nacho-az-mods/actions/runs/39175867655)
passed behavior-tests and sdk-build; all 43 accepted build-source blobs match the
commit. The user authorized publishing the accepted progress. The package is
byte-identical to the selected emulator/hardware candidate: SHA256
`6a33b8dd496bb2130289cf7e49299b3cf24a29e4895afcf4c460184d37bd265f`
(355839 bytes); executable SHA256
`9feec49c06f10dd01e191ed5094f68eb1e7e7b35cd9ed76d757a8477a37a945b`.
The root catalog now advertises 0.2.1. Anonymous catalog/package download,
size/hash/content validation, all seven package members, the icon and pinned
public source manifest passed. See [publication evidence](acceptance/usb-hub-support-0.2.1-publication.json).
No rebuild or further hardware action occurred during publication. Previous
release assets and tags are retained.

Individual loaded-track eject from NACHO PLUGS passed on the real AZ. The user
reported complete playback of one track after eject. SSH confirmed absent backing
and library-alias mounts while the physical device remained connected, retained
exact-media cache proof, unchanged PID 1876957 and mounted CAMI/native USB1.
See [hardware acceptance](acceptance/usb-hub-support-0.2.1-az-loaded-eject.json).
The exact candidate also passed individual and two-playing-deck whole-hub eject,
seek/loop, replacement-media, reserved LOAD and failed-load regressions in QEMU.
Portable sanitizer/model, two-compiler and package checks passed. The source
snapshot and evidence are in [the ledger](acceptance/usb-hub-support-0.2.1-loaded-summary.json).

Long-session native memory and input timing remain unvalidated. Twelve local
cycles kept owned tickets/proofs/FDs/threads bounded, but native RSS grew 244580 KiB
from cycle 2 to 12. Same-connection load/unload controls stayed stable; a partial
unload-before-eject control reproduced growth without a loaded native-detach
request. Two QEMU attempts encountered core input-filter budget enforcement;
failed attempts are recorded separately from acceptance. The user chose publication
of the accepted functional progress with these limits disclosed. Broader loaded
hardware, all-format/cache-capacity and eight-source acceptance are not claimed.
Further real-AZ tests/restarts require their own authorization. Public 0.2.0
assets/tag remain available and unchanged.

## Previous release: USB Hub Support 0.2.0

Release: https://github.com/NachoCasta/nacho-az-mods/releases/tag/usb-hub-support-0.2.0
Source/tag commit: `1c75601c527fbdd821b5872a334f345aecb60396`.
Source CI: https://github.com/NachoCasta/nacho-az-mods/actions/runs/39158328964
Both behavior-tests and sdk-build passed. All 38 accepted source blobs match the
release commit. Package SHA256:
`1c24d4ea3e73dbfc7650e4ac9c47ee310a10ee883c1efbea41455458e6f41f09`
(350722 bytes); ELF SHA256:
`09b8ffa89b73ff9c5c402d81955bd6c567c2b520edb5c3f38fd00adcf865bd5b`.
Anonymous catalog/package download, size/hash/content validation and pinned public
source access passed. See [publication record](acceptance/usb-hub-support-0.2.0-publication.json).
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

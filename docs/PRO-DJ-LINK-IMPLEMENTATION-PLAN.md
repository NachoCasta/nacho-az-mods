# PRO DJ LINK coexistence implementation and validation

## Required behavior

The active development goal preserves native mode behavior: four local decks
with PRO DJ LINK off, or two local decks with external players when it is on.
Hub sources remain locally browsable, loadable and safely ejectable in both
modes. Exporting those sources to external players is out of scope.

Keep all eight independent source servers and caches. No native USB library
slot may be borrowed. Firmware support remains the exact existing EP147 tag.
Published 0.2.1 artifacts remain unchanged; no release is authorized here.

## Evidence gates

1. Audit native member conversion, admission, storage capacity, lookup, source
   enumeration and full track identity. Establish whether private identities can
   coexist with physical members without unchecked native array access.
2. Establish an isolated Ethernet segment between the emulator and a separate
   protocol peer. The current QEMU user-network backend is not an Ethernet
   broadcast baseline. Keep simulated discovery off the workstation LAN.
3. Build a reproducible open-source peer harness with explicit player identity,
   discovery, status, beat, master and sync controls and timestamped observations.
   Establish stock behavior before assigning a failure or success to the mod.
4. Implement private transport ownership and library dispatch admission. Exercise
   IP/player collisions, fd reuse and external requests using private media codes.
   Physical traffic must retain native routing and semantics.
5. Validate exact-candidate browsing, loading, cached eject, native USB access,
   both startup modes, mode transitions, peer reconnect and five-source emulator
   regression coverage. Capture the external network boundary and inspect the
   status of local decks playing hub tracks for private identity leakage.
6. Follow the normal AZ-MODS workflow and [hardware policy](AZ-TESTING-POLICY.md).
   The user removed the separate remove-USB/full-reboot recovery prerequisite.
   Preserve the prohibition on manual internal edits and distinguish unperformed
   recovery tests from observed results.
7. Prepare the candidate package, hashes, evidence ledger and concise final AZ
   procedure. Protocol acceptance does not prove audio phase synchronization or
   interoperability with every actual CDJ model.

Live mode changes require safe source and worker lifetimes. A startup-only
experiment is useful evidence but does not silently replace the full goal.

## Initial inspection: 2026-10-18

Local source and a fresh read-only Ghidra query confirm that the present mod
restricts its synthetic sources to players 3/4 and continually forces the
internal Link cache independently of the user setting. Its physical packet
filter cannot simply be removed safely.

The exact-firmware conversion at `0x013db0c0` accepts compact member numbers
1 through 8 and maps them to type 2 with indices 0 through 7. Other compact
ranges map to other member types. This establishes representability at one
conversion boundary only: it does **not** establish network admission, native
table capacity, remote source UI support or successful private players 5/6.
The earlier design gate remains open. No new identities have been injected.

The current Windows QEMU process was observed using `-netdev user`, six virtual
CPUs and a masked EP147 boot service. Its existing manually prepared runtime
cannot serve as evidence of the normal loader's cold-boot recovery. No physical
AZ access or mutation was performed during this inspection.

Reproduce the static conversion finding with the existing `Probe.java` in a
read-only project using `dec:13db0b0 dec:13db0b8 dec:13db0c0`. Keep proprietary
decompiler output local; record interpreted layouts and test results here.

## Isolated Ethernet infrastructure

`research/scripts/link-ethernet-peer.py` under `src/azhub/` implements the
QEMU TCP socket-network framing, ARP and ICMP echo on `192.0.2.2`. It binds only
to workstation loopback and records packet metadata without payloads. It does
not yet implement a CDJ or establish any Link acceptance.

Six packet tests pass with `python
src/azhub/research/scripts/test-link-ethernet-peer.py`: ARP ownership, ICMP
checksums/payload, truncation, fragmentation/UDP lengths, bounded Link metadata
and nonisolated-address refusal. Syntax compilation and `git diff --check` pass.

A live QMP `netdev_add` socket connection to this peer succeeded. Adding a
`virtio-net-pci` device failed because the existing `pcie.0` bus does not support
hotplug. The unattached backend was removed successfully with `netdev_del`.
No guest NIC was added or configured and no guest Ethernet packet was received;
this is infrastructure evidence, not a passing network smoke test. The next
local emulator launch must include the isolated NIC at startup and retain its
existing management NAT independently.

Further read-only firmware inspection shows `LinkDeviceManager::addDevice`
(`0x013e4a78`) appending shared device objects to a dynamic vector at owner
offset `0x418`; periodic-survival lookup (`0x013e0e48`) scans member records
between owner offsets `0x380` and `0x388`. Neither finding proves all other
member consumers or media/UI paths are safe beyond player 4. Admission and
status/media consumers remain required investigation gates.

## Native protocol baseline: 2026-10-19 UTC

The isolated NIC now runs at emulator startup. Native `eth0` uses the test
segment while `mgmt0` retains SSH NAT. QEMU virtio-mmio enumeration required
reordering device declarations; interface MACs were checked before naming them.
ARP/ICMP passed with both the standalone peer and the WSL TAP transport.
Explicit broadcast addresses are required; changing an existing address did not
set the broadcast in this fixture, so the test addresses were removed/re-added.

The new [Beat Link harness](../src/azhub/research/link-peer/README.md) compiles
against pinned 8.0.0. With no core/hub mapping in EP147, the UI-confirmed native
mode-off fixture announces players 1/2/3/4 and 33. Mode-on announces 1/2 and 33;
the AZ confirms the simulated peer's number 3. A 30-second status run received
300 native updates. The subsequent master-peer trial emitted 31 beat packets
with measured mean interval 499.910 ms at requested 120 BPM. Native decks were
empty: this is not proof of master adoption, follower sync or audio phase.

The independent Ethernet observer measured 1152-byte AZ status payloads. Beat
Link 8.0.0's 512-byte receive buffer truncates them and warns; it also does not
recognize packet type `0x80`. Preserve this limitation while extending protocol
validation. Finder shutdown emits `lost` callbacks, which must not be counted
as spontaneous disconnect events. Early zero-broadcast/unconfirmed-mode attempts
are excluded from accepted baseline evidence.

See [native baseline evidence](acceptance/usb-hub-support-link-native-baseline.json).
The emulator application uses a transient native-baseline unit because the boot
command masks its normal service. Therefore these results do not establish
loader recovery. No real-AZ access or candidate activation occurred.

## Member 7 feasibility: 2026-10-19 UTC

The generated emulator-only overlay now admits member 7, announces its USB
media, constructs independent hub library/info servers and mounts the read-only
alias `/media/player07/usb` from fixture `/dev/sdb1`. The SOURCE screen shows
`HUB_LIB` with LINK 7. Native artist/album and track browsing populated, including
2547 track entries. Loading the fixture's available track `Algo más` succeeded:
PID 71208 reports deck 1 loaded, content ID 495, position 0.047 seconds and not
playing. This is one-source browsing/loading evidence, not audio acceptance.

Overlay executable SHA256:
`ad82054a5b9baa57f717dfe35c1c6d14bf25315318d82c74944f44ac6d65d4e1`.
Reconstruction: `src/azhub/research/scripts/prepare-link-member-probe.py`.
It retains the physical Link filter and rejects kernels other than the local
emulator's `6.6.138-dirty`. Production mod sources are unchanged. Member 8,
multi-source operation, eject, external coexistence and native array-capacity
audits remain outstanding.

The transient emulator service initially omitted the stock application's
`ExecStopPost` cleanup, leaving stale daemon sockets after a restart. Restoring
the native `apl_end.sh` stop action and USB renotification restored the library
listener. Immediate browse screenshots also preceded asynchronous population;
only the populated UI and loaded telemetry above count as success. A suspected
3/4 requester-offset dependency belongs to an inactive optional branch and was
not changed or established as a cause.

Ignored local evidence is under `local/diagnostics/link-coexistence/`:
`member7-browse-ready.png`, `member7-track-selected.png`,
`member7-loaded-status.json` and `member7-loaded.log`. Packet trace output is
capped by the core, so the final log is not a complete session transcript.

### Expanded member test and private routing investigation

The same binary subsequently enumerated three COW fixtures as 7/USB1,
7/USB2 and 8/USB1. Member 8 browsing and a second deck load succeeded. A long
USB2 STOP unmounted all three backing volumes and aliases while native USB1
remained mounted. Both loaded decks subsequently reported playing and advancing
positions in the same process. See the
[bounded feasibility record](acceptance/usb-hub-support-link-member-feasibility.json)
for retained samples and limitations, including identical fixture databases and
post-eject unsupported waveform requests. No audio acceptance is inferred.

Read-only inspection confirms that connecting-state handler `0x013f22b8`,
event 9, compares the announced IPv4 address against the native interface subnet.
Network snapshot getter `0x013df908` copies manager configuration under its
mutex; the mask is at snapshot offset `0x19`. This prevents a simple switch to
private loopback labels without an admission adjustment.

`prepare-link-member-probe.py --private-loopback` builds a separate emulator-only
experiment using `127.77.0.7/8`. A thread-scoped wrapper recognizes those exact
member records and clears only the mask in the second returned snapshot during
their admission. It does not modify stored network configuration. Physical
Link filtering remains mandatory: packet provenance and library connection
admission are not implemented by this experiment. It must not be promoted to
production merely because loopback admission succeeds.

The first private-address runtime failed during startup because its network
snapshot wrapper used an ordinary function return ABI. Instruction inspection
at `0x013df920` confirms that native return storage arrives in ARM64 `x8`.
The reconstruction now uses a 0x50-byte C aggregate return, preserving that ABI.
Keep the failed attempt separate from subsequent runtime evidence.

The corrected wrapper booted, but advertising loopback in both the received
socket address and the keepalive record did not produce observed admission.
The next variant keeps a subnet label only in the synthesized receive address,
while advertising/storing `127.77.0.7/8` in the member record. Both members then
reached the scoped admission hook, acquired independent libraries and appeared
as three source rows. Member 8 browsing and deck 2 loading succeeded in PID
87556. Native load trace `0x1010200000702:0x1ef` identifies member 8 USB1,
content 495; `/media/player08/usb` bound `/dev/sdd1` and deck telemetry reported
the expected track loaded. This is stronger routing evidence than the earlier
identical-fixture test, whose load trace was lost to the logging cap.

Tested private variant SHA256:
`45bec59683266a0009bcdac72ad19b85a0e959740b99c5cd3cc938a1af183e2f`.
Local evidence: `private-v4-loaded.log`, `private-v4-loaded-status.json`,
`private-v4-sources.png` and `private-v4-track.png` under the diagnostics folder.
The generator currently reproduces this variant. Remaining gates include
eliminating or isolating the receive-address subnet facade, tracing ingress
provenance, guarding private library dispatch and preserving external status
semantics. Physical traffic is still blocked; coexistence remains unimplemented.

Library dispatcher task `0x007d6e28` carries a connection argument in task word 2
and passes it to `0x00e6b1c8`. The current `hub_matches` callback has only server
and message arguments, so private slot-code matching alone is insufficient to
authorize an external request. Trace that connection argument back to accepted
socket ownership before allowing physical peers to reach the native server.

## Private library admission experiment: 2026-10-19 UTC

Native parser `0x00ef05c8` retains the transport at connection offset `0x20`.
The transport's pointer at offset 8 refers to a socket object whose first word
is the descriptor used by native receive. `0x00ef8258` forwards the parsed
message and connection ID to the DataStream receiver. Runtime observations
confirmed that a local hub request's accepted socket has the reverse TCP tuple
of a live outbound descriptor tracked by the mod. Replies do not have that
relationship. The declared connection/player number is not ownership proof.

The isolated `link-library-peer.py` harness uses the normal 12523 port query
and native library handshake, then asks only for media information. Before the
guard, an external requester using private slot `0xe0` received normal reply
`0x4000`. This demonstrates why the UDP filter alone is not library admission.
No track/audio payload was requested or retained by the harness.

The `--trace-library-ownership` overlay now includes an experimental guard.
Parser observations create bounded message tickets for private requests from
matching mod-owned loopback clients. Native dispatch consumes a ticket into
thread-local scope; hub server matching requires both that exact message and
the corresponding private player. Native message destruction removes cancelled
tickets. Missing tickets fail closed through the native unsupported-request
path. This remains research code: descriptor races, cancellation, table pressure,
source generations/reuse and complete native message lifetimes need validation.

Guard binary SHA256:
`d7136b0f99beecc6c172283601a2501d0cbc57f2ea8134bbcb884e1bc6439833`.
In emulator PID 96216, external private-slot requests received native rejection
`0x4003` both as requester 3 and while claiming requester 5. An external native
USB-slot-3 control retained normal `0x4000` behavior. Local member 8 browsing
and deck 2 loading still succeeded; native load identity was
`0x1010200000702:0x1ef`. The spoof trial does not establish absence of connection
disruption; reconnect/collision tests remain required.

Ignored evidence: `external-private-slot.json`,
`external-private-slot-guard.json`, `external-native-slot-guard.json`,
`external-spoofed-slot-guard.json`, `ownership-guard-loaded.json` and
`ownership-guard-track.png` in the local coexistence diagnostics directory.
Physical UDP filtering remains enabled. These are admission tests, not a
passing external PRO DJ LINK coexistence result.

## Initial isolated external coexistence experiment

The spoofed requester-5 trial closed the existing private TCP connection.
Opening the member-8 playlist category triggered a reconnect and populated
playlists in the same process. Denying private data did not prevent this
connection disruption; protect handshake identity ownership before production.

`--external-link-probe` requires the private-address and library-admission
experiments and disables the broad physical UDP filter. It is explicitly an
isolated-emulator experiment, not a deployable network-safe candidate. With
binary `b167b21a8ad69ff7f4b242f91a19e6d12243219c08ffba839052ad1dd5949b40`
in PID 101391, mode off announced players 1/2/3/4/33. The confirmed native UI
transition to mode on announced 1/2/33; peer 3 received 302 states during its
30-second empty-deck trial while three hub sources remained visible.

A subsequent peer session remained active during member-8 browsing, deck-2 load
and playback telemetry. It received 661 statuses and 14 AZ beat events. The
independent observer also found 165 outgoing AZ status packets identifying the
private source as player 8, slot 3, type 1. This is an unresolved external source
identity leak, not hub-sharing support or accepted status behavior.

Loaded sync/master adoption has not passed. One sync press fell after its peer
session and is excluded. In a further 123-BPM master-peer trial, AZ deck 2 stayed
master at its track BPM and the peer relinquished master. Compare against a
loaded native baseline before attributing the handoff behavior to the mod.
Also audit the fake status template: it currently copies native status flags
at `0x89` and master-handoff byte `0x9f` without clearing private-player roles.
Beat Link 8.0.0's local source identifies those fields; the synthetic library
members must not inherit a playing deck's master/sync state.

See [initial coexistence evidence](acceptance/usb-hub-support-link-initial-coexistence.json)
for exact counts, hashes and outstanding gates. The emulator is currently in
Link mode on; no real AZ was accessed. These results establish a first combined
protocol/library path, not final compatibility or audio synchronization.

## Idle private roles and tempo following

Production source now includes `link-status.h`: synthetic library status clears
playing/master/synced/on-air/BPM-sync bits (`0x7a` at `0x89`) and resets the
master-handoff destination (`0x9f`) to `0xff`. This helper is called only when
constructing synthetic statuses, not on real deck traffic. It validates the
packet prefix/minimum size and otherwise preserves all bytes. Its focused test
exercises all 256 flag values, unrelated-byte preservation, repeated application,
truncation and wrong packet types. It is included in the portable host suite.

The resulting external probe binary
`68813580cf6405fd52a7a01056a3ba48d22b8ad4d436a4f223cc1619e8b7fa18`
loaded the member-8 track in PID 109605. With peer 3 already master at 123 BPM,
enabling deck 2 sync before starting playback produced native status with sync
true and master false. The UI showed 123.0 BPM, and the peer retained master at
session end. See [tempo-follow evidence](acceptance/usb-hub-support-link-tempo-follow.json).

This changes the earlier observation that following had not yet passed, but
does not prove loaded master handoff or audio phase. The input sequence changed
as well as the private flags, so it is not a causal comparison against the old
binary. A loaded native baseline remains needed. The harness now logs effective
BPM and pitch as well as the unchanged original-track BPM; that added telemetry
was compiled after this trial and is not retroactively attributed to it.

## Outgoing source reference mask and cached-detach counterexample

`--mask-private-status` adds an isolated-emulator send-path experiment. It clears
the known private media tuple at `0x28..0x2a` and content ID at `0x2c..0x2f`
only in external 1152-byte AZ status packets matching a configured hub source.
Local status and all other bytes are preserved. Unit checks cover mismatched
sources/slots/lengths and byte preservation. Production filtering is not enabled
by this helper; its send-path use remains in the generated research overlay.

Binary `fc990347774f5f4de1724d6fef50bbd8913bb9e3f68bd342fde729da52483576`
ran in PID 117102. The external peer observed 198 synced deck-2 states at
effective tempo 125.99995 BPM for its requested 126 BPM, and retained master.
The observer captured no player-8 reference, but after loaded hub eject it saw
137 packets claiming player 1 / USB / content 495. Native cached detach rewrites
the apparent media origin, so source-number masking alone is insufficient.
This experiment fails complete privacy acceptance. See
[status mask evidence](acceptance/usb-hub-support-link-status-mask.json).

The whole-hub eject completed and playback telemetry advanced with all three
devices safe, but final completion was observed after the peer session ended.
Do not claim simultaneous external sync at the moment of eject completion.

Next: classify cached private playback using the existing retained detach proof,
controller/load epoch and exact media identity, including in-flight detach.
Never infer private ownership from a matching content ID or label alone. A
replacement native load must clear the private-origin classification even when
its content ID equals the previous hub track. Preserve the native/failed-load
and source-reuse cases in the regression matrix. The extended AZ status remains
unaudited; clearing the standard tuple is not full-packet privacy proof.

## Cached-source prefix masking experiment

The generated status-mask overlay now consults the retained disconnect ticket
under `safe_lock`, matching controller, load epoch, exact media, current native
owner and submitted pending/completed state. Content ID rejects stale metadata;
it does not establish ownership. A different load epoch rejects the association,
including a native replacement with the same content ID. The actual adapter has
a portable test with simulated native interface/ledger; ASan/UBSan passed.

Binary `3bc529c1c86959aed5109c5c18f4210132bdb62d57044e918ffaf91f8bf73c77`
ran in emulator PID 126814. The capture contained 1067 outgoing AZ statuses,
all with zero standard source tuple/content ID, through loaded whole-hub eject.
All three fixtures reached safe state while the peer session remained active;
deck 2 retained cached playback and followed the 126 BPM peer. The portable
production suite passed plain and ASan/UBSan. See
[cached-source evidence](acceptance/usb-hub-support-link-cached-status.json).

This addresses the observed cached-origin counterexample only. Native same-ID
replacement, failed replacement, queued packet/load races and the extended
status bytes remain unvalidated. The research overlay is not a hardware
candidate; ingress provenance, handshake collisions and lifetime gates remain.

## Pre-registration requester collision guard

Static exact-firmware research identified the target-number handshake at
`0x00eee540`. Its detector caller (`0x00eeec98`) branches on a negative return
at `0x00eeed34`, destroying the pending communicator instead of invoking the
connected listener at `0x00eeed50`. The parsed requester is at communicator
offset 8; the transport/socket chain matches the previously observed parser.
This provides a boundary before registration can displace an existing key.

The experimental ownership overlay now checks configured private requester
numbers at that boundary. It permits a reserved number only when the accepted
socket matches a live private client's reverse loopback tuple. Other requester
numbers retain native handling. It forwards native handshake work first, so the
setup response may reach the peer before rejection closes the connection.

Binary `987c745321262d029faa36b7c50e4d1e24c56e7ff42632002cabdaec74d774f8`
started in emulator PID 139380 with 40 hooks. Two external requester-5 collision
attempts closed without replacing the established loopback socket tuples or
inodes. Requester 3 received native USB media response `0x4000`, while its private
slot request received `0x4003`. After the second collision, member-8 browsing
loaded content 495 on deck 2. See
[handshake guard evidence](acceptance/usb-hub-support-link-handshake-guard.json).

This addresses the demonstrated sequential collision, not socket lifetime or
concurrency correctness. The shared descriptor registry is still experimental;
generation-safe ownership, cancellations, private UDP provenance and broader
reconnect/identity regression remain required before hardware preparation.

## Pinned socket ownership registry

The ownership overlay no longer scans the unsynchronized diagnostic `traced[]`
array for authorization. `link-socket-registry.h` reserves a generation before
connect, completes that exact generation afterward and invalidates it before
close. A bounded duplicate pins each owned socket's inode. The vendor kernel
source lacks `SO_COOKIE`, so the registry uses pinned inode identity instead.
Accepted-socket proofs also retain a duplicate until dispatch/destruction and
revalidate the client generation and both live descriptor identities. Saturation
or generation exhaustion fails closed. There are 64 connection entries and at
most 256 pending message proofs; native lifecycle resource behavior still needs
measurement. A pin is released through raw close to avoid interceptor recursion.

Portable tests use actual loopback sockets and cover client/server descriptor
overwrite, stale connect completion, concurrent invalidation/lookup, capacity,
epoch exhaustion and unchanged final FD count. ASan/UBSan and ThreadSanitizer
passed, including 1000 concurrent iterations. This does not instrument firmware.

Binary `2c166c3631fbdb3ae80ac8a2035a207fb780a5ac03429a88fc4c99ba68071574`
ran in emulator PID 148521. With the member-8 library stable, an external
requester-5 collision closed without changing its socket pair/inodes. Native
requester-3 USB metadata still returned `0x4000`, and the hub subsequently
loaded content 495 on deck 2. See
[socket registry evidence](acceptance/usb-hub-support-link-socket-registry.json).

An earlier comparison overlapped member-8 initialization and changed sockets;
it is excluded from passing evidence. Logs show the owned member-8 connection
replacing owned member-7 because both still use requester 5. Next, separate the
private requester identities and validate simultaneous source use. The original
diagnostic arrays still need a concurrency audit independently of authorization;
source-generation and native cancellation/lifecycle gates also remain open.

## Separate private requester identities

The ownership overlay now maps private member 7 to requester 5 and member 8 to
requester 6, using one shared mapping for outgoing setup/DMST rewriting and
handshake admission. A reserved requester must match the proven socket's member,
not merely any owned socket. This mapping remains an emulator experiment.

Binary `27f65dc5a216cc8c19a3c527c3d58427bae138536eba097b95f06e858eb57dbf`
ran in PID 153417. Both private TCP connections stayed established through
external requester-5 and requester-6 collision attempts and loading two decks.
All four socket endpoint records retained their original tuples/inodes. Native
requester-3 USB metadata returned `0x4000`; private slot metadata returned
`0x4003`. The collision harness now explicitly requires EOF/reset after setup
instead of interpreting a general process failure as rejection.

Deck 1 loaded source 0 with media `0x1010200000602:0x1ef`; deck 2 loaded source 2
with media `0x1010200000702:0x1ef`. The source images contain the same audio/DB,
so this is distinct media routing evidence, not different-content isolation.
Both decks followed the external peer's 126 BPM master. Whole-hub eject reached
safe state for all three fixtures while the peer was still active; both decks
continued cached playback telemetry. See
[private requester evidence](acceptance/usb-hub-support-link-private-requesters.json).

No audio phase, physical CDJ, five-source or Link-off four-deck acceptance is
assigned to this binary. UDP provenance and remaining race/lifecycle gates still
prevent hardware preparation. Raw outgoing status privacy must also be rerun
on the eventual candidate rather than inherited from earlier binaries.

## UDP ingress provenance feasibility

The private admission hook currently recognizes an embedded `127.77.0.7/8`
address, which is not transport provenance. A sender can forge those bytes.
`link-udp-policy.h` now models rejection of untrusted private keepalives,
status senders and media owners, while preserving corresponding external
identities. It validates synthetic packets against the expected private member.
ASan/UBSan tests cover contradictory fields, ordinary external packets and
truncation. This covers only the three generated formats, not all Link controls.

`link-udp-ingress-sample.c` is a separate emulator-only receiver on port 50009.
It received identical private announcement markers through loopback (kernel
interface index 1) and the isolated Ethernet peer (index 3, source 192.0.2.2).
Only the known owned loopback endpoint received trusted-member status; the
external private claim was rejected. The successful run exited zero. An earlier
run timed out before the external sender started and is excluded. The vendor
GPL kernel source contains `IP_PKTINFO` support, but no hardware runtime test
was performed. See [ingress metadata evidence](acceptance/usb-hub-support-link-udp-ingress.json).

This policy is NOT wired into the mod yet. Next: collect kernel interface
metadata in both native receive paths while preserving the caller's ancillary
data, scatter buffers, length and peek/truncation semantics; combine it with
live ownership of the synthetic sender socket before rewriting source addresses.
Audit discovery assignment and control variants too. Current emulator candidate
remains unsuitable for physical networks while these gaps are open.

## Ancillary-preserving UDP receive component

`link-udp-receive.h` collects kernel ingress metadata in the same `recvmsg`
syscall as the payload. It preserves scatter buffers, captures source metadata
even without a caller address buffer, and projects ancillary messages back to
the caller while optionally hiding the injected `IP_PKTINFO` record. Its
`recvfrom` adapter preserves bounded address copies and reported address length.
The caller must enable packet info and supply the remembered original setting;
the component itself does not mutate socket options or decide admission.

The component passed 1032 comparisons against native Linux receive behavior
for return sizes, flags, ancillary types/lengths and copied payload, with
timestamp/TTL options, control capacities 0..128, split buffers, peek and
truncation. Additional cases cover full/zero buffers, short/null address
outputs, invalid length pointers and empty nonblocking reception. GCC
ASan/UBSan and the pinned Linaro ARM64 test inside QEMU both passed. See
[receive component evidence](acceptance/usb-hub-support-link-udp-receive.json).

The component remains unwired. Integration must track original socket-option
ownership and live sender identity, validate the actual loopback interface,
bound prefix inspection to copied bytes (not a larger MSG_TRUNC return), and
never grant trust when ancillary metadata is incomplete. Filtered peek
semantics also require explicit treatment: a rejected peek must not silently
consume a different datagram or indefinitely pin a malicious queue head.

## Partial UDP policy integrated in the research overlay

`--udp-ingress-probe` now installs the receive component in place of both native
receive interceptors for IPv4 datagram sockets bound to ports 50000..50002.
It records the original packet-info option, pins listener sockets and obtains
a separate pin for each receive. A concurrent descriptor close cannot redirect
that syscall to a reused fd. Synthetic sender sockets are registered/pinned at
creation and forgotten at close. Only a matching owned loopback endpoint with
kernel loopback ingress evidence receives source-address rewriting.

Binary `d60ac8be48d1070a5018d5b3ba0e2fc8db461a07c7c8b4f97f540f6bba76a0b5`
started in emulator PID 171279. All three hub fixtures enumerated. The isolated
external sender injected six private claims (members 7/8, keepalive/status/media
response); all six were rejected with interface 3 and no trusted member. A
normal simulated external peer still exchanged statuses. Source 2 subsequently
loaded on deck 2 and followed the peer's 126 BPM master during playback. See
[integrated UDP evidence](acceptance/usb-hub-support-link-udp-integrated.json).

This is still partial isolation: other discovery/control packet variants,
rejected peek behavior, changes to packet-info options after first receive and
native resource lifecycle remain open. The current hook records option ownership
once; it does not intercept later application `setsockopt` calls. No hardware,
audio phase, full privacy or Link-off four-deck acceptance is implied.

## Link-off four-deck startup and failed live transition

The same `d60ac8be...` binary was switched from Link On to Off through the
native Utility menu in PID 171279. The native confirmation explicitly enabled
internal decks 3/4. Two hub rows then disappeared (member7/slot3 and
member8/slot3); only member7/slot7 remained visible while discovery still
reported all three fixtures present. TCP clients closed and the existing
internal-cache maintenance reenabled its cache, but the missing rows did not
recover during observation. This is a failed mode transition, not acceptance.

Restarting the emulator application with the same binary and persisted Link
Off restored all three rows (PID 179820). The wire advertised identities
1/2/3/4 plus the AZ identity 33. All four local decks loaded hub content 495;
deck 2 used source 2, while decks 1/3/4 used source 0. Two samples confirmed
playing=true and advancing positions on all four decks. No audio was captured.
See [four-deck and transition evidence](acceptance/usb-hub-support-link-four-decks.json).

Next: repair source announcement/readiness recovery around native mode changes,
without treating the mod-maintained internal cache as the user's setting.
Native mode propagation includes deferred work; a cache write alone is not a
completed transition. Revalidate both directions and loaded/eject interactions.
The restart above was a diagnostic distinction, not an acceptable final workaround.

## Native mode completion recovery experiment

The completion-only candidate `119f7b11...` and traced candidate `e5a06560...`
exposed an additional mode blocker: the existing forced library cache makes the
native network setter skip an Off-to-On transition. The coordinating task stays
busy and queues a later Off request. The cache is therefore neither a reliable
user-setting detector nor safe to overwrite during native transition work.

The research overlay now records the mode supplied to the native network setter,
restores that shadow before native change detection, suspends cache maintenance
while the coordinated transition is active, and observes deferred completion at
`0x0140a088`. The injector then republishes withdrawn USB1 sources. Replaying
USB2 as well was rejected after it produced a duplicate row in candidate
`841f22df...`. Selective candidate `8661e1bb...` is under emulator validation.

This remains a diagnostic overlay. Cache check/write serialization, native owner
and callback lifetime, source generation locking, rapid pending transitions,
eject overlap, five-source behavior and full external-peer regressions are still
required. No physical AZ action, production integration or publication occurred.

Selective candidate `8661e1bb...`, PID 202006, completed an unloaded and a
single-loaded-deck Off/On/Off cycle with exactly three rows, without restart.
After the first cycle, member8 browsing and loading succeeded. Deck1 remained
loaded/playing and its position advanced across the second cycle. Its UI shows
a warning triangle after source republication; this needs investigation and is
not full loaded-source acceptance. See [structured mode evidence](acceptance/usb-hub-support-link-mode-recovery.json).
The generator without this option still reproduces the prior `d60ac8be...`
binary exactly, confirming that this experiment remains opt-in.

## Mode-cache and source-replay serialization

The mode adapter now holds a mutex across the internal-cache check and write,
with transition entry taking the same mutex before native work begins. This
closes the admitted-writer race left by checking an atomic busy flag alone.
The mutex is released before invoking the native request/completion callbacks.
Source replay checks eject reservation and updates announcement state under the
existing source mutex, shared with discovery and eject admission.

`test-link-mode-recovery.c` includes the actual adapter with mocked native
callbacks. Both ASan/UBSan and ThreadSanitizer passed 20,000 concurrent
cache-writer/native-transition cycles, plus reserved/pending-eject deferral,
selective slot replay and epoch idempotence. This proves the modeled adapter
behavior, not the native callbacks' threading/lifetime contract. Native task
serialization, completion overlap, shadow-state ownership and concurrent
external/native accesses to the firmware cache still require audit.

Serialized candidate `94c87351...`, PID 210526, completed an unloaded
Off/On/Off cycle with three source rows and no duplicates in either mode.
[Serialization evidence](acceptance/usb-hub-support-link-mode-serialization.json)
records the exact binary and test-source hashes. Loaded, external-peer and
rapid-change tests remain separate gates for this exact binary. In particular,
a new transition can start after an epoch is published but before injector
replay; replay must be deferred while that newer transition is active.

## Replay deferral across a newer transition

The replay adapter now takes the mode mutex before inspecting the completed
epoch and keeps it through the source-state update (lock order: mode, source).
If a newer transition is active, it leaves the old epoch unhandled and waits.
The saved mode and epoch are logged consistently. No native call is made while
holding these locks. This only governs replay-state publication; later native
mailbox dispatch and native task serialization remain separate concerns.

The adapter test now starts a newer request after a completed epoch but before
replay. The previous serialized header fails the new assertion; the corrected
header passes ASan/UBSan and ThreadSanitizer, including the existing 20,000
concurrent writer cycles. Native pending-request coverage must still be observed
at runtime rather than inferred from how quickly UI controls are pressed.

Epoch-deferral candidate `650fccab...`, PID 215251, completed an Off-start
On/Off/On UI sequence with three final source rows and no duplicates. Native
traces show each request started with busy=0: this does not validate overlapping
native requests. After the sequence, member8 browsing/loading succeeded and
deck 1 followed an isolated Beat Link master at 126 BPM, with synced/nonmaster
status and advancing playback telemetry. See [epoch and peer evidence](acceptance/usb-hub-support-link-mode-epoch.json).
No audio synchronization, hardware acceptance or full outgoing-status privacy
claim follows from this test. Pending-request overlap, eject interactions and
native callback lifetime remain open.

## Eject/mode overlap and finalized reservations

Candidate `650fccab...` completed a loaded whole-hub eject while a native Link
Off request was in progress. Logs place the Off request between source0 closing
and source1/source2 closing. All three backing/alias mounts disappeared, no hub
rows returned, and deck 1 cached playback telemetry advanced. Intermediate status
files had repeated clock values, so only log ordering establishes overlap.
See [whole-hub overlap evidence](acceptance/usb-hub-support-link-mode-eject.json).

Inspection found that `eject_reserved` intentionally survives successful eject
until physical disconnection. Treating any reservation as active work therefore
blocks mode recovery of other, unejected sources indefinitely. The research
adapter now defers for reserved sources only while `eject_done` is false; any
pending unmount still defers. Reserved sources themselves are always excluded
from replay, even if present-state information is stale.

The actual adapter test covers a finalized, still-reserved source alongside a
live source. It fails against the previous header and passes ASan/UBSan and
ThreadSanitizer with the correction. Candidate `a9709997...` is being checked
with partial eject followed by both mode changes; the whole-hub test above does
not substitute for that check.

Candidate `a9709997...`, PID 223649, passed individual HUB_LIB2 eject followed
by Off/On/Off while the ejected USB remained physically connected. Both other
sources recovered in each mode, with their mounts intact; the ejected source
stayed state2 with no backing/alias mount and did not reappear in SOURCE.
Member8 browsing and subsequent deck 1 loading also succeeded after the cycle.
See [partial-eject evidence](acceptance/usb-hub-support-link-mode-partial-eject.json).
This does not establish loaded-transition, external-peer or audio acceptance for
this binary; those remain separate regressions.

## Exact-firmware socket call inventory

`inventory-link-socket-calls.py` reconstructs a direct AArch64 PLT-call inventory
from the local ELF, emitting only addresses and conservative argument facts.
Raw disassembly is neither emitted nor committed. Five parser tests cover MOV
chains, control-transfer boundaries, unknown/multiple-destination instructions,
register width and unresolved registers.

For the pinned ELF, the inventory contains 30 direct setsockopt calls, five
recvfrom calls and one getsockopt call. All five receive sites pass flags0;
there is no recvmsg PLT import. Setters with option8 use SOL_SOCKET (level1,
SO_RCVBUF), not IPPROTO_IP/IP_PKTINFO. Three option8 calls needed separate review
of their callee-saved w20=1 helper paths. The lone direct getter requests
SOL_SOCKET/SO_ERROR. See [static inventory](acceptance/usb-hub-support-link-socket-static.json).

This narrows the receive-option and MSG_PEEK concerns for the exact native
binary. It does not close those gates generally: indirect/tail calls, raw
syscalls, library code and other mods are outside this inventory. Do not infer
that generic arbitrary-socket option virtualization or rejected-peek handling
has been implemented. Next, establish the native receive-helper call paths and
whether additional library/syscall paths can consume these same Link sockets.

## Expanded read-path audit

The inventory now includes B-to-PLT transfers, recv/read/readv/recvmmsg and
syscall imports. Seven parser tests pass. The exact ELF yields 110 targeted
BL sites and no targeted B sites: 30 setsockopt, five recvfrom, one getsockopt,
six recv, 39 read and 29 syscall calls. All inventoried syscall numbers are
AArch64 gettid178. There is no imported recvmsg/readv/recvmmsg in this scope.

Crucially, recv call `0x138f0b4` in helper `0x138f068` uses flags0x62
(MSG_PEEK|MSG_TRUNC|MSG_DONTWAIT); another path drains using flags0x60. Thus the
previous recvfrom-only flags0 finding must not become a whole-program no-peek
claim. The helper's socket ownership/port requires separate correlation. Generic
receiver `0x247c420` selects recv or recvfrom depending on source-address output
arguments. See [expanded static findings](acceptance/usb-hub-support-link-socket-read-static.json).

The opt-in `--udp-read-audit` overlay logs bounded metadata (operation, Link UDP
port, flags, buffer capacity and native caller) before existing read/recv/recvfrom
wrappers. It neither consumes extra datagrams nor changes options. Only local
ports50000..50002 with IPv4 SOCK_DGRAM are logged; no payload bytes are recorded.
It reports capacity exhaustion explicitly. This is diagnostic overhead, not a
production feature or proof about unobserved native paths.

Audit candidate `3e4f4095...`, PID 240378, observed all three Link UDP ports
using recvfrom flags0/capacity4096 at native return address `0x13fbfdc` during
startup, On/Off/On, external traffic and hub loading/playback. No read/recv call
on those ports was observed; the 32-pattern cap was not exhausted. A second
bounded external status-peer session overlapped the loaded track; the earlier
master session ended before loading and is not counted as loaded coexistence.
See [runtime read audit](acceptance/usb-hub-support-link-udp-read-audit.json).

This establishes coverage of the exercised native receive path, not all possible
call paths or generic socket API semantics. The audit is opt-in and adds syscall
overhead, so it is not a proposed production feature. Two active sources were
present after the prior partial eject; source numbering was compacted at restart
(member8 became member7/slot7). It must not be reported as three-source or
member8 acceptance for this run.

## Negotiated master handoff and reconnect

An initial two-session trial using the original `master` role was not accepted:
there were no wire handoff request/reply packets, the AZ retained master, and
Beat Link logged an unsolicited master assertion before ceding its own role.
Inspection of pinned Beat Link 8.0.0 explains the startup race: if getTempoMaster
is null, becomeTempoMaster asserts ownership directly instead of requesting it.
Discovery does not guarantee that VirtualCdj has received a master status yet.

The harness now has a `handoff` role that waits for an observed master (bounded
ten seconds), logs the previous player, requests transfer, and requires peer
master confirmation within five seconds. It applies the requested tempo after
confirmation. Empty-deck BPM 65535 now emits null effective_bpm instead of an
invented numeric tempo. The Java 11 target build passed against pinned dependencies.

With unchanged mod binary `3e4f4095...`, PID 240378, two corrected sessions of
external player 3 negotiated transfer from native deck 1. Wire metadata confirms
0x26 requests and 0x27 replies on 50001 in both sessions. The loaded hub deck
followed 129 BPM (138 synced/nonmaster states), continued playback through peer
absence, and followed 127 BPM after same-player reconnect (129 such states).
No Sync/Play input was added between the two corrected sessions. Playback
positions advanced across all four samples with the same PID and content 495.
See [handoff/reconnect evidence](acceptance/usb-hub-support-link-master-reconnect.json).

Native master reclamation was not seen in the captured late windows, ending
about 9.3/9.5 seconds after the last peer packet. Do not interpret continued
playback or eventual successful negotiation as proof of that fallback timing;
a native loaded baseline and longer dedicated observation remain required.

The subsequent core-assisted baseline (PID266441, no USB Hub Support mapping)
loaded the same track through native USB2 and completed a negotiated handoff.
After peer loss, deck statuses ended at 9.33 seconds even though the capture
continued; a later screenshot explicitly displayed local MASTER with playback
continuing. Do not equate cessation of status traffic with failed reclamation.
The no-core attempt could not decrypt the fixture library because QEMU lacks
device provisioning, so the comparison retained the core key provider with all
community mods disabled. This is not a completely stock-firmware baseline.
Exact fallback timing and the matching candidate UI observation remain open.
See [comparison evidence](acceptance/usb-hub-support-link-core-fallback.json).
Audio phase, real-CDJ compatibility and full 1152-byte status privacy remain
outside this result.

## Candidate fallback and five-source mode regression

The unchanged `3e4f4095...` candidate, PID272304, passed a timed visual fallback
check: local MASTER was absent in the screenshot at nine seconds after the
harness stop event and present at eleven seconds, without further UI input.
Playback advanced and 145 preceding states showed deck1 following129 BPM.
See [timed fallback](acceptance/usb-hub-support-link-ui-fallback.json).

Adding two protected COW fixtures extended the same process to five hub sources.
All five rows survived On/Off/On. The slot8 source browsed and loaded; four
loaded decks advanced with Link Off, and returning On unloaded decks3/4 while
decks1/2 continued. A subsequent player3 handoff made deck1 follow127 BPM.
See [five-source mode evidence](acceptance/usb-hub-support-link-five-source-modes.json).
The fifth-source icon is absent, the loaded-track warning persists, and all
decks used the same content495. This does not close different-track isolation,
five-source eject, cold-start Off or full-packet privacy gates.

The following five-source eject attempt failed preparation after loaded mode
changes: the native-detach bridge could not identify deck1's loaded track.
All five mounts stayed present and the request did not complete. This is a
functional release blocker, despite the preceding playback and source-row
successes. Investigate the native unload-completion/load-ledger interaction;
do not weaken the proof check. The opt-in `--mode-load-audit` variant adds
state/mode/media/epoch diagnostics without changing that guard. See
[failure evidence](acceptance/usb-hub-support-link-mode-eject-refusal.json).

Diagnostic binary `6037949122690df9e98262d3f9f614ee1ed4642b478c5dff0f0794b2e6448c7b`
then reproduced the relevant callback on PID286679. Deck1 loaded member8/slot3,
epoch1; turning Link Off emitted unload completion with state0, mode2 and
media `0x10200000702`, while the deck remained loaded and playing afterward.
The existing `hub_detach_unload_matches` path clears its source on that event.
This establishes the lost-ledger mechanism. A fix must preserve or establish
exact cached-media ownership across that native transition, including physical
connection generation; simply retaining a source index could misidentify an
old cached track after disconnect/replug. Raw diagnostic logs remain local at
`local/diagnostics/link-coexistence/mode-load-audit.log` with the matching
`mode-load-audit-after.json` telemetry. No functional fix is claimed yet.

The next opt-in `--mode-load-retention-probe` captures physical USB identity at
final load admission, keeps exact-media mode2 unload provenance and requires
that origin to match the reserved USB before creating a new detach ticket.
Binary `168a5cc3...`, PID293978, passed one loaded-track Off/On/Off plus whole-hub
eject: all five backing mounts and aliases disappeared, native completion was
observed, and cached playback advanced from29.195 to46.851 to51.476 seconds.
The final Off completion overlapped the eject sequence. See
[retention evidence](acceptance/usb-hub-support-link-mode-load-retention.json).
This remains research code. Required follow-ups include stale-origin rejection
after disconnect/replug, queued-load/source replacement correlation, two loaded
sources and integration. Do not infer complete physical provenance from this
single positive trial.

The same runtime binary168a5cc3 subsequently passed a negative/positive sequence:
after loading deck2 from member7/USB1 and entering mode2 cache, fixture A was
removed and reconnected. Its sdX and source slot persisted but generation/devnum
changed. Eject refused the old deck2 identity without unmounting any of the five
sources. A fresh deck2 load then permitted all-five eject and cached playback.
See [origin evidence](acceptance/usb-hub-support-link-load-origin.json).
The source now factors the reservation check into `link-load-origin-probe.h`;
its host tests pass ASan/UBSan, and generated binary851e6ff8 builds but is not
installed. Runtime acceptance belongs to168a5cc3, not that rebuild.

Further queued-load research confirms that loadCommonProcessing at0xa7d0f8
can either execute its processing directly or copy the location/options into
an asynchronous task (allocation size0x288). Capturing origin only at final
doLoadTrack is insufficient evidence for requests spanning a source replacement.
Correlate the queued request with its original connection before claiming that
race closed; do not use title/content ID as physical ownership.

The opt-in `--load-queue-probe` now traces the common processing closure at
0xa77390 and allowance closure at0xa69db0. A one-shot emulator marker introduces
an eight-second delay before either original closure. Binary60b0b848 passed
normal loading, then exposed a pending-request reconnect failure: the old
common-stage request reached allowance after removal/reannouncement, but deck2
remained in NOW LOADING approximately56 seconds after submission. No final load
admission was observed. Native mount validation alone therefore does not prove
clean cancellation across replacement. Investigate the allowance completion
and native cancellation contract before adding origin rejection at this stage;
silently returning from a closure could itself strand UI state. See
[queued reconnect evidence](acceptance/usb-hub-support-link-queued-replug.json).

Native cancellation research identifies an allowance-reply path:
LoadableState reply0xa69640 calls its listener then clears stored request/options.
The LoadController listener at+0x130 uses vtable0x29f2780; its reply thunk0xa7e3f8
forwards to0xa7de38. That wrapper copies location/options and result, dispatching
closure0xa7d890 directly or through a0x290 task. Result1 takes the denial branch
and notifies controller listeners with status8. This is distinct from the final
download failure path used by the existing eject guard. The queue diagnostic's
one-shot `/run/azhub-queue-reject-common` marker exercises this reply wrapper
before starting allowance processing, using the still-live common payload.
It is a fault probe, not an implemented stale-origin detector. Runtime UI cleanup
and subsequent fresh-load behavior must pass before using this as a cancellation
primitive; later stages may need different ownership/cleanup rules.

Runtime binary471be662, PID324233, confirmed that this injected rejection returns
to the browser and permits a fresh successful load. However, status8 displays
the recording-specific message "Streaming tracks can't be loaded while recording."
It must not be used unchanged for source replacement. Find the appropriate native
source-unavailable listener result and validate its cleanup, then implement origin
correlation across the actual asynchronous boundaries. See
[rejection primitive evidence](acceptance/usb-hub-support-link-load-rejection-probe.json).

Further static tracing narrows the pre-load notification contract. The native
DragAndDropTrackLoader handler0xa962b0 compares the failed location (including
its auxiliary vector and fields) to its pending location before clearing that
pending request through0xa95108. This correlation must be preserved: a generic
controller reset could cancel a newer request. The GUI LoadLockHandler entry
0x1a8d940 queues a task with vtable0x2d1a5f0; its executor0x1a8cbd8 only maps
pre-load result5,7,8 to caution IDs0x14,0x38,0x3a respectively. Result8 is the
recording message observed above. Other result values produce no caution in
this handler, but that alone does not establish their meaning or effects in
other listeners. There is no demonstrated source-unavailable result in this
pre-load GUI handler. Do not invent a result code or reuse recording denial as
the production cancellation contract. The next adapter must preserve exact
request notification/cleanup and provide truthful source-change feedback.

Binary1718255a adds delay-boundary physical origin traces and enumerates native
pre-load listener targets. On PID333514, fixture A changed from sdb/generation36464/
devnum38 to sdg/generation36888/devnum39 while retaining member7/slot3. Both
identity reads succeeded and the comparison rejected equality; the old native
media request nevertheless continued into allowance. The earlier queued interval
before closure entry remains uncovered. Deck2 already had content495 loaded, so
its loaded telemetry cannot establish completion of the replacement request.
See [queue-origin trace](acceptance/usb-hub-support-link-queue-origin-trace.json).

The same process registered19 pre-load listeners:16 used no-op0x9da950; active
callbacks were0xaad328 (InstantDoublesLoader),0xa96b40 (DragAndDropTrackLoader) and
0x1a8d940 (GUI). InstantDoublesLoader thunk0xaad328 forwards to0xaad018, which
clears its active byte+0x184 and calls0xaabe90, directly or via a queued task,
without checking the supplied location. Thus the normal loader's exact-location
comparison does not protect every listener from a stale cancellation. Preserve
request sequencing before broad notification; do not infer safety merely from
the normal browser retry trial.

The request-entry diagnostic now wraps native loadCommonProcessing in a nested
thread-local scope, capturing physical origin before native submission. Shared
task posting0x6d1838 preserves x1 as the task argument (verified against the exact
firmware entry); its diagnostic hook only traces common-task vtable0x29f7218 with
the matching scoped controller and task owner+0x238. The queued media is+0x240
and task serial+0x10. No task ownership registry or rejection is implemented yet.
Binarye8314181, PID339441, passed a direct deck1/content495 load: request origin
generation36888/devnum39 matched the common closure scope, then allowance,
final admission and completion result0 followed. This establishes the direct
capture path only. See [submission baseline](acceptance/usb-hub-support-link-request-submission.json).

The diagnostic now retains common-task origin in `link-request-tasks.h`, a
bounded64-entry value-copy registry keyed by task address, native serial and
controller. Entries are registered before native posting, consumed at common
closure entry or removed at either native destructor. A failed post uses saved
scalar keys for removal because the task may already have executed or been
destroyed before posting returns. Registry locks are released before native calls.
Duplicate addresses and exhaustion do not overwrite live entries. Missing
provenance remains diagnostic: automatic admission rejection is not implemented.

The native execution thunk0xa775e0 is only eight bytes; it must not receive the
current16-byte hook patch. Instead, the existing common closure hook recognizes
registered payload addresses, checks owner/media and the native serial, then
restores the captured scope for execution. Direct stack payloads are never
treated as task headers unless their preceding address matches a registered task.
The preliminary build404f5413 containing the thunk hook was never installed.
Corrected build9af5c715 compiles cleanly but is not runtime-tested. The actual
registry helper passes ASan/UBSan tests covering copied origin, serial/owner/media
mismatch, pointer reuse with stale cancellation, exhaustion, retirement and9999
reuse cycles. Async execution/destruction and submission-failure behavior still
require emulator coverage; later allowance/download ownership remains open.

Binary452f2f45, PID347650, passed native common-task execution with saved origin.
The one-shot force-async marker overrides only the scoped controller's
needsToExecuteAsync query0x6d0bb8, causing native allocation/submission rather
than synthesizing a task. Task0xffff8d647000/serial7 registered and consumed
generation36888/devnum39 provenance, then deck1/content495 completed normally.
See [asynchronous request evidence](acceptance/usb-hub-support-link-request-async.json).
This validates execution, not cancellation/destructor or failed-post paths.

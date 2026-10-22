# Share hub USB libraries through PRO DJ LINK

Current candidate evidence is summarized in [readiness](HUB-LINK-CANDIDATE-READINESS.md)
and [CURRENT-STATE](CURRENT-STATE.md). The research updates below are chronological,
including rejected designs and superseded pending gates. Final loaded-file controls
use MP3 track1884, native USB slot3 or public COLLECTION slot4 as appropriate;
earlier synthetic track495 pairing claims are superseded by the binding audit.
Independent software discovery/browsing/files are feasible and pass with eight
per-source computer identities41..48 and separate addresses. Actual CDJ acceptance
and target kernel transport loading remain physical gates.


## Goal and boundaries

External players must discover hub USBs, browse their libraries and load/play
tracks while local hub use and native USBs continue. Preserve independent
servers/caches, stable local labels,0.3.2 mode continuity and native four-local
Off/two-local On behavior. Exact EP147 tag/kernel/core support is unchanged.
No implementation or compatibility claim follows from this initial research.
Physical AZ actions require the current action-specific user authorization;
local emulator work may proceed. Publication needs a later decision.

Safe eject follows native shared-USB removal even with an external track loaded:
withdraw the source, close services and unmount without requiring the user to
stop/unload that external deck. Other sources remain available. The external
player handles source loss according to its cache/capabilities; uninterrupted
or full-track cached playback is not promised.

## Initial code facts

- Published0.3.2 is the immutable baseline. Production code is unchanged here.
- `linkfake.c` assigns eight local source pairs in order:
  `(7,3),(7,7),(8,3),(8,7),(7,8),(8,8),(7,2),(8,2)`.
  The native slot codes are USB=3, USB2=7, USB3=8 and SD=2. These are local
  implementation identities, not proof that every external CDJ supports them.
- Private member addresses are127.77.0.7/8 and TCP connects are redirected to
  loopback. No addresses are assigned to interfaces by `network-identity.h`.
  Advertising those loopback addresses externally cannot provide remote access.
- Private DB requests use codes0xe0..0xe7 and socket/message ownership tickets
  in `link-library-ownership.h`; remote clients are deliberately excluded.
  The reserved-requester handshake guard protects local connections from remote
  identity collisions. Do not globally disable these protections to enable sharing.
- `link-library-peer.py` is an adversarial media-info probe, not a browsing or
  playback client. `nfsd.py` is an old single-root experiment that maps arbitrary
  mount paths to one root; it must not become the multi-source export server.
- Outgoing private track identity redaction is intentional in0.3.2. A sharing
  design needs a proven public identity mapping; simply removing redaction would
  leak inaccessible private source references.

## Primary protocol sources inspected

- [Packet types](https://djl-analysis.deepsymmetry.org/djl-analysis/packets.html):
  discovery50000, media/status50002 and separate TCP metadata requests.
- [Track metadata](https://djl-analysis.deepsymmetry.org/djl-analysis/track_metadata.html):
  query TCP12523 for server port; stateful typed messages, DMST source-slot
  selection and paginated menus. The documented older-client requester1..4
  restriction is a compatibility warning, not a confirmed exact-AZ limit.
- [Media queries](https://djl-analysis.deepsymmetry.org/djl-analysis/media.html)
  and [startup](https://djl-analysis.deepsymmetry.org/djl-analysis/startup.html).

These independent reverse-engineering sources guide experiments. Native AZ
traces and model-specific physical tests must establish actual behavior.

## Feasibility gates before production changes

1. Observe the exact baseline emulator with native USBs, native Link On and an
   isolated independent requester. Capture device/media discovery, DB port query,
   root/playlist/track menus and a real track path. Validate actual audio-file
   transfer separately from metadata. Retain only hashes/counts in tracked notes.
2. Inspect native export/RPC/file-service routing and native STOP removal with a
   remote open file. Establish whether file handles/paths distinguish multiple
   roots, when transfers stop, and what keeps unmount busy. Do not use lazy/forced
   unmount as a substitute for safe teardown.
3. Determine representation limits: available public device identities, supported
   USB/SD slots, extended slots7/8, multiple device numbers per IP, client port
   lookup and per-requester state. Test collision handling with native1/2 and
   external3/4, plus modern5/6 where supported. Local7/8 must stay private.
4. Compare candidates: public per-source/member aliases, a multi-slot public
   library, or a consolidated public browsing namespace. These are hypotheses;
   none is selected. A consolidated browser must not silently replace the goal
   of separate discoverable USB sources. Obtain agreement for reduced scope if
   concrete limits prevent the original outcome.

## Implementation and validation gates

Once feasible, implement bounded generation-aware public routing and discovery,
with explicit source lifetime/withdrawal and independent caches. Do not borrow
native USB slots or select public identities already occupied by other devices.
Use an independent client, not a client taught a proprietary private-slot scheme,
to verify real discovery, root/playlist browsing, metadata and audio bytes.

Validate concurrent local/remote loads, multiple remote requesters, source reuse,
peer reconnect, mode transitions, removal while reading and individual/whole-hub
eject. Fresh requests after eject must fail; surviving sources/native USB must
work. Stale file handles must not resolve to a replacement stick. Retain existing
local UI/continuity/master/SYNC regressions. Exercise all eight source mappings
in portable routing tests and the largest available emulator fixture topology.

Final output: exact built candidate/package hashes, reproducible source and
protocol evidence, limits and separate physical AZ/CDJ trial instructions.
Model-specific actual-CDJ testing is required before claiming interoperability;
software-client file transfer is not physical audio or phase acceptance.

## Current checkpoint

Research started; no public export design selected, no production code changed,
no new emulator/hardware test or deployment performed. Next action: native USB
external-client baseline in the isolated emulator network and native export
service inventory. No feasibility conclusion has been reached.


## First live emulator inventory

Read-only localhost-emulator inspection confirms PID967182, exact0.3.2, kernel
6.6.138-dirty and EEEE. The last confirmed native mode is Off; the internal Link
cache is not used as a setting detector. TCP12523 listens on all addresses and
the current database service listens on46425. Three accepted loopback TCP pairs
are present. No UDP111/2049 binding is seen in this snapshot. This is not proof
that NFS/audio transfer is unavailable: enable native Link through UI and trace
an independent native-source transfer before drawing that conclusion.

Local raw socket inventory and a parsed summary remain ignored under
`local/diagnostics/link-sharing-native-preflight.*`. No physical AZ access or
production binary modification was performed.


## Independent native menu baseline

The new `LibraryPeer` calls the unmodified Beat Link8.0.0 APIs. On the isolated
emulator it discovers player1 USB media and retrieves7 root entries,20 playlist
entries and2547 tracks without private-slot mapping. The reviewed Utility image
confirms native Link On. Peer discovery sees native1/2 and mixer33. DB port query
reports46393, while an earlier socket inventory shows46425; do not assume a
single shared advertised/listening port without tracing the native routing.

The90-second transport/observer scope and client exit0; namespace cleanup runs.
Beat Link logs asynchronous CrateDigger mount-listener errors on shutdown, which
need harness cleanup. Menu success is not file transfer or hub-sharing proof.
The [baseline record](acceptance/usb-hub-support-sharing-native-library-baseline.json)
hashes ignored raw receipts. Next: clean client teardown, native audio-file
transport and native remote-in-use source withdrawal, then representation limits.


## Native file-transfer control in progress

The independent Crate Digger0.2.1 `FileFetcher` retrieves native `/C/` path
`PIONEER/rekordbox/export.pdb`:4419584 bytes, SHA256
`d463cbd2742ea8fa1ec741b2f9b204bfeb60bc50b1c2a4c4a4daa1bbb0b3a760`.
Read-only SSH hashes the native USB file identically. The client deletes its
temporary downloaded database and emits only count/hash. Root/playlist/track
menus still pass. This proves native external file access, not audio playback
or access to any hub source.

Kernel socket inventories still do not expose conventional RPC/NFS listeners;
no server-unavailable conclusion follows from that absence. The next trace must
identify actual on-wire RPC ports/procedures and mount-path translation rather
than guessing from `/proc/net` alone. Native audio bytes and remote-in-use eject
are still pending. The bounded scope finishes with transport/client/observer exit0, no transport
errors and namespace cleanup. The [file baseline](acceptance/usb-hub-support-sharing-native-file-baseline.json)
records exact receipt hashes.


## Independent native audio-file baseline

`LibraryPeer --fetch-native-audio` retrieves the complete selected sparse-fixture
MP3 through unchanged Crate Digger FileFetcher:10881597 bytes, SHA256
`295de8ef66f57a41778256d1f381d6ebe31084c2fb71b3e305d2acb955af9321`.
Read-only SSH hashes the native file identically. The temporary download is
removed; no music data is recorded or published by the harness.

A new header-only isolated RPC observer sees portmapper GETPORT on UDP111,
mountd v1 MNT on UDP49834 and NFSv2 LOOKUP/READ on UDP2049. It skips fragmented
IPv4 replies, so call/reply counts do not prove delivery coverage; complete-file
hash equality supplies the transfer control. The conventional service absence
in `/proc/net` did not predict on-wire availability. Native and SSH processes
share the same network namespace; kernel/user routing needs further inspection.
Do not infer service ownership solely from that socket inventory.

Native USB1's fixture root is sda1 without MODS; sdb1 holds the mods USB. A future
native eject control must target only the fixture USB, never the mods USB. Next
experiment should acquire a standard NFS file handle, hold it across native STOP,
and verify the native withdrawal and stale-handle behavior without a busy local
file descriptor artificially blocking unmount. This is not yet performed.

The bounded audio scope completes with transport/client/observer exit0 and
namespace removal. The [audio baseline](acceptance/usb-hub-support-sharing-native-audio-baseline.json)
records exact hashes and the header observer's fragmentation limits.


## Native held-handle eject control

`link-nfs-handle-peer.py` performs standard port lookup, `/C/` mount, component
LOOKUP and1024-byte READ without retaining file data. Native STOP1 held1.7s
then unmounts sda1; sdb1 and all five hub roots remain. Eight subsequent reads
on the same protocol handle return NFS status70 (STALE), at5-second intervals.
This establishes native remote-handle invalidation without a local descriptor
artificially pinning the mount. It does not simulate a CDJ advertising a loaded
or playing track; that stronger native admission/withdrawal control remains open.

After the final read, only the detached native fixture is virtually replugged
through a fresh child COW of its now read-only parent. All seven mounts and the
native database hash return unchanged. Transport/client/observer exit0 and the
namespace is removed. See the
[held-handle record](acceptance/usb-hub-support-sharing-native-handle-eject.json).
Next: external loaded-track status plus native eject, and public source/device
representation limits before selecting the hub-sharing architecture.


## Public representation and endpoint constraints

Inspection of the pinned Beat Link8.0.0 source yields three concrete client
constraints, not universal hardware limits:

- `CdjStatus.TrackSourceSlot` recognizes CD1, SD2, USB3 and COLLECTION4, but not
  the local adapter's extended slots7/8. An unchanged client cannot be taught
  custom hub slots as a shortcut to an independent interoperability result.
- DB port query12523 sends only the fixed service name, without a target device
  number. ConnectionManager caches returned ports by IP address.
- Client verifies the setup reply's server number against its target device.
  A shared endpoint that always identifies as player1 cannot become eight public
  devices merely by sending more keepalives.

An exact baseline trial now confirms the third constraint: native player1 root,
playlist and track menus succeed, but a subsequent root request to native player2
at the same IP fails with `Expected to connect to player 2, but welcome response
identified itself as player 1`. The control uses the same unchanged Beat Link
client; this does not establish a native-AZ defect or actual-CDJ failure. It
establishes that a same-IP alias shortcut does not pass this independent client.

The [startup analysis](https://djl-analysis.deepsymmetry.org/djl-analysis/startup.html)
documents older four-player and newer CDJ-3000 five/six numbering behavior. Do
not consume external deck slots3/4 (or5/6) to represent hub sticks. Separate
public storage/computer identities are a research candidate, not accepted design.

Two additional current open-source implementations are inspected only as primary
research references, without copying production code:

- [prolink](https://github.com/usr-ein/prolink/tree/ae895adc0a97fe2c791ed3bec0d5bcfd446b3fed),
  especially its `serve/dbserver.rs` endpoint and per-message media routing.
- [vynull](https://github.com/vynulldev/vynull/tree/e414ffff6a00e68f8ffd9f6ab819d6bdaa404a6d),
  which distinguishes CDJ-style and rekordbox-style file-service lookup, and
  documents a multiple-rekordbox-source collision caveat. These are leads for
  tests, not evidence that eight public providers will work on the AZ or CDJs.

### Designs still requiring proof

| Candidate | Required feasibility evidence |
|---|---|
| Separate public storage identities with distinct reachable endpoints | Native/external clients recognize each source; addressing does not collide with existing hosts; port query/setup route to the selected source; no use of deck identities |
| Several standard media slots on one public provider | Required slots are actually discoverable/browsable by unchanged clients; eight distinct sticks remain representable without native slot borrowing |
| Multiple computer/collection providers | Independent discovery, file-service and session routing work simultaneously without colliding with real rekordbox or other providers |
| Consolidated collection with USB folders | Changes source presentation; do not substitute this for separate discoverable USBs without user agreement if a concrete limit forces reduced scope |

Reachable aliases require collision-safe runtime address ownership and clean
withdrawal, not arbitrary subnet addresses or loopback advertisement. Metadata
sessions also need separation: the native DataStream communicator is keyed by
requester number, and previous experiments show replacement closes an existing
connection. Rewriting every external connection to one private requester would
break simultaneous local/remote use. Determine native requester capacity and
per-source/per-client menu state before choosing a proxy or native integration.

Next steps: public provider discovery/number acceptance, endpoint routing and
native communicator capacity; retain the remote-loaded-track eject control as
an explicit pending check. No production sharing code is implemented yet.


The shared-address control finishes with all three processes exit0 and namespace
removal. The [control record](acceptance/usb-hub-support-sharing-shared-address-control.json)
contains hashed local receipts. It rules out only the unchanged same-endpoint
shortcut for this independent client, not all multi-source export architectures.


## Computer-provider discovery probes

Two isolated sender-only probes investigate a public storage identity outside
native/external deck numbers. Probe1 sends40 computer-style announcements for
number17 with a research label. Two reviewed Source captures show the existing
native/hub rows but no new provider. This incomplete sender has no startup or
library service; the result is not a feasibility rejection.

Probe2 uses the conventional rekordbox model name, an initial announcement and
three-stage claim sequence before the40 periodic announcements. The observer
sees three native type05 responses on UDP50000, but the reviewed Source capture
still has no new provider row. Generic observer byte fields are not enough to
interpret those replies as successful assignment of number17; do not claim the
public identity was accepted. No DB port-query/metadata server is listening on
the experimental sender, so the complete connection workflow is still absent.

Next probe must receive/interpret discovery feedback and provide a bounded
port-query/metadata endpoint, recording native connection attempts. A native-USB
control proxy may be used strictly as a research provider control, not as a hub
implementation or permission to borrow native USB library slots in production.
Only a genuinely independent per-hub backend can meet the final goal.


## Empty public-provider service controls

Probe3 adds bounded TCP12523/1051 listeners and an empty metadata response path.
Native discovery replies arrive but no TCP connection is observed. Probe4 adds
the secondary type06 computer keepalive and a50002 receiver: the AZ sends
hello0x10 and receives a0x11 hostname response. No new Source row or DB connection
is observed in its timed capture. This narrows the missing workflow beyond UDP
announcements; none of these is a working export implementation.

Probe5 adds an in-memory empty RPC control on50111 (portmapper),40000 (mountd)
and2049 (NFS). The native AZ now performs GETPORT followed by mountd v1 EXPORT.
This is direct evidence of the distinct computer-provider file-service lookup,
rather than an assumption from a reference implementation. The initial minimal
EXPORT has no access-group entries; the reference advertises an explicit network
group. A follow-up must test that representation and interpret the native
availability transition before deciding whether this architecture is viable.
All control exports contain no real files; no native USB slot is borrowed.


Service-control scopes3/4/5 all finish with transport/client/observer exit0 and
namespace removal. The [service record](acceptance/usb-hub-support-sharing-provider-service-controls.json)
hashes the exact scripts and raw local receipts. Probe6 (explicit EXPORT network
group) is prepared locally but not run. The production mod and physical AZ remain
unchanged during this research.


## Public provider visible: bounded feasibility result

Probe6's explicit EXPORT network group advances the native client to MNT,
NFS READDIR/LOOKUP and a slot4 media query. Probe7 supplies valid empty-directory
responses and a192-byte media response to that query. A reviewed Source capture
now shows a separate **Hub USB1 Research** row with the native rekordbox icon.
Native opens TCP12523/1051 and sends setup0 plus metadata request0x3007.

This proves one public computer/library identity can be presented to an unchanged
native AZ client in the emulator without consuming an external deck number or
borrowing a native USB slot. It is not yet library browsing: the empty stub
rejects unsupported metadata and closes idle sessions after2seconds. The probe
serves no real files. Since media and empty-directory responses changed together,
do not attribute visibility to just one field without a separate control.
Both scopes exit0 and remove their namespaces; exact receipts are in the
[visibility record](acceptance/usb-hub-support-sharing-public-provider-visible.json).

Next gate: connect a real independent hub backend to a public endpoint, keeping
local/private sessions separate, then test multiple public providers with distinct
reachable destinations. Eight-source public representation and collision-safe
address ownership are not established. Native/external loaded-track eject and
actual-CDJ model tests remain required. Do not reduce the original scope merely
because this single-provider control succeeds.


## Native requester/session feasibility controls

Before routing a real hub backend, bounded controls test native requester IDs
without replacing local reserved5/6. The first two controls incorrectly expect
a generic response to the native two-argument peer handshake; a type0 context
reply is not a capacity refusal. The corrected standard one-argument setup and
12-byte argument-type array admit requester3 and7..14. Initial menus are offered.

With root magic0xffffff and the actual render total, requester3 renders its
root menu. Requester7 receives a2547-track offer but its render times out. A
reduced3/7 control reproduces that behavior; it is not caused only by opening
nine sessions. These controls do not establish a universal numeric capacity:
private member7 and per-request state may be involved, and a same-command
paired control remains useful. Do not claim extra IDs are usable based solely
on setup success. See the
[session controls](acceptance/usb-hub-support-sharing-session-capacity-controls.json).

All five scopes finish, close their sockets and remove the isolated namespace;
transport/observer exit0 while each incomplete probe exits1. Native967182 remains
EEEE with the exact0.3.2 ELF. No real hub backend is connected to the public
provider yet. Next design work must separate public session state without
replacing local/private communicators or existing external native-library clients.
Candidates include separate native DataStream/export contexts or an independent
bounded public protocol service over each stick's own database; neither has
been selected or validated. This does not reduce the original goal.


## Matched track-page requester control

The follow-up uses identical0x1004 track queries and0x3000 render requests for
requesters3 and7, with64-item pages and actual total2547. Both receive the same
2547-track offer. Requester3 returns all64 items; requester7 times out before
completing its page. This removes root-versus-track and one-item-versus-standard
page differences from the earlier comparison. It still does not establish a
universal numeric limit:7 is an existing private member, and membership/routing
may influence asynchronous replies. Do not turn this into a compatibility claim.

A first real per-hub public backend must not rely on those unproven extra IDs or
replace local5/6 or real external3/4 native-library connections. Separate reply/
session contexts, or an independent bounded public metadata service reading the
selected stick's database, remain candidates. A host-side proof can use the hub
stick's own plaintext export database as a control; it must be identified as a
prototype, not a deployable on-device implementation or complete live export.


## First real hub snapshot public track browser

A separate host-side metadata backend now reads a snapshot of hub0's own
`/media/usb/sdc1/PIONEER/rekordbox/export.pdb`, whose local hash matches the
emulator file. Crate Digger yields2546 tracks and185 playlist/folder entries.
The JSON and raw database stay local/ignored. No native USB library slot or
local5/6 communicator is borrowed. The live mod remains exact0.3.2.

The native AZ client discovers public provider17, sees2546 songs in Source and
renders the real track list in Browse. The backend maintains its own menu map
per connection. It handles the native short type-array dialect and8-argument
render messages; three adversarial framing test groups pass. Earlier strict12
parsing rejected the native dialect; that harness issue is corrected.

The [snapshot browser record](acceptance/usb-hub-support-sharing-snapshot-track-browse.json)
contains source proof, exact prototype source hashes and reviewed images. This
is a host-side immutable metadata prototype, not a live on-device export. The
playlist view is still blank, unsupported metadata/analysis requests cause
retries and no audio service/load exists. Folder-item type correction and
numeric playlist request logging are prepared for the next run. Scope10 exits0
and removes its namespace. Do not present this as complete sharing acceptance.

The matched native requester control6 also completes: requester3 renders a64-row
track page, while requester7 times out after the same2547-track offer. It removes
command/page-size differences from earlier controls but does not establish every
numeric or membership constraint. The standalone metadata backend avoids relying
on that unproven reuse path; its final on-device implementation is still open.

### Snapshot playlist navigation control

The v13/v14 isolated host-provider controls display root folders, nested
playlists and actual playlist tracks in the unchanged native AZ client. The
remaining framing fault was empty binary fields: when the preceding length
is zero, the wire omits the binary field entirely. Five stream-framing tests
and two synthetic playlist/session isolation tests pass. Frozen sources and
local screenshot hashes are in the playlist acceptance record. This remains
a host-side metadata prototype; audio, live backend integration, multiple
public sources and actual-CDJ interoperability are not established.

### Snapshot metadata query control

The enriched private snapshot retains2546 tracks and adds metadata names,
scalar values and analysis paths from the source PDB. The host backend handles
0x2002 with typed menu rows. Five framing and three backend tests pass,
including independent metadata/browser menu contexts and missing-track refusal.
In v15 the native client issues41 metadata queries without logged connection
errors and playlist navigation remains visible. This is request/transport
evidence, not visual metadata-panel or independent semantic acceptance. Audio
and live integration remain open. See the snapshot metadata record.

### Read-only fixture file transport

The host prototype now includes a bounded virtual NFSv2 tree over explicit
operator-selected read-only streams. Network path components never access
arbitrary host paths. It implements attributes, lookup, read, directory listing
and filesystem statistics; writes are refused and withdrawal closes streams
and invalidates handles. Three synthetic test groups cover exact full transfer,
EOF, invalid paths/handles, directory listing and withdrawal.

In isolated v17, unchanged Crate Digger0.2.1 FileFetcher downloads the entire
4,419,584-byte PDB and10,881,597-byte MP3 with exact emulator hub0 hashes. This
client shares the owned network namespace and requires the provider's explicit
fixture-self-client/portmapper111 control. It establishes independent library
wire compatibility, not an actual external CDJ, native deck load or playback.
The native emulator also mounts and navigates while the file backend is enabled.
All scopes exit0 and remove their namespaces. Live on-device backend, multiple
public identities, load semantics and real remote-loaded eject remain open.

### Native client selected-track load

The snapshot prototype implements0x2102 for the selected MP3 decoder1 only,
returning source file size/path and metadata. v18 reaches NFS reads but remains
NOW LOADING while unsupported cue requests loop. Typed empty cue/analysis
responses reconstructed from pinned Beat Link remove the loop in v19. The
native emulator deck finishes loading and shows title, duration and BPM;
play input advances remaining time from04:25.148 to04:17.152 in reviewed
screenshots. No audio recording or decoding verification was performed.
Waveform/beatgrid/cues/artwork remain absent. Other audio decoder mappings
require evidence and are explicitly refused by this initial control.

Five backend, five framing and three file test groups pass. Frozen sources
and local evidence hashes are in the native fixture load record. All network
scopes clean up. This does not establish live on-device serving, multiple
public sources, actual-CDJ compatibility or full-track playback.

### Bounded concurrent metadata sessions

The host provider previously handled TCP connections serially. It now owns
a pool of eight sessions per listener, each with its own menu map and row
negotiation. Two real-socket tests run on Windows and Linux: an idle first
client does not prevent a second client's setup/render, distinct menus remain
independent, capacity refusal closes excess sockets, and shutdown closes and
joins pending sessions. The shutdown control caught a Windows socket shutdown
issue; explicitly closing sockets resolves it on both platforms.

Isolated v20 repeats native selected MP3 loading with the concurrent provider;
title/duration/BPM display and no NOW LOADING match the prior control. The
peer exits cleanly and its stderr is empty. This is not simultaneous real-CDJ
acceptance. Live backend integration and multiple public source destinations
remain the next architecture gates; do not substitute the host prototype for
the required on-device candidate.

### Two-address public representation controls

The isolated transport now accepts explicit contiguous documentation-address
fixtures, defaulting to the original single address and permitting up to eight
only when explicitly selected. Both namespace validation and NAT source/ARP
suppression cover exactly the declared addresses. Three portable guard groups
exercise valid sets and reject extra interfaces/addresses or a wrong scope.
This grants no physical network access or production address allocation design.

Controls v22/v23 compare17 and41 on two addresses, first with shared then
distinct announced MACs; Source displays one public provider row. v24 compares
17 and18 and displays computer icon2 but still one row. Both providers receive
hello/RPC requests; the selected second provider receives media/DB requests.
The prototype does not yet answer activation0x46. Investigate the native
selection/activation path before attributing this to firmware capacity.
All scopes exit0 and remove the namespace. Separate public sticks, eight-source
representation and live integration remain unestablished; no scope reduction
is selected or authorized.

### Two distinct public sources and independent browsing

Activation control now handles first0x46 with0x16, one delayed repeat and
subsequent0x47 replies. v25 still shows one row while the first provider's
periodic identity continues cycling before activation. v26 fixes identity
from startup and displays two Source rows, computer1/provider17 and
computer2/provider18, on distinct fixture IPs. v27 selects each independently:
provider1 renders one selected real track; provider2 renders2546 tracks.
Both receive their own native setup and menu requests. The observed sequence
identifies the corrected behavior but does not isolate every causal variable.

All three scopes clean up; six portable suites/20 groups pass. These are
differently filtered snapshots of one PDB, not separate live sticks, and
provider2 has no actual file service. Next expand capacity/representation,
then live integration and concurrent audio/eject semantics. No actual-CDJ
or eight-source interoperability is inferred.

### Four-source visibility, incomplete browsing

Four explicit fixture addresses advertise providers41..44 with filtered
libraries101/202/303/404 tracks. v28 displays four simultaneous Source rows.
Reviewed menus2/3/4 show202/303/404 tracks; provider1 remains EMPTY and receives
no metadata TCP connection. Native normal UNLOAD TRACK clears the previously
cached source1 track, but v29 repeats the same failure. v30 restores source1's
real file export: it receives setup/root/track/render and metadata traffic,
yet the initial reviewed UI remains EMPTY. The later revisit overlaps scope
expiry and must not be treated as settled active-source evidence.

All scopes clean up. Four visible sources are established, four browsable
sources are not. Instrument numeric contexts and test clean emulator startup
to isolate source1 reconnection/cache versus render state. No capacity limit
or single causal explanation is inferred. CLI numbers45..48 are permitted
for subsequent isolated research only; no runtime acceptance is claimed.

### Corrected Ethernet ownership and cold-start controls

Normal localhost emulator application restart changes PID967182 to1758085;
0.3.2/59 hooks load again. Discovery row order/set changes, so earlier positional
screenshot filenames must not be treated as identity proof. No physical AZ
action occurs. The current restarted local view has two hub rows; no earlier
five-source runtime preservation is inferred.

Separate IP aliases still share an actual Ethernet source MAC. A strict
macvlan fixture adds only declared virtual children of the owned TAP, no
physical interface. Initial v35 packets reveal broadcast traffic from several
IPs still using the first child MAC. Binding every provider socket explicitly
to its matching child resolves this in v36: sampled Link UDP IP/MAC pairs are
unique/exact for all four. Source1 now renders101 tracks. Source2 is absent,
so stable four-source discovery/browsing is still incomplete. Six portable
suites/21 groups pass; all successful scopes clean up. v33/v34 fail strict
parent-schema validation before bridge readiness and clean up; this is not
a firmware result.

Next clean-start the corrected fixture, wait for settled discovery and inspect
actual provider header/count before recording menu success. Emulator MACVLAN
support is not physical4.4.194 support, and production address allocation,
conflict handling and live serving remain unimplemented.

### Four-provider browsing gate passes

v37 clean application startup with simultaneous providers remains unstable.
v38 starts providers5 seconds apart and displays consistent1..3 but not4 while
three file exports are empty stubs. v39 gives every provider a real independent
read-only file backend and staggers startup: Source displays all four, and
reviewed Browse header icons/counts match101/202/303/404. Every endpoint receives
its own native setup and menu/render requests. All scopes clean up.

The controls establish the measured successful conditions, not a uniquely
isolated cause. Inputs remain filtered copies of one PDB. Multi-source audio,
live integration, withdrawal/reconnect and actual-CDJ acceptance remain open.
Next expand to eight explicit fixture providers before selecting representation.

### Eight-provider discovery gate passes

v40 starts providers41..48 on eight explicit socket-bound virtual Ethernet
identities with real independent file backend objects,5 seconds apart.
Reviewed Source captures show1..7 and scrolled5..8. Every endpoint receives
media query, DB setup and0x3007 and replies successfully. Native computer
icons are blank above4 while labels remain visible. This is an observed
presentation limit, not a source discovery limit. Initial clockwise encoder
scrolling reaches its counter range and does not move; later counterclockwise
input reveals the lower rows. The captures, rather than command success alone,
establish visibility.

Six portable suites/22 groups pass. A new file test exercises eight independent
handle namespaces and rejects old handles after replacement while neighbors
remain readable. Scope exits0 and removes the namespace. These are snapshots
of one PDB, not eight live sticks; completed browsing/loading for5..8, live
mod integration, actual-CDJ behavior and physical-kernel virtual-interface
support remain open. No scope reduction is selected.

### All eight public identities have native browse evidence

v41 selects5/6/7 and reviewed menus show505/606/707 tracks, corroborated by
the corresponding endpoint track/render requests. Its attempt at8 stays in
Source and issues no track query; those filenames are not accepted as browse
evidence. v42 uses the verified Source8 selection and actual arrow; native
Browse shows808 tracks and endpoint8 receives the requests. Together with v39,
all eight public identities have browsing evidence, across separate controls.
This is not eight live disks, eight deck loads or simultaneous audio acceptance.
All scopes clean up; production and physical AZ are unchanged.

The local prepared kernel config hash is recorded with UTS_RELEASE4.4.194:
MODULES=y, MODVERSIONS disabled, PREEMPT=y, MACVLAN/IPVLAN disabled. Research
a compatible standalone GPL module or alternative transport; do not infer
physical support from emulator6.6 or broaden firmware. Next advance live
backend/transport integration and lifecycle/concurrency validation rather than
repeating successful snapshot browsing controls without a new reason.

### Target-kernel transport module build feasibility

The unmodified vendor GPL MACVLAN driver builds as a separate named module
against the prepared4.4.194 tree/config using pinned Linaro6.3.1. The builder
checks source/config hashes and release, uses explicit gcc to bypass an old
vendor Python wrapper, and enables CONFIG_MACVLAN_MODULE only for the external
module's header helpers. It never edits the target config. The only matching
conditional in prepared include/ is the macvlan_dev_real_dev inline helper.

The stripped module is29,592 bytes with expected vermagic. Static GPL export
declarations are found for all79 undefined symbols. There is no Module.symvers,
so actual runtime export resolution/ABI/load acceptance remains open. Nothing
is installed, packaged, loaded or published. The local binary stays ignored;
only builder/auditor and factual records belong in Git. See transport build
notes and the module-build record. Keep future runtime loading under exact
firmware/kernel guards and normal USB-mod lifecycle. Live library backend,
address allocation/conflicts and integrated regression tests remain required.

### First live-file C component

`public-file-root.h` introduces caller-owned read-only roots and bounded
per-operation leases. Component walking refuses absolute/ambiguous paths and
symlinks. Final O_PATH pinning plus regular-file validation avoids opening
non-regular nodes and path replacement during reopening. Each read is capped
at8192 bytes, each root at16 concurrent operations. Withdrawal is nonblocking:
new operations fail, accepted reads finish, and the root closes on drain.
Replacement generations increase monotonically. No remote deck lifetime is
held as a lease. Library authorization and opaque NFS/instance handles remain
separate adapter duties; no network service is yet connected to this component.

The new test joins the standard portable matrix. All32 cases pass in each
plain, ASan/UBSan and TSan run; hook registration is independently captured
on WSL because the C/C++ container lacks Python. TSan needs an ephemeral
container exception for per-process personality; no global ASLR change occurs.
The pinned Linaro AArch64 test needs only GLIBC2.17 and runs successfully in
the localhost emulator against synthetic temporary files. The copied binary
is removed. Physical AZ, published release and mod behavior are unchanged.
Next wire real file service/handle generation and native ejection coordination,
then live library metadata and transport ownership.

### C NFS READ body and live emulator samples

`public-nfs-read.h` captures source-owned references with generation, device
and inode, and encodes NFSv2 READ bodies. A replaced inode, foreign source or
withdrawn root is rejected. Partial/EOF/zero padding and bounded requests are
tested. Leases end before network send so a slow client does not hold the root
through its network lifetime. RPC envelope and opaque handle authorization
are still separate adapter responsibilities.

All33 portable cases pass in each plain/ASan+UBSan/TSan matrix, plus the official
hook-registration check in WSL. The Linaro AArch64 test passes in the emulator.
Its explicit emitter reads4096 bytes of the live hub0 PDB and8192 bytes of audio;
a host test decoder verifies standard body fields and exact reference bytes.
This is not a network RPC client, complete-file or playback acceptance for the
C reader. The temporary executable is removed and physical AZ is untouched.
Next integrate RPC/handle/catalog ownership and real source withdrawal, while
completing live metadata and public transport. No release/package changes.

### C RPC READ dispatch control (2026-10-20)

Implemented bounded ONC RPCv2 datagram decoding and NFSv2 READ dispatch over
source-owned references. Reject malformed calls, unsupported writes and invalid
AUTH_NONE/AUTH_SYS syntax before lookup; never trust supplied OS credentials.
All35 cases pass in plain/ASan+UBSan/TSan; hook registration passes separately.
Pinned AArch64 synthetic dispatch tests pass on the guarded localhost emulator.
See [the record](acceptance/usb-hub-support-sharing-public-rpc-read.json).
Next connect instance-scoped opaque handles, directory/mount procedures and socket
transport before claiming a live C file service. Runtime integration and physical
acceptance remain open; published0.3.2 is unchanged.

### Source-owned C opaque file catalog (2026-10-20)

Added bounded independent catalogs with instance nonce/generation handles, stable
duplicate registration and no eviction or retained descriptors. Targeted GCC/Clang
plain, ASan+UBSan and TSan controls pass, including concurrent withdrawal and
neighbor isolation. See [the catalog record](acceptance/usb-hub-support-sharing-public-file-catalog.json).
Before integration, provide runtime nonce generation, trusted live path registration
and a capacity policy above256 entries; directory handles/lookup and RPC sockets
remain open. No full36-case or AArch64 catalog acceptance is claimed.

### C UDP/catalog READ control (2026-10-20)

Connected nonblocking UDP transport to source-owned catalog resolution and C READ
dispatch. Mandatory peer policy, truncation refusal and release-before-send are
exercised. Targeted plain/ASan+UBSan/TSan and pinned AArch64 emulator loopback
controls pass. An independent handwritten Python client reconstructs30001
synthetic bytes and verifies EOF, write refusal, altered-handle and withdrawal
STALE. See [the UDP record](acceptance/usb-hub-support-sharing-public-rpc-udp.json).
This is localhost transport evidence, not unchanged Crate Digger/public provider
or actual-CDJ acceptance. Next implement mount/lookup/directory operations and
full-library capacity before live source/runtime integration.

### Compact dynamic file catalog (2026-10-20)

Replaced the fixed256-reference layout with exact-length paths and geometric
entry allocation, path hash buckets and direct bounds-checked handle resolution.
Per-source65536-reference/8MiB path budgets reject exhaustion explicitly without
evicting issued handles. A synthetic1025-hard-link-path control verifies each
read and stable early handles through growth. Catalog/UDP plain/ASan+UBSan/TSan
and independent Python transfer regressions pass. See [the growth record](acceptance/usb-hub-support-sharing-public-file-catalog-growth.json).
These limits are implementation budgets, not validated USB-library support claims.
Actual-library/global memory, allocation failure and exact boundary controls remain
open alongside directory/mount and runtime integration.

### Authorized C directory operations (2026-10-20)

Added explicit directory/root references with confined operation leases and
GETATTR/LOOKUP bodies over pre-registered catalog nodes. Existing unregistered
files remain hidden; symlink/type/traversal and replacement/withdrawal controls
pass. All38 cases pass in each plain/ASan+UBSan/TSan matrix, hook registration
passes separately and the pinned AArch64 synthetic tree test passes in emulator.
See [the tree record](acceptance/usb-hub-support-sharing-public-nfs-tree.json).
Next connect RPC dispatch and native UTF16LE names, then READDIR/mount/portmapper
and trusted live catalog population. No independent native tree client or runtime
integration acceptance is claimed yet.

### Native-style directory RPC and Unicode control (2026-10-20)

Connected GETATTR/LOOKUP RPC using shared envelope/auth validation and strict
UTF16LE name decoding. Independent Python UDP client traverses accented/emoji
folders from the root handle, queries attributes, reconstructs30001 bytes and
checks invalid surrogates, write refusal and withdrawal STALE. Updated38/38/38
matrices, hook registration and pinned AArch64 tree/RPC test pass. See
[the tree RPC record](acceptance/usb-hub-support-sharing-public-rpc-tree.json).
Next add READDIR/mount/portmapper and unchanged Crate Digger controls; runtime
public addressing, live catalogs/metadata and final concurrency/eject remain open.

### C directory listing and mount facade (2026-10-20)

Added bounded READDIR with stable catalog cookies, strict UTF8-to-UTF16LE
serialization, read-only STATFS and portmapper/mountv1 facade. UMNT acknowledges
only the remote peer; it never withdraws the source. Independent Python wire
client completes discovery/mount/list/lookup/full30001-byte READ and withdrawal.
All39 portable cases pass in plain/ASan+UBSan/TSan, with separate hook registration
and pinned AArch64 mount/export controls. See [the mount record](acceptance/usb-hub-support-sharing-public-rpc-mount.json).
Next validate unchanged Crate Digger against C, then live library/metadata and
public transport/runtime integration. No actual-CDJ acceptance is inferred.

### Unmodified Crate Digger against C service (2026-10-20)

The independent Crate Digger0.2.1 FileFetcher library mounts/traverses/downloads
from the C export in a fresh loopback-only namespace. Synthetic Unicode30001-byte
control, complete4419584-byte PDB and10881597-byte MP3 fixtures match source hashes.
PDB/audio are ignored local copies of prior emulator hub0 files, not live roots.
The port111 harness mode refuses the initial namespace and requires only active
loopback. The first sysfs-based isolation check aborted before launch; corrected
controls query actual namespace interfaces and pass. See
[the C client record](acceptance/usb-hub-support-sharing-c-crate-digger.json).
Next populate trusted paths/metadata from live source state and integrate owned
public transport/lifecycle; complete local/remote concurrent and external-loaded
eject controls remain open. No actual-CDJ or playback acceptance is inferred.

### Independent live-catalog parser research (2026-10-20)

Reviewed existing source-owned native library routing; public metadata must not
collapse external sessions onto private5/6. Added a bounded classic-PDB Python
reader as an independent research oracle using the pinned Crate Digger format
reference. All2546 IDs, titles, audio paths and analysis paths match the prior
Java snapshot; four synthetic structure/cycle/string/deletion groups pass. See
[the reader record](acceptance/usb-hub-support-sharing-pdb-path-reader.json).
This is not deployable C, complete metadata or path authorization. Next port
the bounded traversal to source-generation-aware C and populate trusted catalogs,
then implement the independent live metadata backend and runtime integration.

### Incremental source-owned C PDB reader (2026-10-20)

Ported active track traversal to bounded C with one page per worker step, visited
page/ID guards, strict string bounds and terminal cancellation. Source reads use
short generation-owned leases with inode/size/mtime/ctime validation before/after
each chunk; synthetic withdrawal/replacement refuse safely without persistent
descriptors. All40 portable cases pass per matrix plus hook registration. Pinned
AArch64 code reads live emulator hub0 PDB and all2546 four-field rows match host C
and Crate Digger oracle. See [the C reader record](acceptance/usb-hub-support-sharing-c-pdb-reader.json).
Next stage and validate metadata before publication, authorize catalog paths and
add remaining metadata/playlists. Runtime public integration, concurrent use and
external-loaded eject remain open. No actual-CDJ or playback acceptance is claimed.

### Staged PDB-owned file catalog (2026-10-20)

Connected C track traversal to independent source-owned file catalogs. Only
listed audio/analysis, known DAT companion EXT/2EX paths, PDB and required parent
nodes are registered. Unsafe paths fail, optional missing media is counted,
and readiness requires full traversal plus final source fingerprint validation.
Cancellation/neighbor, hidden unrelated files and changed-PDB controls pass; all41
portable cases pass per matrix plus hook registration. Pinned AArch64 staging on
live emulator hub0 yields2546 tracks,10329 nodes,408643 path bytes and16384 entry
capacity. The image lacks2529 audio paths (17 present), limiting audio coverage.
See [the staging record](acceptance/usb-hub-support-sharing-library-file-staging.json).
Next serve this catalog through RPC, detect post-ready source mutations and
complete independent metadata/public transport/lifecycle and concurrency/eject.

### PDB-derived C network file export (2026-10-20)

Connected staged catalog to C RPC with BUILDING-state refusal and PDB fingerprint
revalidation before valid file/mount operations. Changed source retires export;
malformed/write requests cause no fingerprint I/O. Two unchanged Crate Digger
clients download full PDB/audio host fixtures with exact hashes; successful READ
intervals on distinct UDP ports overlap, proving interleaved servicing. Unrelated
files remain hidden and post-ready temporary PDB mutation returns STALE. Updated
41/41/41 matrices, hook registration and AArch64 synthetic controls pass. See
[the export record](acceptance/usb-hub-support-sharing-library-file-export.json).
This scope is loopback-only with copies/one audio. Next use live emulator network
file roots, complete metadata/discovery and public transport/lifecycle, then
concurrent native playback/external loads/eject and final candidate audit.

### Standalone live emulator C file service (2026-10-20)

C AArch64 server now stages and serves live hub0 through an owned guest namespace
and MACVLAN, with separate111/2049 namespace ownership from native services.
Initial MACVLAN creation lacked a loaded driver; a local unmodified emulator6.6
module builds/loads and enables the scope, then is unloaded. This does not prove
target4.4 runtime compatibility. Two unchanged Crate Digger clients transfer full
PDB/audio with exact hashes and overlapping successful READ intervals. Service
withdrawal drains; USB unmount/external-loaded status and local playback were
not tested. All owned namespaces/interfaces/files/module/QMP objects are removed
and verified. See [the live C record](acceptance/usb-hub-support-sharing-live-c-file-service.json).
Reconstruct using test-live-library-network.py with explicit local server/module/
class artifacts and a new ignored evidence directory. Next complete independent
on-device metadata, public addressing/lifecycle and integrated concurrent/eject
controls; no new mod/package or actual-CDJ acceptance is claimed.

### C track metadata scalar/reference extraction (2026-10-20)

Expanded C tracks with comment, file size, artwork/key/label/genre/album/artist
references, bitrate/BPM/duration/rating. All16 fields match2546 Crate Digger v3
snapshot records; pinned AArch64 reads live emulator PDB and matches host output.
Nonzero synthetic metadata assertions and updated41/41/41 plus hook checks pass.
See [the metadata record](acceptance/usb-hub-support-sharing-pdb-track-metadata.json).
Next retain compact per-source indexes, resolve names/playlists and implement
compatible metadata TCP/menu replies and file-type/analysis support before public
runtime integration and final local/external concurrency/eject validation.

### Compact source-owned track index (2026-10-20)

Added scalar rows and offset-based strings, sealed generation/ID lookup, duplicate
refusal and retirement without filesystem leases. Synthetic2048-track growth,
neighbor isolation and concurrent retirement pass. All42 cases pass per portable
matrix plus hook registration. Host/live AArch64 index output matches all2546
Crate Digger16-field rows. Row/text buffers reserve770048 bytes for this fixture,
excluding allocator/bookkeeping; full/global budgets remain unvalidated. See
[the compact index record](acceptance/usb-hub-support-sharing-compact-track-index.json).
Next connect staged source lifecycle and referenced names/playlists, then metadata
TCP/discovery and final local/external concurrency/eject integration.

### Joint file and metadata source lifecycle (2026-10-20)

Integrated compact track index with the same staged traversal as file registration.
Seal precedes final PDB revalidation and public readiness; both accessors reject
partial/canceled/failed sources. Cancellation or detected PDB mutation retires
metadata and files together; missing audio retains metadata only. Updated42/42/42
and hook checks pass, with AArch64 synthetic/living2546-track staging and unchanged
Crate Digger exact full-file regression. See [the lifecycle record](acceptance/usb-hub-support-sharing-library-metadata-lifecycle.json).
Next parse referenced names/playlists and implement metadata TCP against this
source lifecycle, then public discovery/address/runtime and final concurrency/eject.

## Joint entity lifecycle checkpoint

File catalogs, track metadata and named/playlist indexes now stage and retire
together. See [acceptance](acceptance/usb-hub-support-sharing-entity-lifecycle.json)
for45/45/45 host cases, standalone AArch64 live-library staging and independent
file-transfer regression. The next implementation gate is the bounded metadata
TCP wire/session adapter backed by these live per-source indexes. This does not
replace the remaining discovery/runtime, concurrent local/remote, withdrawal
and model-specific interoperability gates.


## Native loaded-source withdrawal control

The managed diagnostic runner now has explicit `--native-status-observer`,
`--native-status-held-file` and `--native-status-eject` controls. The last option
requires both preceding options and selects a120-second synthetic status scope.
It installs only the explicit emulator diagnostic, invokes the normal supported
AZ-MODS post-start re-announcement, and asserts native export registration before
clients. It does not install the public sharing runtime or touch physical AZs.

The withdrawal helper verifies the current native fixture by sysfs USB1/1-1,
/C/ alias, exact QEMU nativecontrol2 device/bus/port/block-node association,
separate mods USB2/2-1, seven original mounts and database hash. It requires a
successful real1024-byte NFS read and current-PID native accepted status for
player3/source1/slot3/track495 before injecting ordinary STOP1. It checks neighbor
mounts, continued synthetic status, retained-handle STALE responses and exact
owned-device replug/hash restoration. Network cleanup also runs if native
restoration reports a failure; a failed restoration remains a failed control.

This is an emulator-specific baseline, not a public-hub eject test, actual CDJ,
audio renderer or promise about player cache behavior. The helper assumes only
the verified owned topology and refuses other arrangements. Normal four/two-deck
regressions and independent public-hub loaded withdrawal remain separate gates.
Do not infer physical port identity from sdX enumeration or copy these virtual
fixture operations to real hardware.

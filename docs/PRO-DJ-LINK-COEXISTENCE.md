# PRO DJ LINK coexistence research

Research date: 2026-10-17. Scope: EP147 firmware 2.00, exact supported tag
`ep147-9e9454e357389ece`. This is a design investigation, not an implementation
or acceptance result. No AZ connection or hardware operation was performed.

## Proposed target

Allow the AZ to use hub libraries locally while participating in ordinary PRO DJ
LINK with external players. Initially retain the stock Link-mode deck behavior:
two local decks. Preserving four local decks while external players participate
would be another feature with additional identity and firmware constraints.

External players browsing or loading a hub stick is a separate export feature.
Coexistence must not accidentally expose the private hub servers.

The best first experiment is to prove whether native remote-media machinery can
use two additional private member identities alongside physical players. This is
the least invasive candidate that retains the accepted per-source library design.
It is a feasibility gate, not evidence that player numbers 5/6 are supported.

## Evidence from current source

| Boundary | Current behavior | Consequence when physical Link is enabled |
|---|---|---|
| UDP | `linkfake.c`, `foreign_link_packet`, `sendto`, `recvfrom`, `recvmsg`: Link-signature packets outside 127/8 are dropped, except destinations matching a fake IP | Global filtering must become origin- and destination-aware |
| Synthetic transport | One loopback socket per synthetic member; source IP rewritten by matching loopback source port | Keep trusted socket/endpoint identity separate from packet player number |
| Member identity | Configuration accepts only members 3/4; automatic mapping is 3,3,4,4,3,4,3,4 | A real member 3/4 can collide in native member tables even if IPs differ |
| IP labels | `network-identity.h` selects the highest usable subnet addresses without occupancy checks | A physical peer at either address can be mistaken for a fake peer |
| TCP | `connect` redirects any stream connection whose destination IP matches a fake IP to the same port on loopback | IP matching alone is insufficient once native traffic is allowed |
| Request translation | Traced client connections rewrite setup identity, requester 5 and media slots to e0..e7 | Rewrites must remain restricted to private connections; physical traffic must remain byte-identical |
| Library dispatch | `hubservers.h`, `hub_matches`: private slot code and presence select a hub server | Private slot codes are routing labels, not an access-control boundary |
| Library ownership | Each source retains its own MusicLibraryServer, PersistentStorageServer, SQL factory and tagged cache | Preserve this design; do not borrow native USB slots |

An additional isolation issue deserves explicit treatment: `hub_matches()` does
not check connection provenance. Hub servers are inserted into the native shared
dispenser. If a physical connection can submit an e0..e7 request to that dispenser,
the current match predicate does not reject it because it is external. Whether
native protocol admission lets such a request through is untested; hiding source
announcements alone does not establish that the hub library is private.

The current `foreign_link_packet()` is a PLT-level application policy, not a
kernel firewall. A coexistence audit must inventory all send/receive paths,
including connected UDP, `sendmsg`, interfaces and file-service/RPC traffic.
The installed Community wrapper hooks nine PLT entries; `recvmsg()` exists in the
source but is not among that wrapper's listed PLT hooks. Do not infer complete
socket coverage from the existence of a wrapper function.

## New local firmware findings

These facts were extracted using existing `Probe.java` and read-only Ghidra
analysis of `/home/icast/ghproj/EP147`. Proprietary decompiler output remains
outside the repository. Function names inferred from embedded type/debug strings
are distinguished from runtime validation; no new runtime test was performed.

| Address | Static finding | Research implication |
|---|---|---|
| `0x013fa7a8` | `network::SystemManager::changeProDjLinkMode(bool)` updates `0x03f86a08`, has an initialized guard at object +0x3b8, and schedules asynchronous/delayed work | Writing the cache is not equivalent to executing mode transition logic |
| `0x013f0f40` | Calls the preceding setter | Useful propagation boundary, not yet a persisted-setting accessor |
| `0x013de6e8` | `prodjlink::LinkDeviceManager::changeProDjLinkMode(bool)` creates an asynchronous task, vtable `0x02bbf550` | Link device/member state changes separately from the network cache |
| `0x013e49b0` | That task chooses state values 1/2/4 according to object +0x400 and supplied boolean, calls `0x013e4498`, then `0x013dec28` | Determine state enums and resulting identity/advertisement changes before changing this path |
| `0x01408ef0` | A mode task records requested/effective values at owner +0x4a2/+0x4a4, uses in-progress/initialized flags +0x4a1/+0x4a0, calls both network and LinkDeviceManager changes, then a transmitter virtual operation +0x168 | There is coordinated mode propagation and deferred handling; intercepting one cache cannot reproduce it |
| `0x020f1410` | Typed `gui::DeckStateInfoManager::onSettingChanged(...ProDjLinkMode...)` copies the supplied 32-bit value into an asynchronous task | Candidate observation boundary for the real setting value; initial-state availability still needs research |
| `0x020f1ae8` | Its task stores mode at owner +0xfc. When mode equals 1, selected-deck values 2/3 are changed to 0/1, with callbacks | Direct static evidence for selection restrictions separate from the internal Link cache |
| `0x021a7720` | Typed `hui::deck::DeckSelector::onSettingChanged(...ProDjLinkMode...)` queues another task | A second UI consumer must be included in any four-deck proposal |
| `0x021a75c0` | That task calls `0x021a88e8` with 0 for mode 1 and 1 otherwise, with additional listener notifications | The exact UI enable operation and physical controller behavior remain to be mapped |
| `0x013f22b8`, `0x013f1508` | Static readers of `0x03f86a08`; first contains state/identity negotiation branches and mode-dependent choices | Retain the native networking state machine for external Link rather than forcing its cache independently |

The earlier attribution of `0x013f1508` as incoming member admission was explicitly
withdrawn in `GHIDRA-FINDINGS.md`. This investigation does not reinstate it.
Cross-subnet failure is observed historically; the exact incoming admission
function still needs identification.

## Architecture options, ranked for investigation

### 1. Additional private members with isolated transport

First test candidate member IDs 5/6, or another explicitly validated private
representation, while leaving physical member numbers unchanged. Number 5 in the
current requester setup is a database communicator identity; it does not prove
that native UI, mount, status or member tables accept a remote player 5.

Audit `0x013db0b0`/`0x013db0b8`/`0x013db0c0`, capability table `0x02bc1df0`, all
member-indexed arrays/bitmaps, keepalive admission, status lookup, media registry,
browser grouping, `/media/playerNN` path construction and track ownership.
Look for checks limiting players to four, special rekordbox identities and array
lengths before injecting new values. Current mod-side checks and `player-3`
indexing also need replacement with an explicit private-member map.

Private source keys should be `(domain, member, slot, generation)`, where the
domain distinguishes physical from hub sources. TCP ownership should be tracked
per connection lifetime, never inferred only from player number or reused fd.
Use dedicated loopback-only library endpoints or validated connection provenance
at dispatch; reject e0..e7 from every physical session. Keep all per-source
servers and caches independent.

Prefer private loopback labels if native admission can be narrowly scoped to
trusted injected sockets. That requires finding the actual admission gate first;
do not revive the rejected subnet hook. If same-subnet labels are temporarily
required, using apparently unused addresses is insufficient: maintain a distinct
private routing context so a real device at the label address still reaches the
physical network. ARP probing cannot guarantee future noncollision.

Allow native physical traffic through unchanged. Swallow only verified synthetic
destinations/messages; prevent synthetic keepalive, status, media and requester
identities from reaching physical sockets. For a local deck playing hub music,
inspect outgoing status: it can contain the private source member/slot. Define
and test an honest externally meaningful status representation rather than
leaking e0..e7 or pretending the track is on native USB1. Beat/master/tempo traffic
must remain native; the correct track-source encoding is unresolved.

This option fails its gate if native tables cannot represent private members
without pervasive expansion. Do not turn an unchecked member renumbering into
a release candidate.

### 2. Private local media namespace, bypass synthetic Link membership

Keep the native physical Link member table entirely stock. Publish hub sources
through a mod-owned UI/media namespace and route browser, track metadata,
settings, audio paths and loading directly to independent local servers.
This removes competition for physical player numbers and subnet labels.

It requires deeper research into source enumeration and track identity rather
than reusing native remote-member announcements. Existing media lookup
`0x00e5e7d8`, per-source identity correction in `h_mlookup`, and mount orchestration
`0x0139b3b0` -> `0x0139afd0` -> `0x01399f10` are starting points. Audit every
consumer of LocalMediaID and full track identity before choosing private IDs.
This is the architectural fallback if option 1 encounters fixed native limits.
It is larger work but offers a clean long-term separation.

### 3. Reserve physical player numbers for hub sources

Keeping 3/4 reserved is only a constrained setup, requiring external players to
avoid those numbers and the AZ's own advertised numbers. It may leave no valid
layout for the intended equipment. A peer can join or renumber later, so detection
must precede admission; withdrawing loaded sources on a collision is not safe.
Do not market this as general parallel Link support. It is useful only as a
controlled emulator experiment to separate transport bugs from namespace limits.

### 4. Dual firmware instances or protocol gateway

Running a separate native media/member context or translating physical peers
through a proxy might preserve private 3/4 internally, but duplicates state,
requires full request/status/mount translation and risks collisions in shared
singletons. Network namespaces alone do not split EP147's in-process member
tables. This is more invasive than the preceding options and is not recommended
as the first implementation.

## Concrete next work and validation gates

1. Create an emulator-only diagnostic for member admission and lookup. Enumerate
   all relevant array capacities and player-to-member conversions before trying
   IDs beyond four. Trace current 3/4 as the baseline; then test candidate private
   IDs with one hub stick, browser, metadata, waveform, track load and audio reads.
2. Inventory accepted physical sessions, dispatcher connection IDs, library bind
   addresses, EDB 12523 discovery and file-service/RPC paths. Implement connection
   provenance before allowing the physical network. Exercise external e0..e7
   requests and ensure they never reach a private server.
3. Build an isolated emulator protocol peer harness. Emulate physical peers 1..4,
   including the exact current synthetic IP labels, join/leave, renumbering,
   duplicates, changing subnet, discovery, status, media queries and database
   requests. Do not transmit these fixtures on the workstation LAN.
4. Compare native physical request/reply bytes with the mod disabled and enabled.
   Verify each physical session is unchanged, private sessions are translated only
   once, fd reuse cannot retain ownership, and physical peer loss cannot withdraw
   a hub source or poison a cache. Include malformed/fragmented TCP messages.
5. Capture emulator network traffic at the physical boundary. Assert zero private
   announcements, labels, media codes or private library access. Check hub-loaded
   local-deck status, master/beat traffic, metadata requests from peers and native
   USB exports. Packet absence alone is not proof of native functionality.
6. Run simultaneous native USB/hub browsing and loading, concurrent external
   requests, removal/reinsertion, two local deck playback, Link mode off/on and
   startup with the persisted mode already on. Preserve five-source emulator
   regression coverage; eight-source behavior is still not hardware acceptance.
7. A protocol harness establishes only what it models. Real interoperability
   requires separately authorized AZ and external-CDJ tests with a defined
   baseline, device models, firmware, player numbering and rollback plan.

## Setting guard and mode transitions

A startup incompatibility guard and clear warning remain useful independent
safety work, but do not solve coexistence. The typed listener addresses above
give stronger research leads than polling `0x03f86a08`. A validated initial
accessor, thread/lifetime guarantees and complete stop/drain behavior are still
needed. Current initialization has no shutdown callback and repeatedly forces
the internal cache; a mode transition must not race that polling.

For the first coexistence prototype, choose the mode before source publication
and retain it for the session. Defer live mode switching until browsing/loading,
active audio handles, source withdrawal, worker cancellation, hook ownership and
kernel rollback have explicit completion signals. Never silently remove a
playing source to accommodate an external player.

No runtime code, hooks, package, version, catalog, release or hardware state was
changed by this investigation.

## Deferred mode completion and internal-cache conflict (emulator research)

The native completion task at `0x0140a088` clears owner +0x4a1 and either
notifies listeners or starts a pending request (+0x4a3, requested +0x4a2).
Returning from `0x01408ef0` alone does not establish completion. The experiment
observes completion only after forwarding the task and finding both busy and
pending clear; owner +0x4a4 provides the native effective mode.

Runtime tracing exposed a conflict with the existing internal library override:
`0x013fa7a8` returns early when cache `0x03f86a08` equals its requested mode and
owner +0x3b8 is initialized. On an Off-start session, the override has already
set that cache to 1. An On request then skips native network transition work;
the coordinating task remains busy and queues the subsequent Off request.
The trace candidate recorded request=0 with busy=1/effective=1. The completion
hook alone therefore cannot repair this failure.

The research-only `--mode-recovery-probe` keeps a shadow from native network
requests, restores that value before the native setter, and suspends internal
cache maintenance until native deferred completion. It then schedules source
announcements on the injector thread, deferring while eject work is reserved.
This is an emulator experiment, not a production integration: cache check/write
serialization, startup interception order, owner lifetime, source generations,
loaded/eject interactions and additional slots require further validation.

An initial replay of every source restored missing USB1 rows but duplicated
member7/slot7, whose native row survives the transition. The next experiment
replays only slot3. USB3 and five/eight-source behavior must not be inferred from
these three-fixture observations. All firmware decompilation and screenshots
remain ignored local evidence; the published mod and physical AZ are unchanged.

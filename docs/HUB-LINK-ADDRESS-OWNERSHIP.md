# Public source address ownership

The integrated sharing fixture currently consumes separately prepared network
namespaces and fixed documentation addresses. It does not allocate production
LAN addresses. Separate MACs/IPs preserve public endpoint isolation without
using native USB library slots. The existing endpoint factory validates the
live interface and enters its namespace only from a short helper thread.

An apparently unused address in the AZ's subnet is not an allocation policy.
It may belong to a disconnected host or a DHCP pool. ARP probing alone does not
provide a lease or reserve an address against a DHCP server. Keep DHCP acquisition,
address conflict detection, interface lifecycle and Link identity collision
handling separate; success in one does not establish the others.

## First implemented component

`public-address-conflict.h` implements a socket-free candidate lifecycle based
on [RFC5227 sections2.1–2.4](https://www.rfc-editor.org/rfc/rfc5227.html#section-2.1).
The caller supplies monotonic milliseconds and independently generated uniform
jitter. Three confirmed transmitted probes precede a two-second quiet period.
The next action permits interface configuration, then two announcements follow.
Readiness requires at least the first successfully transmitted announcement.
An unconfirmed action stays pending; delayed or failed sends cannot silently
advance the protocol. Exhausted timestamp arithmetic retires the candidate.

ARP payload parsing checks Ethernet/IPv4 hardware/protocol sizes, operation and
length before inspecting addresses. A foreign claim conflicts throughout the
lifetime; a simultaneous foreign probe conflicts during acquisition. The caller
must supply all local MACs so reflected broadcasts are ignored. This implementation
elects immediate retirement on an ongoing conflict, as permitted by section2.4(a),
rather than defending an address while services continue. Conflict withdrawal
must eventually drain the existing source manager before removing its address.

The retained per-interface retry context now enforces cross-candidate backoff
after ten conflicts. Changing candidate addresses does not reset that context;
neighbor interfaces have independent contexts. Backward timestamps and exhausted
deadline arithmetic cannot bypass the limit.

`public-address-arp.h` supplies bounded nonblocking Ethernet ARP send/receive on
an already owned interface. It checks the live MAC/type/UP flags and binds the
packet socket to that interface. Incoming interface/protocol/hardware metadata,
packet length and matching Ethernet/ARP sender MAC are checked; outgoing and
other-host captures are ignored. Sending does not commit the protocol action:
the caller must separately confirm success with a fresh monotonic timestamp.
See the Linux [packet socket reference](https://man7.org/linux/man-pages/man7/packet.7.html).

These components are not a complete RFC5227 implementation: automatic allocator
wiring, lifecycle resets, full local MAC inventory and network-change detection
remain pending. They do not select an address, acquire DHCP,
create an interface, open public service sockets or mutate native networking.
It is not yet wired into `public-mod-emulator.h` or the production mod.

## Reusing the native DHCP client

Read-only emulator inspection finds BusyBox1.27.2 `udhcpc` with foreground,
explicit interface/script and bounded initial-attempt options. The native
default script also changes DNS/routes and can invoke `killall udhcpc` on
failure; it must never run for a hub public identity. This observation concerns
the emulator's userspace, not separate physical-client runtime acceptance.

The experimental `research/scripts/public-dhcp-event-hook.sh` instead reports
bounded versioned records through explicitly inherited fd3. It invokes no
external commands, changes no files and never calls the native default script.
The exact interface must match the launcher's declared owned interface. Its
fields are proposals, not executable commands; invalid environment fields
produce an explicit invalidation event. A dedicated nonblocking event pipe,
child supervision and overflow/EOF retirement are still needed in the allocator.
Do not substitute a logfile or polling persistent controller settings for IPC.

`public-dhcp-event.h` validates complete bounded records, exact interface,
IPv4/subnet/host bounds and lease integers before publishing output. Retirement
events carry no reusable address. Very long leases are capped to the client
timer's maximum; short server-granted leases are never extended. The upstream
[BusyBox1.27.2 client](https://raw.githubusercontent.com/mirror/busybox/1_27_2/networking/udhcp/dhcpc.c)
is a reference for event/timer behavior, not proof of an identical vendor build.
Missing lease/subnet/server fields fail closed; fallback remains integration
work. Initial-conflict DHCPDECLINE delivery is now covered separately below.

Parser controls pass plain/ASan+UBSan/TSan, and the pinned AArch64 test runs in
the emulator. The reporting hook runs under both host `/bin/sh` and emulator
BusyBox shell; hostile-field controls run without any external commands in PATH.
A real emulator `udhcpc` process on an isolated unaddressed dummy interface
delivers `deconfig` and `leasefail` through inherited fd3 and exits after one
attempt. Its namespace is removed; native DNS/routes and EP147 PID are unchanged.
This proves the reporting channel, not DHCP offer/ACK, renewal, expiration,
address application or live network ownership.

The subsequent live DHCP fixture now completes a real OFFER/ACK exchange with
the emulator's unchanged native client. A repository-authored UDP server in an
isolated WSL TAP namespace offers only192.0.2.3 to one fixed virtual MAC. The
native hook reports the exact address, /24 mask,60-second lease and server IP.
The proposed address is not applied. Native EP147/DHCP process IDs, DNS and
routes remain unchanged. `test-live-dhcp-emulator.py` reconstructs this bounded
control with an explicitly supplied matching emulator MACVLAN module.

The optional transport `--fixture-dhcp` mode suppresses NAT DHCP replies only
when their BOOTP client MAC matches that test identity. Native management/DHCP
traffic remains eligible; packet controls cover short/malformed packets and the
native MAC negative control. This mode does not change normal transport behavior.
The fixture server is not an independent general-purpose DHCP server and proves
neither renewal/expiration nor automatic address application. DHCP semantics are
referenced against [RFC2131](https://www.rfc-editor.org/rfc/rfc2131.html).

## Required integration gates

`public-network-lease.h` now supplies the single-controller lease/address
lifecycle. A validated proposal starts a new candidate generation. Same-address
renewal keeps that generation; changed address/mask/server queues a replacement
without canceling retirement. Readiness requires both confirmed interface
configuration and matching-address ACD readiness. Expiration, invalidation or
explicit retirement removes readiness immediately. Separate acknowledgements
enforce worker/socket drain before address removal, and removal before a fresh
candidate can start. A queued replacement that expires during drain is discarded.
Neighbor lease contexts are independent. These are control-plane invariants,
not proof that kernel network resources have actually changed.

This controller requires a conservative grant-time lower bound on the same
monotonic clock as its poll time. The version1 hook record alone does not
supply that guarantee. Do not stamp a queued event with its eventual receipt
time and treat it as a newly granted full lifetime. The supervised one-shot
acquisition below now supplies that lower bound. The controller is not wired
into the mod, and simulated renewal/expiry is not live DHCP renewal acceptance.

Targeted plain/ASan+UBSan/TSan and pinned AArch64 emulator controls pass. They
cover delayed/replayed grants, expiry boundaries, ARP/configuration readiness,
drain/removal acknowledgements, stale-generation acknowledgements, queued lease
expiry, neighbor isolation and timestamp/generation exhaustion. One combined
control drives the actual ACD state machine through probes, quiet period and
first announcement before activating the lease, then expires it.

### Supervised one-shot acquisition

`public-dhcp-acquire.h` records a monotonic lower bound before spawning each
fresh client attempt. The eventual grant necessarily follows that point; using
it for expiration conservatively shortens the lease instead of extending a
queued event. A fresh nonblocking pipe belongs only to that attempt. The result
is accepted only after a validated bound record, successful child exit, EOF and
child reap. Malformed/partial/duplicate records, nonzero exit, timeout and
cancellation do not publish a grant. Parsing/read work is bounded per poll.
The controller's existing source lease must independently expire while a new
acquisition is pending.

The parent uses `posix_spawn` with a clean explicit environment, reset signal
mask, separate process group and isolated event/namespace descriptors. No native
LD_PRELOAD or default DHCP hook is inherited. The independently executed
`public-network-helper.c` enters the declared namespace, checks the actual
interface MAC/type/UP flags, closes unrelated inherited descriptors and executes
the native foreground client with `-n -q` and bounded initial retries. It cannot
run with the initial namespace or without valid inherited descriptors. Build it
explicitly with `make sharing-network-helper`; it is not a current release asset.

The supervisor requires exclusive child-reaping ownership; it rejects nondefault
SIGCHLD/auto-reap configurations before spawning. A production broker/lifecycle
integration must preserve that ownership rather than compete with EP147/core
child handlers. Cancellation signals only its own process group and escalates
after a bounded grace period; polling must continue to terminal for cleanup.

Plain/ASan+UBSan/TSan controls cover terminal success/failure, invalid/partial
records, nonzero exit, cancellation, deadline, context reuse and descriptor
closure. The exact pinned AArch64 control and separate standalone helper also
complete real OFFER/ACK in the emulator. The C lease controller consumes the
proposal using the pre-spawn reference and stays unready pending ARP/configuration.
Its parent namespace and native EP147/DHCP processes, DNS and routes are unchanged.

A future refresh is a new bounded acquisition using the same MAC/client identity,
not an unsupervised native renewal daemon. Startup deconfig belongs to that new
attempt; it must not revoke an older still-valid lease. Same-address grants can
extend the existing lease generation, while changed grants require retirement.
Production refresh scheduling, DHCPNAK and ongoing-conflict DECLINE integration,
general server lease reuse and live expiry continuity remain unvalidated. This
initial supervisor trial alone establishes initial acquisition; the bounded
post-DECLINE recovery control below extends that scope for one fixture retry.

### Integrated address flow control

The explicit `--address-flow` emulator control now combines the native supervised
grant, C lease controller, real ARP adapter and native interface configuration.
After three confirmed probe transmissions and the quiet interval, it applies
only the validated address/prefix/broadcast on the owned virtual interface,
verifies the actual IPv4/mask, transmits both announcements and obtains lease
readiness. A host UDP echo confirms bidirectional reachability from192.0.2.3.
The socket is closed, the empty service drain is acknowledged, and the exact
address is removed and verified absent before removal acknowledgement.

`public-address-flow-probe.h` is a research harness, not the production allocator.
It uses a single-threaded test process's forked child to enter the namespace and
executes native `/sbin/ip` with explicit argument vectors. Never call this fork
path inside EP147. It does not configure routes/DNS, create music endpoints or
publish a source. Production needs asynchronous owned-interface operations,
complete local MAC inventory, lifecycle supervision and live conflict/expiry
integration. The probe uses fixed test jitter, not a production random source.

The exact pinned AArch64 probe passes against the isolated TAP, and the outer
runner verifies absent final IPv4, unchanged native process IDs/DNS/routes and
parent namespace, then removes its namespaces/module/temporary files. This is
one successful initial address lifecycle, not refresh, cable-change, eight-source
or target4.4 acceptance. The existing acquisition unit controls also pass again
plain/ASan+UBSan/TSan after adding this guarded probe branch; that does not make
the live address-flow branch sanitizer-instrumented.

The negative `--address-flow --arp-conflict` control also passes. After issuing
a real DHCP ACK, the host fixture observes the first zero-sender ARP probe and
injects a matching foreign ownership claim. The C ARP adapter reports conflict
before configuration; the lease candidate retires, the exact negative result is
required by the parent, and actual IPv4 remains absent. The host observer sees
neither an address announcement nor IPv4 use from that fixture MAC during its
bounded observation window. Native process IDs/DNS/routes and parent namespace
remain unchanged. This exercises initial conflict rejection, not ongoing-service
retirement. DHCPDECLINE notification and one bounded retry are now covered below;
production retry scheduling remains pending. Rejecting local configuration alone does not
establish complete DHCP conflict handling.

### Initial DHCPDECLINE notification

`public-dhcp-decline.h` builds the explicit notification following a matching
observed ACD conflict. It broadcasts from0.0.0.0 rather than the disputed address,
includes requested address/server/client identifiers and computes IPv4 and UDP
checksums. The sender checks the actual interface name/type/MAC in its current
namespace before use. Packet fields/options follow
[RFC2131](https://www.rfc-editor.org/rfc/rfc2131.html) and
[RFC2132](https://www.rfc-editor.org/rfc/rfc2132.html).

The native helper now explicitly pins its MAC-based client identifier using
`udhcpc -x`; the DECLINE uses that same identity. In the isolated emulator
conflict control, the server's UDP socket receives and validates the notification,
source address, rejected address, server and client identity. The client never
configures the disputed IP. This is stronger than a queued-send return value,
but does not establish behavior of arbitrary DHCP servers or actual CDJs.

The retained retry policy separately enforces a ten-second quiet interval after
DECLINE and preserves the stricter sixty-second backoff after ten conflicts.
Its boundary controls and packet tests pass plain/ASan+UBSan/TSan. A bounded live
probe retry is covered below; production retry scheduling is not wired into the mod.

### One live recovery after DECLINE

`--address-flow --arp-conflict --retry-after-decline` now passes the complete
bounded recovery in the emulator. The first192.0.2.3 grant conflicts, is declined
and is never configured. The retained retry context then enforces10001ms of
guest monotonic quiet time. A fresh native client uses the same explicit MAC-based
client identity, obtains192.0.2.4, and starts the next C lease generation only
after retirement/removal acknowledgements for the first candidate.

The second candidate completes ARP/configuration/announcements, exchanges UDP
from its new address and removes the exact address. The host independently
observes about10.101 seconds from DECLINE reception to the next DISCOVER and
validates both exchanges/client identities. Host and guest times are separate
clock domains; the guest policy supplies the exact quiet-period check. Native
process IDs/DNS/routes and parent namespace survive, and all owned resources
are removed. The acquisition unit controls pass again plain/ASan+UBSan/TSan;
the live recovery branch is not sanitizer-instrumented.

This is one repository-authored two-address fixture, not general DHCP-server
interoperability, an unbounded retry daemon or continuous lease refresh. Music
endpoints are still absent. Production broker, owned interface lifecycle,
refresh/expiry and source-manager integration remain required.

1. Create only owned namespace/interface resources, with cleanup on partial
   failure. Never change the AZ's native address, route or DHCP client. Never
   adopt a pre-existing resource based only on its name.
2. Acquire a valid independent DHCP lease when available. On an isolated network,
   investigate IPv4LL selection and routing using
   [RFC3927](https://www.rfc-editor.org/rfc/rfc3927.html). IPv4LL availability
   alone does not prove reachability from a client with a routable address.
3. Wire the retained cross-candidate conflict backoff, supply a full local MAC inventory,
   handle failed sends and validate packet/interface provenance. Never advertise
   before address acquisition is committed.
4. Resolve public Link identity collisions independently of IP collisions. Retain
   private local identities and per-source caches. Test two AZs, real rekordbox
   and existing player identities; fixed fixture numbers41–48 are not a proven
   production allocation policy.
5. Drain workers and close their endpoints before deleting owned interfaces or
   namespace handles. Reacquisition creates a fresh source generation/nonce.
6. Validate DHCP, isolated-network fallback, duplicate addresses, simultaneous
   startup, cable/network changes and cleanup in the emulator. Validate exact
   target4.4 module/runtime separately; emulator6.6 results do not prove its ABI.

## Leased public library control

The explicit `--address-flow --library` trial now serves one actual mounted hub
library after native supervised DHCP and ARP acquisition. Its endpoint spec uses
the validated grant address/mask/broadcast, and a new source-owned worker receives
fresh sockets and nonce. No native library slot or private cache is borrowed.
The fixture grants600 seconds; the worker remains gated by the C lease deadline
and a shorter bounded test duration.

Independent unchanged Beat Link discovers provider41 on192.0.2.3 and browses
all2546 tracks,185 ordered playlist nodes and8359 memberships against the existing
oracle. Unchanged Crate Digger concurrently fetches the complete PDB and selected
MP3 with exact expected hashes. The test then requests withdrawal, joins the
worker and closes endpoints before acknowledging drain and removing the exact
address. Actual final IPv4 absence and native process IDs/DNS/routes/parent
namespace are checked. Owned networks/module/temporary files are removed and
the local-socket transport reports no errors.

`public-address-library-probe.h` runs in a separate research process, not inside
EP147. Its owned `/tmp` markers are test coordination, not a production broker
API. This proves one initial DHCP/ARP/library coupling, not automatic namespace
creation, production mod integration, continuous refresh, live source eject,
concurrent local playback, multi-source leased exports or actual-CDJ acceptance.
The acquisition unit controls pass plain/ASan+UBSan/TSan after adding this branch;
the live library branch itself is not sanitizer-instrumented.

## Earlier component evidence

`test-public-address-conflict.c` passes plain, ASan/UBSan and TSan controls and
the pinned-compiler AArch64 binary runs in the guarded localhost emulator.
It covers malformed/truncated ARPs, timing boundaries, explicit send completion,
parallel probes, foreign established owners, local echoes, ongoing conflicts,
retirement and timestamp overflow. It sends no network traffic. This is component
evidence, not live address acquisition or external-player interoperability.

The separate live ARP adapter control passes plain and ASan/UBSan runs in a fresh
WSL namespace with only an unaddressed virtual Ethernet pair. Competing probes
and announcements retire both acquiring and active candidates; wrong local MAC
validation fails before use. The namespace is removed after each run. A pinned
AArch64 build of that standalone adapter control succeeds. The later integrated
address-flow controls above exercise the adapter in the emulator; target4.4
runtime remains unvalidated. The retained retry tests separately pass
plain/ASan+UBSan/TSan. No production interface or physical AZ was accessed.
Reproduce the isolated live test with:

```sh
gcc -O2 -Wall -Wextra -Wno-unused-function \
  src/azhub/linkfake/test-public-address-arp.c -o /tmp/azhub-arp-test
sudo python3 src/azhub/research/scripts/test-public-address-arp.py \
  --binary /tmp/azhub-arp-test
```

## Anonymous namespace allocation component

`public-network-namespace.h` creates a network namespace in a disposable thread,
opens `/proc/thread-self/ns/net`, then joins. The owned CLOEXEC FD keeps the
namespace alive after the creator exits; no bind mount or /run path is required.
The controller does not change namespaces. An operation cannot be reused while
its thread or returned descriptor remains owned. Endpoint/interface retirement
must precede releasing the eventual network ownership reference.

Eight sequential pinned AArch64 creations pass in the localhost emulator, with
typed descriptor validation, controller namespace continuity and unchanged EP147
PID. Targeted host sanitizer controls pass the restricted privilege-denial path;
they do not establish successful privileged creation. Interface creation, DHCP,
broker integration and target4.4 runtime remain pending.

Local vendor4.4 source review confirms `/proc/thread-self` support and rtnetlink
IFLA_NET_NS_FD destination selection. The macvlan newlink operation receives the
source namespace for parent IFLA_LINK lookup, while destination link creation uses
the selected target namespace. Relevant facts are in kernel/fs/proc/thread_self.c,
net/core/rtnetlink.c around1490/2350 and drivers/net/macvlan.c around1253. These
source facts do not replace an actual MACVLAN creation/runtime control.
See `acceptance/usb-hub-support-sharing-namespace-create.json`.

## Direct namespace MACVLAN allocation

`public-network-link.h` creates a source interface via exclusive RTM_NEWLINK with
explicit parent index, target namespace FD, name, locally administered MAC and
nested MACVLAN bridge mode. The worker verifies the existing parent MAC/type/UP
state, sends through a fresh nonblocking route socket and requires the matching
kernel ACK. Controller polling joins the operation; no address is selected and
no parent-interface settings are changed. A timeout is ambiguous and does not
permit releasing enclosing ownership without reconciliation.

The hash-verified pinned AArch64 control creates an anonymous namespace and a
real UP interface directly inside it, checks name/MAC and absent IPv4, then closes
its ownership references. Native PID/address/routes/DNS/controller namespace are
unchanged. The module unload succeeds afterward, demonstrating no virtual device
reference remains; temporary files are removed. Targeted host sanitizers pass
request/native-namespace refusal controls, not live privileged creation. DHCP,
broker allocator integration and target4.4 runtime remain pending.
See `acceptance/usb-hub-support-sharing-namespace-link.json`.

## Allocated namespace/interface to native DHCP

`public-network-acquire.h` now sequences anonymous namespace creation, exclusive
MACVLAN allocation and supervised one-shot native DHCP in a standalone controller.
Polling never spawns from EP147 or changes its SIGCHLD. Cancellation retains state
until pending creator/link threads and DHCP reaping finish, then closes its owned
namespace. Successful acquisition returns the namespace plus the native proposal
and conservative grant-time reference; it is not ARP/configuration readiness.

The pinned AArch64 live control obtains192.0.2.3 from the isolated fixture after
creating its own namespace/interface: the runner no longer prepares either guest
object. Native PID/DNS/routes/DHCP processes remain unchanged, and owned module,
TAP and temporary resources clean up with no transport errors. Targeted host
sanitizers cover cancellation/privilege failure without launching DHCP. Successful
allocation is live emulator evidence, not host sanitizer or target4.4 acceptance.
ARP/configuration, continuous refresh and SDK broker integration remain pending.
See `acceptance/usb-hub-support-sharing-allocated-dhcp.json`.

## Exclusive address configuration operation

The owned-namespace address operation now supports RTM_NEWADDR with CREATE/EXCL
and explicit LOCAL/ADDRESS/BROADCAST attributes as well as exact removal. Before
configuration it verifies the declared interface type/MAC/UP and requires absent
IPv4; an already configured interface returns EADDRINUSE. The caller remains
responsible for matching DHCP lease and successful ARP claim before starting it.
The controller polls/join; only matching kernel ACK is successful configuration.

A hash-verified pinned AArch64 emulator control allocates an anonymous namespace
and MACVLAN, configures192.0.2.3/24, independently observes the IP from a namespace
inspection thread, rejects a second configure, removes the exact address and
observes IPv4 absent again. Native PID/address/routes/DNS stay identical; closing
namespace ownership permits module unload and all temporary files are removed.
This isolated operation test does not run DHCP/ARP or services. Integrated
acquisition→ARP→configuration and broker readiness remain pending.

## Namespace-bound ARP transport factory

`public-address-arp-namespace.h` opens the validated ARP transport in a disposable
namespace thread, then joins and transfers its nonblocking CLOEXEC socket to the
controller. The controller remains in the original namespace; AF_PACKET traffic
uses the descriptor's source namespace/interface. Failed/wrong-kind creation
closes its owned namespace reference and cannot yield a transport.

A hash-verified pinned AArch64 control allocates its namespace/MACVLAN, takes the
ARP descriptor and submits one candidate probe from the controller without
entering that namespace. Configure/remove regression and native-state/module
cleanup pass. This is one kernel-accepted send, not independently observed wire
traffic or a complete RFC5227 acquisition. Host sanitizers cover native refusal
and wrong-kind cleanup. Full DHCP→ARP→configuration integration remains pending.
See `acceptance/usb-hub-support-sharing-arp-namespace-factory.json`.

## Automatic allocation to ARP/configured readiness

`public-network-ready.h` now joins namespace ARP creation, candidate probing,
exclusive asynchronous configure and lease activation. Configuration begins only
at ACD CLAIM after probes/quiet interval; kernel ACK gates configured proof and
announcements. Polling continues after READY to detect lease loss/conflicts.
Failure retires the lease without releasing borrowed endpoints or caller-owned
namespace. Quiesce joins in-flight factory/configure operations and closes ARP;
it is not address-removal proof and preserves caller ownership for reconciliation.
Queued replacement grants remain preserved when a lease is already draining.

The allocated AArch64 control now passes real native DHCP→ARP→configure→READY,
independently observed UDP reachability, quiesce and exact netlink retirement.
The runner creates neither guest namespace nor interface. Native state remains
identical and module/TAP/temp cleanup completes with no transport errors. Host
sanitizers cover wrong namespace/backward-clock retirement and pending-factory
cleanup. The positive live branch uses fixed fixture jitter and is not sanitizer
instrumented. Libraries, SDK allocator integration, integrated conflict/DECLINE/
retry and refresh remain pending; production requires independent random jitter
and complete local MAC inventory.
See `acceptance/usb-hub-support-sharing-allocated-ready.json`.

## Automatically allocated real library control

The controller-owned path now passes with a real mounted hub library: anonymous
namespace/MACVLAN, native DHCP600-second grant, ARP/configured READY, independent
UDP reachability and a source-manager worker on a pinned root. Unchanged Beat Link
discovers the provider and validates2546 tracks/185 nodes/8359 ordered memberships;
concurrent unchanged Crate Digger clients fetch exact PDB/MP3 hashes. Readiness
polling continues while serving. Completion withdraws/joins/closes endpoints before
ARP quiesce and exact netlink address retirement. Native state and complete
module/TAP/temp cleanup pass, with no transport errors. The full portable matrix
passes76/76/76 at this boundary.

The first attempt stopped before clients because its readiness marker lacked
metadata_port; resources cleaned and the corrected second attempt passed. This
standalone research path is not yet the SDK helper's allocation operation, uses
fixed jitter and does not test loaded remote decks, local playback concurrency,
renewal or integrated conflict/retry. The live branch is not sanitizer-instrumented.
See `acceptance/usb-hub-support-sharing-allocated-library.json`.

## Managed initial conflict and namespace-correct DECLINE

The readiness controller now preserves observed CONFLICT proof until notification
rather than stopping ACD immediately. A dedicated operation duplicates namespace/
ARP references, enters that source namespace, sends bounded DHCPDECLINE using the
existing validated packet sender and joins before ARP quiesce/namespace release.
This avoids opening its packet socket in the broker controller's native namespace.
A queued send is not acknowledgement; attempt/error diagnostics survive source
cleanup, and notification failure does not fabricate server receipt.

The actual SDK helper control passes a native grant followed by an independently
observed probe and injected foreign claim. The server receives DECLINE from
0.0.0.0 with the correct rejected IP/server and original client MAC identity. Its
bounded post-conflict observation sees no conflicting announcement or IPv4 use.
No library ACTIVE marker occurs. Managed exit, exact rollback and owned cleanup
pass. Targeted host sanitizers cover namespace refusal/failure, readiness and
allocator cancellation regressions. Automatic retry/backoff through this managed
path, ongoing conflicts/renewal, loaded decks and target4.4 remain pending.
See `acceptance/usb-hub-support-sharing-sdk-arp-conflict.json`.

## Native and cross-source initial grant exclusion

Allocator readiness now snapshots native-controller IPv4 addresses before accepting
an initial completed DHCP grant. `public-address-inventory.h` deduplicates at most32
addresses and fails closed on overflow/read errors without changing any address.
Candidates matching a native address are refused before ARP/configuration.

A candidate also cannot reuse a different slot's candidate/active reservation.
Completed simultaneous pending grants use slot priority where neither has entered
readiness; an existing readiness/active reservation wins regardless of slot. Failure
returns EADDRNOTAVAIL, not the EADDRINUSE reserved for observed ARP conflict/retry.
The original source is not modified by this check. This is separate from local-MAC
reflection filtering: ignoring reflected local ARP must not authorize duplicate IPs.

Native snapshot bounds/deduplication/overflow and cross-source/native exclusion
controls pass plain, ASan/UBSan and TSan. Pinned AArch64 helper build passes. These
changes postdate the successful two-source/local-loop runtime hashes. Real managed
duplicate/native grant rejection and a positive regression remain unvalidated.
This is an initial grant snapshot, not continuous native address-change tracking;
renewal, reassignment reconciliation and appropriate decline/retry policy for these
local exclusions remain pending production work.

## Managed duplicate-grant result

The isolated server now deliberately grants192.0.2.3 to source0 and source4 with
distinct client MACs. Actual SDK/helper allocation rejects source4 before readiness:
current-run status is EMPTY/EADDRNOTAVAIL and no source4 ACTIVE appears. Source0
continues independent discovery, complete2546-track/185-node/8359-membership menus
and exact PDB/audio downloads. Global STOP, exact original restoration and owned
cleanup complete normally. See
`acceptance/usb-hub-support-sharing-sdk-duplicate-grant.json` for exact hashes.

The rejected provider's discovery absence was not independently monitored; the
negative publication evidence is the helper state plus inspected before-ARP guard.
This does not prove native-IP rejection, continuous native address changes, renewal
or successful two-source allocation on the new hash; those remain separate gates.

## Managed native-address exclusion result

A separate isolated DHCP control offers the native controller's192.0.2.15 address.
The managed helper rejects it with EADDRNOTAVAIL before ACTIVE/readiness. Global
STOP, managed exit, exact original restoration and owned-scope cleanup pass; native
address, routes, DNS hash and namespace match before/after. See
`acceptance/usb-hub-support-sharing-sdk-native-grant.json` for exact hashes.

The runner identifies the application via its own transient unit's process tree,
then verifies the executable/mapped mod, avoiding short-lived fork children that
can make `pidof EP147` ambiguous. The initial preflight used an unavailable stat
utility (replaced by supported `ls -Lid`); subsequent verified cleanup is distinct
from the final passing trial. Network comparisons are asserted after owned cleanup
so a comparison failure cannot abandon the fixture resources.

This validates initial native-address exclusion only. It does not establish
continuous reassignment/renewal handling or the positive two-source gate on this
helper hash.

## Positive regression after initial address exclusions

The exact helper used by both duplicate/native grant negatives now passes the
complete positive two-source runner: distinct grants, independent full parallel
menus and exact PDB/audio from both, native selected eject/replug retaining the
neighbor's original handle/TCP menu session, rediscovery/old-handle STALE and exact
fresh PDB access. Global STOP, original-byte restoration and cleanup pass. Native
address/routes/DNS/namespace snapshots match before/after. See
`acceptance/usb-hub-support-sharing-sdk-address-guard-positive.json`.

This closes the positive two-source gate for initial exclusions; it does not
validate renewal, native reassignment during ACTIVE, no-DHCP fallback or physical
models. The earlier native/local-loop evidence uses the preceding helper hash and
must retain that boundary.

## Supervised refresh component (not integrated)

The initial-exclusion boundary passes the full82/82/82 host matrix.
`public-dhcp-refresh.h` now schedules a supervised fresh acquisition with the same
client MAC at half the current lease duration. This is not RFC2131 unicast RENEWING.
While the old lease is valid, failed acquisition backs off five seconds, bounded
by expiry, without extending ownership. A fully completed same-configuration ACK
extends the lease from the acquisition's conservative start timestamp, preserving
generation/configuration. Changed configuration requests retirement and leaves the
old address untouched; applying/reacquiring a replacement remains integration work.
Expiry is checked before accepting queued ACKs. Backward clocks fail closed.

Stop cancels and waits for the owned DHCP child to reach reaped/EOF completion
before releasing the borrowed namespace. Host controls cover scheduling/backoff,
unchanged-generation extension, changed/late ACK retirement and backward clocks
with synthetic completed acquisition records. A separate actual spawned sleep
helper proves cancellation/process-group termination/reaping/pipe closure without
network mutation. Plain, ASan/UBSan and TSan pass. This is not an actual network
refresh, native udhcpc refresh acceptance or allocator runtime integration.

## Allocator refresh integration: controls/build pending live trial

Allocator READY now arms and polls the supervised refresh component. Accepted
same-configuration grants preserve lease/public generation and the existing
services, recheck native/cross-source exclusion and increment bounded diagnostics.
Release cancels/reaps any refresh before ARP quiesce, address/link removal and
namespace release. A pending earlier slot cannot displace an existing active
reservation during this recheck. Targeted refresh/allocator plain/ASan+UBSan/TSan
controls pass. Changed configuration still retires with ESTALE; automatic
replacement/reacquisition is not implemented by this integration.

The emulator helper logs accepted refresh count, public/lease generation and new
expiry. The isolated DHCP fixture can serve repeated60-second grants, and the
runner's explicit `--refresh-grants` mode checks accepted same-generation refresh
for both source0/4 while independent full-menu sessions remain open. Python syntax
and pinned builds are checked separately; no live refresh result is claimed yet.
This remains fresh supervised acquisition with the same MAC, not RFC2131 unicast
RENEWING. Continuous native address monitoring and no-DHCP policy remain pending.

## Managed same-configuration refresh result

The explicit60-second fixture trial now passes real native-client reacquisitions:
source0 accepts three and source4 four refreshes, preserving public generation1
and lease generation1 while independent complete TCP menu clients and exact file
fetches continue. Subsequent native selected eject/replug preserves the neighbor's
original RPC root and TCP menu connection; selected rediscovery/old-handle STALE
and exact fresh PDB access pass. Global STOP, native network snapshots, exact
original restoration and owned-scope cleanup pass. See
`acceptance/usb-hub-support-sharing-sdk-refresh.json` for exact hashes/events.

This is fresh supervised acquisition with the same MAC, not RFC2131 unicast
RENEWING. Same-configuration extension is demonstrated. A later managed native
eject trial observes a live pending refresh client before eject and its absence
after backing unmount, with neighbor continuity and complete restoration/cleanup;
see `acceptance/usb-hub-support-sharing-sdk-pending-refresh.json`. Managed failure/
expiry controls remain open. Earlier local-deck loop evidence is
on the preceding helper hash, not this new refresh-integrated executable.

## Changed-configuration recovery under development

Changed refresh configuration now reports ENETRESET after requesting retirement.
The inventory client treats that error as recoverable only once the helper has
reported authenticated EMPTY for the exact current generation. ESTALE remains
a hard root/identity failure. Withdrawal clears the desired root and suppresses
recovery, even if a repeated EMPTY arrives with ENETRESET. Ordering, hard-error
refusal and withdrawal controls pass plain/ASan+UBSan/TSan; AArch64 builds pass.
The managed changed-address trial now passes on the new binary pair; see
`acceptance/usb-hub-support-sharing-sdk-changed-refresh.json`. Old-address
withdrawal, neighbor session continuity and independent full menus/exact files
at the new address are covered, with global STOP and full cleanup. This is not
native eject, external loaded-deck or actual-CDJ acceptance. The isolated DHCP server
can switch slot0 from192.0.2.3 to192.0.2.4 after an explicit host marker; it refuses
that control when slot1 is selected, preserving unique fixture address ownership.

## IPv4LL ownership groundwork

[RFC3927](https://www.rfc-editor.org/rfc/rfc3927.html), sections2.1/2.6/2.11,
requires selection within169.254.1.0–169.254.254.255, on-link delivery and
continued DHCP acquisition. A self-assigned address must not masquerade as a
DHCP lease or bypass continuous conflict detection.

`public-link-local.h` samples that range without modulo bias. The address
lifecycle now records explicit DHCP/LINK_LOCAL provenance. A link-local candidate
has no synthetic DHCP server, duration or expiry; ARP/configuration/readiness
remain separate gates. Changes between origins use existing retirement ordering.
Sampling, entropy failure, disallowed ranges and both origin-transition controls
pass normal/ASan+UBSan/TSan, with DHCP lease/refresh regressions preserved.

Before integration, distinguish a native client's completed no-offer result from
malformed output, cancellation and setup failure. Reap the client before fallback,
exclude native/other-source addresses, maintain DHCP attempts and validate route
scope and conflict recovery. No live IPv4LL or standards-compliance claim yet.


## Authenticated retirement observability

The reusable inventory retains each source's latest accepted EMPTY generation
and error. The SDK emulator adapter logs this acknowledgement independently of
its end-of-poll phase snapshot: a bounded receive batch can consume DRAINING and
EMPTY and send the next ALLOCATE before that snapshot. Therefore absence of an
EMPTY phase snapshot is not evidence that retirement was skipped.

The retained evidence changes only after token/current-generation validation.
A stale generation cannot overwrite it. Component controls cover same-batch
conflict retirement followed by generation advancement and stale-message refusal
in normal, ASan/UBSan and TSan modes. This observation does not change the
allocation or native unmount gates, and is not external loaded-deck acceptance.

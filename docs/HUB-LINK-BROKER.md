# Public networking broker integration

The current leased-library control is a separate research process. Production
integration must not move its single-threaded fork path into EP147 or compete
with the core's child waiters. No production broker is active yet.

## SDK boundary

The pinned core1.2.3 header supplies `launch`, `launch_alive` and `launch_stop`.
It permits executables only inside this mod's assets/data directories, runs them
outside EP147's systemd scope without the mods' preload and owns their launch
tracking/reaping. The options expose resource limits/logging, not inherited FD
transfer. Source inspection also confirms inherited descriptor closure before
launch and signal-default reset. Do not bypass those services from the mod or
change EP147's SIGCHLD behavior to accommodate the DHCP supervisor.

Use one SDK-managed auxiliary process, then transfer per-source namespace
references over an authenticated local channel. The auxiliary process can own
its own DHCP child waiters without entering the native process's reaping domain.
The explicit SDK probe now passes in the emulator as described below. Production
broker dispatch and lifetime integration remain separate requirements.

## Implemented transport and probe

`public-broker-wire.h` defines bounded AF_UNIX SOCK_SEQPACKET messages with
explicit version, operation, source slot, generation, per-launch token, payload
length and descriptor count. Integers are encoded big-endian rather than copying
C structure layout. Payloads are capped at4096 bytes; source slots are0–7.
SOURCE currently carries exactly one FD. Its production payload schema and
operation/generation authorization have not been implemented.

The peer helpers check kernel credentials and, when requested, the executable
inode against an already opened reviewed executable. Every received command can
require the per-launch token. Received descriptors are CLOEXEC and close on
malformed, truncated or token-mismatched messages. The sender retains its own
reference. Linux descriptor/credential semantics are described by
[unix(7)](https://man7.org/linux/man-pages/man7/unix.7.html).

`public-network-helper --broker-probe SOCKET TOKEN PARENT_PID` is deliberately
IPC-only. It checks ownership/mode of the private directory/socket, validates
the parent's kernel PID/UID, sends HELLO, acknowledges a transferred namespace
inode and exits after STOP. The parent trial checks helper PID/UID/executable and
the token. The namespace is never entered or modified. This mode neither creates
interfaces nor starts public library services, and is not a generic broker API.

## Evidence and remaining integration

Wire controls pass plain/ASan+UBSan/TSan and pinned AArch64 execution in the
localhost emulator. They cover credentials/executable mismatch, token mismatch,
FD transfer identity, CLOEXEC, short/oversized packets and missing/extra/truncated
ancillary data with descriptor-count checks. The named-channel control separately
executes the helper and passes HELLO→namespace acknowledgement→STOP under all
three host modes. Its pinned standalone helper/control also pass in the emulator
with unchanged native PID/DNS/routes and complete descriptor/socket-path cleanup.
Those earlier standalone tests do not themselves establish SDK launch or public
networking. The subsequent core-managed runtime control is recorded below.

Next implement production operation dispatch, namespace and
interface ownership, lease/ARP lifetime and source-worker/eject acknowledgements.
Authentication primitives alone do not authorize arbitrary namespace descriptors
or stale source generations. Process loss must retire public sources and release
their resources while preserving local private libraries and native USB slots.
Exact target4.4 runtime, full broker resource budgets and actual-CDJ trials remain
separate requirements.

## SDK-managed runtime control

`broker-probe-emulator` adds an explicit compiled and environment-gated control
for exact emulator kernel6.6.138-dirty. After mod readiness, HubTransport calls
the real core `launch` API on its own USB helper asset. The native SDK supplies
the executable argument and manages the child; the mod never calls waitpid or
changes native SIGCHLD. Credentials/executable/token checks, HELLO, namespace FD
acknowledgement and STOP complete, then `launch_alive` reports no running child.
Both the managed PID and actual peer PID are absent before acceptance. They are
equal in this observed run; no assumption about future wrapper PIDs is needed.
The helper also rejects a nonempty LD_PRELOAD variable before HELLO.

The first preflight stopped on unavailable `stat`, before changing the mod.
Two managed trials launched/authenticated successfully but rejected the namespace
because the probe assumed its `/proc/self/fd` link must display `net:[inode]`.
A namespace opened through its bind mount instead displays `/run/netns/NAME`.
The corrected probe checks `fstatfs` NSFS_MAGIC and inode identity. The supported
vendor4.4 GPL headers/source contain that namespace filesystem magic; this is a
compatibility fact, not target4.4 runtime acceptance. NSFS validation alone does
not distinguish namespace kinds or authorize production source operations.

The corrected managed trial passes inside EP147 with the exact mapped candidate
hash, real core launch, descriptor identity and managed shutdown. A separate
named-channel regression also passes with a bind-mounted namespace descriptor.
`test-sdk-broker-probe.py` restores the original package, stage, approval list,
optional start marker and absent helper asset, then starts normal0.3.2 and removes
its namespace/temp files. All failed trials also restored/cleaned their owned
state; the initial preflight never changed package bytes. The UI screenshot was
inspected for empty emulator decks before temporary installation.

This is IPC-only: it neither enters the namespace nor configures public addresses
or starts library workers. The supported launch boundary is now exercised; the
actual production network broker, source lifecycle and resource limits are not
implemented by this probe. No physical AZ/CDJ action or publication occurred.

## SOURCE operation payload under development

`public-broker-source.h` defines a versioned, bounded SOURCE body containing
source incarnation identity, expected mount device/inode, source root, interface,
locally administered unicast MAC, proposed public player number and label.
Address selection remains with the lease controller. All integers have explicit
big-endian encoding; exact lengths and zero reserved fields are required. Embedded
NULs, noncanonical path components, control characters and invalid interface
names are rejected. Decode commits its output only after complete validation.
The existing authenticated packet envelope carries source slot and generation.

Plain and ASan/UBSan component controls pass, including every truncated length,
extra bytes, reserved-field changes and maximum root/label sizes. The complete portable suite at this boundary passes66/66/66; its terminal log is
`local/integration/sdk-broker-probe-v1/source-host-suite.log`. The earlier65/65/65
record remains tied to the SDK probe.
This codec is not wired to production dispatch and does not authorize filesystem
access, player allocation, a namespace kind or replayed generations. Dispatch
must compare the opened live mount with the declared identity and enforce source
lifecycle ordering before acting. No namespace or network is created by decoding.

The source component also opens each directory component using directory-relative
`openat` with NOFOLLOW/CLOEXEC, compares the resulting device/inode and returns an
owned pinned descriptor. Tests replace the original directory and insert both
leaf and intermediate symlinks: replacements are rejected while the prior opened
object remains pinned. This is object identity validation, not proof of mount or
source authorization. Workers now offer an explicit descriptor-based start operation: the controller
duplicates the verified FD before creating the thread, and root attachment
duplicates that FD without reopening the pathname. Both owned references close
on failure or withdrawal/stop. The legacy path API remains for existing callers.
A worker regression renames the valid library, creates an empty replacement at
the original path and successfully stages through the pinned FD after the caller
closes its reference. Broker dispatch still needs to select this operation; the
new path is not yet exercised by a live SDK-managed library export.

The source manager now accepts verified directory descriptors explicitly. It
retains a CLOEXEC duplicate for the pending incarnation and passes that object
to descriptor-based worker start. Replacement or wanted0 closes the manager's
reference; withdrawal closes it immediately and preserves the existing hold
against publication before first poll. An identical request cannot clear that
hold. Reusing an active incarnation identity with a different directory object
returns ESTALE. Worker references remain independent until stop/join.

Focused plain and ASan/UBSan manager tests cover caller close before first poll,
rename/path replacement before staging, same-identity replacement rejection,
withdrawal before first poll and descriptor retirement. This remains a controller
API: authenticated broker dispatch and live source inventory/generation validation
are not implemented merely by adding descriptor ownership.

## Asynchronous generation ordering

`public-broker-generation.h` reserves a strictly newer per-slot generation before
starting work. Failed generations remain consumed. Replacement waits for explicit
drain acknowledgement; a late ready callback or an old detach cannot affect a
new incarnation. STOP is terminal for the authenticated session and moves all
nonempty slots into draining. It does not require a remote deck to unload.
Eight slots remain independent, and UINT64_MAX cannot wrap to a reusable value.
Plain and ASan/UBSan component controls pass. This single-controller ordering
component is not yet connected to authenticated operation dispatch: callers must
provide real worker/network drain evidence before acknowledging a generation.
The previous complete66/66/66 suite predates this added component.

## Controller dispatch adapter

`public-broker-dispatch.h` now connects SOURCE decoding, consumed generations,
verified root opening and descriptor-based manager/worker staging. It runs after
channel authentication; calling it directly does not authenticate a sender.
A mandatory prepare operation must authorize the inventory/root/namespace and
retain its own network references. The portable test substitutes a loopback-only
network fixture, not a production allocator. Failed preparation consumes the
generation, and retirement uses actual manager join/endpoint closure followed by
an asynchronous network-release acknowledgement. No EMPTY/drained transition is
reported while network release remains pending. STOP drains all existing slots
and prevents new publication for the session.

Targeted plain/ASan+UBSan and TSan controls exercise actual staging threads, replay
and stale detach, delayed network retirement, a surviving neighbor and STOP.
The adapter has not yet been called by the SDK-managed helper or a live network
allocator. Real namespace type validation, inventory authorization, address lease
lifetime and transport status replies remain required at that boundary.

The adapter now validates NSFS references as network namespaces before prepare:
identity with the known `/proc/self/ns/net` object proves the current namespace;
a different object must pass kernel `setns(..., CLONE_NEWNET)` in a disposable
thread. The controller thread never enters it. This rejects regular directories
and mount-namespace descriptors; it still does not authorize inventory ownership.
Unexpected worker termination, including a clean stop without an explicit detach,
now retires its generation rather than leaving ACTIVE status behind.

The exact pinned AArch64 dispatch control also passes in the localhost emulator
with unchanged EP147 PID and removal of its temporary executable. It exercises
real worker staging and typed descriptor rejection, but its network preparation
and retirement remain loopback fixtures rather than live interface allocation.

## Lease-owned endpoint context

`public-broker-lease.h` retains a namespace FD and exact endpoint spec only for a
READY lease with matching address/mask/broadcast. Endpoint open rechecks current
lease generation/readiness before calling the real namespace socket factory.
A periodic readiness failure requires dispatch retirement; removal is forbidden
until worker join and endpoint close. The release operation advances the lease
to REMOVING, asks an asynchronous callback to remove that exact retained address,
and acknowledges removal only on success. Pending removal keeps its namespace
reference and configured state. An already queued replacement is preserved and
advances only after the old address is acknowledged removed.

Targeted plain/ASan+UBSan/TSan controls pass, using synthetic lease/ARP proof and a
removal callback fixture. They do not execute endpoint socket open, DHCP or real
address mutation. Caller integration must provide inventory authorization, actual
acquisition/configuration/removal and periodic retirement on lease loss. The
complete68/68/68 matrix predates this additional component.

The lease-aware controller poll now couples readiness loss to dispatch retirement.
A combined control starts two real staging workers with synthetic60/120-second
leases, advances the fixture clock to the first expiry and confirms worker join
and endpoint close before the removal callback. Pending address removal keeps
that source DRAINING while the other source remains ACTIVE/READY. Removal
acknowledgement completes the first retirement; STOP then retires the neighbor.
This uses synthetic DHCP/ARP proof, loopback endpoints and fake address removal;
it does not establish live DHCP refresh, real interface mutation or SDK runtime
integration. The same pinned AArch64 control passes in the localhost emulator
with unchanged EP147 PID and removal of its temporary executable.

## Exact asynchronous address removal

`public-address-remove.h` implements a short-lived namespace thread and exact
RTM_DELADDR request using explicit interface index/prefix/IPv4 attributes. It
refuses the native namespace, validates the actual interface MAC/IP/mask/broadcast
inside the target namespace and accepts only a matching kernel ACK on its fresh
netlink socket. The controller polls completion and joins before releasing the
operation; no shell, child reaper or files are involved. The two-second ACK wait
runs off the controller thread. A missing ACK is not proof of removal.

Targeted plain/ASan+UBSan/TSan controls pass for request/ACK parsing, native
namespace refusal and wrong-kind asynchronous cleanup. The pinned AArch64 build
passes. These controls perform no successful address mutation. A live isolated
removal, lease callback integration and SDK runtime remain pending; the earlier
complete69/69/69 suite predates this added component. Operations must remain
allocated until joined, and may start only after actual endpoint/worker drain.

A subsequent live control now passes real removal: in an owned emulator namespace,
192.0.2.3/24 and192.0.2.4/24 are configured on azpub7. The exact hash-verified
AArch64 operation receives a kernel ACK for removing only192.0.2.3; independent
address listing confirms192.0.2.4 remains. Native PID/address/routes/DNS/namespace
are unchanged. The runner removes all owned namespace/interface/module/files.
Portable negative controls repeat under plain/ASan+UBSan/TSan. No music endpoints
ran; live lease-release/eject integration and target4.4 runtime remain pending.
See `acceptance/usb-hub-support-sharing-live-address-remove.json`.

## Live endpoint-to-address retirement

`public-broker-address-retirement.h` supplies one retained removal operation per
incarnation. Failure remains terminal until explicit reset, changed specs return
STALE and a live operation cannot reset. The lease removal callback now runs this
operation in an emulator control after actual source-manager drain. The real
namespace socket factory opens all service endpoints, an empty synthetic PDB
stages to RUNNING from a pinned root, then withdrawal/wanted0 stops and joins the
worker and closes endpoints before exact netlink removal. Only kernel success
allows lease removed acknowledgement. A neighboring address stays configured;
native state survives and all owned resources are removed.

Targeted plain/ASan+UBSan/TSan cover the portable negative branch. The live branch
uses the pinned AArch64 build without sanitizer instrumentation. Lease/ARP proof
and the empty library are synthetic: this does not establish real DHCP acquisition,
independent remote browse/load, native eject/unmount, or a loaded remote deck.
SDK command-loop integration and complete live lease allocation remain pending.
See `acceptance/usb-hub-support-sharing-live-broker-retirement.json`.

## Authenticated command-session adapter

`public-broker-session.h` now verifies parent kernel PID/UID, duplicates the
seqpacket channel and sends HELLO with its per-session token. Each poll advances
source draining even under response backpressure, handles at most one input and
retains at most one response. Received namespace FDs close after dispatch. Source
status carries generation/phase/error; STOP completion is a distinct status sent
only after every source is EMPTY. EOF or invalid token initiates retirement and
keeps context alive until drained. A controller fault is not terminal and does
not authorize releasing worker/network state.

The control sends actual SCM_RIGHTS SOURCE messages through a socketpair into
real manager/staging threads. It covers STOP completion, parent disconnect while
ACTIVE and wrong token before publication. The hash-verified pinned AArch64
control passes in the localhost emulator, preserving EP147 PID and deleting its
executable. Network callbacks remain loopback fixtures. This session adapter is
not yet wired into the SDK-launched executable mode: source inventory, real
allocator preparation and runtime status handling remain integration work.

The response-backpressure control now saturates the helper send queue against a
non-reading parent, verifies the retained HELLO cannot send, then disconnects the
parent while a source is ACTIVE. Polling still joins/closes the source and waits
for an intentionally pending network release before terminal completion. Targeted
plain/ASan+UBSan/TSan pass. This fills the earlier saturation-test limitation; the
SDK executable-mode and real allocator integration gate remains unchanged.

## Executable service integration control in progress

The explicit `sharing-broker-service-emulator` target now builds a separately
executed helper mode using the authenticated session, dispatcher, verified-FD
manager and actual namespace socket factory. It is compiled separately from the
ordinary helper and rejects kernels other than6.6.138-dirty. Its one-source
fixture only accepts the exact synthetic `/tmp/azhub-sdk-library` root declaration,
azpub7, fixed test MAC/player/label and externally prepared192.0.2.3 address.
The runner owns namespace/address creation and cleanup. STOP/parent loss drains
workers/endpoints before releasing helper namespace references.

The shared private socket connection/authentication routine is reused by the
existing IPC-only probe. Pinned AArch64 ordinary/service helper builds pass.
Runtime service-mode/SDK launch acceptance has not yet been performed. This
one-source integration fixture does not reduce the goal's supported-source scope
or implement production DHCP/namespace allocation. The ordinary helper's existing
DHCP/probe modes remain separate; no default deployment or publication occurs.

## SDK-managed executable service result

The subsequent explicit service trial now passes inside EP147. The parent sends
an encoded synthetic SOURCE with its namespace FD; the actual SDK-launched helper
reports STARTING then ACTIVE after real namespace socket creation and staging.
STOP reaches EMPTY/service completion before core liveness ends. Managed and peer
PIDs are absent. Exact mapped mod/helper hashes are recorded. The runner restores
the original package/stage/approval/optional marker state and absent helper asset,
starts normal0.3.2, and removes its namespace/interface/module/library/temp files.
Empty emulator decks were visually inspected before installation.

This supersedes only the earlier executable service launch gate. Address/interface
preparation is still external, the PDB is empty and no independent client browsed
or loaded a track in this trial. Production inventory/namespace/DHCP allocation,
full source concurrency and native eject/loaded remote tests remain pending.
See `acceptance/usb-hub-support-sharing-sdk-broker-service.json`.

## SDK helper with real library and independent clients

The held service control now passes with the real mounted emulator hub root
aliased by bind mount to the declared test root. EP147 launches the exact helper
through core, sends SOURCE/namespace FD, waits for ACTIVE, then holds until the
runner finishes independent observers. Unchanged Beat Link discovers player41
and validates all2546 tracks/185 ordered nodes/8359 memberships. Concurrent
unchanged Crate Digger clients fetch exact-hash PDB and MP3 bytes. A runner-owned
done marker releases STOP; helper endpoints/workers drain and both managed/peer
PIDs disappear before success. No music is copied into source or uploaded.

Original package/stage/approval/marker/absent helper state is restored. The bind
alias is unmounted before deleting its empty mountpoint, preserving the original
library; guest namespace/interface/module, host TAP transport and temporary files
are cleaned. The hold/ready/done markers are explicit emulator instrumentation,
not production activation IPC. Address/interface setup remains external. This
single fixture does not establish production allocation, eight-source concurrency,
local playback concurrency, native eject or a loaded external deck. Actual-CDJ
and target4.4 runtime acceptance remain separate.
See `acceptance/usb-hub-support-sharing-sdk-broker-library.json`.

## Remaining allocator integration boundaries

The standalone allocated-network/library path does not yet replace SOURCE's
externally supplied namespace FD in the managed-helper fixture. Production
integration needs a distinct authenticated allocation operation carrying the
verified root capability and incarnation, with namespace/interface ownership
inside the helper. Do not reinterpret the current SOURCE namespace descriptor
as a root descriptor without a distinct versioned operation. Pending allocation
must be cancellable before publication; detach/parent loss must still reap the
DHCP client and join allocation/configuration threads before unmount can proceed.

DHCP-only fixture success does not settle networks without a DHCP server,
production MAC/player collision allocation, refresh, cross-source budgets or
model-specific external-player limits. Those gates remain part of the original
objective, together with loaded-deck safe eject and simultaneous local playback.
No fixture success is a reduced completion definition or actual-CDJ acceptance.

## Distinct allocation command and publication gate

ALLOCATE is now a distinct authenticated wire opcode with one root capability FD,
not SOURCE's namespace reference. The decoder retains bounded payload/generation
rules. Dispatch validates root type/read access/device/inode and pins a CLOEXEC
reference without reopening its pathname. An optional allocate callback starts
owned network preparation; its ready callback is polled before manager publication
and through ACTIVE. Pending cancellation closes the retained root and waits for
release; readiness loss after activation retires the source. Legacy SOURCE keeps
its existing externally prepared-namespace semantics.

Targeted wire/dispatch plain/ASan+UBSan/TSan and pinned AArch64 controls pass,
including pending cancellation, pathname replacement before staging and readiness
loss. Mod/service helper builds pass. The allocation callbacks are fixtures in
these controls: the real DHCP/ARP controller still needs wiring into the executable
and a subsequent actual SDK ALLOCATE trial. The earlier full76/76/76 suite predates
this command. Adding an opcode alone is not production allocator acceptance.
See `acceptance/usb-hub-support-sharing-broker-allocate.json`.

## Real allocator callbacks

`public-broker-allocator.h` now connects ALLOCATE to owned namespace/MACVLAN/native
DHCP acquisition, lease/ARP/configuration readiness and the namespace endpoint
factory. Inventory authorization, peer policy, monotonic clock and jitter are
mandatory caller inputs. Readiness polling includes declared native MAC inventory,
the parent MAC and all allocated source MACs. Dispatch publication still waits
for usable readiness; no native USB library slot is borrowed.

Release runs only after manager drain, cancels/reaps unfinished acquisition,
quiesces pending ARP/configure threads, and retires a configured address through
the exact netlink operation before closing namespace ownership. A removal fault
retains ownership and is not successful drain. Pre-publication rollback with no
acknowledged configuration closes the isolated namespace after all work joins.

Targeted plain/ASan+UBSan/TSan plus pinned AArch64 controls cancel eight pending
root capabilities through these actual callbacks, verify no publication and
restore descriptor count to baseline. The emulator preserves EP147 PID. This
cancels before DHCP/MACVLAN creation: successful callback readiness/retirement and
SDK executable integration are still unvalidated at this boundary. It is not
successful eight-source networking or actual-CDJ evidence.
See `acceptance/usb-hub-support-sharing-broker-allocator.json`.

## Positive real ALLOCATE callback path

A standalone research control now passes the encoded ALLOCATE frame directly to
dispatch with a pinned real hub root. Actual allocator callbacks create namespace/
MACVLAN, supervise native DHCP, complete ARP/configure and open source-owned
endpoints before ACTIVE. Unchanged independent clients discover/browse all2546
tracks/185 nodes/8359 ordered memberships and concurrently fetch exact PDB/MP3
hashes. STOP runs the real release callback through worker join/endpoint close,
ARP/configure quiesce and exact address removal before final namespace release.
Native state survives and complete module/TAP/temp cleanup has no transport errors.
Portable77/77/77 passes at this boundary.

This supersedes the earlier positive-callback gate, not SDK IPC/executable
integration. The command is a direct dispatcher call here. The fixture uses one
source, fixed jitter/MAC/player, a600-second DHCP grant and hardcoded root
inventory. Concurrent local playback, remote-loaded native eject, refresh,
conflicts, no-DHCP networks and actual-CDJ/target4.4 acceptance remain pending.
See `acceptance/usb-hub-support-sharing-broker-allocated-library.json`.

## SDK-managed owned allocation result

The explicit `sharing-broker-allocate-emulator` executable now passes the full
SDK/IPC allocation boundary. EP147 opens the selected root, sends ALLOCATE with
its verified root capability, and the core-launched helper owns anonymous
namespace/MACVLAN creation, native DHCP/reaping, ARP/configuration and endpoint
publication. The runner supplies no guest namespace/interface/IP. Its host-side
isolated DHCP fixture issues a600-second grant; unchanged clients discover the
source, browse2546 tracks/185 nodes/8359 memberships and concurrently download
exact PDB/MP3 hashes. STOP drains workers/endpoints and retires network ownership
before completion and managed/peer process exit.

The exact mod/helper/hook and mapped mod hashes are checked. Root and namespace
inode fields are distinct in this mode. Original package/stage/approval/marker
state and absent helper/hook assets are restored; bind alias, module, host TAP,
fixture processes and temporary files are cleaned. Normal EP147 is active again.
The preflight UI showed empty decks. This supersedes the SDK-owned-allocation
integration gate, not production activation or full acceptance.

The mode remains explicitly compiled/gated to kernel6.6.138-dirty and one declared
fixture root/MAC/player with fixed jitter. Production inventory and identity
policy, no-DHCP networks, renewal/conflicts, multisource/local playback and native
loaded-remote eject remain pending. Previous full77/77/77 controls precede this
new executable mode; this positive live branch is not sanitizer-instrumented.
See `acceptance/usb-hub-support-sharing-sdk-allocated-library.json`.

## Managed IPC loss during pending DHCP

A subsequent negative control withholds OFFER after observing native DISCOVER.
The runner identifies the live udhcpc PID by its owned hook, confirms no readiness
marker and signals EP147's probe to close the peer channel. The helper cancels/
reaps its pending acquisition, retires namespace/interface/root references and
exits managed lifetime. The native client, managed PID and peer PID are absent
before success; no ACTIVE status or library-ready marker occurred. The same
helper hash as the positive SDK-owned-library trial is used.

Original state and absent helper/hook assets are restored. Namespace/module/bind/
TAP/fixture/temp cleanup completes and normal EP147 is active. This proves the
pending-acquisition disconnect path, not native USB unmount/eject, a loaded remote
deck, neighbor concurrency or renewal. The live branch is not sanitizer-instrumented.
See `acceptance/usb-hub-support-sharing-sdk-pending-dhcp-cancel.json`.

## Managed conflict recovery and next integration boundary

The SDK recovery fixture now consumes a second publication generation only after
EMPTY for the failed first generation. It retains the verified root capability
and public identity. Allocator retry policy lives outside disposable namespace
state: DECLINE records a minimum ten-second delay, and repeated conflicts retain
the stricter rate limit. The independent DHCP fixture observed10.126 seconds
before fresh discovery, then granted192.0.2.4 instead of the conflicted192.0.2.3.
Player41 and the client MAC remained unchanged. Independent complete menus and
exact concurrent PDB/audio downloads pass, followed by managed STOP, exact
rollback and owned-resource cleanup. See
`acceptance/usb-hub-support-sharing-sdk-conflict-recovery.json`.

A targeted allocator control also cancels while waiting for backoff, before any
namespace acquisition or DHCP start. It verifies retained retry policy, no
worker publication and descriptor cleanup, then accepts a later generation.
Plain, address/undefined and thread sanitizer executions pass this control.

The existing `public-mod-emulator.h` inventory adapter is not the production
allocator: it consumes runner-created namespace/address fixtures. Its existing
`eject_reserved`, `unmount_pending` and manager-drained gates identify the next
integration boundary. Replace the direct manager connection with authenticated
helper commands while preserving these ordering requirements:

1. Capture each present root as a verified directory capability; correlate its
   device/inode and source incarnation independently from command generation.
2. Publish ALLOCATE only after helper authentication. Keep inventory transitions
   pending under IPC backpressure; never acknowledge a command before sending it.
3. On eject reservation, stop publication/retries and send DETACH for the exact
   current generation. A reservation must win over recovery and replacement.
4. Allow native unmount only after the matching EMPTY acknowledgement, which
   proves worker join, endpoint closure and network retirement. Peer loss alone
   cannot prove that owned helper resources have drained.
5. Reconcile replacement roots only after the preceding generation drains.
   Preserve neighboring source operations and cache ownership throughout.

Production identity allocation, renewal/no-DHCP policy and helper restart
reconciliation remain prerequisites; this fixture does not validate them or
actual-CDJ interoperability.

## Inventory client reconciliation component

`public-broker-client.h` now owns desired root capabilities separately from the
last successfully sent generation. Its single-controller send operation commits
only after a complete seqpacket send; EAGAIN/EINTR leave desired state pending.
A changed root/incarnation first detaches the published generation and waits for
its authenticated EMPTY status before allocating the replacement. Stale statuses
cannot release the newer generation. Eject clears desired state, suppressing an
otherwise eligible address-conflict retry. Losing the helper connection keeps
any outstanding generation undrained; it is not an unmount acknowledgement.

The socketpair control fills the actual IPC queue, transfers/verifies a directory
capability, checks replacement and stale-status ordering, retains a neighboring
ACTIVE source and cancels conflict recovery before testing channel loss. Plain,
ASan/UBSan and TSan pass; the pinned AArch64 compiler accepts the component.
It is included in the host suite. This is not yet wired into the native inventory
or SDK helper lifetime. Authenticated HELLO and helper-death reconciliation remain
caller responsibilities; the component refuses further sends after channel loss.

The client now also runs against the actual authenticated broker session in a
portable rejection control. Rejected ALLOCATE yields an immediate error and a
repeated EMPTY change notification; both are accepted idempotently for the
current generation, while stale generations remain rejected. A hard ENODEV does
not cause an automatic retry loop. All three host modes pass this extension.
The SDK emulator probe now uses this client for ALLOCATE sends and source status
reconciliation, including deferred sends under backpressure. AArch64 build passes;
the previously recorded live recovery hashes predate this client wiring and must
not be cited as runtime acceptance of the changed probe. Native inventory/eject
integration remains pending.

The subsequent managed emulator trial now validates this client wiring at runtime:
initial conflict, correct-source DECLINE, independently observed10.093-second
backoff and recovered discovery at192.0.2.4 with player41/client identity retained.
Complete menus and exact concurrent PDB/audio downloads pass. Managed helper exit,
exact original-byte restoration and owned-scope cleanup complete successfully.
See `acceptance/usb-hub-support-sharing-sdk-client-recovery.json` for the new
mod/helper hashes. This supersedes the client-wiring live gate only; automatic
native inventory/eject integration and the wider goal remain pending.

## Initial native inventory and unmount gate wiring (not runtime validated)

The SDK probe now has an explicit `AZHUB_SDK_INVENTORY=1` option, exposed by
runner `--inventory-gate`. It requires ALLOCATE and excludes cancellation/conflict
fixtures. On first publication it correlates the selected root capability with
one present native hub source by device/inode, retaining the declared fixture
network policy. While held ACTIVE it observes presence, pending unmount and eject
reservation, withdraws desired publication and sends exact-generation DETACH.
Both existing alias-unmount paths call its gate before releasing the mount.
Only matching EMPTY or completed managed retirement opens that gate; a probe
failure leaves the selected source blocked. The source remains pinned while
retirement is pending. All mutations stay on HubTransport.

Pinned AArch64 whole-mod build passes. This wiring has not yet undergone live
inventory/eject validation; previous runtime evidence predates it. It handles one
selected fixture source and withdrawal only, not replacement/replug publication,
multisource production policy or recovery after helper loss. Normal builds and
probe runs without the explicit option retain their existing paths. Next verify
native matching and independent clients, then trigger native safe eject with
source withdrawal before unmount and retained neighboring local sources.

The first explicit inventory-gate trial now matches native source0 at
`/media/usb/sdc1` and passes independent discovery, all2546 tracks/185 ordered
nodes/8359 memberships and exact concurrent PDB/audio downloads. Managed STOP,
original-byte restoration and owned-scope cleanup complete. No eject was
triggered, so this does not validate the unmount gate. See
`acceptance/usb-hub-support-sharing-sdk-inventory-gate.json`. The subsequent
change closing the parent's root FD upon withdrawal is outside those exact
runtime hashes and awaits the native-eject trial.

## Native selector eject with retained external session

The corrected v4 runner passes the same exact mod/helper hashes as v3, including
complete rollback and cleanup. Before eject, a separate host process opens the
source's metadata service, obtains an RPC root handle and independently discovers
the source with unchanged Beat Link DeviceFinder. After native selector eject,
repository-authored probes observe both RPC aliases unavailable, the retained
metadata TCP session closed and DB-port lookup unavailable. DeviceFinder expires
the source. Native source closure follows helper retirement; alias/backing mount
removal and unchanged neighboring USB mounts are verified. This supersedes v3's
cleanup failure. See
`acceptance/usb-hub-support-sharing-sdk-native-eject-observed.json`.

The observer does not simulate a loaded deck or audio playback. RPC probes are
repository-authored and do not independently prove NFS-client interoperability.
The one-source inventory mode remains a gated emulator integration, without
production activation, replacement/replug or multiple public source acceptance.

## Direct native root and replug preparation (build only)

The inventory fixture now opens the declared native `/media/usb/sdc1` capability
directly; its runner skips creating a temporary bind alias in inventory mode.
The emulator-only helper authorizes this exact native path in addition to its
older explicit alias path. Player/MAC/interface/label and slot restrictions remain.
After a withdrawn generation reaches EMPTY, a present, nonreserved source can
supply a newly verified root capability and incremented incarnation; publication
uses the next client command generation. A reserved/pending-unmount source cannot
republish. Status logging now includes the current generation and phase.

Pinned AArch64 mod/helper builds and Python syntax checks pass. These changes
postdate the observed v4 eject hashes and do not yet have runtime acceptance.
The DHCP fixture has a bounded reconnect option for a second identical client
identity/address grant. Next extend the native-eject runner to replug the owned
virtual device, observe generation2 ACTIVE and verify old-handle STALE plus fresh
file access. Production identities, general mount inventory and multisource
activation remain outside this one-source fixture.

## Native replug v1 exposed acquisition collision (not acceptance)

`local/diagnostics/sharing-sdk-native-reconnect-v1` passes initial direct-root
publication, independent complete menus/files and native eject with external
session withdrawal. After virtual-device replug, generation2 never reaches ACTIVE.
The helper reports EADDRINUSE before a second DHCP exchange; repeated publication
generations follow because the client previously treated every EADDRINUSE as ARP
conflict. Runner times out, then exact originals and owned scope clean up with
its normal finally path. This is a failed reconnect trial, not acceptance.

Allocator readiness now translates acquisition-stage EADDRINUSE to EEXIST. Only
an actual readiness/ACD conflict remains eligible for the parent's conflict
retries. Targeted host controls exercise that distinction. The cause of the
acquisition collision is still under investigation; explicit virtual-interface
retirement, rather than assuming final namespace-FD closure immediately removes
the parent MACVLAN identity, is the next lifecycle boundary to inspect. Do not
claim that namespace/endpoint cleanup alone proves immediate identity reuse.

## Explicit virtual-link retirement component result

The link operation now supports RTM_DELLINK from a disposable namespace thread.
It validates the native parent identity, refuses the native namespace, enters the
owned namespace and verifies the exact named interface MAC before deleting its
index. Fresh netlink kernel ACK and thread join precede success. Allocator release
now waits for this operation after endpoint/address retirement before closing
its namespace and reporting EMPTY. Failed deletion retains ownership/error.

The emulator component control rejects a wrong MAC without removing the interface,
then deletes/verifies absence, recreates the same name/MAC, verifies it and deletes
it again. Native process/address/routes/DNS/namespace are unchanged and module/temp
cleanup passes. See `acceptance/usb-hub-support-sharing-link-retirement.json`.
Targeted network-link/allocator controls pass all three host modes. Managed SDK
reconnect with this change remains pending; no root cause claim is established
by the component control alone. Pending acquisition failures still require a
separate audit of their final namespace/link cleanup path.

## Managed reconnect v2: address-finalization reentry failure

The explicit-link-retirement variant reaches DRAINING during native eject, but
never EMPTY: the parent sees connection reset and the unmount gate stays closed.
The runner times out before replug, restores exact originals and cleans its owned
scope. Evidence remains local in `sharing-sdk-native-reconnect-v2`; this is not
successful reconnect or successful native eject for that variant.

Inspection found that after address-removal ACK and `lease_removed`, a subsequent
release poll still saw `ready.applied=1` and tried to finalize the already-IDLE
lease again. That returns EPROTO, faults the session and closes its channel.
The allocator now clears `ready.applied` after successful finalization. A control
seeds acknowledged address removal and a later retained link-removal failure,
then polls twice: both preserve that later error/namespace ownership, without
re-finalizing the lease. Plain host execution passes; sanitizer checks follow.
The original acquisition collision remains to be retested after this fix.

## Managed native replug v4 passes

The corrected source/helper complete direct-root native selector eject and virtual
stick replug without restarting the native application. Generation2 reaches ACTIVE
with the same public MAC/player41/IP. Independent DeviceFinder rediscovers it,
repository RPC controls get STALE for the old root on both aliases and obtain a
fresh handle; independent Crate Digger downloads the exact PDB again. Initial
complete menus and concurrent exact PDB/audio also pass. Original bytes and all
owned resources clean up normally. See
`acceptance/usb-hub-support-sharing-sdk-native-reconnect.json` for exact hashes.

This validates the explicit-link-deletion and address-finalization fix together
for the managed lifecycle. It does not isolate the original EADDRINUSE cause,
prove physical CDJ behavior or cover production/multisource/renewal/loaded-deck
acceptance. v3 stopped at package-location preflight after log rotation, before
candidate installation; the runner now supports a unique verified USB package
fallback. Helper diagnostics use SDK-supported logging to a unique mod data file,
collected locally and deleted during scoped cleanup.

## Multi-slot emulator authorization preparation

The managed replug boundary passes the complete79/79/79 host regression. Its
exact runtime hashes predate the following authorization extension.

`public-broker-emulator-policy.h` declares eight isolated fixture slots, each
with its own root (`sdc1` through `sdj1` in the prepared emulator topology),
MAC, player41..48, interface and label. Cross-slot combinations and the fixture's
native USB1/2 roots are rejected. Only slot0 permits the historical temporary
alias. The helper independently opens the declared path without symlinks and
compares the verified object with the transferred root capability before starting
allocation. This remains gated to the exact emulator kernel and fixture topology;
these disk letters/player numbers are not a production inventory policy.

The declaration controls pass plain/ASan+UBSan/TSan and join the host suite.
Multiple simultaneous managed allocations/native inventories and neighbor eject
have not yet been exercised with this helper. The parent SDK probe still operates
one selected source; that is the next integration boundary.

## Reusable per-source observation component

`public-broker-inventory.h` now reconciles eight independent observations into
client-owned root capabilities. Incarnations survive disappearance/reservation;
unchanged observations retain the same FD. A changed device/inode or declaration
gets a new verified capability even without a disappearance interval. The client
then orders old-generation retirement before replacement. Reservation, missing
root and overflow withdraw desired publication without claiming remote drain;
neighbor slots retain their capabilities. Observation errors still require the
caller to send/poll retirement.

Eight-root controls exercise replacement without a gap, stable observations,
eject priority, missing paths and incarnation overflow isolation. Plain,
ASan/UBSan and TSan pass. SDK single-source inventory now uses this component,
seeding its initial incarnation from the first publication; its original root FD
closes after successful IPC transfer because the client/helper own their copies.
Pinned builds follow; new runtime wiring has not yet been validated. Multiple
simultaneous managed sources remain pending and previous replug runtime hashes
must not be used as acceptance of this subsequent change.

## Multiple managed native inventories: compiled, not yet exercised

The SDK inventory fixture now accepts an explicit hexadecimal
`AZHUB_SDK_INVENTORY_MASK` containing slot0. Additional selected slots have their
own declared root, observation/incarnation, pinned capability, IPC generation and
native-source mapping. Unmount gates route each native source to its own client
slot. Responses and STOP retirement statuses reconcile by slot/generation rather
than assuming slot0. A separate `.multi` marker appears only once all selected
slots are ACTIVE. Legacy source0 readiness remains separate.

The isolated DHCP fixture supports `--source-slots 0,4` (or another distinct bounded
set containing0), assigning each declared client MAC its own documentation address.
With reconnect enabled it waits for a second grant only for slot0. This is an
explicit fixture, not a general DHCP server or production address policy.

AArch64 compilation and Python syntax checks pass. Read-only emulator hashes show
`/media/usb/sdc1` and `/media/usb/sdg1` contain the same known PDB, so those two
independent mounted volumes can use the existing full-menu oracle without copying
music. No simultaneous runtime result is claimed yet. Runner/client orchestration,
neighbor continuity through eject/replug and final cleanup validation remain next.

## Two-source runner and transport boundary

The explicit `--two-sources` runner selects native slots0/4, waits for both ACTIVE
and uses concurrent complete independent Beat Link menus. Its external neighbor
control retains the other source's root handle and original TCP menu session
through selected-source eject/replug. Client wrappers now accept noncontiguous
fixture indices; the underlying Beat Link/Crate Digger libraries are unchanged.
Only the two pre-existing matching PDB fixtures use this oracle.

The first trial (`sharing-sdk-two-sources-v1`) fails independent discovery before
menus/eject. Exact originals and owned scope restore normally. The DHCP fixture
logs slot4 OFFER without its ACK, while the helper reports ACTIVE for slot4.
Inspection found the transport suppressed competing NAT DHCP replies only for
slot0's MAC, leaving slot4 eligible for QEMU DHCP. This identifies a fixture
isolation defect; the actual second grant/address was not captured, so no exact
wrong-address claim is made.

The transport now suppresses NAT DHCP only for an explicit bounded set of fixture
MACs (`--fixture-dhcp-slots 0,4`), preserving unrelated/native DHCP. Nine transport
controls pass, including selected versus unselected clients, malformed/truncated
packets and invalid/duplicate slot lists. The corrected two-source live trial is
still pending. This is not multiple-source runtime acceptance.

## Two-source v2: menus/neighbor pass, reconnect observation defect

With NAT DHCP suppression corrected for both selected MACs, the v2 trial receives
both intended grants and independently discovers/fully browses both libraries in
parallel. An additional Crate Digger fetch from slot4 matches the complete PDB
hash. The neighbor's original RPC handle and TCP menu session survive selected
source native eject and the runner's replug signal. The final source0 rediscovery/
fresh download fail; the runner restores originals and cleans its owned scope.
No full two-source lifecycle acceptance is claimed.

The readiness wait searched the entire append-only mod log and could match an
older generation2 ACTIVE line. That permits sending the reconnect signal and
starting fresh file access before the current generation is ready. Logs show
the current generation eventually ACTIVE, but that alone does not rescue the
failed external assertions. The runner now scopes lifecycle predicates and
ordering checks to the current managed launch, validating its live peer command
against the unique run socket (or completed run IDs for terminal collection),
and requires slot0 explicitly. It preserves current-run logs on failure.

Earlier records that searched unscoped logs have weaker ordering evidence than
this corrected verifier; their independent external observations remain their
recorded scope. Repeat the full two-source trial before claiming acceptance.

## Two-source v3: lifecycle passes, probe STOP rejects neighbor status

With current-run readiness/ordering predicates, both source menus pass concurrently,
independent slot4 PDB/audio downloads match exact hashes, and selected native eject/
replug preserves the neighbor's original RPC handle and TCP menu session. The
reconnected source is independently rediscovered, old handles are STALE and a fresh
PDB fetch matches. The final probe fails in STOP acknowledgement, then exact original
restoration and owned-scope cleanup complete. Evidence remains in
`local/diagnostics/sharing-sdk-two-sources-v3`; full runner success is not claimed.

The STOP branch retained its old `slot==0` assumption, rejecting slot4's valid
retirement status even though the client had reconciled it. It now accepts each
selected slot's exact generation, and requires every client slot EMPTY before
accepting global STOP completion. The corrected probe must pass the full runner
before this boundary becomes complete acceptance.

## Two managed native sources: complete v4 runner passes

The corrected probe/runner passes slots0/4 with distinct root capabilities,
MACVLAN namespaces, client MACs, DHCP addresses and public players41/45. Unchanged
DeviceFinder discovers both and concurrent Beat Link clients browse all2546
tracks/185 ordered nodes/8359 memberships on each. Crate Digger downloads exact
PDB/audio hashes from both within the runner.

Selected-source native selector eject/replug preserves the neighbor's original
RPC root and original TCP menu connection. Cross-source handles are rejected.
Source0 is rediscovered with generation2, both RPC aliases reject its old handle
as STALE and an exact fresh PDB fetch succeeds. Predicates/order checks use only
this run; global STOP waits for every client slot EMPTY. Managed exit, exact
original-byte restoration and owned-scope cleanup complete with exit0. See
`acceptance/usb-hub-support-sharing-sdk-two-sources.json` for exact runtime hashes.

This supersedes the earlier two-source verifier failures, not the broader goal.
Only two matching fixture libraries are covered; general production policy,
renewal/no-DHCP behavior, simultaneous local/remote playback, external loaded-deck
eject, more-source budgets and target4.4/model-specific physical acceptance remain.

## Native/local concurrency runner preparation

The completed two-source boundary passes the full81/81/81 host matrix.
A new explicit runner option `--local-playback` now prepares a native deck using
physical emulator UI controls, verifies the load's alias filesystem identity
against `sdc`, starts a four-beat loop and samples the same native process/content
identity, loaded/playing flags and position during independent external clients
and after eject/replug. It enables native load tracing and saves a local pre-load
screenshot. Source-row coordinates are specific to the prepared emulator topology;
the load cannot pass solely on matching titles or PDB content.

Python syntax passes; this new mode has not been executed yet. Its planned
telemetry evidence is native engine state/position, not an audio-signal capture
or external loaded-deck acceptance. Do not cite the completed two-source trial
as simultaneous local playback acceptance.

## Local concurrency v1: wrong native row rejected

The first local-playback trial loads the duplicate library through native USB5,
not the intended `sdc` source. The independent native alias/backing device check
rejects it (8:97 versus8:33) before external client acceptance. Exact original
restoration and owned-scope cleanup pass. This validates the guard, not local/
remote concurrency. The saved native screenshot corroborates the USB5 badge.

Native source order changes after replug. The runner now saves the actual source
menu and waits bounded90 seconds for a locally reviewed `native-source-row.txt`
in its ignored evidence directory. The orchestrator can inspect the screenshot
and provide the measured row offset, without asking the user or changing device
internals. It still verifies the native load's physical backing independently;
matching labels/titles alone cannot pass. The repeat trial is pending.

## Native/local plus remote clients: v3 runner passes

After selecting the reviewed first visible source (row offset-1), the backing
identity check confirms the native load belongs to `sdc`. Content495 remains
loaded/playing on the same native PID across73 samples while both public sources
undergo independent complete parallel menus and exact PDB/audio downloads. A
four-beat loop advances/wraps in the sampled2.939..4.932-second interval. The
selected local loaded backing is safely ejected and replugged, with the native
loop still loaded/playing afterward. The neighbor's original RPC/TCP menu state
survives, source0 rediscovery/old-handle STALE/fresh exact PDB pass, and global
STOP, exact original restoration and owned-scope cleanup complete successfully.

See `acceptance/usb-hub-support-sharing-sdk-local-remote.json`. The same exact
mod/helper hashes as the two-source v4 protocol trial are used. This is native
engine state/position evidence without an audio signal capture. It does not
simulate an external loaded deck, prove actual-CDJ behavior or validate production
activation/renewal/large-source budgets/target4.4 runtime. The older v2 selector
bound rejected the correct-1 offset before loading; its cleanup completed.


## Target MACVLAN asset loading under integration

The target-only `--broker-share` helper now prepares its MACVLAN prerequisite
after authenticating the parent control connection and before any allocation or
namespace creation. `public-macvlan-asset.h` independently requires kernel
`4.4.194`. The normal EP147 process never executes this loader. Emulator helper
modes do not compile this target-only call.

If `/sys/module/usb_hub_macvlan` is already registered it is reused. Otherwise,
the helper resolves its actual executable through `/proc/self/exe`, requires
its parent directory to be named `assets`, pins that directory without following
a leaf symlink, and opens only sibling `usb_hub_macvlan.ko` with `O_NOFOLLOW`.
The module must be a regular file. `finit_module` receives empty parameters and
flags0; no force flags, modprobe fallback, system module installation or internal
settings edits are used. EEXIST is accepted only if the expected module is now
registered; apparent success without registration fails. All descriptors close.

This registers a kernel driver in memory; it does not promise that disabling or
reloading a mod unloads the driver. Interface/namespace lifetime remains owned by
the helper, and the kernel module's own reference counting still applies. The
framework's persistent staging/settings behavior is unchanged.

Portable syscall-mocked tests cover the exact kernel gate, sibling asset policy,
missing/symlink/nonregular refusal, load errors, registration races and descriptor
cleanup. The target helper exits126 on the6.6 emulator with module inventory
unchanged. Neither result proves target4.4 symbol resolution, ABI or successful
loading. The existing target module build/static-export record remains the
strongest target evidence; a separately authorized physical trial is still needed.
No package, release or physical installation follows from these local builds.

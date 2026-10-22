# Isolated PRO DJ LINK protocol peer

This research harness uses Beat Link 8.0.0 to establish native emulator behavior
before testing USB Hub Support coexistence. It is not a music server or an audio
renderer, and successful packet exchange does not establish audible beat sync.
Beat Link is an external EPL-1.0 dependency; dependency binaries stay local.

## Network prerequisites

### Physical LAN receive-only preflight

The user authorized an exploratory Wi-Fi trial on the existing home LAN. This is
separate from the isolated emulator acceptance below. Do not remove the emulator
harness isolation guards or treat Wi-Fi observations as Ethernet/audio acceptance.

`../scripts/link-lan-observe.py` is a bounded receive-only Windows-compatible
preflight. Supply the actual host IPv4, target AZ IPv4, subnet prefix and duration
(1..60 seconds). It verifies the local address exists, exclusively binds UDP50000,
50001 and50002, and records only target-origin PRO DJ LINK packets. It sends no
traffic and does not change interfaces or firewall rules. Packet prefixes and
addresses belong in ignored local output. If rekordbox or another application
owns these ports, stop and ask the user to close it after saving work; do not kill
the application or silently share its sockets. No observed packets alone cannot
distinguish firewall/broadcast filtering from native Wi-Fi protocol behavior.

`LinkPeer ROLE PLAYER SECONDS BPM --physical-lan LOCAL_IPV4 AZ_IPV4` explicitly
opts into the user-authorized physical LAN trial. Only status or negotiated handoff roles and durations
up to60seconds are allowed. Assigned private IPv4/subnet and unique matching host
interface are checked; discovery must find the explicit AZ and no other DJ device,
and the requested player number must be free. The emulator invocation and its
isolation guards remain unchanged. Windows trials used
`-Djava.net.preferIPv4Stack=true`; keep physical build output in a separate ignored
directory so the prior emulator class files remain intact. No master request is
made by status role. Handoff requires an observed native master, requests ownership
and aborts if confirmation does not arrive within five seconds. It does not send a
remote SYNC command: the user enables SYNC on the physical deck. Capture stdout/stderr
and retain shutdown warnings honestly.

### Normal-loader primary interface

The current normal-loader fixture uses one native interface at `192.0.2.15/24`,
gateway `192.0.2.254`, DNS `192.0.2.253`, with management SSH forwarded only on
workstation loopback. The isolated peer owns `192.0.2.2`. Its QEMU backend has:

```text
-chardev socket,id=linkcap,host=127.0.0.1,port=15101,server=on,wait=off
-object filter-mirror,id=linkmirror,netdev=net0,queue=rx,outdev=linkcap
-chardev socket,id=linkinject,host=127.0.0.1,port=15102,server=on,wait=off
-object filter-redirector,id=linkingress,netdev=net0,queue=tx,indev=linkinject
```

Prepare the host-only namespace explicitly in a root WSL shell, keeping that
shell open while the transport runs:

```sh
ip netns add azhub-link
ip -n azhub-link link set lo up
ip netns exec azhub-link ip tuntap add dev azlink0 mode tap
ip -n azhub-link addr add 192.0.2.2/24 brd 192.0.2.255 dev azlink0
ip -n azhub-link link set azlink0 address 02:00:00:00:02:02
ip -n azhub-link link set azlink0 up
```

From Windows, at the repository root:

```text
python src/azhub/research/scripts/link-primary-tap-transport.py --seconds 600 --nat-port 15104
```

The bridge verifies the existing namespace and creates a temporary QEMU NAT
capture channel through the fixed loopback QMP endpoint4446. It forwards ordinary
NAT management traffic and suppresses NAT packets claiming the TAP peer's address.
Without that separation, slirp sees mirrored native SYN-ACKs for connections it
did not create and sends resets, invalidating TCP library tests even though UDP
discovery and tempo work. Both TAP traffic and NAT management return through the
existing injection channel. No guest file, interface or process is modified.

Normal completion removes the temporary redirect before closing its channel.
Inspect any cleanup error. Before forcibly terminating the bridge, remove QMP
object `azhubtapguard` so management traffic returns directly to NAT. A capture
port already held by QEMU is a setup refusal; inspect its owner or select another
unused loopback `--nat-port`. The bridge does not restart QEMU or the native application.

Run `../scripts/test-link-primary-tap-transport.py` for framing and ownership
guards without networking. Use `--target 192.0.2.15` with `link-library-peer.py`
and `link-udp-private-peer.py`; both reject arbitrary LAN targets. The library
probe fetches media information only, not track content. Its reserved requester
5/6 trials test admission, while requester3 is the native response control.

### Earlier separate-interface baseline

Use a separate QEMU socket backend listening only on `127.0.0.1:15100` and a
dedicated virtual NIC. The guest's native `eth0` must use `192.0.2.1/24` with
broadcast `192.0.2.255`. Keep SSH on the separate management NAT interface.
Current Windows QEMU does not support adding a PCI NIC to its existing root bus
at runtime; include this NIC at boot. With virtio-mmio devices, declaration order
can change guest interface numbering. Check MAC addresses before configuring it.

`../scripts/link-wsl-transport.py` connects that socket to a WSL TAP. It creates
the dedicated `azhub-link` network namespace with only loopback and `azlink0`
(`192.0.2.2/24`, broadcast `192.0.2.255`). Run it from Windows with Python after
QEMU starts. WSL must support network namespaces and `/dev/net/tun`. Existing
namespace creation fails explicitly: inspect and remove an unused test namespace
before starting a fresh transport. Never attach a host bridge or physical NIC.

The Java harness refuses any non-loopback interface other than `azlink0` or an
unexpected non-link-local address. It must run inside that namespace. The older
`link-ethernet-peer.py` is a standalone ARP/ICMP smoke peer; it cannot share the
QEMU socket with the WSL transport.

## Build and run

From this directory, using Maven and Java 11 or later:

```sh
mvn compile dependency:copy-dependencies -DoutputDirectory="$PWD/../../../../local/link-peer-build/dependencies"
```

Build products default to the repository's ignored `local/link-peer-build`.
Override `research.build.directory` and `maven.repo.local` when necessary. With
`BUILD` set to that absolute Linux path, run:

```sh
ip netns exec azhub-link timeout 65 java -cp "$BUILD/classes:$BUILD/dependencies/*" LinkPeer status 3 30 120
```

Arguments are role (`observe`, `status`, `master`, `handoff`, `follower`), player (1..4),
duration in seconds (1..3600), and BPM (40..250). Select a free physical player
number; the current explicit-number mode can deliberately collide with an
existing device, so inspect discovery first. `observe` only receives discovery
and beats; it does not start VirtualCdj or request detailed status. `status`
announces the peer and sends status. `master` also starts its clock and requests
tempo-master ownership. `follower` enables its simulated playback and sync.
An outer process timeout bounds discovery/startup as well as the observation.

Use `handoff` when a native deck is already master. It waits up to ten seconds
for an observed master status, logs its player number, requests ownership, and
requires confirmation within five seconds before applying the requested tempo.
The original `master` startup may assert ownership directly if Beat Link has not
received the existing master's status yet; do not use that race as evidence of
native handoff failure. Independent wire capture must confirm the request/reply,
and continued status must establish that ownership remains consistent.

Stdout contains JSON event records, stderr library diagnostics. `stopping`
precedes cleanup: subsequent `lost` callbacks may be caused by finder shutdown
and are not evidence of spontaneous peer disappearance. Only explicit live
disconnect/reconnect trials can establish that behavior.
Unknown BPM65535 is retained in `bpm100` with `effective_bpm: null`; do not treat
empty-deck placeholder values as a measured tempo.

Run `../scripts/link-packet-observer.py` inside the same namespace for independent
bidirectional Ethernet metadata. By default it retains no payloads; the explicit
`--status-payloads` option retains complete1152-byte status payloads for privacy
analysis. Keep those captures local and ignored. The observer distinguishes wire
packet lengths from the Java library's shorter receive buffer.

## Established limits

Native AZ status datagrams were observed at 1152 bytes, while Beat Link 8.0.0
receives status into a 512-byte buffer. Its status fields therefore describe
only the received prefix. It also warns about AZ packet type `0x80`. Do not
assign complete AZ packet support to these results or hide these warnings.
The initial accepted baseline covers discovery, peer assignment, incoming native
status and peer beat transmission. Audio, loaded-deck master handoff, follower
behavior and mod coexistence remain separate gates.


## Independent native-library baseline

`LibraryPeer` uses the unchanged Beat Link8.0.0 `MetadataFinder` and `MenuLoader`
APIs to discover native player1 USB media and fetch root, playlist and track
menus. It emits item counts, never titles or returned binary data. It supports
only isolated `azlink0` at192.0.2.2; there is no physical-LAN option or custom
private hub-slot mapping. Run inside the guarded emulator namespace with the
existing bounded TAP transport and an outer process deadline. Do not run it on
the host or mistake menu retrieval for audio-file transfer or hub sharing.

Build through this Maven project, or compile with `javac` against the existing
local dependency directory. The native baseline currently passes7 root entries,
20 playlist entries and2547 tracks. Beat Link emits asynchronous shutdown
warnings from CrateDigger's mount listener after DeviceFinder stops; these are
recorded separately from successful menu retrieval and need harness cleanup
before treating the client as a clean regression gate.


`LibraryPeer --fetch-native-database` additionally fetches native `/C/`'s exported
rekordbox database through unchanged Crate Digger0.2.1 FileFetcher. The alternative
`--fetch-native-audio` fetches one explicitly selected local sparse-fixture MP3.
Both modes emit only byte count/SHA256 and delete the temporary copy in `finally`.
No physical-LAN mode is available. Successful audio-file transfer is not an audio
renderer, a remote deck load, or actual-CDJ acceptance.

`../scripts/link-rpc-observer.py --seconds N` records isolated RPC call/reply
headers, endpoint ports and byte counts. It never records paths, handles or file
payloads. It skips fragmented IPv4 datagrams rather than reassembling them;
therefore a trace can have many NFS READ calls but few recorded READ replies.
Use the independent complete-file hash comparison for transfer correctness,
not unmatched RPC counts. Capture and namespace setup remain emulator-local.


`LibraryPeer --probe-shared-address` first repeats the native player1 control,
then requests native player2's USB root using the same unmodified MenuLoader.
It reports success count or exception class and logs the error text locally.
The current native endpoint replies as player1; unchanged Beat Link rejects the
player2 target. This probes destination identity, not actual-CDJ compatibility.
A public hub design must resolve this rather than relaxing the independent
client's target checks or borrowing a native USB slot.


`FixtureFileClient` uses unchanged Crate Digger FileFetcher against the isolated
host-side fixture provider at192.0.2.2. It refuses the host namespace and any
active non-loopback interface other than azlink0. The provider must explicitly
enable `--fixture-self-client`, which permits the same-namespace client and
adds an isolated portmapper111 control. Default emulator-facing provider mode
continues to accept only192.0.2.15. This control verifies independent file
transport implementation, not a physical client, deck playback or on-device
export. It fetches only the selected database and fixture audio, prints byte
counts/hashes and deletes temporary results.

The provider's optional `--fixture-files` takes an ignored local JSON mapping
virtual absolute paths to explicit operator-selected local files. It pre-opens
read-only streams; network lookup strings never become host filesystem paths.
The virtual tree exposes only those files and their parent directories. No
write operations exist. Withdrawal closes all streams and invalidates handles.
Run `python src/azhub/research/scripts/test-link-fixture-files.py` for synthetic
transfer/EOF, traversal/write refusal, listing and withdrawal controls. This
research backend is not packaged or enabled by the production mod.


The host provider uses a bounded pool of eight sessions per TCP listener. Each
metadata connection owns its menu map and negotiated row format; requester
numbers do not share mutable menu state in this prototype. Excess sockets are
closed, and shutdown closes and joins all owned sessions.
`test-link-sessions.py` exercises two real socket sessions with one idle client,
independent menu contents, capacity refusal and pending-greeting shutdown. It
runs on Windows and Linux without network discovery or a physical interface.
This is a transport/session control, not multi-CDJ hardware acceptance.


The public provider fixture supports explicit addresses192.0.2.2..9 only
inside its guarded single-TAP namespace. Both the provider and primary bridge
require `--fixture-address-count N` matching the exact namespace address set.
The default remains one address. The bridge suppresses NAT IPv4/ARP source
traffic for each declared fixture address. This does not implement production
IP allocation or permit physical LAN aliases.

`link_activation.py` models the initial0x46 acknowledgement, delayed0x16
repeat and subsequent0x47 replies for one provider; its tests exercise timing
and independent provider state. It is reconstructed protocol research, not
a native-PC capacity conclusion. Public identities remain fixed in periodic
announcements. Multi-provider visibility must be measured separately.


### Synthetic loaded-deck status control

`LoadedDeckStatusControl --self-test` builds public USB/rekordbox status frames
for native source identities1/2 and the eight public fixture identities41..48.
It requires the unchanged pinned Beat Link8.0.0 `CdjStatus` parser to recognize
player3, loaded/playing state, source identity, USB slot and track495, and verifies
that the library's status template was not modified. This self-test sends no
traffic and is not native admission, external loaded-deck or audio acceptance.

Live invocation requires the existing isolated `azhub-link`/`azlink0` topology:
`--emulator-link-1 SECONDS` through `--emulator-link-8 SECONDS`, or
`--native-source1 SECONDS` / `--native-source2 SECONDS`, with duration5..180.
Native source numbers are protocol identities; mapping them to physical USB
ports still needs verification. The control never ejects or unmounts anything.
A native baseline must identify and preserve the mods USB separately.

Unmodified VirtualCdj provides discovery/player-number negotiation and the real
50002 socket. Its automatic status sender stays disabled because its API cannot
select a remote loaded-track source. Repository code copies its status template
and fills public loaded-track fields, then validates every emitted frame with
CdjStatus. Reflection accesses only the pinned template/socket, not device memory
or modified library classes. Status remains loaded/playing until the bounded
control ends; it does not automatically unload after source withdrawal.

This is an explicitly synthetic protocol stimulus. Pair it with independent real
library/file clients and an on-wire/native-admission observer before claiming an
externally-in-use eject control. It does not decode/render audio or emulate a
physical model's cache, firmware, UI, sync or response to source loss.

# Physical Ethernet sharing candidate0.4.4

## Result and tested scope

The unpublished0.4.4 candidate runs on the real XDJ-AZ with exact firmware tag
`ep147-9e9454e357389ece`, kernel4.4.194 and the existing Community Mods1.2.3 setup.
One physical hub stick was tested through independent Windows Beat Link8.0.0 and
Crate Digger0.2.1 clients. DeviceFinder discovers public computer identity41;
complete menu comparison verifies3172 tracks,166 ordered nodes and11022 playlist
memberships against independent PDB/table parsing. Complete PDB (2973696 bytes)
and MP3 (3590545 bytes, rekordbox track167) transfers match the source USB hashes.
Native USB1 also serves the exact installed runtime file through its native NFS
export, with the hub backing/private alias still mounted.

See [physical acceptance](acceptance/usb-hub-support-sharing-044-physical-ethernet.json)
for hashes, evidence paths and the distinction between protocol/file observations
and playback. No external audio renderer or actual CDJ participated. Do not claim
multiple/eight-source hardware acceptance, Wi-Fi, arbitrary CDJ model support or
long-session acceptance from this result. The user reported local hub operation
on0.4.0;0.4.4 retains exactly that runtime ELF, while these updates affect the helper.

## Diagnosed failures and ownership

The native Ethernet IPv4LL address has label `eth0:avahi`. Linux getifaddrs exposes
that label, so exact-name comparison against eth0 omitted the usable IPv4 network.
The helper now includes labels belonging to eth0 while rejecting lookalike devices
such as eth01. Regression controls fail against the old header and pass with the
fix. Missing native IPv4 also produces an explicit startup diagnostic.

Secondary-MAC unicast delivery on the tested rk_gmac-dwmac interface needs a parent
receive-mode reference. A bounded physical control measured TCP success during
membership and timeout after automatic release, with flags0x1003 ->0x1103 ->0x1003.
This establishes the tested receive-mode dependency; it does not establish the
precise hardware-register defect or support for other drivers.

The candidate acquires one PACKET_MR_PROMISC membership on a protocol-zero packet
socket after authenticating the parent/helper session and validating the native
network. That socket receives no payload. It holds its own receive-mode reference
for the helper session and closes after graceful retirement; process exit also
releases it. It never rewrites saved interface flags or removes another owner's
reference. No manual persistent controller/driver edit, broad firmware change or
new kernel patch is involved. The original hub-depth and GPL MACVLAN modules remain
separate; physical module loading does not promise module unload on mod disable.

The Windows client also needs `-Djava.net.preferIPv4Stack=true` on this multi-interface
host. With that option, unchanged DeviceFinder sees the native AZ and public hub
source; raw UDP observation independently sees Link-signature announcements.
No Windows firewall rule was changed. An already installed/allowed Java runtime
was used; installed paths are machine resources, not portable requirements.

## Reproduction and limits

Use [the build procedure](HUB-LINK-CANDIDATE-BUILD.md) and the exact hashes in the
physical acceptance record. Current source/package snapshot details are in
[the0.4.4 package record](acceptance/usb-hub-support-sharing-044-package.json).
All builds and packaging are local-only; installation and physical operations
require the authorized USB/framework workflow with fresh empty-deck checks.

The reference PDB, entity oracle and selected MP3 path/hash stay under ignored
`local/diagnostics/physical-ethernet-library-v1`. Nothing from those inputs is
uploaded. Physical file clients delete the downloaded temporary file after exact
length/hash verification. The repository contains reconstruction/control sources,
not music or exported library contents.

Compile the repository Java controls against the pinned unchanged dependency jars:
PhysicalEthernetDiscovery, PhysicalDbLibraryControl, PhysicalFileServiceClient,
NamedTableSnapshot and LibraryFixtureSnapshot. Set an assigned local Ethernet
IPv4LL address and the actual provider address from this session; addresses are
not stable across reconnect/reallocation. Run Java with IPv4Stack=true.

- Discovery: `PhysicalEthernetDiscovery --physical-ethernet LOCAL_IPV4 20`.
- Complete menus: `PhysicalDbLibraryControl PORT PDB ENTITY_TSV LOCAL_IPV4 SOURCE_IPV4`.
- Exact file: `PhysicalFileServiceClient LOCAL_IPV4 SOURCE_IPV4 RELATIVE_PATH BYTES SHA256`.

These controls use source41/COLLECTION slot4 for the tested single-source menu;
they are not a general arbitrary-device browser. The relative path/hash comes
from the independent PDB/source reference, not a fabricated provider response.
A successful file transfer does not establish a player's response to source loss.
Use [the model-specific plan](HUB-LINK-PHYSICAL-TRIALS.md) for actual-CDJ discovery,
playback, loaded eject and cache behavior.

## Preserved behavior and rollback

Current helper loaded-source eject/reconnect passes in the emulator, including
continuing paired synthetic status and retained neighbor sessions. A first mode
cycle failed recovered-client EOF/STALE after returning On; it remains recorded
as a failure. Both a validation-trace repeat and an exact normal-helper repeat
subsequently pass the full before/after menu/file control. No PDB mismatch was
observed in the traced repeat, and an isolated inode-cache control did not reproduce
the fault. Its cause is not established. Do not treat those repeats as proof of
long-session or intermittent-failure absence. Final physical GUI Off/On observation
is pending user availability; the agent has not edited the persisted Link setting.

Per-source servers/caches and native slots remain separate. Exact runtime hashes,
current-helper loaded-eject/neighbor/reconnect controls, parent reference lifecycle,
portable sanitizer checks and Link mode controls are recorded separately from
physical acceptance. Earlier all-eight and four/two-local-deck engine controls retain
their emulator scope; do not relabel them as current eight-source hardware proof.
IPC loss with outstanding generations still refuses unmount rather than infer safety.

Known stable rollback on the mods USB:
`/media/usb/sda1/MODS/backups/usb-hub-support-before-040-20261021-163608/original`.
It contains0.3.2 runtime SHA256
`a3d6c7ba438494008edf66fdda43d4be5ae702ed3afa4cdcb57791b9964d1add`,
verified read-only after the physical trial. The immediate prior candidate backup
is also recorded, but is not a substitute for the stable rollback. Any rollback
must use the normal USB/framework staging, exact approval and empty-deck reload;
it has not been performed or claimed as a recovery test. Publication is separate.

# Current repository note — 2026-10-17

Development moved to NachoCasta/nacho-az-mods (PRIVATE), separate from the core repository.
Read ../../../docs/CURRENT-STATE.md before the historical handoffs below. User confirmed0.1.4
works perfectly after the SSH install;0.1.5 is publication metadata only and not installed.
Screenshots remain LOCAL/gitignored; no screenshot backups requested. Ask before real-AZ tests.

# Multi-USB on the XDJ-AZ — research notes and handoff

Working notes for the effort to show more than two USB sources on the XDJ-AZ. Written so the
work can resume in a fresh session. The deck is firmware **2.00** (EP147 md5
`8daa1de55d7328f458f6d855cdc3cdde`, kernel 4.4.194 aarch64 on rk3399).

## Latest: 0.1.4 waveform/source-name fix accepted in emulator; AZ unchanged

User confirmed 0.1.3 works on hardware (browsing, load and playback). Hardware
then showed RGB/blue and bottom/center waveform switching, plus HUB sdb1 labels.
Version 0.1.4 fixes both locally. Ask the user before any further AZ test or
mutation; only read-only SSH diagnostics were authorized during investigation.

Cause: send_status copied a global 1152-byte native deck status, alternately
populated by different AZ decks. Its device preferences alternated populated
USB1 RGB=3/position=2 and empty zeros every 200ms. Native checkAndNotify applied
these to the synthetic media, generating real waveform preference changes.
source-settings.h now replaces serialized foreign settings rows and publishes
each stick's own PIONEER/DEVSETTING.DAT (NOT MYSETTING.DAT). Validate fixed known
140-byte format, header/magic and CRC16-CCITT; zero fields inherit populated native
USB1 preferences, else native defaults. Refresh read-only every two seconds.
Slot3 -> kind0 row0xd9, slot7 -> kind1 row0x468, slot8 -> kind8 row0x474;
slot2 -> kind2 row0xf9. No native hooks added; do not freeze visual events.

Final emulator PID77151 approved through normal Community Mods UI. Five labels
appear correctly. All five native setting arrays hold RGB=3, position=2. Tested
B with valid blue=1/center=1 preferences while other four remained RGB/bottom;
browser and loaded decks displayed independent styles. Injected 200 alternating
RGB/bottom and zero native transport templates over20s;30 native samples retained
each source's own values. Restored original B file and verified all five RGB/bottom
in30 more samples; already-loaded B changed to RGB, proving real updates still work.
A Algo mas played03:22->01:40, then paused; sparse fixture audio present. Fifth
source all-tracks2547 and Algo mas load on deck3 verified. Screens waveform-014-*;
research/waveform-014-*-settings.log records native arrays. Existing browser15 and
source-name14 checks pass; settings fixture/adversarial tests and ASan/UBSan pass.

Names: per-device udev volume label (duplicate labels supported), by-label fallback,
then physical hub port such as USB 4; correct UTF-8 -> UTF-16BE publication. SDK UI
shows HUB_LIB/HUB_LIB2/etc from actual image labels, instead of Linux device nodes.
Real AZ still runs0.1.3 and was not modified while preparing0.1.4. No commit made.

## Historical: hardware0.1.2 failure and0.1.3 subnet fix

User reports no HUB source on real AZ. Read-only SSH showed0.1.2 loaded25hooks,
azhub kernelmodule loaded, hub sdb1 mounted with DB, but repeated unacknowledged
announcements. AZ is192.168.1.115/24; old code always advertised10.0.2.50/.51.
User explicitly requires asking before ANY further real-AZ test. No further
AZ connections/tests/changes have been made since that instruction.

Local reproduction: native emulatorIP192.168.1.115/24 (10.0.2.15 retained as
secondary for host SSH). Exact original0.1.2 auto discovery reproduces retries.
New network-identity.h selects two usable internal peer labels in current
subnet, without assigning OS addresses. Native advertised keepalive IP preferred,
then default-route interface; frozen until EP restart. Needs contiguous /29 or
larger IPv4 subnet. Fully unconfigured networking, interface changes and multiple
active subnets remain unverified. Manual configurations keep explicit IPs.

New25hook binary0.1.3 PID35295 starts with zero hub sticks, then QMP adds five
without EP restart. Runtime labels192.168.1.254/.253; all five separate DBs open.
All five full Reggaeton284 views pass, fifth alltracks2547 passes. C/D waveform
previews delayed but populated on revisit. Ethernet10s Link0, loopback224.
Network policy14cases passes host and ARM64. Other topology8/browser15/mount18
checks pass. Fifth-source Algo mas loads decks3/1 and PLAY1 advances03:20->02:50, then
paused. Version0.1.3 package is being finalized; hardware retest awaits
user approval. Original0.1.2 ZIP retained as historical artifact.

Emulator reboot fixture required intact core1.2.3 on USB (old image was truncated)
and rebuilt missing xhci-pci module. Rebuild scriptresearch/scripts/build-emulator-xhci.py;
module is emulator-only, never bundled for AZ. Emulator90-hub-emulator-test.conf
clears automatic post-start staging race, retains explicit community/enabled,
and runs hub-excluding usb-renotify. No local USB library DB is open in this run.
First native subnet-gate hook experiment was disproven and fully removed; see
GHIDRA-FINDINGS correction. Current fix adds no native hooks.

## Current handoff: Community MOD 0.1.2 ready for manual AZ test

Package: [hub-libraries-0.1.2-dev.zip](linkfake/dist/hub-libraries-0.1.2-dev.zip).
Instructions: [AZ-TEST.md](linkfake/AZ-TEST.md). All code remains uncommitted.

Emulator PID544361 loaded the exact packaged binary with25 hooks, diagnostics
off, five independently owned server pairs and five open DBs. All five sources
passed populated Reggaeton284 full lists on first entry (acceptance-[A-E]-full.png).
The reinserted B also browses2547 tracks. Fifth-source E browses2547 and loads
verified-present Algo mas on decks3 and1 in four-deck view. PLAY1 advanced03:20
to03:07, then paused. Persisted ProDjLinkMode is0. Ten-second Ethernet capture
observed0 Link signatures, while loopback observed221. Audio output not captured.

Private tagged caches and browser event deferral are now SDK defaults. Actual
removal closes one DB, clears its alias and releases fallback backing mount;
reattachment reopens/browses in the same EP PID. Separate temporary sdg1 mount
and cleanup both ran automatically via HubMount, without vendor rootfs changes
in the mod package. Concurrent mount scans use getmntent_r. Tests: browser15,
topology8, mount-device policy18, kernel policy6, plus previous actual ARM64
ABI adapter acceptance. Package passes deck list_pkg validation; packaged binary
hash matches the running binary. assets/ includes exact4.4.194 azhub.ko and helper.

This is the requested emulator-to-community-mod handoff, not hardware acceptance.
Real AZ remains untouched; combined loader awaits the user's AZ test. All five
fixtures are clones with full2547-entry DBs but only17 audio files. Distinct
library contents, playing-source removal, same-path replacement without observed
absence, extended stress and captured audio remain unverified. Slot8 source icon
is blank, source names use device names, and displayed capacity is a stub.

The entries below preserve investigation history and are superseded by this
handoff status where they differ.

## Mount worker integration in progress (2026-10-17)

SDK build now enables private tagged caches by default (25 hooks with optional
DB probe off) and starts a separate HubMount worker. Packaged mount-hub.sh
handles nested USB block names beyond stock udev's sda..sdf range. Missing-device
cleanup waits for EP open files and bind aliases to clear, then uses normal
umount; no force/lazy unmount. Actual assets live on the mod USB.

First worker runtime exposed getmntent's shared static buffer: parallel scans
briefly published an alias as a sixth source. Discovery and worker now both use
getmntent_r with independent buffers. Corrected emulator PID544361 loads25 hooks
and initially registers10 owned servers (five pairs). Physical hotplug acceptance
is underway; package not yet refreshed, real AZ untouched.

## Latest follow-up: tagged-cache isolation passes controlled removal

The crash cause was confirmed: all owned servers shared TaggedTrackCacheService
entry0102, and removing one cleared it for the others. The experimental build
now gives each stick a private tagged cache. On emulator PID533159, removing B
empties only B's cache; A and E continue browsing2547 tracks. Restore returns
all five caches/DBs without EP restart. Native ARM64 ABI tests cover all eight
receiver adapters, including x8 C++ aggregate returns and stack arguments.

This fix is still opt-in in the running diagnostic build, not in the ZIP.
Automatic mounting/cleanup for device names beyond sdf remains unfinished;
mount-hub.sh is only a draft. Goal remains active; no AZ deployment yet.

## Open issues found during final 0.1.1 check

Do not treat the successful browsing sweep below as final readiness. The last
hotplug check exposed the stock udev sd[a-f] mounting limit, then PID499084
crashed on HubLib4 at native12553b0 (null DB lookup handle). EP147 automatically
restarted as507534. The 0.1.1 archive is an investigation build; automatic mount
fallback and the native library crash remain required work. Real AZ untouched.

## Current status: community test build 0.1.1 (2026-10-17)

The emulator now runs five hub sticks through **five independently constructed
library/track-info server pairs**, using linked-player identities3/4 without
borrowing local USB1/2/3 library servers. The normal Community Mods SDK build
loads with17 hooks; linkfake.so is absent from preload. Packet hex logging and
optional client/cancel diagnostics are off. Persisted ProDjLinkMode remains0.

On PID499084, all five sources passed populated playlist previews and full
Reggaeton284 views on first entry, with metadata/waveforms. The fifth also
shows all2547 tracks. Verified-present Algo mas loads on decks1 and3 in four-deck
view with both waveforms; deck1 PLAY advanced03:22->03:05. Audio output was not
captured. Sparse fixture: each full database has only17 actual audio files.

The empty-view cancellation race is addressed by retaining/coalescing native
WaveformRenewEvent objects until pending hub list requests finish. Navigation
and native event destruction discard retained repaint events.15 ownership and
lifecycle checks pass optimized and with ASan/UBSan; topology8 and mocked kernel
asset policy6 checks pass. Runtime confirms actual deferred-event replay.

Package: [hub-libraries-0.1.1-dev.zip](linkfake/dist/hub-libraries-0.1.1-dev.zip).
Instructions and current limitations: [AZ-TEST.md](linkfake/AZ-TEST.md).
Package includes SDK binary, manifest, exact4.4.194 azhub.ko asset, README and
LICENSE. Deck package-list validation passes. Real AZ is untouched; the user
will perform hardware testing. Existing azhub.ko kernel patch was tested on
hardware earlier, but this combined SDK package/kernel loader is not yet AZ-tested.

Additional emulator evidence: actual USB unplug/reinsert closes/reopens a
source without EP restart; five changed-path replacement cycles reuse a pair.
These lifecycle tests preceded0.1.1. The current images are clones; differing
library contents have not been tested. Same-path replacement without an
observed absence, removal of a playing source, stress testing, source labels/
capacity and slot8 native icons remain development limitations.

The dated sections below retain research history; their old blockers and
runtime configurations are superseded by this status and the latest entries.

## Goal

The user wants **several extra USB sticks** usable at once, not just a third. A USB hub on one
port is the physical path; the deck UI must then show the sticks as sources.

The user's goal as set (2026-10-16), verbatim: "have USB working from the HUB in the emulator, being
able to browse it in rekordbox mode (playlists, tacks, etc, not just folder view). it should not be
using the existing usb 1, 2 or 3. it should be using the equivalent of the cdj slots when they are
paired over pro dj link (without actually using pro dj link), so that we can connect 4+ USB. once
this is workgin on the emulator, convert this into a community MOD, so that I can test it in the AZ"

## Status (end of 2026-10-16 session)

| Piece | State |
|---|---|
| Hub enumerates on the deck (`azhub.ko`) | done, committed |
| Hub + stick behind it in the emulator | done (xHCI, see "Emulator") |
| Rekordbox mode in the emulator (OneLibrary key via intact `azmod.so`) | done |
| Extra "N LINK" sources with no network (`linkfake.c`, in EP147) | done, 2 players (3, 4), several slots per player |
| A LINK source browsed in rekordbox mode, track loaded and playing | done for one source, **but the library is still served by the AZ's own dbserver through the stick announced as local USB2** |
| A library per fake member without using USB1/2/3 (needed for 4+ sticks) | **not done**, the open blocker |
| Community mod (claim_hook instead of LD_PRELOAD) | not started |
| Real deck test | not started (deck stays read-only until the mod passes in the emulator) |

## What already works (committed)

**`azhub` kernel module** (`src/azhub/`, commit `821c68f`, not pushed). The deck kernel is built
with `CONFIG_ATC`, which sets `MAX_TOPO_LEVEL = 1` in `include/linux/usb/hcd.h`, so `hub_probe()`
rejects every external hub ("hub nested too deep", error -7, raising the "Hub not supported"
caution). The module patches the one comparison `hub_probe+0x84` (`cmp w0,#1` -> `cmp w0,#6`,
the stock Linux limit) with `aarch64_insn_patch_text`, after verifying the 4 words at
`hub_probe+0x7c` (`3953ef00 9100c354 7100041f 540002c1`). `rmmod`/reboot restore it.

**Proven on the deck:** with `azhub` loaded, a hub in USB2 enumerates, a stick behind it mounts
at `/media/usb/sdb1`, and announcing it with `mount /media/usb/sdb1 vfat protect:0` >
`/proc/udev_usb2` makes it **appear and play as USB2**. Playback confirmed by the user.

This alone does not add sources — it only lets a hub stick take the USB1/USB2 slot. The rest of
this doc is about getting *more than two* sources.

## Breakthrough (2026-10-16): extra sources shown in the emulator

**Two extra USB sources appear at once on the emulator's SOURCE screen**: "3 LINK · USB 1 · HUB
USB" and "4 LINK · USB 1 · HUB USB B". They sit next to the local USB1, are selectable, and
show the free space we report. They come from two fake Pro DJ Link players running inside the
emulator guest (`research/scripts/fakeplayer.c`). See "Link path proven in the emulator" below
for the exact recipe and what is still open.

## EP147's media model (from strings/RTTI + Ghidra; see "Reverse-engineering setup")

EP147 has a fixed set of internal media slots, fed by `/proc` mailboxes that a userspace writer
fills with `mount <path> <fs> protect:<ro>` / `umount <path>` lines:

| Mailbox | Slot | Notes |
|---|---|---|
| `/proc/udev_usb1` | USB1 | written by stock `device-mount.sh` for bus id `1-1`/`3-1` |
| `/proc/udev_usb2` | USB2 | written for bus id `2-1`/`4-1` |
| (none) | USB3 | EP147 models it (`MscLibSrvUsb3`, `TInfRepSrvUsb3`, `isUsb3Available`) but there is **no mailbox** for it locally |
| `/proc/udev_sd1` | SD | EP147 reads the line but the AZ UI does not show an SD source |
| `/proc/udev_nfs` | NFS/Link remote media | how remote players' USBs are mounted locally and announced |

The mailbox driver is the vendor's `fs/proc/udev_usb.c` (`USB_CNT = 2`). The reader thread in
EP147 is the "Storag..." thread; constructor `FUN_023278a0`, poll/read loop `FUN_02323670`
(`poll` on the fd, dispatches each line to listeners). The strings `connect,` `disconnect,`
`mount ` `umount ` live at `0x31e7030/40/50/58`; `FUN_0226eab0` builds the table that maps each
mailbox path to a slot id (0=usb1, 1=usb2, 3=sd, 4=nfs...).

### Deck-side experiments (all read-only to the deck, nothing written to its storage)

- **SD mailbox**: `mount ... > /proc/udev_sd1` with the stick bind-mounted at both a USB path and
  `/media/sd/mmcblk1p1` — EP147 **read** the line (mailbox drained) but **no source appeared**.
  The SD slot is suppressed somewhere past the mailbox (UI/library). Not pursued.
- **NFS mailbox**: stick bind-mounted at `/media/player02/usb` (the `/media/player%02d<x>` layout
  EP147 uses for remote players) and announced via `/proc/udev_nfs` — read, **no source** (no
  Link member existed, so nothing backed it).

## The NFS/Link path — most promising for *many* sticks

The remote-player mount path in EP147: `/home/root/pdj/FuseFilsine` is spawned to mount a remote
USB at `/media/player%02d<letter>` (letters from a table at `0x2bb6958`: `/sd`, `/usb`, `/usb2`,
`/usb3`, `/internal1` -> slot ids 66..70), then it is announced through `/proc/udev_nfs`. So a
remote USB is, internally, **a local mount + a mailbox line**, exactly like USB1/USB2.

When a Link member appears, EP147 fans the event out to 10+ subsystems via `onAddMember`
(`usecase::link::LinkConnectionHandler`, `client_server_framework::LinkEventHandler`,
`usecase::sync::SyncMemberSystem`, `plink_man::PlayerLinkInfoManager`/`...Send`,
`link_member::RemoteServiceVersion`/`LinkPlayTime`, `link_monitor::MixerLinkMonitor`, ...).
Export discovery: `nfs::NfsManager::onReceiveUdp_ExportList` / `onPlink_NfsDetect`, with a device
property exchange (`NfsDevpropReq`/`Ret`) and a certification step (`nfs::INfsRbmCertification`,
`NfsManager::notifyCertification`). Encrypted transport exists (`FNS_encrypt`/`FNS_decrypt`).

### Network experiments (read-only; the deck is on 192.168.1.115, PC on .53)

Pro DJ Link packet layout is Deep Symmetry's dysentery. Scripts in
`research/scripts/`: `sniff.py`, `capture.py`, `fakecdj.py` (announce only), `fakecdj2.py`
(announce + status claiming USB + media-slot answer). Keep-alive is type `0x06` on UDP 50000;
captured the AZ's own template (name `XDJ-AZ`, MAC `1c:bc:ec:1e:8a:ec`).

**Key finding:** EP147 **leaves UDP 50000/50001/50002/50004 and NFS 111/2049 open.** (The user
later confirmed Pro DJ Link Mode was **on** on the deck during these tests.) When `fakecdj2.py` announced a fake "CDJ-3000" player 3 *and* sent CDJ status
(type `0x0a`) with USB-loaded flags, **the deck sent an RPC portmap GETPORT for program 100005
(mountd) to the PC** — i.e. it tried to mount our "USB" over NFS. Just keep-alive + status was
enough to trigger the mount attempt; no media-slot query (`0x05`) was needed. We did not answer
the RPC, so it stopped. **Nothing ever appeared on the AZ screen in any network test** (but the
user was not always watching; this is unconfirmed, not a negative result).

CDJ status offsets that matter (dysentery vcdj page): `0x21`/`0x24` device number, `0x6f` Ul (USB
local: `00`=loaded, `04`=none), `0x75` L (link media available), `0xb7` Mp (CDJ-3000 USB/SD
present bits), `0x7c-7f` firmware ASCII, CDJ-3000 status length `0x200`. Media slot query/response
are type `0x05`/`0x06` on 50002 (layout in dysentery media page; slot USB=3, SD=2).

## The plan (agreed with the user): the mod runs with NO network

The network tests are only to **learn the sequence**; the mod reproduces it *inside* EP147 via
`LD_PRELOAD` (AZ-MODS already injects and hooks EP147, and interposes libc incl. `recvmsg`):

| Step | Learned via network | In the mod (no network) |
|---|---|---|
| player appears | UDP keep-alive to deck | inject a synthetic Link member into EP147 state / event |
| "has USB" | UDP status packet | same, in-process |
| media info | UDP media slot response (0x06) | same, in-process (name = stick label) |
| mount the USB | deck spawns FuseFilsine (NFS) | intercept the spawn, bind-mount the hub stick |
| announce mount | FuseFilsine writes `mount <mp> nfs` to `/proc/udev_nfs` | same line, written after the bind mount |

## Link path proven in the emulator (2026-10-16)

### Pro DJ Link Mode gates it, at one flag (answered)

Measured in the emulator. A fake player sends keep-alives with `ProDjLinkMode=0` (the setting in
`/home/root/settings/SETTINGS.DAT`): EP147 only answers with unicast keep-alives. With `=1` it
immediately sends 0x80, its status 0x0a (1152 B, as two virtual players 1 and 2) and the media slot
query 0x05.

Ghidra (verified in disassembly): member promotion and the status/0x05 sends read one cached byte,
`DAT_03f86a08`, written by `network::SystemManager::changeProDjLinkMode` (`0x13fa7e8`). It has
exactly 4 readers, all in `network::SystemManager`:

| Site | Instruction | Force-on patch |
|---|---|---|
| `0x13f2a3c` | `ldrb w23,[x1,#2568]` then `cbz w23` (promotion) | `mov w23,#1` = `52800037` |
| `0x13f26e0` | `ldrb w0,[x0,#2568]` | `mov w0,#1` = `52800020` |
| `0x13f2c20` | `ldrb w0,[x0,#2568]` | `mov w0,#1` = `52800020` |
| `0x13f167c` | `ldrb w0,[x0,#2568]` | `mov w0,#1` = `52800020` |

The downstream chain has no mode check of its own: onAddMember fan-out → NfsManager → FuseFilsine →
media registration → SOURCE row. The decks-3/4 disable reads the setting item, not this cache.
That last point is **inferred** from the xref set and not traced to the end. If it holds, patching
the 4 reads gives remote media while keeping 4 decks. **Not yet tested.**

### Recipe that makes a source appear

Run inside the emulator guest, with `ProDjLinkMode=1`:

```
ip addr add 10.0.2.50/24 dev eth0
FP_FW=9.99 FP_NAME="HUB USB" /tmp/fakeplayer 10.0.2.50 10.0.2.15 3000 3 &
ip addr add 10.0.2.51/24 dev eth0
FP_FW=9.99 FP_NAME="HUB USB B" /tmp/fakeplayer 10.0.2.51 10.0.2.15 3000 4 &
```

What each step triggers:
1. Keep-alive (0x06, UDP 50000) → EP147 promotes the member and sends 0x80, its 0x0a status and a
   0x05 media query (`Dr`, `Sr=3` = USB) to the player's 50002. A one-off type 0x20 (44 B) also
   goes to 50004 (purpose unknown, unanswered, harmless).
2. Our status 0x0a: the deck's own status as template, with device number → ours, `Ul`(0x6f)=0,
   `L`(0x75)=1, `Mp`(0xb7)=1 and firmware at 0x7c. Firmware "0.00" raises *"Linked to player with
   older firmware"*; "9.99" does not.
3. Our media slot response 0x06 (on 50002, 0xc0 bytes): `D` at 0x24..0x27 (`00 00 00 D`), `S` at
   0x28..0x2b, volume name UTF-16BE at 0x2c, date at 0x6c, tracks 0xa6, rekordbox flag 0xaa,
   playlists 0xae, total/free size u64 at 0xb0/0xb8. **`fakecdj2.py` put the IP at 0x24; with
   that, EP147 re-queries every 5 s and never registers the media.** Once fixed, EP147 asks once.
4. EP147 `msgsnd`s to `FuseMountDaemon` (SysV queue, mtype 333, reply mtype 666 = `"0"`) the
   command `/home/root/pdj/FuseFilsine <ip> 111 /media/player%02d/usb C 0`. The daemon `vfork`s
   and `execl`s it through `sh -c`. FuseFilsine is a userspace NFS client on FUSE. Here it mounts the
   guest's *own* NFS export, because the alias IP is local, so the "remote" USB is the emulator's
   USB1 content.
5. The SOURCE row appears as `<player> LINK · USB 1 · <FP_NAME>`, with "Available" = our free size.
   The mount stays up while keep-alives continue. When they stop, EP147 drops the member and
   unmounts.

Constraints found:
- **Only players 3 and 4 get a row.** Player 5 is promoted and even mounted but never listed. In
  Link mode the AZ is players 1 and 2. The Cloud login list shows exactly "PLAYER No.1/3/4". So
  with one slot per player we have **2 extra sources** today.
- `/proc/udev_nfs` is a required step, but not enough on its own. After mounting, FuseFilsine
  writes `mount <mp> nfs` there (its strings also list `/proc/udev_usb1/2/3` and `/proc/udev_sd1`,
  chosen by argv[6]). Only after that line does EP147 send the 0x05 media query. Writing it by
  hand with no member, or with a bad 0x06 answer, adds no row. The row comes from media
  registration (`MediaInfoManager::onMediaInfoMan_MountCompletion`, subscriber
  `indicator_control::browser::Source`, trampoline `FUN_021c2de0`).
- Device number 3 collides with the AZ's own number when Link Mode is **off** (its keep-alive copies
  ours). Irrelevant with the mode on.

### No-network prototype: `linkfake/linkfake.c` (WORKS in the emulator)

`linkfake.so`, LD_PRELOADed into EP147 (emulator: `LD_PRELOAD=/home/root/linkfake.so` in
`/home/root/settings/az-usbload/az-preload.env`). Config `/home/root/linkfake.conf`, one line per
fake player: `<3|4> <fake ip> <local dir> <volume name>`. With no alias IPs and nothing on the wire,
SOURCE shows "3 LINK · HUB A" (bind mount of a second, unannounced stick `/media/usb/sdb1`) and
"4 LINK · HUB B". How it works:
- An injector thread sends each player's keep-alive and status to `127.0.0.1:50000/50002` from its
  own loopback socket. `recvfrom`/`recvmsg` rewrite the source of those datagrams to the fake ip.
- `sendto` swallows everything addressed to a fake ip. It keeps EP147's 0x0a as the status
  template and answers 0x05 with 0x06.
- `msgsnd` rewrites the FuseFilsine command into `mkdir -p <mp> && mount -o bind,ro <dir> <mp> &&
  (sleep N; printf 'mount <mp> nfs' > /proc/udev_nfs) &`. N is staggered per player because the
  mailbox holds one message.
- `connect`: after registering a medium, EP147 tries TCP **12523** on the player (its dbserver)
  about 10 times a second for ~10 s. We refuse it, and the row still appears.
- Built with the deck's Linaro: `aarch64-linux-gnu-gcc -O2 -shared -fPIC -o linkfake.so linkfake.c
  -ldl -lpthread`. Active only when `/proc/self/exe` is EP147, because children inherit
  LD_PRELOAD.

Open: browsing and playback were not tested. The emulator's test sticks have no rekordbox library.
It is still unknown whether browsing a LINK source reads the library over the mount or needs the
remote dbserver (12523, refused today). Unmount and teardown (member loss, `umount` commands) are
unhandled.

### Rekordbox browsing of a LINK source works (emulator, 2026-10-16)

"3 LINK" browses in full rekordbox mode (Artist/Track/Key/Playlist/History/Matching/Folder/Date
Added), and a track loaded from it plays in deck 1 with its rekordbox waveform and BPM. The audio
is read from the `/media/player03/usb` bind mount. Required pieces:
- **OneLibrary key**: the emulator only decrypts `exportLibrary.db` through AZ-MODS core's
  `onelibrary_key_emulator.c` (flag `/home/root/settings/azmod-onelibrarykey.on`). Without it every
  stick is FOLDER-only, local or remote. The emulator's installed `azmod.so` was truncated (205 KB:
  SIGBUS in every process). The intact one is `~/xdj-az-emu/build/mods/1.2.3/azmod.so` (1.6 MB).
  Preload both: `LD_PRELOAD=/home/root/settings/az-usbload/azmod.so:/home/root/linkfake.so`.
- **Browsing goes through the dbserver protocol.** EP147's RemoteDBServer client is looped back to
  EP147's own server (`LINKFAKE_TCP_LOOP`). Three rewrites on that connection (linkfake.c
  `patch_slot`/`fix_setup_reply`):
  1. Requester number 1 → 5, in the setup argument and in every DMST `D` byte (`LINKFAKE_REQ`).
     With 1 the server answers 0x0100 and closes.
  2. **The setup reply's player number → the fake player's number** (`LINKFAKE_SETUP_AS_TARGET`).
     Without it the client gets the root-menu count but never sends 0x3000: "Waiting...", then a
     reconnect every 15–20 s.
  3. DMST slot → the AZ slot that holds the stick (`LINKFAKE_SLOT=7` = local USB2). **Temporary**:
     the stick still occupies USB2. Next step is to make the server serve the fake member's
     mount without a local slot.
- Client protocol facts (Ghidra): 0x3007 = FeatureAvailabilityInfoRequest (`FUN_0112ddd8`; a 0
  reply does not block). List replies go through `FUN_01142bc8`: count ≠ 0 → it sends 0x3000
  (`FUN_01142a88`). Reply matching is in `FUN_00e63ef0` (same stream, txid, expected type).
  Timeouts/retries in `FUN_00e66668`.
- Writes to the traced socket go out through write/send/sendto(NULL)/writev/sendmsg; all are
  hooked.

### Ghidra map of the remote-media chain (for the in-process mod)

- Gate: above. Mount orchestration: `FUN_0139b3b0` (msgget key `0x1e8518`) → `FUN_0139afd0` (gates
  on member state `FUN_013db270(dev+0x19)==2` and slot letter `'B'..'F'`) → `FUN_01399f10`
  (formats the FuseFilsine command from slot table `0x02bb6968`, stride 0x1c).
- Storage lifecycle: `usecase::device::UsecaseStorageDeviceController`: `connected` →
  `receiveNFSDevicePropertyInfoData` (`FUN_00bb9b50`) / `notifyDevicePropertyRecieveFinish`
  (`FUN_00bbd970`) → `selectionOfMountOrConnectWait` → `mounted` → `onMounted(MediaID)` →
  `mount_device::MediaInfoCollection::addNewMedia(StorageDeviceIfID, LinkMemberID, slot)` →
  `onMediaInfoMan_MountCompletion(MediaID)` → SOURCE row.
- Other UDP processors: `ProDjLinkDistributor::msg_ExportList_Proc` (`FUN_013cc968`),
  `msg_NfsDevpropReq_Proc` (`FUN_013cd438`), `LinkDeviceNameRequest::onReciveUdp_GetRBMNameRet`
  (`FUN_013e8b80`). The emulator recipe did not need any of them.
- Path → device number: `FUN_0111e180` reads the 2-digit player number at offset 13 of
  `/media/playerNN…`.

### Limits (still to verify)

- Raise the player cap: where is 3..4 enforced, and can it be patched to allow 5, 6, …?
- Several slots per player: a member can hold SD(2) + USB(3) + USB2(7) + USB3(8) (`FUN_00bae338`),
  and there are 26 remote-media entries in total. Tried in the emulator without success: status
  `Sl`(0x73)=0, also flipped from 4 to 0 at runtime, `Mp`(0xb7)=0x6c, and unsolicited 0x06 for slots
  2/7/8. EP147 still queries only slot 3. The query sender is `FUN_013d75f8`
  (`ProDjLinkTransmitter` vtable +0x48, entry `0x2bbdd60`; payload = ip, D, S). What picks the slot is
  not found; leads are `IAbilityMap` (vtable +0x50, `FUN_013d7718`) and
  `LinkMemberMediaSlotComposition::isUsb2Available`/`isUsb3Available` (`0x1869b48`/`0x1869e98`).
  In-process plan: hook `FUN_013d75f8` and re-call it with S=2/7/8, or force those two to true.
- Player cap: candidate table `{3,4,3,4}` at `0x2bc1df0`, read in kind 3 of `FUN_013f22b8`. It is
  the only literal 3/4 limit found. Patching it to e.g. `{3,4,5,6}` is untested.
- Playback and browsing from such a source: not yet tried.

## Reverse-engineering setup (all local, reproducible)

- **GPL kernel source**: Pioneer v2.00 tarball
  `https://www.pioneerdj.com/product-large-files/v_200/XDJ-AZ.tar.gz` (~1.4 GB), extracted in WSL
  at `~/xdjaz-gpl/XDJ-AZ`. Kernel tree + the Linaro 6.3.1 toolchain it ships
  (`prebuilts/gcc/linux-x86/aarch64/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu`).
- **Build a module**: `<scratchpad>/scripts/kenv.sh` sets ARCH/CROSS_COMPILE; must pass
  `CC=$(CROSS_COMPILE)gcc` (skip the tree's python2 `scripts/gcc-wrapper.py`) and
  `HOSTCFLAGS="-std=gnu11 -fcommon -w"`. `build-mod.sh <dir>` builds an out-of-tree module.
  Required vermagic: `4.4.194 SMP preempt mod_unload aarch64`.
- **Ghidra 12.1.4** in WSL at `~/tools/ghidra_12.1.4_PUBLIC` (Java 21). EP147 analyzed project at
  `~/ghproj`. Helper `<scratchpad>/scripts/gh.sh` runs `ghscripts/Probe.java` with commands
  `dec:<hex>` (decompile fn containing addr), `xref:<hex>`, `str:<text>`, `vt:<hex>:<n>`
  (vtable slots); output goes to `~/probe.out` and `<scratchpad>/gh-out.txt`. Static helpers:
  `vstr.py` (string at vaddr), `xref.py` (adrp+add string xrefs over objdump), `nfstab.py`.
  Strings/RTTI dumps: `~/ep147.str`, `~/ep147.rtti`, disassembly `~/ep147.dis`.
  All these scripts (and `ghscripts/Probe.java`) are copied in `research/`. The `<scratchpad>`
  paths above were session-specific. The GPL tree, Ghidra and the WSL home survive.

## Emulator (for testing mod code without risking the deck)

[xdj-az-emu](https://github.com/Kyle-Hosman/xdj-az-emu) cloned at `~/xdj-az-emu`. Runs the deck's
own EP147 under QEMU (TCG on this x86 PC — no audio realtime, everything else works). Deck system
already pulled (`build/initramfs-work-pulled/`, firmware 2.00, same EP147 md5). QEMU + Docker
Desktop installed; **Docker needs `tonistiigi/binfmt --install arm64` registered** (done once per
Docker restart) or the arm64 build steps fail with `exec format error` / `gpgv died`.

**BUILT AND BOOTS (2026-10-16).** `./build.sh` completed; `build/{Image,initramfs-patched.cpio.gz,
az-mnt.img,usb.img,ssh/az-emu_ed25519}` all present. Build ran from Git Bash with `PATH` including
`/c/Program Files/Docker/Docker/resources/bin`. Boot on Windows:
`$env:AZ_QEMU='C:\Program Files\qemu\qemu-system-aarch64.exe'` then
`powershell -ExecutionPolicy Bypass -File tools\az-boot-win.ps1`. NOTE: the launch script throws a
cosmetic StrictMode error ("property 'ProcessId' cannot be found") while finding the QEMU pid, but
QEMU **does** start — ignore it; confirm with `tasklist | grep qemu` and
`tail <LOCALAPPDATA>\Temp\az-emu-win\serial.log`. Screenshot with
`py tools\az-drive.py shot <name>` -> `<LOCALAPPDATA>\Temp\az-emu-win\shots\<name>.png`. First boot
reached the SOURCE screen showing USB1 = AZ_USB (same EP147 as the deck). Push a mod/core with
`tools/az-push-mod.py` (needs AZ-MODS core already installed; every boot installs the published set
unless `-NoMods`/`$env:AZ_EMU_NOMODS=1`). The emulated USB1 is `build/usb.img`.
The emulator has no external Link network, but **Link experiments do run inside it**. The guest's
EP147 broadcasts its keep-alives on 10.0.2.255. Fake players on secondary guest IPs
(`ip addr add 10.0.2.5x/24 dev eth0`) are seen as real members. Guest tools: busybox (no `nc`,
`timeout`, `grep -b`, `od -A`), `bash`, `strace`, `xxd`. No python, so test tools are static
aarch64 C binaries built with the Linaro toolchain in WSL and copied with `tools/az-ssh 'cat > …'`.
`SETTINGS.DAT` is `key\0 01 05 01 <value> 00 00 00` records. `ProDjLinkMode`'s value byte is at
offset `0x71e` in the emulator's file. Stop EP147 before editing it. The original (mode off) is kept
at `/home/root/settings/SETTINGS.DAT.bak-linkoff`. The emulator's `/proc/udev_*` nodes come from
`~/xdj-az-emu/guest/modules/udev_usb1/udev_usb1.c` (adding nodes there is easy if needed). EP147 is
launched by systemd with `LD_PRELOAD=/home/root/ep122_shim.so:/home/root/master_out.so`, prefixed by
`LD_PRELOAD` from `/home/root/settings/az-usbload/az-preload.env`.

**AZ-MODS core in the emulator.** The installed `azmod.so` was truncated: 205 KB, SIGBUS in every
process that preloads it, EP147 crash-looping. Install the intact published one
(`~/xdj-az-emu/build/mods/1.2.3/azmod.so`, 1.6 MB) over
`/home/root/settings/az-usbload/azmod.so`. It is required for rekordbox mode, because its
`onelibrary_key_emulator.c` supplies the OneLibrary key. Note that `az-usbload` rewrites
`az-preload.env` when a USB stick is plugged in. Hard-killing QEMU during a mod install is what
corrupted the file.

**USB hub in the emulator (works, a track loaded from a stick behind it).** QEMU's `usb-hub` is
full speed: EHCI refuses it (speed mismatch), and OHCI dies under load ("OHCI Unrecoverable Error").
xHCI works, but the emu kernel has `CONFIG_USB_XHCI_PCI=m` with no `.ko`. Build it out of tree in
the builder image:
1. `docker buildx build --platform linux/arm64 --target builder --load -t azemu-builder -f
   docker/Dockerfile .` (cached)
2. Compile the kernel's own `xhci-pci.c` with `M=`. Set `IS_ENABLED(CONFIG_USB_XHCI_PCI_RENESAS)`
   to `0` in local copies of `xhci-pci.c`/`xhci-pci.h`.
3. `insmod` it after boot.

`tools/az-boot-win.ps1` now accepts `AZ_QEMU_EXTRA` (';'-separated extra args) and `AZ_MEM`
(bytes). Leaving them unset keeps the old behavior. Hub boot:
`$env:AZ_QEMU_EXTRA="-device;qemu-xhci,id=xhci;-device;usb-hub,bus=xhci.0,port=1,id=hub1;-drive;file=<img>,if=none,id=hubdrv,format=raw;-device;usb-storage,bus=xhci.0,port=1.1,drive=hubdrv"`.
The stick shows up as `2-1.1` and is not auto-announced (same as the deck). Test images with the
user's library (PIONEER/ plus a few Contents folders, copied read-only from the user's USB) are in
`build/usblib.img` and `build/hublib2..4.img`.

## Deck access

`ssh -i ~/.azmods/ssh/xdjaz_ecdsa root@192.168.1.115` (DHCP; rediscover by ping-sweep + port 22,
deck answers `uname` `rk3399 4.4.194`). No `scp`/`sftp` on the deck: copy with
`ssh ... 'cat > /tmp/x' < x` or `tar -cf - ... | ssh`/`ssh ... tar`. Keep deck interaction
**read-only** until the mod is proven in the emulator (network packet tests and `insmod` of the
RAM-only `azhub.ko` are fine; do not write files to its storage).

## Next steps

Done: the Link-mode gate is answered, extra sources show with no network, real content is
bind-mounted, and one LINK source browses in rekordbox mode and plays a track. The user accepts
Link Mode on (decks 1–2) for a first version.

1. **Open blocker:** each fake member's library has to be served without the stick occupying a
   local slot (USB1/2/3). Today the AZ's own dbserver serves the stick through local USB2.
   Ghidra findings so far:
   - The dbserver side is per-slot `music_library::MusicLibraryServer` objects (vtable `0x2bb1268`),
     each answering requests whose slot tag (`obj+0xd0`) matches.
   - All of them are created at startup in `FUN_013809f0`.
   - The library mount handler is `FUN_0117c1b0` → `onMediaMounted` `FUN_01373ca8`.
   - LocalMediaID = `(member<<8)|sdid`.
   Not implemented.
2. **More sources**: raise the 3..4 player cap (table `0x2bc1df0`). Several slots per member
   already work (status template or mount lines for `/media/playerNN/usb2` etc.); each slot still
   needs its own name.
3. **Community mod**: port `linkfake.c` to the AZ-MODS community mod ABI (`claim_hook` on EP147
   functions). A dlopened mod cannot interpose libc the way LD_PRELOAD does.
4. **Keep 4 decks**: test the 4-instruction gate patch with the setting left off.
5. Keep `azhub` as the shippable baseline regardless.

## Independent-server prototype (2026-10-16 continuation)

Implemented the first emulator-only per-player server experiment in
`linkfake/hubservers.h`, enabled by `LINKFAKE_PROBE=1 LINKFAKE_HUBSRV=1`.
The emulator configuration no longer contains `LINKFAKE_SLOT=7`.

Verified in the guest: a fresh music-library object and a fresh track-info object
are constructed for player 3, initialized, started and appended to the actual
ServerMessageDispenser list. The client's USB requests use private code 0xe0,
claimed by per-object vtable matchers. The new music server reaches libopen with
ID 0x0302 and path /media/player03/usb. Feature request replies now return.
This is NOT yet a proof of rekordbox browsing or independence from USB2: the
emulator still auto-announces sdb1 as local USB2, and that announcement must be
removed for the acceptance test. Four distinct libraries and the community mod
remain outstanding.

Important corrections to the earlier research:
- The music factory's C++ result is indirect in AAPCS64 register x8. Its explicit
  arguments are x0=vector, x1=name, x2=sdid, x3=alternate-id pointer. Treating the
  decompiler's output pointer as x0 is wrong. The C 24-byte aggregate return
  wrapper selects x8 and successfully creates objects in the emulator.
- Server fields used by the global service lookup contain integer service keys,
  not weak_ptr control blocks. Incrementing a guessed reference count there
  crashed an experimental build; the corrected build does no such increment.
- MusicLibraryServer::onServiceActived is a streaming lifecycle callback; replaying
  it is not USB initialization. That experiment was removed.
- Reply sender initialization is vt+0x58. Its argument comes from the stock
  server's *( *(stock+0x1d0)+0xd0 ) object's first qword. stock+0xe8 is a different
  service key; using it produced `message sender is not registed` and map errors.
- The memory registry probe now uses the actual provider observed by h_mlookup.
  Calling e69560 with only a zero qword was an invalid lookup: that function
  expects a two-word lazy handle containing a service key in its second word.
- Copied vtables preserve the two-word RTTI/offset prefix.

Prototype limitations are listed in linkfake/README.md. Fixed delayed mount,
service wiring copied from stock initialized objects, lifecycle/hotplug, private
protocol-slot effects and multiple sticks still require verification. The real
AZ has not been touched. Guest rollback backups are documented in that README.

Independent mount evidence after the JUCE ownership fix: emulator PID 330642;
local USB2 re-notifier PID 330518 stopped, `umount /media/usb/sdb1` announced to
/proc/udev_usb2 (physical mount retained). SOURCE now shows local USB1 and the
player-3 LINK USB1, without a local USB2 row. The dedicated server still has
/media/usb/sdb1/PIONEER/rekordbox/exportLibrary.db open (fd 59). This is stronger
than the previous constructor-only logs, but playlist/track UI verification is
still required. Reboot/restart can re-enable the emulator's stock USB announcer;
repeat suppression for subsequent independence tests.

**Verified milestone after ownership fix (PID 330642):** the player-3 LINK source
browses a populated Artist/Album list and Playlist tree while local USB2 is absent.
Opening Reggaeton displays populated track contents (Supernova, GRAN VÍA, etc.).
Screenshots: research/shots/hub-independent-root.png,
hub-independent-playlists.png, hub-independent-playlist-tracks.png. This verifies
rekordbox browsing from one independently constructed library server without
borrowing a local USB1/2/3 library server. Playback and 4+ distinct sticks are
still unverified; community packaging has not started.

The emulator remains on the corrected LINKFAKE_HUBSRV build. The re-notification
helper was stopped for this running instance and local USB2 was unannounced,
while sdb1 remains mounted and used by the dedicated hub library server. A restart
can re-announce USB2: suppress it again for independence tests. Preserve the
before-hubsrv backups; see linkfake/README.md for rollback.

Latest follow-up: the dedicated track-info server now obtains file identity from
its paired music server (scoped 0x1981560 lookup adapter, interface music+0x160).
Retested loading “Algo más” on current PID 340101 with local USB2 absent. It loads
with duration/BPM/key; waveforms are still blank with WaveformReplyer typeid
warnings. Prior corrected build's PLAY advanced the remaining-time counter;
audio output has not been captured. See GHIDRA-FINDINGS.md for exact current
configuration, PIDs, lookup details, limitations and next multi-slot steps.

2026-10-17 follow-up: four dedicated server pairs now run for player3 USB/USB2
and player4 USB/USB2, each opening a separate hub mount's exportLibrary.db.
LINKFAKE_SLOT is unset. Emulator usb-renotify helper excludes reserved hub mounts
so no local USB2 row is announced. A registry identity bug was verified and
adapted in owned server threads; details in GHIDRA-FINDINGS.md. Four-source UI
acceptance is STILL INCOMPLETE: A playlist contents work, C/D playlist trees
render but their tracks stay blank even though TCP returns actual track items;
B was also blank in latest sweep. Investigate client association/cache next.
Current PID 379856, no community package yet, hardware untouched.

Playback fixture reminder: each of the four emulator hub images currently has
17 audio files despite the full DB. Verified Algo mas is present on every stick.
Only load stat-verified existing files. Reggaeton entries test DB browsing only.

Latest emulator milestone (PID403207): all A/B/C/D sources have displayed full
Reggaeton playlist284 with metadata/waveform previews (B-step-selected.png,
open-playlist-A/C.png, D-reselect.png). State-aware UI navigation matters;
blind double taps can hit a different view and are not a reliable failure test.
B linked-player USB2 loaded actual-file Algo mas with scrolling and overview
waveforms; PLAY advanced03:22.647 to03:17.327 (B-algo-loaded/playing.png).
Audio output not captured; trace-off repeat, 4+ expansion/lifecycle and community
SDK package still outstanding. Hardware untouched. Sparse17-file fixture noted.

Five-stick milestone: SDK build auto-discovers fifth hub fixture sdf1 as linked
player3 slot8; all5 separate DBs open, no localUSB2. fifth-all-tracks.png shows
2547 tracks, fifth-playlist-retry-full.png full284 with metadata/waveforms.
First playlist entry was empty; reopening worked, so reliability remains open.
CurrentPID431178; TRACE_CLIENT off/local-only on. Playback fromfifth, lifecycle,
real-kernel support asset and final AZ community package still outstanding.


Actual emulator USB lifecycle (PID460175): discovery fixture path now symlinks
/proc/mounts, so scans use real kernel mounts. QMP device_del koi-usb2 removed
sdc1; its DB fd and player03/usb2 alias disappeared, other4 DBs stayed open.
device_add usb-storage drive=koi-hub2 bus=xhci.0 port=1.2 id=koi-usb2 restored
sdc1, its DB and alias in the same EP147 process. Reinserted library displays
artist/album and full Reggaeton284 with metadata/waveform previews:
physical-reinsert-library.png, physical-reinsert-playlist-selected/full.png.
Changed device paths and replacement without observed absence remain unresolved.
Development ZIP now generated by linkfake/package.py in linkfake/dist/, containing
SDK .so, manifest, assets/azhub.ko, test instructions and LICENSE. Not hardware
validated or a completion claim; see AZ-TEST.md for current limitations.

Post-physical-reinsert playback verified on PID460175: select known-present
Algo mas from sdc1 all-tracks, load deck1, both waveforms render. PLAY advances
remaining03:22->03:11 with waveform scrolling (physical-reinsert-algo-selection,
-loaded, -playing.png). Paused again after test. Audio output not captured.
All5 hub sticks currently attached; discovery mount-table path symlinks real
/proc/mounts. Development package exists, but empty-browser bug and source
replacement/path-reuse robustness remain active work; no real AZ deployment.


Source reuse build (SDK SHA25660617e8041ebd51c3eb42db9132a9fb3e2de36082ea730736e1057ff73cf3733):
config parser now parses into a local candidate, scans even at8 allocated slots,
and reuses absent pairs after unmount_pending clears. Keeps native server objects,
player/slot, transport and private request code. Gate FuseFilsine requests for
absent sources. Socket setup failures now close their fd. Active assignments stay
unchanged. Built and allowed through normal emulator UI, PID469735.
research/scripts/test-source-reuse.sh passed five successive changed-path fixtures
for sdc1: /media/usb/koi-reuse-1..5. Each cycle withdrew source, waited8sec,
reintroduced under new path, verified reuse/mount logs, samePID and5 DB handles.
Ten total lifetime paths (initial5+5new) with only initial10 registered servers
during the cycles. Fixture uses real device metadata and directory symlinks, not
five distinct-content replacement sticks. It restores real /proc/mounts on exit.
That immediate restoration allocated a sixth pair (player4 slot8) while prior
removal was pending, then settled at5 visible sources/DBs and12 retained servers.
Thus startup/post-removal path reuse passes; concurrent arrivals may allocate
until the8-pair limit. Same-path replacement with no observed absence remains
unhandled. No hardware changes. Development ZIP regenerated with this binary.

Browser cancellation investigation: current emulator PID489740,20 hooks
(14normal+browser guard+5diagnostics), TCP packet hex logging OFF. Env still has
TRACE_CLIENT=1,TRACE_CANCEL=1,BROWSER_GUARD=1; real mount discovery symlink intact.
Default community build has guard OFF until accepted; no new dist ZIP generated.
New browser-guard.h and12-case test-browser-guard.c pass optimized + ASan/UBSan
checks, covering pending empty hub case vs local/other peer/wrong agent/completed/
nonempty/unrelated media. Initial source identity comparison missed client high
flags; now compares low48 bits (client ticket e.g.01012200000202 vs registry
identity012200000202). This was caught by the positive test before final runtime.
Runtime guard field probe confirms actual agent/browser offsets. Corrected guard
skips empty-pane renewal at31777.776 and31780.210 player3slot7. Playlist root now
populated (guard-fixed-B-playlist-root.png). However selecting Reggaeton leaves
side preview blank (guard-fixed-B-selected.png): main tree nonempty makes guard
fall through. Earlier31777.978 pending request with empty=0 was cancelled too.
Thus guard is PARTIAL, not a complete reliability fix or accepted package change.
Need preserve pending main/lower requests even when other pane has rows, ideally
coalesce/defer WaveformRenewEvent rather than discard legitimate waveform color
updates. Do not blanket suppress cancellations or apply stale events to a new
source. A deferred event would require native UI completion/ownership handling:
caller owns incoming event via pointer argument; normal handler consumes it and
sets pointer0. Existing guard leaves it unchanged so caller deletes normally.
All source/build edits uncommitted; current dist remains c182a27a... prior reuse
build, without quiet-log change/diagnostics/guard. Real hardware untouched.


Mount worker acceptance follow-up: PID544361 remained unchanged through actual
QMP removal of koi-usb2 (sdg1). Its DB closed, native alias released at35625.731,
and backing mount released at35628.669. Reinsert became sdc1, stock-mounted,
and regained its DB and populated2547-track browser with waveforms. All five DB
FDs remained open afterward. A separate temporary FAT image on hub port1.6
became sdg1: HubMount automatically mounted it (journal10:57:04), then released
it at35729.630 after physical unplug. This temporary image contained no library;
it tests filesystem fallback rather than a sixth source. QMP node and image
removed afterward. Screenshot mount-worker-B-tracks-after-hotplug.png. Corrected
concurrent scanner maintained five sources; final package/readiness sweep pending.


2026-10-17 hardware feedback / source names (not deployed)
--------------------------------------------------------
User confirms 0.1.3 browses and plays hub tracks on the real AZ. Two new issues:
waveforms alternate RGB/blue and bottom/center alignment, and source labels show
HUB sdb1 rather than the filesystem label. Read-only SSH diagnostics authorized;
ask before any new AZ test, deployment, reload or other mutation.

Source-name fix implemented in linkfake/source-name.h and discovery.h: read
per-device udev ID_FS_LABEL_ENC (preserves spaces/UTF-8 and duplicate labels),
fallback to /dev/disk/by-label; unlabeled devices use their physical hub port,
e.g. USB 4 or USB 4.2 for nested hubs. Publish proper UTF-16BE media names, with
surrogate-safe truncation to the native field. Fourteen behavior checks pass on
host, ASan/UBSan, and actual ARM64 emulator. Emulator discovery finds all five
real fixture labels; evidence research/source-name-emulator-test.log. SDK builds.
Not yet checked in native source-list UI, packaged, or installed on AZ. AZ labels
confirmed read-only: sdb1 CAMI, sdc1 NACHO PLUGS (latter subsequently unplugged).

Waveform cause remains under investigation; no waveform fix applied. Current
send_status copies one global native 0x0a template, captured alternately from AZ
decks, every 200ms. Real AZ sample observed six differing templates in twelve
samples (research/az-013-waveform-template-sample.json); causality not proven.
Do not blanket suppress WaveformRenewEvent: must honor actual color/position.
Native DeviceSettingManager vtable 0x2c0fbd0; ctor15e3f58; color setter15e2610,
position15e26e0, getters15e4da8/15e4e70, notify-all15dfb08, notify-one15e0390.
Arrays color+0x98, position+0x770, member stride0x18. 15e21f0 serializes eight
settings for each media kind into nine 8-byte rows then calls IPLinksetDevSetting
vtable+0x10. Getter indices via15df808. Next trace incoming link settings parser
and status field offsets, then reproduce/verify emulator before proposing AZ test.
Real AZ still runs accepted 0.1.3; local build changed only names so far.

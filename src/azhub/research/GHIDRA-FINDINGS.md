# EP147 Ghidra findings (firmware 2.00, md5 8daa1de55d7328f458f6d855cdc3cdde)

Reference for the multi-USB work. Addresses are EP147 virtual addresses (non-PIE, same at
runtime). "Verified" means checked in disassembly or at runtime in the emulator; everything else
comes from Ghidra decompiler output and may be wrong in details. See `../NOTES-MULTI-USB.md` for
the experiments and the overall plan.

Tools: `scripts/gh.sh dec:<hex> xref:<hex> str:<text> vt:<hex>:<n>` (Ghidra headless,
`ghscripts/Probe.java`), `scripts/vmem.py <EP147> <hex>[:len] s<hex>` (raw bytes / strings at a
vaddr). EP147 copy in WSL: `~/fw/EP147`; disassembly `~/ep147.dis`; RTTI names `~/ep147.rtti`.

## Media ids

`trackid::LocalMediaID` is 2 bytes: byte 0 = **sdid**, byte 1 = **link member** (verified at
runtime: local USB1 mount = `0x0102`, local USB2 = `0x0122`, read as little-endian u16).

sdid = `type | (instance << 5)`, type in the low 5 bits (verified from the tag table at
`0x2bb63c8` and the server factory):

| type (low 5 bits) | meaning | instances built at startup |
|---|---|---|
| 2 | USB | `0x02` USB1, `0x22` USB2 (`0x42` USB3 has names `MscLibSrvUsb3`/`TInfRepSrvUsb3` but **no server is built**) |
| 4 | rekordbox Cloud library | `0x04`, `0x24`, `0x44` (Cld1..3, one per login slot: players 1/3/4) |
| 6 | Beatport | `0x06`, `0x26`, `0x46` |
| 7 | TIDAL | `0x07`, `0x27`, `0x47` |
| 8 | Apple Music | `0x08`, `0x28`, `0x48` |
| 0xb | Spotify | `0x0b`, `0x2b`, `0x4b` |

## Music library servers (browsing)

- Builder: `FUN_013809f0(holder)`, runs once at startup. Holder layout (8-byte words):
  `[1]`,`[3]` two "Ms" objects (0x228 B); `[5..0x25]` step 2 = 17 server `shared_ptr`s in the
  order Usb1, Usb2, Cld1-3, Bp1-3, Td1-3, Ap1-3, Sp1-3; `[0x27]` 0x2e8 B object (3000 ms, "Ms");
  `[0x28]` the mount event handler; `[0x29..0x39]` one 0x18 B tag holder per server
  (`FUN_01384ae8(obj, &sdid)`); `[0x3a]` a 0x40 B object passed to every server (vtable +0x90).
- Per server, in order: `FUN_0137bc60(&out, &vec3, &name, &sdid, &flag)` (factory),
  `FUN_013860c8(holder+slot, &out)` (store), vtable `+0x88(tagholder)`, `+0x90(holder[0x3a])`,
  `*(server+0x140) = hash(name)` (thread name hash, `FUN_0137eef8`; not used for routing),
  `FUN_0137ef60(server)` (start its thread), vtable `+0x98(holder[0x27])`.
- Factory `FUN_0137bc60` switches on `sdid & 0x1f`: 6 → `FUN_013795e8` (Beatport), 7 →
  `FUN_01379af8` (Tidal), 8 → `FUN_0137a008` (Apple Music), 0xb → `FUN_0137a518` (Spotify), any
  other → `FUN_0137aa28` = generic `music_library::MusicLibraryServer` (USB **and** Cloud).
- `MusicLibraryServer` vtable `0x2bb1268` (runtime objects are 0x200 B apart). Named slots
  (from embedded `__PRETTY_FUNCTION__` strings): 21 finalize `0x1375610`, 23 onMediaMounted
  `0x1373ca8`, 24 onMediaUnmounted `0x1373928`, 25 getFileIdentityInfo `0x1376050`, 26
  onMediaInfoMan_MountCompletion `0x13735b8`, 27 ...unMountCompletion `0x1373248`, 28
  onServiceActived `0x1372a08`, 29 onServiceTerminated `0x1372698`, 34/35 onTaggedTrackCache
  Updated/Cleared. Server's sdid byte is at `server+0xd0`. It is a
  `client_server_framework::ServerThreadBase` (own thread, `server+0x28`).
- Mount event handler `FUN_0117c1b0(handler, LocalMediaID*, juce::File path, bool, listener)`
  (`musicLibrary_MountEventHandler.cpp`): walks the vector of `shared_ptr<server>` at
  `handler+0x58..+0x60`, picks the one whose `+0xd0` byte == sdid, calls its vtable `+0xb8`
  (onMediaMounted) under the handler mutex. Unknown sdid → "target server is not exist".
  Handler ctor `FUN_0117bf08(h, &vecA, &vecB)`: vecA (2 entries, the "Ms" objects) copied to
  `h+0x40`, vecB (all 17 servers) to `h+0x58`.
- onMediaMounted is called **only for local media** (verified: runtime probe saw USB1/USB2 mounts,
  never the LINK bind mount at `/media/player03/usb`). Caller return address `0x117c42c`.
- `FUN_01352ee8` ("libopen") is called right after with the same id and the mount path.

## Track info repository servers (waveforms, beat grids, cues)

Parallel set of servers, names `TInfRepSrv{Usb1,Usb2,Cld1..3,Bp1..3,Td1..3,Ap1..3,Sp1..3}`
(`TInfRepSrvUsb3` string exists, unused), built in `FUN_0198f260`. Mount handler
`FUN_01928398` (`trackInfoRepository_MountEventHandler.cpp`): `FUN_01926dd0(id)` gives a server
type (0 or 1), then `FUN_01927c98`/`FUN_01927e30` look the server up by id ("target server is not
exist" if none) and call `FUN_01974ff8`/`FUN_01987388` to mount.

## dbserver request routing (verified at runtime)

How a Pro DJ Link dbserver request (TCP, port announced on 12523; 45xxx in the emulator) reaches a
library:

1. Socket accept `FUN_026ad520` (thread `CnntDtcSocket`). Field reads `FUN_026aa7xx..9xx`.
2. `FUN_00ef05c8(conn)` parses one message into a 0x40 B `DataStreamMessage`: `+0` u16 type
   (0x3007 = request seen first, 0x4000 = menu reply...), `+8` txid, `+0x28..+0x38` vector of
   args. Loop `FUN_00ef8258` (thread `CycEvntChck`) hands it to `conn[2]->vt[+0x10]`
   (`FUN_00efb508` → `FUN_00efab90`, `data_stream::DataStream::receiveInternalMessage`), which
   posts a task.
3. `FUN_007d6e28(task)` = `ServerMessageDispenser::receiveMessage` lambda: `task[0]` message,
   `task[1]` owner. The owner for dbserver requests has vtable `0x2934fe0`, and a `std::list` at
   `owner+0x2c0` (node = {next, prev, server*}) holding **34 servers**: the 17
   `MusicLibraryServer`s (vtable `0x2bb1268`; Beatport `0x2b97450`, Tidal `0x2b8bdb0`, Apple
   `0x2b92f00`, Spotify `0x2b8a1d8`) followed by the 17 track-info-repository servers (vtable
   `0x2cafeb8` for USB/Cloud, `0x2cb0528` for streaming), all in the order Usb1, Usb2, Cld1-3,
   Bp1-3, Td1-3, Ap1-3, Sp1-3. If nobody claims the message: WARN
   `request is unsupported command=CMD_...` (ServerMessageDispenser.h:125), then `FUN_007d6bd8`.
4. Each server is tried with `FUN_00e6b1c8(server, msg, conn)` (returns 0 = taken):
   - if `server->vt[+0x80]` is the default `FUN_00e6c338`: `arg0 = FUN_00f05680(msg, 0)` (DMST
     u32), slot = `(arg0 >> 8) & 0xff`, mapped to an sdid by a switch: 2→`0x03`, 3→`0x02`,
     6→`0x04`, 7→`0x22`, 8→`0x42`, 9→`0x06`, 10→`0x07`, 11→`0x08`, 12→`0x24`, 13→`0x26`,
     14→`0x27`, 15→`0x28`, 16→`0x44`, 17→`0x46`, 18→`0x47`, 19→`0x48`, 0x14→`0x0b`,
     0x15→`0x2b`, 0x16→`0x4b`, anything else → `0`. The server matches if `server+0xd0 == sdid`,
     or `server+0xd1` (an alternate sdid) has nonzero low 5 bits and equals sdid. At runtime
     USB2's servers (both kinds) have `+0xd1 = 0x03`, so the SD slot is served by USB2.
   - otherwise it calls `server->vt[+0x80](server, msg)` (x0 = server, x1 = msg) and treats a
     nonzero low byte as a match. **A server with a patched vtable can claim any slot value.**
   - then `server->vt[+0x48](server, u16 type)` must accept the command type, then
     `FUN_00e6b070(server, conn, msg)` queues it on the server thread.
5. Verified: rewriting the client's DMST slot to 7 makes the USB2 servers answer for a LINK
   member (browsing, track load and waveform all worked); slot 8 (sdid 0x42, no server) gives
   "unsupported command=CMD_ROOT_MENU".

Other slot→sdid tables (same values, used elsewhere): `FUN_01116bf0(this, u8 slot)` (called by
`StorageDevCtrl` thread from `0xbbe1b8` for the remote mount: slot 3 → 0x02), `FUN_01116b10` and
`FUN_01117438` (index 0..0x17 → sdid). The holder object `holder[0x27]` (vtable `0x2bb5d28`,
"MscLibTimer", 3000 ms) keeps a vector of `server+0xc8` interface pointers (registration via
server `vt[+0x98]`); it is not the dbserver router.

## EDB daemon (TCP 12523)

`FUN_026af7f0` is the daemon loop that listens on 12523 (`FUN_026ae5f0(0x30eb)`, env
`EDB_LISTEN_PORT` in `FUN_026ae920`). Per-connection read → `FUN_026af350`; a second listening
socket's messages → `FUN_026af210`/`FUN_026af050`. Not yet traced to the library servers.

## Pro DJ Link / remote media (from the earlier sessions)

See `../NOTES-MULTI-USB.md` sections "Pro DJ Link Mode gates it", "Ghidra map of the
remote-media chain" and "Limits": Link-mode cache byte `DAT_03f86a08` and its 4 readers,
FuseFilsine mount orchestration `FUN_0139b3b0` → `FUN_0139afd0` → `FUN_01399f10`, storage
lifecycle in `usecase::device::UsecaseStorageDeviceController`, media query sender
`FUN_013d75f8`, player-cap table `0x2bc1df0`, client protocol functions `FUN_0112ddd8`,
`FUN_01142bc8`, `FUN_01142a88`, `FUN_00e63ef0`, `FUN_00e66668`.

## Per-player server construction experiment (continuation)

New code: `../linkfake/hubservers.h` (emulator-only, opt-in
LINKFAKE_HUBSRV plus LINKFAKE_PROBE).

Verified with runtime logs:
- Fresh MusicLibraryServer and PersistentStorageServer instances can be created
  and started; they can be appended to the real dispenser list.
- Vtable +0x80 can claim private DMST slot 0xe0. Feature and root-menu requests
  reach the added music server and receive replies when its sender is initialized.
- Root-menu reply currently has count zero (`ListServer.cpp:894`, mountPoint is
  not exist). No independent playlist/track browsing proof yet.
- Shared service keys copied from stock initialized servers: base +0xd8..+0xe8;
  music +0x190..+0x1c0. These are integer global lookup keys, not weak_ptr control
  blocks. Do not dereference them as reference-counted pointers.
- Music reply-sender setter is vt+0x58 (0x13740d8). Obtain its argument from
  `commands=*(stock+0x1d0); reply=*(commands+0xd0); sender=*reply`.
  The unrelated key at stock+0xe8 does not work as this argument.

ABI correction (checked in disassembly): factory 0x137bc60 receives x0=vector,
 x1=name, x2=sdid pointer, x3=alternate-sdid pointer, x8=result pointer. The
result is only 8 bytes. A C 24-byte aggregate can call it with x8, but must NOT
be returned directly to the stock caller (which reserves only 8 bytes). The
assembly entry shim captures x8 and the C helper writes exactly one qword.

Correction to the earlier `libopen` label: 0x1352ee8 is a **mount-point object
constructor**, not evidence of a successful database open. It copies LocalMediaID,
path and arguments and initializes the ExternalStorage mount-point vtable and
path constants. A log of that function alone does not prove a database was opened.
The current failure needs tracing from that constructor to actual mount-point
initialization and attachment to each command handler.

`e69560` takes a two-word lazy service handle (cached object, global service key).
The old injector probe passed a zero qword and therefore did not resolve the
registry. The probe now uses the provider captured from genuine e5e7d8 calls.
Registry lookup returns a 0x20-byte reference wrapper even for missing entries;
its non-null return does not prove media registration.

No AZ hardware writes or tests in this continuation. USB2 is still auto-announced
by the emulator, although LINKFAKE_SLOT=7 was removed. Remove that announcement
before claiming an independent-source acceptance test.

Further implementation notes: the factory vector is mutable per-source cache
state. The latest build allocates a private in-place shared cache for each stick
using the layout at 0x1380e9c (allocation 0x50; control vtable 0x2bb6040;
object at +0x10 with vtables 0x2bb5f00/0x2bb5f58). This removes the accidental
sharing of USB1's vec[0]. Retain the cache until process exit. The latest factory
hook uses a small assembly entry to capture x8 and writes only the native 8-byte
result; the 24-byte aggregate exists only when calling the original from C.

Latest initialization refinements (still under emulator verification): populate
server+0x150 with `FUN_01385378(server)` exactly as the stock builder does; this
returns server+0x28 (the task thread), not an object-map ID. Each music and info
server needs that mount-interface thread reference.

ExternalStorageMountPointCreator is vt 0x2bb4bd0, slot 2 = 0x137e130. It constructs
the SQLite mount point and calls its vt+0x10 initializer (0x13532a8). The
initializer checks directory/file access before creating database adapters.
The current experiment passes each stick's original `f->dir` to the native
library mount callback; `/media/playerNN/usb` remains a read-only audio alias.
Do not infer library success from the 0x1352ee8 constructor log.

Private request slots require two stages: the per-object vt+0x80 matcher selects
the dedicated server using e0..e7; its vt+0x48 command acceptance wrapper restores
native USB slot 3 only after the command is accepted. This keeps stock servers
out of routing while allowing native command handlers to decode the request.
Parsed DataStreamMessage args: vector begin/end at +0x28/+0x30; each tuple is
0x30 bytes; unsigned-int variant discriminator is 1 at tuple+0, value at +8
(verified from 0xf05680). This conversion is currently under emulator test.

Truncating the AZ status template to 0x200 bytes did NOT suppress extra remote
slots: it triggered USB2/USB3 mount attempts. That experiment was reverted.

JUCE ownership correction (verified by decompilation): 0x24876f0 only stores the
input C pointer (StringRef). It does NOT allocate an owned juce::String. Factory
names accept StringRef and must not be destroyed with 0x2469aa0. The mount callback
copies a juce::File/String with 0x2470df0, which increments the header at data-0x10.
Use 0x24a1180 to construct the owned mount-path string, then 0x2469aa0 to release
it after queuing the task. Earlier prototype runs used StringRef for the File and
could corrupt memory; their empty-library results are not conclusive. Corrected
build deployed to the emulator; needs retesting before any hardware packaging.

## Track-info follow-up and current emulator state

The current deployed build (PID 340101 at verification) additionally routes each
new track-info server's file-identity lookup to its matching music server.
PersistentStorageServer processing is primary vt+0x50 (0x19802c8); its lazy lookup
helper is 0x1981560 and gets the service key from server+0x170. That key is unset
on newly constructed servers. A scoped thread-local adapter returns the matching
music server's IFileIdentityInfoGetter interface at music+0x160. Its vt+0x10 thunk
is 0x1376c90, which adjusts this by -0x160 and calls 0x1376050. The adapter marks
its direct process-lifetime handle with a null mapped-object ID.

Retest: “Algo más” loads with duration/BPM/key from LINK player 3 while local USB2
is absent. Earlier corrected build PID 330642 also advanced its playback counter
from 3:22 remaining to 0:57 after PLAY. Audio output was not captured/listened to.
Current fileIdentityInfo warnings are gone, but waveforms remain blank and
WaveformReplyer_400Pt.h emits invalid typeid warnings. Do not claim waveform/cue
completion. The warning site 0xeed4c0 compares request RTTI against 0x2b24998.

Stopped current emulator USB re-notifier PID 339973 and unannounced sdb1 from
/proc/udev_usb2 again. EP147 PID 340101 is left running on the new build, paused
on “Algo más.” The local USB2 announcement was briefly restored and removed in
a finally block to compare status snapshots. A four-snapshot comparison found no
stable differing template bytes, so no status-bit patch was inferred from it.

Next major scope remains 4+ sticks and the community mod. Present config still
has exactly ONE stick: `3 10.0.2.50 /media/usb/sdb1 HUB LIB`. Per-stick servers are
implemented, but config and client fd routing are still keyed only by player.
For several slots per player, add a configured media-slot field and key requests,
media replies and mount commands by (player, slot). The current prototype maps
all requests for one player to one private slot and only mounts its /usb library.
Do not claim multiple independent libraries until these paths and UI are tested.

Existing emulator images on Windows: C:/Users/icast/xdj-az-emu/build/usblib.img and
hublib2.img, hublib3.img, hublib4.img (each 1.5 GiB). They can supply extra hub
ports for tests via QMP, without the AZ hardware. Community port must use
azmod-mod.h claim_hook/claim_vhook rather than LD_PRELOAD; no community artifact
has been produced yet. Keep the full requested objective active.

### Media identity correction (2026-10-17 continuation)

Verified on emulator PID 375489 using native e5fb00 path resolution, not pointer
heuristics. Existing LocalMediaID 0302/0402 produces AllInOne references
030200000001/040200000001 and resolves to local /media/usb/sda1. IDs 0322/0422
fail resolution. Thus the earlier interpretation of the high byte as player was
incorrect for registry lookup. Replacing the reference identity with
010200000202, 012200000202, 010200000302, 012200000302 resolves respectively to
/media/player03/usb, /media/player03/usb2, /media/player04/usb,
/media/player04/usb2. All return status 0. Backend pointers remain unchanged.

linkfake.c now replaces the out-of-bounds registry dump with a native path
probe (initializes/releases owned strings and frees the 0x20 reference). The
probe build is deployed and all four mount queues completed. A subsequent
source change adapts registry results only on the owned HubLib/HubInfo threads
or paired info callback, matching the source's expected ID. This change builds
with -Wall but is NOT DEPLOYED OR UI-VERIFIED yet. Current runtime remains the
probe-only build. Next: deploy the scoped adapter, verify actual thread naming
and all four playlist browsers, then investigate remaining waveform behavior.
Do not claim the C/D blank browsers are fixed from path evidence alone.

### Scoped remote-reference adapter deployed; client rendering remains incomplete

Current emulator EP147 PID 379856. Both the remote-reference adapter and the
consistent per-owned-server mount ID 0x0102 are deployed. Every dedicated
server is constructed with SDID 2 and has its own cache; on its HubLibN/HubInfoN
thread (names verified in /proc), registry reference identity is translated to
its actual SinglePlayer member/slot. Stock threads are untouched. All four
exportLibrary.db files are open from separate sdb1..sde1 paths.

Screenshots mediafix-C/D-playlists show populated player-4 playlist trees.
Their selected Reggaeton track pane stays empty; mediafix-B browser also empty.
consistent-D-contents and consistent-D-late remain empty after switching all
owned mount IDs to 0102. consistent-A-contents shows populated Reggaeton tracks.
Therefore the internal-ID cleanup alone does not complete four-stick browsing.

Important narrowing evidence: decoded incoming TCP fd62 transaction17 for
HUB D contains 25 track menuitem responses (4101), with Supernova, GRAN VIA,
etc.; header/footer also received. The first Supernova item has EXACTLY the
same 18 argument values as working HUB A fd61 transaction31:
[0,2597,20,<Supernova UTF16>,2,<empty>,262148,16842752,2542,1,33554688,
3,4,6,<2A UTF16>,17604,2,<empty>]. Do not continue assuming DB or playlist
access failure: server returns real track contents while client does not render
D. Next inspect client transaction/menu association, setup identity and cache
limits for player4/USB2; retain four-stick and community objectives.

### Protocol comparison and distinct-requester experiment

Compared full headers/footers of working A txn31 vs blank D txn17 on PID379856:
4001 arguments [1,0], 4201 has zero arguments, identical except transaction ID.
First track's 18 arguments identical too. D sends command 0001 after response;
do NOT assume this is cancellation without decoding its native meaning.
A targeted trace places its async send at ef5010 -> eff9b0 -> f00098, generic
UDSCommunicator/DataStream send queue, not the originating client callback.
This trace alone does not identify the browser's rejection mechanism.

Tested unique requester numbers: player3 ->5, player4 ->6 in setup and DMST.
PID384711 is running that experimental binary; HUB D remained blank, including
playlist root in requesters-D-contents.png. Outgoing DMST confirmed requester6.
This hypothesis did not fix browsing. Source now puts the experiment behind
LINKFAKE_PEER_REQUESTERS (unset by default); source was edited after deployment,
so running binary still uses distinct requesters until rebuilt/redeployed.
Next restore baseline requester5 and trace actual client receive/menu dispatch;
no evidence yet that the server reply data itself differs from working A.
Baseline restored afterward: rebuilt with the distinct-requester experiment
opt-in and redeployed. Running EP147 uses requester5 again, with scoped registry
adapter and per-server mount0102 retained. Prior experiment log archived as
/tmp/linkfake.peer-requesters.log. Build passes -Wall.

### Receive-path map and emulator display recovery

Native receive chain: ef8258 parses via ef05c8, calls observer vt10; efb508
thunk adjusts -8 into efab90, posts task (vtable2b2db40, execute efd850).
efd850 -> efd3b0 broadcasts to observers list at DataStream+120, stopping when
observer vt10 returns0. ClientThreadBase receive e67678 translates incoming
peer via13db0b0/13db0c0, matches ticket member at+8 AND txn+20, then expected
command+1a. Render 4001/4101/4201 share a ticket. Ticket pending vector is
client+110..118, mutex+128. Targeted e67678 trace added but default OFF behind
LINKFAKE_TRACE_CLIENT. No runtime matching evidence obtained yet.

Black screen in trace run persisted after disabling hook: actual cause found
in X display, not proven hook failure. Xrandr reported 320x200 current with
Virtual-1 connected but no active mode; Xorg logged Allocate new frame buffer
320x200. Restoring Virtual-1 via DISPLAY=:0 xrandr --output Virtual-1 --mode
2560x1600 immediately restored visible UI in display-restored.png, but EP147
occupies 1280x800, so then set mode1280x800. Running PID390642 has client trace
OFF; dedicated server mounts all completed. Do not mistake disabled display
for failed startup. Next enable trace once display mode is verified after
restart and reproduce D then A with source screen screenshots first.

### Sparse audio fixture and successful receive trace (PID393017)

User reminded us exportLibrary.db is complete but audio copy is sparse.
Verified all four current Contents trees have 17 files. Algo mas by #TocoParaVos
is present on sdb1/sdc1/sdd1/sde1 (exact UTF8 filename visible in guest).
Use it or another stat-verified existing file for load/playback; the Reggaeton
playlist with Supernova etc. is ONLY a DB browsing fixture. Missing audio files
must not be diagnosed as hub playback routing failure. Prior Algo mas load
used an actually present file. DB browsing should work independently of audio.

Enabled LINKFAKE_TRACE_CLIENT=1 in guest preload env, restarted EP147, verified
X mode1280x800 and source screen before taps. Runtime393017 trace runs and UI
works, so earlier black screen was display mode, not evidence against hook.
clienttrace-D.png and clienttrace-B-contents.png show populated Reggaeton
track panes! clienttrace-A/C-contents show empty track panes in same sweep.
Thus source issue is intermittent/context-dependent; cannot claim all4pass.
Trace matches D and later C render headers/footers to correct member/txn:
client vt2b80108, peer4 -> ticket member302, result0. result1 from other
observers is expected fanout and does not itself prove rejection. Even blank C
has matched header/footer messages, so next trace should move inside render
request ticket/output publication, not repeat basic socket or registry checks.
Logging may affect timing, but A/C still failed with it. No hypothesis proven.
Guest trace currently remains ON. Full community package still outstanding.

### Render publication trace, explicit playlist open

Current PID403207, LINKFAKE_TRACE_CLIENT remains enabled. Added optional hooks:
114dff8 TrackListRequestTicket publish (native vector byte count, total, full
media identity, flags, listener); 1142460 list cancel; e61ff0 cancel scheduling.
All builds pass -Wall. Cancellation origin is GUI MusicLibraryCommunicationAdaptor
1fc7358, called by request owner destructor1fd2d08, through music client116a80c.
This can be normal when UI navigation replaces a request; not proven bug itself.

Important testing correction: tap selected Reggaeton again to enter full playlist.
D-reselect.png shows full 284 tracks with BPM/key AND waveform previews for
player4 USB2. open-playlist-A.png and open-playlist-C.png show full lists too
(inspect shots for exact current state). B's open-playlist-B.png is empty with
count0000. No four-source acceptance yet. Some earlier empty side previews were
not direct full-playlist failures; preserve that distinction.

Publication examples: callback1fd01b0, vector2600bytes, total284.
D media01012200000302 flags0/0; A media01010200000202 flags0/1 then0/0;
B media01012200000202 flags0/1 ONLY; C media01010200000302 flags0/1 then0/0.
This is evidence for preview vs main request mismatch, not failed DB access.
For B investigate why opening from preview does not issue/complete the main
TrackListRequestTicket (flag62=0). Do not blindly patch source caps or database
lookups again. Sparse audio reminder remains: only17 actual files per image.

### HUB B full playlist verified with state-aware navigation

On existing PID403207, reopened B, captured B-step-playlists.png with gray
Reggaeton selected and populated side preview. ONE tap selected/opened it:
B-step-selected.png shows full 284 tracks, BPM/key and waveform previews, player3
USB2. Together with open-playlist-A/C and D-reselect this proves each configured
source can browse full playlists. It does not prove reliable arbitrary automated
navigation or distinct content: the four images are clones. Prior blindly sending
two taps could send the second into the already changed view; an empty screenshot
from that sequence is not sufficient to diagnose library failure. Observe views
between actions. Some prior cancellation traces are normal UI request-owner
destruction during navigation, not a demonstrated prototype bug.

Next checks: B and D load Algo mas (verified actual audio), retry browsing with
TRACE_CLIENT disabled; fifth stick for 4+; lifecycle/discovery and SDK community
port. Do not mark goal complete. Current playback navigation hit az-drive knob
counter limit; reverse the knob to recenter before forward navigation. Tapping
an already selected track opens the load-deck chooser, so don't blindly focus it.

### Secondary-slot load and waveform milestone (PID403207)

B-step-selected.png verified full Reggaeton list284. B's Track view also shows
2547 DB tracks. Selected actual-file Algo mas (track35, zero-based34):
B-algo-check.png. az-drive knob was saturated; reversed60 detents then forward34
without retapping the selected track (retapping opens load chooser).

B-algo-loaded.png: source badge [3]USB2, Algo mas on deck1, remaining03:22.647,
scrolling waveform and full overview waveform populated. B-algo-playing.png:
after PLAY remaining03:17.327 and scrolling position moved. This verifies
secondary hub slot load, waveform retrieval and playback progression with a
confirmed existing audio file. No captured audio output yet. Earlier blank
waveform limitation is resolved in this build's B test after remote-reference
correction; verify others before generalizing. All hub DBs separate, no local
USB2 source. Four source full-playlist screenshots now exist but images cloned;
trace-off repeat, fifth stick, standalone SDK port and lifecycle remain.

### Community-port prerequisite implemented: post-startup server creation

Refactored hubservers.h into hub_create_music(i) and
hub_finish_servers(common,timer), shared by startup and new late path.
LINKFAKE_LATE_SERVERS=1 skips BOTH native factory137bc60 and holder13809f0 hooks.
Native constructor called directly through verified indirect-result ABI.
hub_register scans initialized stock objects, extracts common handle via
commands(stock+1d0)->component(+30)->a0 and timer component+78 ->nested+60.
These offsets verified against1377430 and1377458 setters in Ghidra. It creates
new per-stick caches/servers on the dispenser thread, then attaches service keys
and registers the pairs as before. No stock DB/cache state reused.

Built -Wall clean; deployed and enabled late option in guest. Current EP147
PID411854: logs confirm 4 initialized pairs, late construction common/timer,
8 servers registered and all4 mount tasks completed. late-source.png shows
localUSB1 plus A/B/C/D, no localUSB2. late-B-selected and late-B-full verify
playlist contents/full list on secondary player3 USB2 using this late path.
TRACE_CLIENT is OFF. Before late change, traceoff-B-full also verified ordinary
browsing with extra diagnostic hooks disabled. This is still a preload build,
not an SDK package. Next SDK claim_hook/PLT port, auto discovery/lifecycle,
external Link traffic isolation, fifth-stick acceptance and packaging.

### Community SDK variant built (not runtime tested)

Added community.c descriptor hub-libraries, exact firmware ep147-9e9454e357389ece,
azmod.json and build.sh (LF line endings). Community build excludes constructor,
uses RTLD_DEFAULT libc lookup because it is loaded late, hides all symbols except
az_mod_v1_entry, and replaces raw make_hook patches with core claim_hook.
Claims nine verified libc PLT slots plus five native entry hooks, using late
server path. Research client/warning/open hooks are not claimed by SDK variant.
Failed claims counted, originals remain callable; injector not started on failure.

Both preload and SDK variants build. nm confirms only entry plus allowed linker
boilerplate symbols exported. Shared research source has unused diagnostic
functions in SDK build; build.sh suppresses that specific unused-function warning.
Development config /home/root/settings/azhub.conf required. Read COMMUNITY-PORT.md.
Not deployed: emulator still runs known late-preload PID411854, no SDK loader or
hook acceptance evidence yet. No hardware package or hardware changes. Next test
actual community loader with preload linkfake removed (retain core azmod.so),
then discovery/lifecycle, isolation, extra-stick acceptance and packaging.

### Actual community core load and approval passed; browser incomplete

Emulator's installed /usr/sbin/az-usbload predates community staging (no community
code, no az-community helper), although running core has SDK and approval UI.
For development only staged azmod.json and hub-libraries.so at settings/
az-usbload/community/hub-libraries with enabled file, copied linkfake.conf to
settings/azhub.conf. Backed preload env up as az-preload.env.before-community-hub,
removed ONLY :/home/root/linkfake.so (kept emulator core azmod.so). This is not
verification of modern USB install/apply or a hardware package.

PID415707 correctly refused new hash: status6 New mod found: allow?. Navigated
MOD MENU -> Community Mods -> Hub USB Libraries, clicked Allow in emulator.
community-allow-page/allowed.png evidence. Restarted EP147. PID417406 status1,
loaded0.1.0, 14 hooks accepted in /tmp/azmod-community/hub-libraries.log. Maps
show staged hub-libraries.so, no linkfake.so. Logs show late construction and
8 servers registered; separate sdb/sdc/sdd/sde DB FDs open. All4 hub mount queues
completed. SDK native/PLT claim compatibility and approval path now verified.

First SDK B browsing attempt blank (sdk-B-selected.png, sdk-B-tracks.png), despite
correct private-slot requests e1 and DB open. Do not claim SDK browsing works.
Next compare adapter behavior with preload (including RTLD_DEFAULT resolving
core libc wrappers vs RTLD_NEXT), response/ticket delivery and setup identity.
Research hooks are excluded SDK while mandatory five native + ninePLT remain.
Current runtime SDK variant, preload env rollback available. No AZ changes.

### SDK browsing works after source re-entry; initial readiness still suspect

No binary changes, still PID417406 SDK14 hooks, no linkfake preload. Re-entered
source A: sdk-A-root.png populated artist/album; sdk-A-playlist-selected.png
populated Reggaeton; sdk-A-full.png full284 with BPM/key/waveforms. Re-entered
B via SOURCE: sdk-B-retry-tree and selected show populated tree and side
contents, BUT sdk-B-retry-full is empty0000. Only A full284 is verified through
SDK this turn. Thus some SDK browsing works, while B full-list remains broken.
No evidence yet that changing libc resolution would help; do not change it on
speculation. Investigate startup source/library readiness (current mount queued
after media announcement) and request replacement navigation separately. Need
cold-start reproducibility and SDK C/D/playback before packaging. This turn
added evidence only, no implementation change. All files remain uncommitted.

### SDK optional diagnostics: B full list verified, preview ticket absent

Added optional two hooks in LF_COMMUNITY probe_install behind existing
LINKFAKE_TRACE_CLIENT: e67678 receive matching and114dff8 TrackList publish.
SDK diagnostic variant builds; core accepted16 hooks after normal Allow UI
for new hash (sdk-trace-allow.png). Current PID423224, TRACE_CLIENT=1 env,
no linkfake preload. sdk-trace-B-selected.png side preview blank: rendertxn17
headers/footer reach every observer result1, pending ticket vector empty.
Cannot diagnose why ticket was removed from this trace alone.

Explicit second tap into full playlist generated txn19: receive header/footer
result0, correct member202, publish fullmedia01012200000202, flags0/0,
vector2600bytes,total284, callback1fd01b0. sdk-trace-B-full.png verifies actual
full284 with metadata and waveform previews through SDK on player3USB2. Thus
B full-list failure is intermittent; previous SDK14hook emptyfull is still valid
evidence, not fixed by a proven functional change. No need redo registry or
libc resolution on speculation. Compare request lifecycle and diagnostic timing,
then verify C/D + SDK playback and extra-stick/discovery before packaging.
Current staged binary differs from earlier14hook build; manifestsameversion.

### Automatic startup hub discovery implemented and live-probed

Added discovery.h: enumerates /proc/mounts via getmntent, permits only safe
/dev/sd* sources under /media/usb/, resolves /sys/class/block/<partition>,
requires an actual USB port-chain component with dot-delimited child ports,
excludes interface strings such as2-1:1.0 and PCI dots, requires readable
PIONEER/rekordbox/exportLibrary.db. Sorts topology/device and deduplicates device;
atomically emits up to8 records. First4 map to existing player3/4 USB/USB2;
then USB3 and SD slots (latter expansion not yet verified). Names HUB sdb1 etc.
Fake IPs retain known10.0.2.50/.51 labeling; standalone AZ behavior still requires
verification. No real network address setup performed by discovery.

community_init now prefers settings/azhub.conf override; if absent it generates
/tmp/azhub-discovered.conf before claiming hooks. No source configuration needed
for startup with already mounted sticks. No hotplug/removal lifecycle yet.
Both builds pass. Compiled discovery CLI and ran it against current emulator:
/tmp/azhub-discover /proc/mounts /sys/class/block /tmp/azhub-discovery-test.conf
reported discovered4, sdb1/sdc1/sdd1/sde1 only; direct sda1 excluded, assignments
match manual setup exactly. Eight topology regression cases pass on WSL host.
SDK integration branch is built but NOT deployed: runtime remains PID423224,
previous SDK trace build with manual config, no new hash approval performed.
Next test SDK without manual config, then fifth stick and lifecycle/isolation.

### SDK discovery and local-only mode runtime test passed (limited scope)

Current PID428171, SDK14 hooks, TRACE_CLIENT removed. Manual config renamed to
settings/azhub.conf.manual-test (preserved); /tmp/azhub-discovered.conf produced
all4 proper mappings automatically. Reapproved new hash through normal Allow UI.
discovered-sdk-sources.png shows HUB sdb1/sdc1/sdd1/sde1, no localUSB2. All4
native DB mount queues completed. discovery-A-selected and discovery-A-full show
Reggaeton full284, BPM/key and waveform previews on discovered source without
client diagnostics. Other SDK sources/playback/reliability still need verification.

Implemented LINKFAKE_LOCAL_ONLY, on by default in SDK wrapper: sendto drops
nonfake/nonloopback IPv4 packets with exact Link signature; recvfrom/recvmsg
ignore physical-source Link packets (EAGAIN), preserve loopback and other data.
recvmsg gathers signature across iovecs. Filter is signature-scoped, not general
Internet blocking. Coverage of all possible native physical paths is not proven.
Log confirms blocked physical Link transmit while source promotion still works.

Built research passive AF_PACKET counter link-packet-count.c; ran /tmp/
link-packet-count eth0 5: frames9 udp1 link_signature0. Positive control lo3:
frames60 udp60 link_signature60 (intentional nonzero result when packets found).
No packet content captured/logged. This supports isolation over observed5seconds,
not exhaustive absence proof. No tcpdump available in guest. Remaining: fifth
stick, startup/hotplug/removal and kernel asset/hardware packaging, actual SDK
playback, all-source final acceptance. No hardware writes, goal incomplete.

### Fifth physical hub stick browses through SDK (PID431178)

Created new emulator fixture C:/Users/icast/xdj-az-emu/build/hublib5.img (1.5GiB)
from existing usblib.img only after checking target absent. QMP blockdev koi-hub5,
device koi-usb5, xhci.0 hub port1.5. Physical guest device sdf1, database present.
Added /media/usb/sdf1 to emulator-only reserved-mounts before plugging it so
stock re-notifier cannot borrow localUSB2. Discovery CLI found5 records.
Restarted EP147 (no mod rebuild/hash change), startup auto-discovery records:
A sdb1 player3 slot3; B sdc1 player3 slot7; C sdd1 player4 slot3;
D sde1 player4 slot7; fifth sdf1 player3 slot8 (private requeste4).

PID431178 SDK14 hooks, extra clienttraceOFF, local-onlyON. All five separate DB
fds open; fifth announcement acknowledged and mount queued. sdk-five-sources.png
shows5 hub sources plus localUSB1, no localUSB2. Native UI sorts fifth before
player4 sources; fifth row is now y770 full coords, old C/D rows shift down.
Slot8 has no stock icon/caption but HUB sdf1 row/library tile works.
fifth-selected.png -> fifth-playlist-selected/full initially empty tracks.
Do not erase this failure. Selecting Track yields fifth-all-tracks.png with2547
and BPM/key/waveform previews. Return to Playlist produces populated sidepreview;
one tap selected gray Reggaeton enters full284 in fifth-playlist-retry-full.png.
Thus fifth full-library browsing is verified through SDK, with repeated-entry
reliability issue outstanding. Five images are clones; no distinct-content proof
beyond independent source mapping and separate file descriptors yet.

Next SDK playback from verified existing Algo mas on fifth, startup readiness/
request lifecycle fix, hotplug/removal, kernel support asset and community test
package. No hardware writes; do not mark complete. QMP additions vanish at full
QEMU restart unless persisted in boot config; EP restarts preserve them.

### Fifth source SDK playback progression verified; hardware boot dependency

PID431178, SDK14 hooks, discovery/local-only enabled, no extra client tracing.
Verified /media/usb/sdf1/Contents/#TocoParaVos/#Tocoparavos/#TocoParaVos - Algo
mas.m4a exists, 7572054bytes. fifth-algo-selection.png selects this DB track35
on fifth source. fifth-algo-loaded.png: scrolling and overview waveforms,
03:22.647, sourcebadge player3 with blank stock slot8 caption. AfterPLAY,
fifth-algo-playing.png shows03:18.159 and moving waveform. SDK5th load and
playback progression now proven; no captured audio yet. Images are clones,
so retain routing/independent-FD evidence, not distinct-library-content claims.

Hardware packaging dependency verified from existing repo: src/azhub/azhub.ko
exists183848bytes, azhub.c verifies exact4.4.194 hub_probe signature and patches
maxdepth1->6; already tested by user on AZ in earlier sessions, not this turn.
Emulator6.6 needs no such patch. Community mod currently does NOT load this asset.
On AZ, startup discovery alone is insufficient: stock kernel can reject hub
before community init; mod must first enable hub support and then handle delayed
stick discovery/replug. Do not package startup-only discovery as full standalone
solution. Need module asset validation/load and delayed/dynamic discovery at least.

Removal research: native MusicLibraryServer onMediaUnmounted1373928 takes
(this,LocalMediaID*,finishListener), posts task safely to own thread. Info-server
unmount callback still unidentified; primary PersistentStorageServer vtable ends
at+0xa8, so do not guess music's+0xc0 on it. Current hub poll only checks mount
dir existence, which survives unplug; lifecycle must inspect actual mounts/sysfs.
Next implement delayed source creation/registration (including no sticks atinit),
then removal/replug, module startup and reviewable hardware package. Stillactive.

### Delayed and append-only discovery implemented and fixture-tested

Refactored config parsing into linkfake_load_config; nfakes is atomic and a fully
initialized batch is published once. SDK can stay loaded with0 initial sticks;
worker scans every2seconds instead of returning -2 on empty. Added development
AZHUB_DISCOVERY_MOUNTS override to test with a mount-table fixture; default stays
/proc/mounts. Manual config still overrides discovery.

First build: PID438719 loaded with empty generated config and no hub SOURCE rows
(delayed-no-sources.png). Replaced empty fixture with actual /proc/mounts snapshot:
samePID published5 sources, registered10 servers, opened5 separate DBs and showed
all5 (delayed-five-sources.png). No EP restart between empty and populated state.

Extended append-only scan: new mounts receive the next unused virtual source;
existing dir mappings stay unchanged even if a sorted scan inserts earlier rows.
Dispenser tracks per-owner registered count and appends only new native pairs.
Fixed stale bind risk: compare source/alias device+inode, unmount mismatched old
bind before replacing it; aliases left by previous EP run must not route audio
to another stick when discovery order changes. Both variants build -Wall clean.

Current PID442760, SDK14hooks, local-only, no extra trace. Initial fixture exposed
sdb1 andsdf1 only: registered4 servers. Revealed all5 mounts: samePID published5
(previous2), registered10(new6). All5 DB fds open. Existing sdb/sdf identities
preserved. Mountinfo proves correct actual bind sources:
player03/usb=sdb1, player03/usb2=sdf1, player04/usb=sdc1,
player04/usb2=sdd1, player03/usb3=sde1. appended-five-sources.png verifies rows.
These differ from all-at-once startup order intentionally; preserve insertion
assignments while running. No browsing regression run for new append path yet.

Cleanup: removed AZHUB_DISCOVERY_MOUNTS from persistent preload env. Running
worker still has it, so replaced test fixture with symlink /proc/mounts; it now
sees actual live mount table. Nextrestart uses default directly. Physical sticks
were already mounted; this proves discovery/event publication, not physical
unplug/reinsert. Removal/reuse of an old mount path is STILL NOT handled; compare
actual mounted/sysfs identity next. Kernel asset loading remains unimplemented.

### Kernel asset loader integrated and policy tested (not deployed)

Added kernel-asset.h, called from SDK init before discovery and any hook claims.
Known emulator release6.6.138-dirty skips compatibility module. Exact AZ4.4.194
reuses /sys/module/azhub if already present; otherwise obtains core assets_dir,
opens assets/azhub.ko, calls finit_module, closes descriptor, logs refusal/result.
Unknown kernel returns-21; missing assets and kernel refusal fail init before
hooks. Module's own verified four-instruction signature remains second gate.
Verified existing module .modinfo: vermagic4.4.194 SMP preempt mod_unload aarch64,
licenseGPL, no dependencies. Repo azhub.ko remains183848bytes, no rebuild needed.

Six mocked-syscall policy cases pass in test-kernel-asset.c: emulator skip,
unknown release refusal, already loaded reuse, absent assets refusal, kernel
refusal with close, successful load with close. Both preload/SDK variants build.
These tests do NOT load any kernel module or establish real-AZ SDK loading.
Newest binary is local only; current emulator PID442760 remains previous append
SDK build. Need deploy new SDK hash and verify emulator skip, then package asset.

Also still outstanding for standalone AZ: mod currently relies on the native
ProDJLinkMode setting being enabled (emulator config is on). Must implement/test
internal cached mode gating while retaining user-facing4deck mode, or explicitly
resolve that dependency before declaring standalone ready. Four cached readers
and data byte3f86a08 documented earlier; no new mode change made this turn.
Removal/reinsert safety and intermittent initial browser requests remain open.
No hardware writes or installed hardware artifact; objective active.

### Internal activation with user Link mode OFF and deck3 load verified

Added community_enable_internal_link, invoked by discovery worker every2seconds:
sets exact-firmware writable cache byte3f86a08 to1 when0, without changing the
persisted user setting or calling changeProDjLinkMode. Physical packet filter
remains on. Verified decomp writer actualfunction13fa7a8 (earlier address13fa7e8
lies inside it): cache assignment + timer behavior. No code patch/hook added.

Controlled baseline: stopped EP, backed SETTINGS.DAT up as
SETTINGS.DAT.before-hub-fourdeck-test, changed only value of exact binary record
ProDjLinkMode NUL010501<value>000000 from1 to0. Old SDK PID447672 retriesannouncements,
no hub source rows (linkoff-baseline.png). Staged new build (includes kernel-asset
loader), approved throughnormalUI. Rapid deliberate test restarts tripped core
community quarantine; used its actual Resume control (community-resume-control)
and restarted once. This was not a mod crash. Avoid sub90second manual restart
sequences; normalReload exemption is preferable when modernloader exists.

Current PID452316 SDK14hooks loaded, notsuspended. Internal cache enabled log;
all5 mount taskscomplete; fourdeck-internal-sources.png all5 rows. Host binary
read of SETTINGS.DAT confirms ProDjLinkMode still0. Fourdeck-selected.png shows
actual fourdecklayout after tapping4DECK, with lanes3/1/2/4. Then selected actual
file Algo mas from fifth player3slot8 and loaded ondeck3: fourdeck-fifth-loaded-
deck3.png shows Algo mas,03:22, BPM120.2, scrolling+overviewwaveforms ondeck3.
This is functional deck3load evidence, not merely fourdeckbutton appearance.
No deck3play/audio capture yet. /tmp/link-packet-count eth03 reportedframes1,
udp1,link_signature0 with internalactivation. Kernelhelper's emulator skip also
executed (initloaded successfully on6.6.138-dirty); no realkernelmoduleloaded.

User-facing mode dependency resolved in tested emulator configuration. Remaining:
removal/reinsert safety, initial browser reliability, distinct-content/final
acceptance and packaging with kernelasset. RealAZ untouched. ProDjLinkMode
should remainOFF now; backup preserves priorsetting ifrollbackneeded.


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

Package validation: archive passes the actual list_pkg function from deck/lib/az-community
under the emulator guest shell (root manifest/ELF, paths, duplicate/case/size checks).
ZIP integrity/ELF64 AArch64 and exact module vermagic checked by package.py.
SDK binary SHA2569945dd9836c730ab2779c424eb97c98b888020f8445da43c1fc346c52a8d326f
matches the running staged mod. Eight topology and six mocked kernel-policy cases
pass. Package-list success is not an end-to-end installer or hardware test.
Post-reinsert all-tracks initially blank0000; Playlist->Track retry renders2547
(physical-reinsert-alltracks[-later/-retry].png), so empty-view bug affects both
playlist and all-tracks requests, independent of actual missing sparse audio.

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

Post-reuse restored sdc1/player4 slot8 browser verified: Reggaeton full284
with metadata/waveforms (reuse-postrestore-playlist-full.png). Side preview
was initially empty (reuse-postrestore-playlist-selected.png); full view worked
on the next deliberate tap. Empty-view reliability remains unresolved.
Current development archive SHA256c182a27a12d6253a571cb051a0f4c145a80f8b00458bb867772dc1eb9af31c66.

SDK diagnostics expanded behind LINKFAKE_TRACE_CANCEL plus TRACE_CLIENT: adds
1142460 list cancel, e61ff0 cancel scheduling, 1fd2d08 request-agent destructor
(19 hooks total; default still14). First19hook PID476476 B alltracks2547/full
Reggaeton284 passed, so not a reproduction of empty UI. Optional diagnostics
remain source-only; dist ZIP is still prior tested reuse build.
Ghidra confirms 1fd2d08 cancels pending request if adaptor+18, reqID+30 nonzero
and status+34==0. Virtual deleting destructor1fd3920 duplicates this code and
is not separately hooked. 1fd34e8 request reply accepts only matching64-bit
reqID+28 and32-bit txn+30; wait state+38 must3(current) or4(lower); otherwise
warns wrong reply. 1fd01b0 marshals TrackList results to UI ->1fc7958 ->
1fcf800 listener dispatch. No suppression/retention patch made.
Code review: send wrapper used patched stack buffer after its lexical scope;
now calls real_send within that scope (as write already did). Inspected old
AArch64 disassembly: it did keep stack allocated through BLR, so this source
lifetime fix is not evidence for the observed empty-view cause.
SDK forced packet hex logging, coupled to routing fd tracking. Removed forced
TCP_LOG; fake TCP connections now tracked regardless of logging, dump always
records setup state but emits hex only when TCP_LOG explicitly set. Preparing
quiet19hook runtime to reproduce without packet-dump timing/volume overhead.

Quiet19hook PID480573 reproduced blank Playlist root (quiet-trace-B-selected.png).
Request owner destruction at31157.346 cancels Playlist ticket vt2b7d510 txn17,
then header/footer arrive31157.355/.412 and all observers reject(result1).
More decisive single category tap: from populated Track2547, tap Playlist once;
quiet-trace-playlist-single-tap.png stays blank. Ticket371 header accepted at
31224.817, owner destruction31224.818 cancels it31224.819, footer rejected31224.837.
Caller chain is200f2d4(base update-event destructor) <-1ffe3bc <-201fd7c
(ListUpdateHandler replacing current operation) <-2021420 <-2057cf8 <-2058740.
Ghidra:2057c20 DeviceSettingUpdateEventFacade calls20213f0, constructs event
via20193a0/201e478: type11, vt3165bf0. vt+48 ->201e4e0 is WaveformRenewEvent,
confirmed source-path string. It updates existing waveform images/style and
does not fetch the missing list. This repaint replaces/destroys pending list
operation, cancelling its request; results are discarded afterward.
Base current event owns request-agent shared_ptr at+28/+30 and BrowserData+58.
MusicLibraryRequestAgent vt3161140: request media+28, ticket+30, status+34.
Proposed narrowly scoped guard at201f5d8: only incoming vt3165bf0, a known-present
hub media, that request-agent type with pending ticket/status0, and empty main/
lower row vectors at BrowserData+248/+250. Leave caller owning incoming event
so it deletes normally; preserve current operation. Optional BROWSER_GUARD=1
is staged for emulator testing, not yet accepted/default or added to dist.

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

Deferral implementation replaces empty-row suppression (not yet runtime accepted):
3 optional guard hooks201f5d8 apply-event,1fc9ae0 generic UI reply-dispatch,200f0d0
base update-event destructor. Normal+guard17 hooks; with diagnostic5 ->22.
A pending hub MusicLibraryRequestAgent retains the newest matching WaveformRenew
event in a16-entry native-UI-thread TLS queue; sets caller pointer0 to take
ownership. Repeated renewals delete prior retained event through native vt+8.
After genericUI reply returns, completed request flushes renewal through native
apply-event. Navigation drops queued event before replacing current operation.
Base event destructor drops records before releasing current agent/context.
Queue entries carry handler/current/agent identity, preventing stale replay after
context change. Queue-full fallback lets caller delete repaint and keeps request.
Native201f5d8 really uses onlyx0/x1 (x2 overwritten immediately; decompiler
spuriousparam3/4 come from compiler CAS variables), so replay passes zeros for
extra registers without keeping caller stack pointers.
Optimized15-case ownership/lifecycle suite and ASan+UBSan pass: retain/coalesce/
complete/navigation/destruction/local/other-media/completed/wrong-agent/absent/
original-declines-consumption/stale-current/stale-agent/full-queue. AArch64 build
clean. New SDK staged; awaiting normal Allow UI. Dist still prior c182a27a.

Deferral runtime positive PID495601,22 hooks: B playlist root, selected
Reggaeton preview, full284, Track2547 all populated (defer-B-playlist-root,
-selected, -full, -alltracks.png). At32219.947/.151 refresh deferred and
32220.154 applied afterreply; same at32222.178 ->32222.251. This proves actual
retain/replay branch, not just no cancellation occurring. No crash during sweep.
Next normal build has guard enabled by SDK itself, version0.1.1; packetlogging
and TRACE_CLIENT/TRACE_CANCEL off. Staged, awaiting normal Allow; expected17hooks.
Do not claim full five-source acceptance or update ZIP until trace-off checks.

Normal0.1.1 acceptance PID499084: SDK17 hooks,TRACE_CLIENT/TRACE_CANCEL and
TCP_LOG off, ProDjLinkMode persisted0. All5 separate DB FDs open. A/B/C/D/E
Reggaeton preview and full284 passed on first entry (normal-defer-[A-E]-selected/
-full.png). E alltracks2547 (normal-defer-E-alltracks.png). Actual deferral
branches replay for A,D,E in trace-off build. Fifth Algo mas selected, loaded
on deck3 then1 (normal-defer-E-algo-selection, -deck3-loaded, -two-decks-loaded);
4DECK view shows bothwaveforms, PLAY1 advances03:22->03:05 (-playing.png), then
paused. Short passive capture eth0:14frames10UDP0Link; loopback620frames68Link
positive control. Limited capture, not exhaustive proof of all network routes.
0.1.1 development package built; ZIP integrity/ELF64 AArch64/modulevermagic
checks and actual deck list_pkg validator pass. ZIP SHA256
b0e5f2f685c71231be8fd55287cbc5699b10ab9051e2cae9e06478d085767c73.
SDK binary SHA256b6f7e8123f9fa605619261288384beb78d113071ead9ea45fe7968ef60f8192f.
Real AZ untouched. Existing0.1.0 ZIP retained as historical rollback artifact.

FINAL CHECK FAILED; GOAL REMAINS ACTIVE. 0.1.1 five-source browsing and playback
sweep passed, but must not recommend package for AZ yet. QMP unplug koi-usb2
(sdc1) removed own DBfd and03/usb2 alias; stale04/usb3 bind from a previous
6-pair process remained. Linux reinsert assignedsdg1; vendor98-local.rules
only matches sd[a-f]/sd[a-f][1-8], so no device-mount@sdg1 job was ever started.
Current device-mount.sh itself accepts sd[a-h] and handles nestedbus2-1.2
without localUSB1/2 announcement. Need standalone mod fallback for hub devices
that vendor udev skips; do not patch real AZ rootfs or rely on manual mounting.
Reserved /media/usb/sdg1 for emulator renotifier, unmounted stale04/usb3 bind,
manually ran existing /bin/bash /home/root/scripts/device-mount.sh add sdg1.
That restored primary mount. It was a diagnostic intervention, not acceptance.

Unexpected SIGSEGV at10:12:58 UTC (before manual mount): HubLib4 tid499725 on
PID499084. Native12553b0 ldrx25,[x21] withx21=0, fault0. Backtrace12fb950,
1303c44,136016c,13698a4,core+6c600,e6a558. Core crash report saved in
research/hublib4-011-crash.log (append-only includes three OLD MsgDispenser
crashes; relevant last section beginsline557 timestamp1792231978). Native
12552e0 calls virtualprovider+28 target1829990, returns null handle, derefs
without checking. 1829990 searches its media/DB vector+0xa0..a8 for requested
LocalMediaID ushort (entry+98), returnsNULL when absent. This differs from
e5e7d8 media registry lookup that always returns an object. Do not conflate them.
12552e0 param1+8 media key, +10 provider; crashx20(this)=ffff7c931020, x23
requesteditem=0x820, x25 virtualtarget1829990. Further caller12fae60 huge list
entry builder ->1303bb0 menu request ->135ff08 ListRequestHandler. Need inspect
actual requestedLocalMediaID and provider ownership/registration; possible
shared backend cache mismatch is a hypothesis, not proven. Do not paper over
by blindly suppressing the native null dereference. UI renewal guard is not
on crash stack (crash is HubLib4, not nativeUI thread). Two decks1/3 had loaded
fifth Algo mas; PLAY1 thenpaused. Timing followed unplugging a different stick,
so removal vs delayed playback/list refresh causality is unresolved.

Systemd auto-restarted EP as507534 (coreloaded0.1.1,17hooks, not quarantined).
When sdg1 was manually mounted, native5 DBfds opened; new process had first4
sources then appendedsdg1. Effective current assignments differ from sorted
config: sdb1=3slot3,sdd1=3slot7,sde1=4slot3,sdf1=4slot7,sdg1=3slot8. Do not
assume old Brow590 is sdg1 now. QMP drivekoi-hub2 remains port1.2 idkoi-usb2.
All5 physically attached. Currentprocess no loaded tracks after auto-restart.
Discovery path stillsymlink/proc/mounts; TRACE_CLIENT/CANCEL/TCP_LOG off, browser
deferral on bydefaultSDK. Archive README now marks investigation/notAZ-ready.

DB cache cause confirmed with diagnostic18hook PID514395: HubLib4 andHubLib0
both query TaggedTrackCacheService providerffff67ee8200, ID0102, vectorcount1
while all5 independently opened SQL DBs exist. RTTIvt2c6d0f8 (typeinfo2c6d048
name at2c6d018) -> tagged_trk_cache::TaggedTrackCacheService.
Withdrew onlysdg1 via discoveryfixture (no physical unplug, no tracks loaded).
At33792.562 ownsource3slot7 absent andunmountqueued;33794.691 aliasreleased.
At33802.840 HubLib0(notremoved) requests item2114, key0102 wanted0102, SAME
provider count0 found0, thennativecrash. This causally reproduces sharedcache
unmount wiping other sources' entry; dual-deck playback is NOT required.
EP autorestarted517548 andcore quarantinedmod (state5: Suspended itcrasheddeck).
Do not keep restarting/bypasscore; normalResumeUI afterfix is available.
Restored discoveryfixture symlink/proc/mounts. No rootfs/hardwarechanges.
LiveREAD of514395 /proc/PID/mem beforewithdraw confirmedvt2c6d0f8 and1entry
(provider+a0/a8), usingBusyBoxdd bs1 skipdecimal64 count16. Aftercrash oldaddress
innewPIDreadzeros is NOT valid sameprocess evidence; count0probe beforefault IS.
Servicevirtualmethods vt: index2=1829d28(register/open),3=1828968(remove),
4=1829610(update),5=1829990(lookup, x8sret),6=18282d0(query),7=18285b0(x8sret),
8=1827ff0,9=1827d20. Ctor1828db8; serviceallocated0xc0, RWLock+8, vector
shared_ptr entries+a0/a8/b0, backendkey+b8. Nativefactory7d6598 usesit.
Need per-stick TaggedTrackCacheService scoped alongside own music/info servers;
currently only per-stick SQLfactorycache is private, taggedcache remainsglobal.
Do not assume remote-slot ID mismatch: diagnosticquerykey/wantedboth0102,
entry0102found beforewithdraw. 186af10 is globalLocalID normalizer, not a simple
getter:186ab90 selects optionalnormalizer bylow5bit sourcekind, otherwise
returnsinputushort. SDKsource db-cache-probe.h observes normalnon-sret12552e0
caller safely, never directlyhooksx8lookup1829990. Dist remainsmarkednotAZready.

Private tagged-cache draft: structfake nowowns process-lifetime tag_cache.
Native TaggedTrackCacheService ctor1828db8 initializesvt2c6d0f8, RWLock+8 via
2489f50, emptyvector+a0/a8/b0, limit100+b8, THEN globallyregistersservice.
Private instances reproduce ONLYprefix, avoidingglobalregistration. Eight
native virtualmethods are ABI-transparentreceiver-adapted perHubLib/HubInfo
thread or pairedhub_info_libraryTLS; stockcallerskeeporiginalservice. Redirects
open/close/update/lookup/otherquerymethodsalike, not onlynullquery suppression.
Entryassembly preservesx1..x8 andstackargs, changesonlyx0receiver, tailbranches
tooriginaltrampoline. Hiddenx8aggregate returns are thereforeuntouched.
NativeAArch64staticABItest test-tag-cache-abi.c compiledvendorGCC andran in
emulator: all8adapters passscalar/x8aggregate/10arguments(includingstack).
Featureopt-in PRIVATE_TAG_CACHE=1; DBprobereports effectiveprivateprovider.
Cleanbuild; stagedforruntime. Currentcorequarantinefromreproducedcrash requires
normalResumeUI thennewhashAllow; nevereditapproval/quarantinefilesbyhand.

Private-tag runtime NOT accepted yet. NormalTryAgainUI cleared per-mod
quarantine; rapidmanualtestrestarts thenhitglobalstartupquarantine, recovered
throughCommunityResumeUI. UIReloadnow requiresconfirmation(fullx730y590) and
writes /run/az-community.req apply, but oldemulatorwatcherdoesnotconsumeit.
AfterUIReloadrequested, manualsystemctlrestart honorscoreintent exemption.
LatestPID529941 initfailed -4: coreclaim_hook rejects first3 taggedmethods
1829d28/1828968/1829610 due16-byte source-kindcheck+conditionalbranch+RET.
Remaining5 methods accepted. Newsourcepatch uses body entry+0x10 BEFORE
prologue (1829d38,1828978,1829620), so receiver/args unaffected exceptunused
scratchw2 orw3. x8 adapters unchanged. Alsofixed SDKinit to notparse/publish
sources orlaunchinjector when anyprobehookclaimfails (previouscodecould
return-4 only AFTERstartingworkerwithpartialhooks). Mustrestartfailedprocess
withcorrectedbuildbeforetesting; do notbrowsepartial-initstate.
Fixedcontextownershipcheck in h_mlookup andtagreceiver: onlynonnull paired
TLS maymatchsrv_music. Previously TLS0==unconstructedserver0 couldincorrectly
selectsource0 forstockthreads duringstartup. ExactHubLib/Info namesstillmatch.

Private tagged-cache removal/restore PASS on PID533159 (26hooks: normal17 +
8tagcache adapters +1DB diagnostic). NormalResume/Allow/Reload UI used; no
approval/quarantine state files edited. Core accepts correctedbodytargets
1829d38,1828978,1829620. Newcacheinstancescreatedon ORIGINAL registermethod0
caller1376f44, actualHubLib0..4 threads, not lazily fromdiagnosticquery.
Source0 ffffa2aa9480;1 ffffa0e80480;2 ffff9e262480;3 ffff9d88b480;4 ffff9bb88480;
originalglobal ffff874e8200 preserved. Eachprivatecache has1ID0102 entry.
Source1(sdg1,player3slot7) hiddenfromdiscoveryfixture at34909.784; itsDBfd
closedandaliasreleased. Live/proc/PID/mem cachevectorcounts afterwithdraw:
{0:1,1:0,2:1,3:1,4:1}. SAMEPID. SourcesAandE bothbrowseall2547trackswhileB
absent (private-tag-A/E-while-B-absent.png); DBprobesshow eachusingitsOWNprovider
withfound1 ID0102. Stayedalive >1minute; thisexactsequence previouslycrashed
HubLib0 withsharedcount0. Restoring real/proc/mounts symlinkreannouncedB,
reopenedall5DBfds, livecachecountsall1 withsamePID. EvidenceJSON:
research/private-tag-removal-state.json. No nullquerysuppression applied.

Privatecachefeature remainsopt-in LINKFAKE_PRIVATE_TAG_CACHE=1 inemulatorenv;
SDKdefault notyetenabled. TRACE_DB_CACHE=1 on; TRACE_CLIENT/CANCEL/TCP_LOG off.
CurrentstagedSDKdiffersfromdist0.1.1 investigationZIP; DO NOTrepackandrecommend
untilnormaldefault25hook build +mountfallback +physicalhotplug/fivebrowse/play
acceptance. All5devicespresent, sdg1 remainsmanuallymounted, fixtureprocmounts
restored. No realAZchanges.

Draft assets/mount-hub.sh addedbutNOT integrated/packaged/runtime-tested (sh-n
passes). Validatesdevicebasename/block/sysfsnestedUSB, supportsstockvfat/exfat/
hfsplusoptions, explicitwritemodehardwareRO, neverannounceslocalUSBslots and
neverrecursivelydeletesmountdirectories. Need ownmountworker, offinjectorthread,
forvendorrule-skippeddevicesg+ (includingmulti-letter names); wait/avoidracing
stocka-fmounts. Mustalsotrackfallbackmounts andunmountmissingdevices AFTER
nativeDB/aliasrelease, sincevendorremoveudevrulesalsoskipg+. Do notjustmount
withoutcleanup or relyonmanuallycallingvendorhelper. Emulatorrenotifier must
skipnewhubmounts dynamically (currentlyreservedlist includessdb..sdg1), so
automountworker testcannotannounceg/h aslocalUSB2. AssetsremainonthemodUSB;
currentemulatorstagingwasmanualandmayneedreal MODS/community-mods/hub-libraries
folderonsda1 forcoreassets_dir resolution, withmatchingmanualowner.


### Hardware discovery failure / subnet reproduction (2026-10-17)

Read-only AZ check (before user's new requirement to ask before any AZ test):
PID10430, corefw ep147-9e9454e357389ece, kernel4.4.194, mod0.1.2 loaded25hooks,
azhub module present. Hub SanDisk sdb1 topology2-1.4 mounted and DB readable.
Announcements repeated without media-query acknowledgment; no HubLib/HubInfo.
WiFi mlan0 is192.168.1.115/24; discovery still hardcodes10.0.2.50/.51.

Native013f1508 receives keepalives. Getter013f59e0 uses x8 sret and returns
parsed interface1 record IP at *(state[1]+0x78)+8; 013f5960 returns MAC+2.
013df908 is x8 sret current interface config under native mutex. Own IP snapshot
bytes+8, mask bytes+0x19. 013f1610 xor,013f1614 tst withmask,013f1618 b.ne013f18e0
rejects mismatching networks before member registration. This is an explicit
subnet admission gate, independent of Link-mode cache.

Reproduction: rebooted local emulator and rebuilt its missing xhci-pci module
using scripts/build-emulator-xhci.py (emulator-only, never an AZ package asset).
Fixed truncated old core on emulator USB with intact published1.2.3 core.
Initial automatic late discovery10.0.2.50 on10.0.2.15/24 acknowledged at343.327.
Then manual fixture192.168.1.250 on same10.0.2.15/24 repeatedly retries without
acknowledgment, old25hook binary PID11486; subnet-mismatch-before.log records it.
Startup config already creates HubLib0, unlike zero-source hardware startup,
but neither gets a source row without membership acknowledgment.

Experimental internal-subnet.h scopes a zero comparison mask to recognized
synthetic IP AND exact synthetic MAC (020a2d0000player) only. No interface or
persistent settings are changed. Entry013f1600 displaced four arithmetic
instructions; adapter preserves live GPRs, all32 SIMD registers and NZCV,
restores original SP before trampoline, clears w20/w26 only on identity match.
These registers compose the mask at013f1608, then are overwritten downstream.
Core normal branch is preserved for nonmatches. Nine policy checks plus actual
ARM64 ABI tests (all SIMD, volatile GPRs, flags, private/ordinary masks) pass.
Runtime acceptance pending. No real AZ tests or changes in this follow-up;
user now explicitly requires checking first before ANY further AZ test.


#### Correction: first subnet hook experiment rejected; use address selection

013f1508's index1 getter records are interface/state data; the experimental
identity matcher never fires for synthetic keepalives. Thus that earlier gate
analysis does NOT prove this is the incoming-player admission function. The
one-hook experiment was removed completely (headers, tests and SDK install).
Do not ship or reinstate it on the basis of the earlier entry. The ordinary
cross-subnet failure reproduction still stands, independent of that attribution.

Current local fix is network-identity.h: getifaddrs with preferred native
outgoing keepalive address, then default-route interface, then another UP IPv4
interface. Contiguous subnet mask and >=/29 host space; excludes own address,
network/broadcast, loopback and multicast. Selects two highest usable peer
labels and freezes them for that process. No OS addresses assigned; all actual
transport remains loopback and physical Link filtering unchanged. Automatic
source publication waits for a suitable interface if none is ready at init.
Manual config keeps explicit addresses. Policy14cases passes host and native
ARM64 guest; guest currentprimary192.168.1.115 selects192.168.1.254/.253.

To reproduce hardware more closely, emulator eth0 now hasprimary192.168.1.115/24
and secondary10.0.2.15/24 (SSH forward stillworks), defaultgateway10.0.2.2.
Nativejournal confirms Linksystem192.168.1.115. Exact original0.1.2 baseline is
being run with autodiscovery10.0.2.50 to compare new dynamic selector afterward.
Emulator-only90-hub-emulator-test.conf clears ExecStartPost staging race and
runs hub-excluding usb-renotify; explicit community/enabled file retained.
All changes/tests in this follow-up are emulator/local; real AZ not contacted.


### 0.1.3 subnet fix acceptance / awaiting permission for AZ

{
  "version": "0.1.3",
  "emulator_pid": 35295,
  "native_interface": "192.168.1.115/24",
  "ssh_secondary_address": "10.0.2.15/24",
  "automatic_internal_labels": [
    "192.168.1.254",
    "192.168.1.253"
  ],
  "baseline_012_same_native_network": "repeated unacknowledged announcements",
  "binary_sha256": "89eddbd7fef4fa89b43ddcef6f6bbe487a893641b840cf589eacb100e6a9e54b",
  "archive_sha256": "b25a880169abb5bc1d870ea93019c8098333e8caa6057be0dfb6a5cade27f41c",
  "hook_count": 25,
  "starts_without_hub": true,
  "late_attach_same_pid_sources": 5,
  "separate_open_databases": 5,
  "playlist_rows_each": 284,
  "fifth_all_tracks": 2547,
  "fifth_present_audio": "Algo mas",
  "loaded_decks": [
    3,
    1
  ],
  "deck1_remaining_seconds": [
    200,
    170
  ],
  "paused_after_test": true,
  "persisted_link_mode": 0,
  "ethernet_link_signatures_10sec": 0,
  "loopback_link_signatures_10sec": 224,
  "network_policy_cases": 14,
  "package_deck_list_pkg": "pass",
  "az_tests_since_user_required_approval": 0,
  "az_013_installed": false,
  "limitations": [
    "configured IPv4 subnet /29 or shorter required by current auto selector",
    "reload after subnet changes",
    "unconfigured networking and multiple active subnets unverified",
    "clone sparse audio fixtures",
    "audio output not captured",
    "first C/D waveform previews delayed, populated on revisit",
    "playing-source removal unverified"
  ]
}

Current code uses network-identity.h only, no subnet-gate native hook.
Original0.1.2 fails on native192.168.1.115, new build passes0->5late sources,
all284-tracklists, fifth2547tracks and decks3/1 Algo masPLAY03:20->02:50.
Final package passes deck list_pkg and byte/hash matching against running ELF.
Do not run even read-only AZ tests until user approves. No AZ updates made.


### 0.1.4: per-source waveform preferences (emulator accepted)

Hardware0.1.3 browsing/playback worked, but its one global outgoing status template
alternated native decks. Read-only AZ sample found0xda color0/3 and0xdd position0/2
(research/az-013-waveform-template-sample.json), correlating with200ms fake status.
Native UsecaseDeviceSettingController checkAndNotify async body0xb3db58 compares
these values with DeviceSettingManager getters and calls setters+notify-all when
changed. Thus copying these bytes advertises actual changing source preferences.

DeviceSettingManager vtable0x2c0fbd0, ctor15e3f58; color setter15e2610, position
15e26e0, getters15e4da8/15e4e70, notify-all15dfb08, notify-one15e0390. Color array
+0x98 and position+0x770; member stride0x18, device kind within stride. Important
correction: file reader15e1710 handles DEVSETTING.DAT, NOT MYSETTING.DAT. Filename
is initialized from string0x294b370. 15e0d30 validates native file CRC using15e08b8.
Known export format140bytes: header length96 at0, PIONEER DJ at4, payload length32
at100, little-endian magic12345678 at104, preferences113..120, CRC16-CCITT init0
of104..135 stored little-endian32-bit at136. Zero means unspecified. Visual color
enums1/3/4; position1/2. Fixture devsetting-fixture.dat is a real emulator export.

15e21f0 serializes eight fields (WaveSelect,color,cue-list,key-style,position,
three extras), then IPLinkset_DevSetting secondary vtable+0x10 at10dc278 calls
10d3f18 to pack status payload. Packet payload starts+0x24. USB slot3 -> kind0 ->
packet0xd9; slot7 -> kind1 ->0x468; slot8 -> kind8 ->0x474; SDslot2 -> kind2 ->0xf9.
Do not confuse wire slot7 with DeviceKind8! b3bfb0 maps media-kind7->device-kind1,
8->8,3->0,2->2. Initial local candidate had this wrong; native runtime sampling
caught it before final package. Current binary has correct independent rows.

source-settings.h validates source file, publishes each slot's own preferences,
clears foreign serialized rows and inherits unspecified fields from populated
native USB1 row, else native defaults. It ignores empty native rows for fallback
updates. Read-only source refresh every2s. Does not freeze or suppress valid UI
waveform updates and adds no hooks. Final PID77151, five separate open DBs.
Temporary B fixture blue/center stayed independent of four RGB/bottom sticks;
200 adversarial alternating native transport templates over20s did not change
source settings in30 native samples. Restored B file byte-for-byte; all five
native source settings3,2 in30 samples and loaded deck3 changed to RGB. A Algo mas
playback advanced03:22->01:39 then paused. Fifth all-tracks2547 and Algo mas load
also pass, both RGB waveforms visible. Native sampler is read-only; template
stress writes ONLY emulator mod transport buffer, not native manager preferences.
See community-014-acceptance.json and waveform-014-*-settings.log/shots. Real AZ
unchanged;0.1.4 hardware validation requires user authorization.

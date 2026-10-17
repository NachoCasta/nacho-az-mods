# Hub USB Libraries: development test build

Version **0.1.4** is a development build for manual AZ testing after emulator
acceptance. It includes per-stick tagged caches and automatic mounting/cleanup
for hub devices beyond the stock sda..sdf rules. Version 0.1.3 was confirmed browsing and playing on the real AZ; this new
0.1.4 binary has only been tested in the emulator.

Version 0.1.4 fixes waveform color/position switching by publishing each
stick's own `PIONEER/DEVSETTING.DAT` preferences instead of copying settings
from alternating local-deck status messages. File length, magic and CRC are
checked. Missing/invalid or unspecified values fall back to populated native
USB1 preferences, or the native defaults before any are available. The file
is refreshed every two seconds without writing it. Source names now use the
filesystem label, including spaces and Unicode; unlabeled sources use the
physical hub port (for example USB 4).

Version0.1.3 fixes the emulator-specific automatic player addresses. With the
AZ on192.168.1.x, version0.1.2 still advertised10.0.2.x and never acknowledged
the source. Automatic discovery now selects internal peer labels from an UP IPv4
interface/netmask, preferring the native advertised address and default-route
interface. No IP is assigned to an interface; actual transport stays loopback.
The source addresses are selected once per process; reload after changing
network subnets. The selector currently requires a contiguous IPv4 subnet with a /29 or shorter prefix. Fully unconfigured networking and simultaneous interfaces on
different subnets remain unverified; no Internet or external Link player is needed.

Runs inside EP147 through Community Mods. Each discovered hub stick receives its
own music-library and track-info servers, separate from the AZ local USB servers.
Internal linked-player identities are used; physical Pro DJ Link packets are
filtered. The persisted Pro DJ Link mode may remain off, allowing four-deck use.

## Requirements

- Exact firmware tag `ep147-9e9454e357389ece` (the researched EP147 build).
- Community Mods core 1.2.3 or later.
- AZ kernel 4.4.194. The bundled GPL azhub.ko checks the kernel instructions
  before changing the hub-depth limit. Other hardware kernels are refused.
- USB hub and rekordbox-exported sticks containing PIONEER/rekordbox/exportLibrary.db.
  This build supports that database format; other library formats are unverified.

## Manual installation

Extract the package into `MODS/community-mods/hub-libraries/` on the mod USB,
so azmod.json and hub-libraries.so are directly inside that directory. Keep
the entire `assets/` folder with it on the USB, including `azhub.ko` and
`mount-hub.sh`; the loader reads these files from the stick.
Set `community-mods.hub-libraries = on` in `MODS/azmods.cfg` and ensure community
mods are enabled. Apply through the Mod Menu or boot with the mod USB. Allow the
new mod in Community Mods if prompted, then apply/restart through the normal UI.

After the mod loads, reconnect a hub that the stock kernel previously rejected.
Wait for discovery and serialized source announcements (initial startup takes
roughly 20 seconds, plus several seconds per source). Sources appear with their volume label (or physical-port fallback)
under internal linked-player identities 3 and 4. Select a source, its library tile, then
Playlist or Track. The AZ local USB1/2/3 servers are not used for these sources.

Use known-present audio files for load/playback testing. Our emulator images have
2547 DB tracks but only 17 audio files; Algo mas is present on all five fixtures.
A missing audio file is independent of whether its DB entry can be browsed.

## Evidence and remaining work

Version 0.1.4 emulator acceptance (PID77151): native UI shows volume labels for
five sticks; each native source receives its own color/position. One stick was
temporarily blue/center while four stayed RGB/bottom, then restored byte-for-byte;
the loaded deck accepted the legitimate RGB change. During20s of alternating
populated/empty transport templates,30 samples retained independent preferences.
All five restored sources stayed RGB/bottom in30 more samples. Known-present
Algo mas played on deck1, and the fifth source browsed2547 tracks and loaded
on deck3 with RGB scrolling/overview waveforms. Sanitizer checks passed.


Emulator: five hub sticks with separate database handles; playlist284 and
all-tracks2547 views; fifth-source Algo mas loads with both waveforms and an
advancing play counter; four-deck view and deck3 load with persisted Link mode
off. Controlled removal and actual emulated USB unplug/reinsert close/reopen the
removed source in the same EP147 process; playlist browsing works after reinsert.
All five images are clones, so distinct-content isolation is not yet verified.
Audio output has not been captured. User confirmed real AZ browsing/loading/playback
with version 0.1.3; version 0.1.4 hardware validation is pending.

The SDK build defers waveform-only refreshes until pending browser requests
finish, preventing the observed list cancellation race. Each hub source owns a
private tagged-track cache as well as its own SQL cache and server pair, so
withdrawing one source no longer deletes another source's cache entry. Normal
operation uses25 hooks; optional database tracing is off. The preceding0.1.2
binary passed five populated284-track playlist views, fifth-source2547-track
browsing, deck3 and deck1 loads, and deck1 PLAY03:20->03:07 on emulator PID544361.
The0.1.3 subnet regression on PID35295 starts with no hub, then adds five sticks
on native192.168.1.115/24. All five open separate DBs and show full284-track
playlists; fifth-source all-tracks2547 works. Initial C/D waveform previews were
delayed but populated on revisit. Ethernet10s has0 Link signatures; loopback224. Fifth-source verified-present
Algo mas loads on decks3/1; PLAY1 advances03:20->02:50 then is paused. No
additional native hooks were added for this fix (still25).

The mount worker runs separately from source keepalives. It mounts only nested
USB devices that the stock sda..sdf rules miss, using the vendor filesystem
policy (vfat/exfat/hfsplus). It releases a missing device's backing mount after
EP147's open files and native aliases clear. Emulator acceptance includes actual
sdg1 automatic mounting and cleanup, plus a library stick unplug/reinsert and
restored2547-track browsing in the same process. This is development acceptance,
not extended stress testing. Packet hex logging is off by default.

Known issues: slot8 has a blank native source icon/caption; capacity is a stub.
Assignments are retained while a source is present. After removal completes, a
dormant server pair can serve a new mount path. Five successive path replacements
passed with five open DBs in one emulator process (ten lifetime paths). A new
source arriving before old removal completes may allocate another pair, up to
eight pairs. Same-path replacement without an observed absence and removal of a
loaded/playing source remain unverified. Avoid using this build for a set.

## Disable

Disable Hub USB Libraries in Community Mods and apply/restart. A full AZ reboot
also restores the stock kernel instruction; the patch is not written to firmware.
Keep the mod USB available while enabled because its assets remain on that USB.
No real AZ was modified while preparing version 0.1.4.

## Build

From this directory run `bash build.sh` with the documented AArch64 toolchain,
then `python3 package.py`. Sources are in this repository under src/azhub;
research and exact firmware offsets are in research/GHIDRA-FINDINGS.md.

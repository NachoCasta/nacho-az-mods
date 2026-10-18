# Safe ejection research and proposal

Research date: 2026-10-17. Target: firmware 2.00, exact tag
`ep147-9e9454e357389ece`, Community Mods SDK/core 1.2.3.
This is a proposal, not an implemented feature or hardware acceptance.
Investigation used repository code, the preserved SDK header, and local EP147
analysis opened with Ghidra `-readOnly -noanalysis`. No AZ connection, button
press, installation, or device mutation was performed.

## Recommendation

Build one device-scoped eject coordinator, then expose it in two ways:

1. A Mod Menu action, **Safely remove USB**, opens a list of physical sticks.
   This supports removing one stick while the other hub sources remain usable.
2. Holding the native **USB STOP** button for the connector carrying a hub
   requests removal of every storage device below that connector. This is the
   natural whole-hub operation, once port mapping, hold/cancel behavior, and
   completion indicators have been validated.

The UI path can use existing SDK APIs. The button path has a concrete native
dispatch point, but must not be enabled until the stock hold/release behavior
and indicator ownership are understood. Implement the coordinator and UI first;
add the physical button as another caller of the same operation. Neither path
should silently stop playback or use forced/lazy unmount.

## What is established

### Existing mod teardown is disappearance cleanup

`src/azhub/linkfake/community.c:community_refresh_presence()` marks an absent
source `present=0`, clears pending announcements, and sets `unmount_pending=1`.
`hubservers.h:hub_mount_poll()` queues owned MusicLibraryServer and
PersistentStorageServer teardown, writes an alias-removal message to
`/proc/udev_nfs`, waits three seconds, and attempts ordinary alias unmount.
The server calls currently pass `NULL` completion listeners.

Those delays serialize a single-message mailbox and reduce races. They are not
proof that native operations completed, files were flushed, or the backing
filesystem was unmounted. The function releases `/media/playerNN/...` aliases;
it does not eject a still-connected physical stick.

`mount-worker.h:community_mount_worker()` scans every three seconds and runs
`assets/mount-hub.sh` for eligible, unmounted fallback devices. Removing a
filesystem without inhibiting this worker will remount it. Stock udev handles
`sda..sdf`; the fallback handles later names. An eject implementation must cover
both ownership paths rather than only modifying the fallback worker.

The helper normally mounts writable filesystems. Its optional read-only mode
is selected from the block device's `ro` attribute. A bind alias requested with
`bind,ro` does not establish that the underlying filesystem is read-only or that
pending writes do not exist. Ejection must account for the backing filesystem.

`hub_open_device()` scans only `/proc/self/fd`. That is useful for existing
cleanup, but cannot establish absence of other processes' opens, memory maps,
working directories, exports, or future queued requests. Kernel unmount success
and native lifecycle completion remain necessary.

### Native USB STOP has an identifiable route

Local EP147 control-flow observations, recorded as facts rather than copied
firmware source:

| Address | Observation |
|---|---|
| `0x21af510` | `hui::media::UsbStopButton` constructor; stores an interface ID with kind 2 in the low five bits and the supplied port in the upper three bits. |
| `0x21b0050` | Constructs two such controls with zero-based ports 0 and 1. |
| `0x31bbd68` | Button vtable; event handler is slot 2. |
| `0x21aeb50` | Event dispatch resolves `usecase::device::IStorageDeviceMountRequest`; nonzero event state calls its vslot `+0x18` with the stored interface ID, zero calls `+0x20`. |
| `0xd6e4c0` | Application setup constructs `UsecaseStorageDeviceController` and registers its subobject at `+0x18` as `IStorageDeviceMountRequest`. |
| `0xbc30d8` | Storage controller constructor; mount-request subobject vtable is `0x2a58570`. Its type metadata names `IDisconnectController` and a vector of `ILoadState` dependencies. |
| `0xbb4840` | Mount-request vslot `+0x18` thunk adjusts `this` by `-0x18` and enters `0xbb4530`. |
| `0xbb5218` | Mount-request vslot `+0x20` thunk adjusts `this` by `-0x18` and enters `0xbb4f08`, the async `cancelStorageDeviceDisconnect` wrapper. |
| `0xbb4530` | Asynchronous `requestStorageDeviceDisconnect(LocalMediaID const&)` entry; this is not itself proof of a physical-port eject API. |
| `0xbc9510` / `0xbc7ac0` | Request/cancel queued task bodies reduce the stored interface ID to native port 0/1 and call `0xbc89c8` / `0xbc7128`. |
| `0xbc89c8` / `0xbc7128` | Native request/cancel processing sets/clears a per-port state byte at controller `+0x700+port`, resolves mounted media, iterates deck load states, and starts/cancels native scheduled work. These are substantive operations, not a passive UI notification. |
| `0xbc5ed0` | Native per-deck query used by both paths; obtains a load-state handle from controller `+0x5f0`, checks a loaded track, resolves its record and copies identity data from record `+0x80/+0x88`. This is a candidate source-specific busy query, not a validated public ABI. |

This supports a semantic hook near USB STOP rather than guessing raw mixer
frame bits. It does **not** yet establish the event state's hold duration,
which layer owns the timer, whether release cancels pending work, when the LED
stops blinking, or whether native handling can turn off a shared port's power.
Preserve a complete press/release pair; forwarding the stock press while delaying
hub teardown could permit stock ejection to finish prematurely.
The native request path additionally follows dependent cloud/streaming media
and compares each deck's media identity. Its call chain shows why bypassing it
with a shell `umount` would omit substantial application coordination. The
request/cancel workers receive native port identities, not one synthetic hub
source; calling them directly cannot implement selective child ejection.

### Native server completion can replace arbitrary waits

The existing calls have actual typed listener arguments:

| Entry | Native interface |
|---|---|
| `0x1373928` | MusicLibraryServer `onMediaUnmounted(LocalMediaID const&, music_library::IInternalMountEventFinishListener*)` |
| `0x1975468` | PersistentStorageServer `onMediaUnmounted(LocalMediaID const&, track_info_repository::IInternalMountEventFinishListener*)` |

Their queued task bodies are `0x1375220` and `0x1978ba8`, respectively. Both
invoke the supplied listener's vslot `+0x18` with the media ID after their
teardown work. This is a concrete route for per-source completion acknowledgments.
It still requires an emulator check of listener ABI, lifetime, exact callback
meaning, and failure behavior before adding a C/C++ adapter. Keep listener
objects alive through late callbacks and distinguish transaction generations.
All owned servers currently use internal media ID `0x102`; correlate completion
by listener/source instance, never by that ID alone.

Native firmware also exposes an unmount preprocessing pipeline:
`IStorageDeviceUnmountPreprocessRegister`,
`IStorageDeviceUnmountPreprocessFinishReceiver`, and
`receiveStorageDeviceUnmountPreprocessingFinish(MediaID,int32_t)` at `0xbb83a8`.
Native callback entries include music-local completion `0xbb3900`, music-Link
completion `0xbb3c18`, info-Link completion `0xbb3f18`, and info-local completion
`0xbb4218`. These are async enqueue wrappers, not synchronous completion tests.
Firmware strings identify preprocessing in USB recording, cloud sessions, and
streaming services. Reusing native preprocessing may be preferable for stock
mount ownership, but synthetic sources must never be passed off as native USB1/2.

### The SDK already supports the proposed UI

The preserved `publishing/sdk-reference/include/azmod-mod.h` exposes:

- `register_row` with `AZ_ROW_ACTION` (registration during init, at most 16 rows).
- Custom surfaces, focus, touch/knob/BACK handling, and painting.
- `confirm`, `banner`, `post_ui`, and `thread_start`.
- `deck_info` and `on_deck_event` for four-deck load/play changes.

One action plus a custom list is preferable to registering a changing menu row
for each discovered stick. All callbacks should enqueue work; filesystem scans,
flushes, and unmounts belong on a worker, never a UI or input-bus thread.
Check `AZ_HAS` for every optional API. Confirm the preserved header against the
pinned SDK before implementation; do not upgrade the SDK as a side effect.

`az_deck_info` has `loaded`, `playing`, and `content_id`, but no source identity
or media path. Its fields are atomically read individually, not a transactionally
consistent snapshot. Track IDs and titles repeat across copied libraries, so
they cannot identify the stick being ejected. Native `ILoadState`/media identity
research and a load admission gate are required for source-specific busy checks.

The [official XDJ-AZ manual, page 51](https://downloads.support.alphatheta.com/manuals/all-in-one-dj-systems/XDJ-AZ/XDJ-AZ_DRI1936C_manual_EN.pdf)
describes holding USB STOP until the indicator completes its blinking sequence.
It also documents restricted continued playback when removed-device tracks are
cached. Thus a mod policy that initially refuses ejection of a loaded hub source
is deliberately conservative; it is not a description of stock requirements.
Cached continuation needs separate validation and should not be assumed for
synthetic Link sources.

## Device identity and scope

An eject target is a physical storage device, not a source label, player number,
media slot, `sdX` name, filesystem UUID, or selected playlist. Resolve the mount's
major:minor and sysfs partition parent to its disk and USB device ancestor.
Include every partition/LUN and mount that removal of that USB device affects,
including partitions without a rekordbox library. Such partitions are absent
from `discovery.h`'s source list but still matter for safe removal.

Store a live connection generation plus canonical sysfs ancestry and device
numbers. Serial numbers are supplementary, not guaranteed unique or present.
Revalidate immediately before every mutation. Device-node reuse and identical
same-port replacements must not inherit a stale transaction.

For the physical button, map the USB device's root-port ancestry to native
USB1/USB2. `azhub_nested_usb_path()` only recognizes dotted USB topology; it
does not map a physical connector. Do not infer the mapping from `sda`, discovery
sort order, player 3/4, or internal slot 8. USB 2/USB 3 companion buses, internal
hubs, nested hubs, and multi-LUN readers require explicit grouping. A validated
port map is a gate for the physical-button feature.

Selective eject affects only that device. Whole-hub eject affects all descendant
storage devices of the chosen physical branch; unrelated native USB and another
hub branch remain untouched. New devices appearing below a branch during a
whole-hub transaction must be quarantined or cause an explicit scope-change
failure. Never declare the hub safe while a newly mounted descendant remains.

## Proposed user flows

### Individual stick through the UI

1. Open USB Hub Support → Safely remove USB. List physical sticks with volume
   label, validated parent connector/hub path, and capacity when reliable. Mark
   duplicate labels distinctly. Group all partitions under one stick.
2. Select a stick. Confirm removal by label and identify all affected libraries.
   If a deck has a loaded track from it, show the deck number and refuse in v1.
   Paused, cued, stopped, and ended tracks count as loaded. Do not auto-unload.
3. After confirmation, show **Preparing USB…**. Disable duplicate requests for
   the same device while letting other sticks operate normally.
4. Only after all completion conditions succeed, show **Safe to remove [name]**.
   Keep it listed as ejected until physically unplugged. Its hub LED may remain
   on: power is not the safety signal, and the native connector indicator cannot
   represent individual children.
5. On failure show **Could not eject [name] — leave it connected** plus a useful
   reason and **Retry**. Never show a successful result after timeout or EBUSY.

### Whole hub through its native USB STOP button

Preserve the familiar hold gesture and connector meaning. For a connector with
no hub descendants, forward native behavior unchanged. For a connector with a
hub, start a branch-scoped transaction at the validated native hold threshold.
Check all descendants before withdrawing any source; a busy child blocks the
operation and identifies the affected deck/stick. Releasing before the threshold
must preserve native cancellation semantics.

Show **Preparing hub on USB 1…**, then **Safe to remove hub on USB 1** only when
every descendant is ready. Do not permit the stock port indicator to imply
success earlier than the coordinator. Until LED/native ownership is understood,
ship the UI path alone rather than claim the physical button works.

If one child fails after another unmounts, report partial progress and keep the
hub connected. Successfully ejected children remain inhibited; retry the rest.
Never remount an ejected child automatically to make a failed group operation
look atomic. The success condition is whole-branch safety, not rollback of all
filesystem side effects.

## Coordinator and completion contract

Use a single worker-owned transaction state machine:

`AVAILABLE → RESERVING → QUIESCING → FLUSHING → UNMOUNTING → EJECTED`

Failures go to `FAILED`, physical disappearance to `DISCONNECTED`. A source or
branch reservation is established **before** load/browse admission is checked.
Discovery, fallback mounting, alias creation, source announcements, identity
lookups, and new request dispatch must all consult the same reservation. In-flight
requests need cancellation or bounded draining with a known completion signal.
Do not turn `present` alone into a concurrency primitive: existing fields are
read from multiple threads and only some use atomic access.

The operation should proceed as follows:

1. Resolve and reserve device generations, affected sources, partitions, mounts,
   aliases, and native services. If observation is incomplete, fail closed.
2. Gate new loads/operations, then check all four decks and recording/export/
   settings writers against the reserved identities. Eliminate the check-then-load
   race; repeated sampling without an admission gate is insufficient.
3. Withdraw only affected source advertisements and release browser/transport
   requests. Call owned server unmount APIs with per-source completion adapters.
   Wait for both server acknowledgments and any dependent queue/cache/export
   teardown, preserving other sources' SQL/tagged caches and servers.
4. Coordinate native stock mount ownership and any native preprocessors. Release
   alias mounts through the serialized mailbox, check submission failures, and
   verify alias disappearance in mount information. A pathname's accessibility
   alone does not show whether its bind mount still exists.
5. Flush each backing filesystem, check errors, and ordinarily unmount every
   affected alias and filesystem in dependency order. Include bind aliases outside
   the mod's own known paths. A global `sync` or elapsed delay is not a completion
   test. Do not delete block devices, disable the hub, or cut port power as a
   substitute for closing files and unmounting.
6. Recheck scope/generation and mount state. For FUSE filesystems, validate the
   block-to-mount association and helper/daemon teardown explicitly; mount source
   strings and `st_dev` alone may not identify the underlying block partition.
   Report success only after successful ordinary unmount and no remaining
   affected mounts/exports. Hardware acceptance must also verify filesystem
   integrity and post-eject writes.
7. Retain inhibition until observed physical removal. No auto-rediscovery, fallback
   remount, alias creation, or synthetic advertisement while ejected. Mod restart
   behavior must be defined: an in-memory tombstone alone does not survive an
   EP147 restart while an ejected stick remains connected. Determine whether to
   preserve session state outside EP147 or explicitly require unplug before a
   reload; never silently promise inhibition across reloads.

Use immutable request snapshots, a coordinator mutex/message queue, and explicit
atomic publication for cross-thread state. Never hold that mutex across firmware
callbacks, flushes, helper execution, or UI calls. Token/generation matching must
discard late completions after unplug/reinsert. Mailbox writes for mount and
unmount must use the existing single serialization owner.

Cancellation is safe before teardown starts. After withdrawal begins, finish or
show a retained failed/quiesced state; BACK should dismiss the status screen
without cancelling an unmount in progress. A retry must revalidate identity and
completed steps, not blindly replay an old device path. Unexpected unplug is a
disappearance event, not successful safe ejection.

## Implementation stages and research gates

1. **Trace native behavior locally.** Follow `IStorageDeviceMountRequest` vslots
   `+0x18/+0x20`, native hold timer/LED lifecycle, disconnect controller and
   `ILoadState` media identity. Trace stock udev mount/disconnect ownership and
   preprocessing completion. Determine whether recording on a child is possible
   and how its writer is registered. No native slot may be borrowed.
2. **Build the device inventory and inhibition layer.** Include non-library
   partitions, block generations and FUSE mounts; synchronize mount helper spawn
   with reservation. Add focused host tests for grouping, replacement and races.
3. **Validate teardown completion in the emulator.** Supply both listener types,
   prove all target caches/handles release, and establish a source-specific load
   gate. A temporary prototype may reject eject whenever any deck is loaded if
   source attribution is unavailable, but this is too restrictive to advertise as
   selective safe eject.
4. **Add the Mod Menu flow.** Register one action/surface, confirmation, status and
   retry. Keep filesystem work off the UI thread. Validate exact candidate binary
   without deployment side effects.
5. **Add the physical button after independent validation.** Map connectors, hold
   semantics, native forwarding/cancellation and LED completion. Test whole-hub
   partial failure and scope changes before release.
6. **Optional later work:** safe continuation of fully cached loaded tracks, or an
   explicit re-enable action while the stick remains plugged in. Neither is
   needed for the initial conservative behavior.

## Meaningful validation

Host tests should exercise device/partition grouping, duplicate labels/UUIDs,
non-library partitions, USB companion paths, missing sysfs/mount reads, device
reuse, inhibition versus helper spawn, timeout/late callbacks, idempotent retries,
and group partial completion. These tests should assert externally relevant
invariants, not merely mirror the state-machine implementation.

Emulator tests should cover stock and fallback mounts; FAT/exFAT/HFS+ as fixtures
permit; active browsing and requests; all four decks loaded/paused/playing/cued;
two identical libraries; individual eject leaving other hub/native sources
usable; busy alias and unrelated process-held files; FUSE teardown; write/flush
failure; simultaneous unplug/reinsert; repeated requests; discovery and stock
udev remount attempts; more attached devices than the eight visible sources;
mod reload with an ejected connected device; and branch changes during eject.
Use disposable writable fixtures and verify no later write occurs after success.

Only after emulator acceptance, request separate user authorization for a named
hardware test. Use disposable media, verify physical USB1/USB2 mapping and hold
behavior, inspect actual mounts/completions, and check filesystem integrity on a
computer after removal. Cover selective stick removal and complete hub removal
alongside native USB playback. Normal continued playback from an unrelated source
must be checked independently from cached playback on the ejected source.

## Remaining uncertainties

- Exact native hold timer, cancellation boundary, indicator and power behavior.
- Tested native deck-source identity/read admission hook and media-to-device map.
- Which native preprocessors can be applied safely to synthetic Link sources.
- Completion listener ABI/lifetime under failures, and whether acknowledgment
  covers every dependent handle needed for unmount.
- Stock udev coordination and inhibition across EP147/core restarts.
- Physical connector mapping and FUSE-backed partition accounting.

These are bounded research tasks with concrete entry points. They prevent claiming
safe ejection today; they do not make a dedicated UI or a physical-button design
infeasible. No executable/package/version was changed by this investigation.

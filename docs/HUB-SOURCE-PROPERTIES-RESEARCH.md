# Native source properties

These findings apply only to supported EP147. Raw analysis remains ignored in
`local/diagnostics/source-presentation-research-v1/`.

Current result: configured metadata acquisition and initial media announcement
are implemented. The capacity candidate also passes previously unused nested
USB8 arrival, complete software-client menus/files and loaded eject/cache checks.
See CURRENT-STATE.md and its linked acceptance records. The research log below
preserves earlier open questions and failed approaches in chronological context;
those statements do not describe the current implementation status.

## Native metadata route

- The native storage controller receives a `mount_device::PropertyTable` with
  its local-library mount-finished notification (`0xbbaad8`).
- Property request ticket `0x1134b70` handles response `0x4902`, named
  `CMD_RET_PROP_TABLE`, and parses a `0x94`-byte data block.
- Rekordbox information handler `0x1357030` obtains that data through information
  server reader `0x12715b0` and a mount-point virtual method at offset `0x38`.
- SqliteMountPoint's table is `0x2baf7e8`; its method at `0x38` is `0x134f210`.
  NoDBMountPoint has a separate method (`0x134db40`), whose fallback semantics
  have not been adopted as a configured-name source.
- The SQLite-backed getter explicitly reads `deviceName` and
  `backGroundColorType` from its database reader. This identifies configured
  metadata independently from the filesystem label and historical playlist names.
  It does not yet prove that our private libraries invoke this getter.

## Value layout and ABI

The returned plain data block has a UTF-16LE name in its first32 units, with a
zero terminator, and a background-color byte at `0x7c`. Its network counterpart
places the name at `0x2c` and color at `0xa8`. The native getter receives the
mount point in x0 and the aggregate result buffer in x8, verified from its entry
and its native caller. The getter does not use the DEVSETTING waveform-color
field for this background color.

`source-properties.h` decodes the fixed native block, preserving valid name case,
accents and surrogate pairs. Invalid/blank/unterminated names are unavailable;
color validation is independent, accepting the nine native UI choices0-8.
It does not yet change discovery or media announcements. Its focused controls
pass under ASan/UBSan.

`LF_SOURCE_PROPERTIES_TRACE` explicitly enables a bounded getter observer. It
returns the original value unchanged and logs at most32 observations with the
mount path, decoded fields and any matching owned hub source. Default candidate
builds do not install it. Native observation, ownership confirmation and final
metadata publication/lifecycle handling remain pending. No physical test or
source-color/name completion is claimed.

## First observer trial

The explicit observer binary
`1fcfb15f12851817511030c108df4abdd000060b41929f46a437e9e089384631`
compiled and loaded through supported Community Mods Allow/Reload. The native
getter hook was accepted; normal Source operation continued. No getter observation
was logged after startup or a protected emulator native-USB2 detach/replug. Its
original EHCI port and backing node were restored; Linux renamed the device to
sdh1, so further controls must identify its physical route rather than assume sdb1.

This is a negative acquisition result, not decoded metadata acceptance. The
current private mount requests pass a null completion listener. Investigate the
mount completion callback and owned library access before deciding whether an
automatic getter observation can acquire the required data. The observer remains
explicit and read-only; it does not populate production source names/colors.

## Owned information-server access

Native mount-task body `0x1376c98` installs the newly created mount point into
its source's command modules. The library at `server + 0x1d0` holds those modules;
its field `0x10` is the information server, whose first word retains the mount
point. Read-only inspection of the five current private servers confirmed five
distinct SqliteMountPoint objects with exact matching hub mount roots and getter
`0x134f210`. This is not a native USB1/2 library lookup.

The second observer's task-wrapper hook at `0x1377400` was declined by the
installer. Initialization failed closed with `-93`; no metadata acceptance was
reached. The current observer instead hooks the full task body at `0x1376c98`
and invokes the getter after it returns, on the owning library worker before
the next queued task.
It first matches the task's server against an owned present hub source, verifies
the exact mount-point table and root, then observes the returned value. It still
does not publish names or colors. Runtime observation remains pending.

The full-body observer now passes in the emulator. Its exact mapped binary is
`ddb8813e23cd3be50b4fbd1e392317fa209191e3bbeb8a6b35743110898c4ed8`.
All five private roots return configured name `Nacho Chips`, preserving its case
independently of their HUB_LIB volume labels. Background colors are6 (Aqua) for
USB1/2/3/5 and0 (Default) for USB4, consistent with the earlier UI restoration.
See [scoped acquisition evidence](acceptance/usb-hub-support-source-properties-acquisition-emulator.json).
This proves acquisition, not presentation. A subsequent production integration
now publishes snapshots guarded by source mount generation, exact root and route.
The media encoder prefers the configured name, retains volume/USB N fallbacks,
and copies native background color into response byte `0xa8`. Refresh checks the
representative transport's readiness for every sibling source; each source tracks
its own revision. It does not withdraw sources to update their presentation.
The exact candidate `d5202cef4ffd476d16e31b7977dd95e81c3950442a8f68e0e300b0913470c220`
now loads through supported UI activation; its mapped module hash was verified.
All five source acquisitions succeed, but a settled Source screenshot still
shows volume names and default blue. Sending an unsolicited revised media
response is therefore insufficient in this tested path. This is a failed
presentation trial, not acceptance; see
[first runtime result](acceptance/usb-hub-support-source-metadata-first-runtime.json).
Portable checks pass separately; see
[host evidence](acceptance/usb-hub-support-source-presentation-host-checks.json).

Further static tracing separates `LinkDeviceNameRequest` from the storage-info
provider: receive callback `0x13e8b80` queues task body `0x13e8060`, which converts
the returned UTF-16 name through `0x13e66a8`, assigns its retained Link-device name
and completes request listeners through `0x13e7798`. That path alone does not
establish a storage metadata refresh. Native GUI listeners also expose
`onMediaInfoMan_Changed(MediaID)` independently of mount/unmount notifications.
The next investigation must bind the storage-info update and notification to
the owned hub MediaID without withdrawing it. No direct process-memory edit or
physical AZ operation was used for this tracing.

The primary storage manager exposes an update operation at `0x111d6a0`
(vtable `0x2b7b538`, offset `0xa0`). It resolves a MediaID, forwards an owned
information shared pointer through virtual `0x1a8` or `0x1b0`, then notifies
information listeners through its virtual `0x10`. The ordinary replacement
method `0x111d2a0` (`0x1a8`) assigns the record's retained information at
`+0x30/+0x38` under the manager lock with native reference-count handling.
This is a concrete candidate update boundary, not a validated callable adapter.
Its input identity ABI, information-object construction, dispatcher thread and
generation revalidation must be established before production use. In
particular, do not replace it with an unowned pointer or manual record writes.

Identity helper `0x1119698` takes a four-byte LinkMemberID pointer and a
one-byte storage-interface pointer, then resolves a complete MediaID through
the collection at manager `+0xa8`. Its return includes the record's driveletter;
the update caller must preserve that resolved identity rather than inventing it.
The diagnostic-only property observer now also records the native update's
arguments, shared-pointer pair and return address, bounded to32 observations.
Its built emulator hash is
`f1bade09c924b2a67a2a1453b3edde3f578ce797042fc2615335f2f1c348a795`;
framework activation is pending, so no runtime update observations are claimed.
An additional TSan run exercises5000 metadata mount/invalidate cycles against
20000 concurrent reads, checking complete name/color pairs and rejection of
retired tickets. This is a portable concurrency result, not native integration.

The storage-update observer subsequently loads in emulator PID1545593. Five
owned-property acquisitions are observed, but there are no logs from
`0x111d6a0` during that startup. This is a negative result for that wrapper in
this workload, not proof that metadata is never set. Static property-receive
task `0xbc25e0` instead calls the storage-set interface directly after resolving
the MediaID, using a retained `shared_ptr<PropertyTable>`. Diagnostic follow-up
adds observation of ordinary replacement `0x111d2a0` itself; that changed
observer is not yet deployed. The native property-receive enqueue method is
`0xbbd970`, which copies its member/storage IDs and shared property ownership
into a controller task. Reusing it still requires a proven live controller
binding and generation checks for the external hub identity.

The follow-up observer builds as
`4d25822e1365735f98349e6f1c20b7264e91fc5c46105dfd3add34e2c8352546`
and has been copied to the emulator mods USB with supported apply requested.
It has not yet passed native UI approval/reload and mapped-runtime verification.
Static task `0xbbe4b0` shows the native shared PropertyTable allocation:
an80-byte control/payload block, native control vtable `0x2a58bb0`, three retained
JUCE strings and scalar fields through object offset `0x38`. The owned SQLite
mount point's virtual `0x30` is `0x13510a8`, which fills that native class value;
it is distinct from the plain148-byte data getter. This offers a native value
construction path, but allocation/destruction ABI and controller dispatch still
require validation before any production adapter is introduced.

An alternative candidate prepares the already available private library before
the first media announcement. Current order sends fallback metadata first and
queues that library only after the response; the unsuccessful refresh trial
shows why this ordering matters. `hub_prepare_announcement` now queues an owned
library under its eject lease, waits for a completed generation-matched snapshot,
and permits fallback after10 seconds if metadata/server availability fails.
Completed invalid properties count as completion and retain the existing
fallback behavior. No source is withdrawn for this preparation. The actual
helper-body test passes private-worker sequencing, missing servers, timeout,
completed-empty properties and eject lease refusal. The candidate builds locally;
it has not yet been activated or accepted in the emulator.

The direct-replacement observer subsequently passes native UI Allow/Reload and
mapped-module hash verification in PID1572366. Calls from `0xbc26d4` reach
`0x111d2a0` for external hub MediaIDs including `0x200000602`, `0x2200000602`,
`0x200000702` and `0x2200000702`; each carries a distinct retained information
object. The native USB observation arrives from `0xbbe6d8`. This confirms the
property-receive task reaches the direct setter, while the higher-level update
wrapper remains unused in this startup workload.

The initial-preparation candidate, hash
`fc4eabba8d5593b9f97c652f953c24f37643b7cf02eaf3cd5fefb3584cb4a34f`,
has now been copied to the emulator mods USB and supported apply requested.
Its mapped activation and visual result remain pending. The media emitter now
also suppresses revision updates before the source's native announcement;
the actual-emitter regression includes this condition. Updated hook, loader/
presence and announcement-body controls pass; the portable matrix is rerunning.

Native task body analysis also identifies its supported
`IInternalMountEventFinishListener` callback at virtual offset `0x10`. When a
listener is provided, native code computes a PropertyTable while retaining the
mount point and calls the listener with local ID, system type, root, alternate
root and properties. Our current private mount requests pass NULL. That callback
remains an alternative to the current full-task hook; its
lifetime, failure handling and generation checks still need implementation/tests.

### Late arrival into a previously unused slot

The initial-preparation runtime passes five-source startup and existing-slot
reconnects, but moving valid stick E into nested physical USB8 exposes a separate
failure: that slot has no server pair yet. Preparation times out, the first
media response advertises the fallback name/color, and only the subsequent native
query constructs its servers. Its configured metadata then arrives too late for
the initial Source view. The ignored screenshot `nested-valid-late-usb8.png`
records this failure; it is not a successful nested metadata acceptance.

The next candidate prepares all eight independent server pairs on the existing
native initialization/dispenser paths. Dormant slots open no database, remain
excluded from source matching, and keep their own mount caches. Source admission
preserves those preinitialized pointers. The actual loader-body regression now
checks that preservation, and loader, announcement and hook checks pass. The
emulator target builds with SHA256
`6cc52cacba0861282e37807297c993f61e708d282c399e0e094f1e7bce50d270`.
Supported framework apply has discovered it as a new candidate requiring Allow;
mapped activation and the late-slot repeat remain unverified. No physical AZ
operation or publication is covered by this result.

The supported UI Allow/Reload subsequently loads this exact candidate in EP147
PID78634 (mapped-module SHA256 verified). All eight private server pairs are
initialized. Moving valid stick E from physical USB5 to nested route `3-2.7.2`
creates previously unused internal source5, displayed as USB8. Its metadata-ready
record (`name_valid=1 color=6`) precedes its first announcement. The saved ignored
`capacity-v3-nested-late-usb8.png` shows `Nacho Chips`, Aqua and USB8/HUB after
native USB1/2 and hub USB1/2/3/4. This passes that scoped late-arrival check;
it does not establish eight simultaneous sources or physical acceptance. Stick E
was subsequently returned to its original port and the temporary nested hub removed.

The capacity candidate passes all 103 portable components in plain, ASan/UBSan
and TSan modes. Its two-source complete network menus and six exact file reads
pass, as do five complete menu clients without concurrent file jobs. Two earlier
five-menu plus 12-file attempts fail during initial Beat Link Client construction.
The test client opens its socket before decoding its local independent PDB oracle;
that preparation can consume the provider's unchanged 5000 ms handshake bound.
The checker now indexes before connecting; the combined repeat is pending.
This changes test preparation, not the provider's deadline or menu assertions.

During that repeat, native UI loads `Algo más` from hub USB1 into deck1 and starts
a loop. The exact GPL-core observer records three `LEEE` samples with advancing
deck1 positions in ignored `capacity-v3-loaded-core.json`, retaining PID78634.
This is bounded engine-state evidence, not audio or post-eject/cache acceptance.

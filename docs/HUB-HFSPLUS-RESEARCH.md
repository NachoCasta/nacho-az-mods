# HFS+ library layout and private analysis roots

Scope: exact firmware `ep147-9e9454e357389ece`. This is emulator research,
not physical HFS+ acceptance or a firmware compatibility expansion.

The private reduced fixture preserves HFS+ geometry, metadata, both rekordbox
databases and analysis files, with three populated audio samples. Other audio
records remain present but their omitted data reads as zeros. Loading an omitted
track is not a valid compatibility test. The original USB was read only during
acquisition; only disposable emulator overlays were written.

## Three separate boundaries

1. Discovery required `PIONEER/rekordbox/exportLibrary.db`. The HFS+ export uses
   `.PIONEER`. On identical emulator mounts/sysfs the original scanner admitted
   only the FAT neighbor; filesystem-aware selection admits both in port order.
   Settings and public PDB initialization need the same directory selection.
2. Private native library mount notification at `0x1373ca8` needs its fourth
   argument to identify the Mac layout. The constructor at `0x1352ee8` retains
   that flag and `0x134f138` selects the library directory. Correcting discovery
   alone does not open the hidden native database. Correcting the mount flag
   restores browsing and valid sample loading, but still leaves both graphs blank.
3. Native analysis-root selection at `0x1978970` consults the global native USB1
   mount reference (`0xe5ff30`, filesystem enum comparison 3). That reference
   cannot describe independent mixed-filesystem hub repositories. The adapter
   resolves roots only for this mod's private repository instances and delegates
   all other calls to the original function. Retired owned instances return an
   empty native File rather than falling back to a different source.

## ABI and lifecycle constraints

The native File is eight bytes but has nontrivial C++ semantics: its return uses
AArch64 x8. The bridge preserves x0/x1/x8 on native fallback and carries unwind
information for exceptions thrown by the native constructor. A dedicated
AArch64 C++ test exercises mixed calls, native arguments, the return slot and
exception unwinding. The portable C test covers independent roots, reserved and
retired instances, initialization bounds and lease cleanup.

Before irreversible hook installation, the adapter checks the first four native
instructions: `a9b97bfd d2892a02 f2a9c9e2 910003fd`. A mismatch refuses activation.
Root acquisition uses the existing admission lease and metadata ticket mutex;
taking the discovery mutex here would deadlock synchronous native mount callbacks.
The lease is released before constructing the native File.

## Current evidence and limits

The current exact emulator runtime renders detail and overview graphs for a
populated 22.040-second HFS+ sample simultaneously with a FAT hub track. The prior
mount-flag-only runtime is the preserved blank-waveform negative control.
Native Link On/Off/On retains both sources and graphs with the same process;
the core-published HFS+ playhead advances afterward. No audio signal was captured.
See [progress evidence](acceptance/usb-hub-support-hfsplus-progress.json).

The stock emulator kernel lacks HFS+ and UTF-8 NLS modules. Matching modules were
built for that emulator only; they are not release assets or AZ kernel changes.
The large fixture's public PDB reaches the existing catalog capacity (ENOSPC),
so external sharing of this library is not accepted by the local playback tests.
A fresh framework session passes loaded HFS+ eject with backing/alias removal,
retained cache and advancing engine, reconnect and fresh deck2 waveforms. Native
FAT playback also advances alongside the loaded HFS+ hub track. Cold startup
passes after the existing native filesystem checker completes; nested playlists
and populated track rows are visible. Full portable plain/ASan+UBSan/TSan checks
and the separate SDK UI contract harness pass. Physical acceptance remains open.
Raw research, screenshots, media and databases remain local and ignored.

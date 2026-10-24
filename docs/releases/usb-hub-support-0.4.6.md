# USB Hub Support 0.4.6

HFS+ rekordbox USB libraries can now be browsed and loaded through a hub,
including analyzed Browse previews, scrolling waveforms and track overviews.
No reformatting, directory renaming or library re-export is required.

## Changes

- Recognize the hidden `.PIONEER` library layout used by HFS+ exports.
- Pass the correct layout flag to each private native library server.
- Resolve analysis files from each owned source instead of the native USB1
  filesystem reference. Native repositories retain their original handling.
- Check the expected firmware instructions before installing the new analysis
  adapter. Firmware compatibility remains unchanged.

The executable is byte-identical to the candidate installed and tested on the
owner's AZ: SHA256 `76bad01155355a8cfca6ee3ee877f8175fba4d0bc6f08f627dd28b3bc57fbca2`.
The helper and kernel assets are unchanged from 0.4.5.

## Validation

A private reduced image of the affected HFS+ USB reproduces the layout gap.
Emulator checks cover playlists/tracks, populated sample loads, both waveform
views, mixed HFS+/FAT and HFS+/exFAT operation, native FAT playback, Link On/Off/On,
loaded-source eject with retained cache and advancing engine, reconnect/fresh
load, and cold startup after the native filesystem check completes. Audio was
not captured. Only populated sample files were used for load assertions.

All 105 portable components pass plain, ASan/UBSan and TSan checks. The SDK UI
contract harness passes all three modes. Additional AArch64 C++ controls cover
the indirect File return, native fallback arguments and exception unwinding;
instruction-guard controls reject every single-byte change in its pinned window.

The owner confirmed ordinary USB operation after exact-candidate installation,
approval and reload on the physical AZ. This is manual user-reported acceptance.
The original HFS+ USB had been returned to its owner, so HFS+ acceptance for this
release is emulator-based; a physical HFS+ trial remains deferred.

## Compatibility and limits

Exact firmware `ep147-9e9454e357389ece` (2.00), kernel 4.4.194 and core >=1.2.3.
The physical regression used core1.2.5. The stable ID remains `usb-hub-support`.

Experimental Ethernet sharing remains available, but the large affected library
reaches the existing public catalog capacity. External sharing of that library
is not validated. Actual-CDJ interoperability, Wi-Fi sharing, audible phase
alignment, eight-source hardware operation and long-session acceptance remain
open. Native Link On disables local decks3/4. Wait for safe-to-remove confirmation.
Framework-managed internal staging is persistent, not RAM-only or USB-only.

No new research screenshots, USB images, music or proprietary files are included.

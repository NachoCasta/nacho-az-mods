# Native hub badge rendering research

This is static research for firmware 2.00 / `ep147-9e9454e357389ece`.
No runtime badge acceptance is established by these findings. The candidate
adapter is reconstructed in `src/azhub/linkfake/source-badge-render.h`.
The local Ghidra project was opened read-only with analysis disabled. Firmware,
raw decompilation and disassembly remain outside tracked documentation.

## Ownership and update ordering

`0x1c88618` is the native queued `setMediaID` callback. Its first argument points
to function storage. The first machine word of that storage points to a heap
closure containing a controller pointer at offset 0 and an eight-byte MediaID
at offset 8. The callback compares incoming identity against controller offsets
`0x18..0x1d`, derives an image description, invokes controller virtual offset
`0x20`, and only then updates the controller's cached identity. Consequently a
render hook must use the incoming closure identity, not the still-old cache.

The existing `safe_source_identity` registered-source check is a candidate
ownership guard. A numeric player/slot conversion alone is insufficient. A
thread-local scope around the queued callback can carry its incoming ownership
through nested rendering calls. Scope must unwind after every original callback;
blank and non-media update paths must retain native behavior.

## Widget and image path

| Widget | Vtable | Paint function | Main image | Native destination |
|---|---|---|---|---|
| SourceDeviceIconWidget | `0x2e34e68` | `0x1c83888` | widget + `0xd0` | 90 by 90 |
| StatusAreaDeviceIconWidget | `0x2e32658` | `0x1c82a68` | widget + `0xd0` | x=5, y=0, 95 by 70 |
| WaveformLoadedDeviceIconWidget | `0x2e36388` | `0x1c840e8` | widget + `0xd0` | 170 by 41 |

Exact-v3 emulator observation confirms the status-area association with the
Browse header. Its native paint also draws a five-pixel color stripe.

Controller offset `0x28` is the widget pointer. `0x1c88820` updates the native
main image and LINK/player children. Its second argument points to a sixteen-byte
image description comprising two seven-byte records at offsets 0 and 8. Byte 5
of the first record controls LINK/player visibility, and byte 6 is the zero-based
player image index. Source controller vtable `0x2e39ad0` calls this function
directly. The waveform controller vtable `0x2e3ab88` calls `0x1c88900`, which
calls the base updater and separately handles the root-device marker using the
second record. Clearing the second record in a scoped copy suppresses that marker.

Source and waveform constructors are `0x1c83a98` and `0x1c84380` respectively.
Both use child ImageStack widgets for LINK and player numerals. Source child
getters at virtual offsets `0x160` and `0x168` are `0x1c840d0` and `0x1c840d8`;
they return widget fields `0xd8` and `0xe0`. Waveform equivalents are `0x1c84c58`
and `0x1c84c60` with the same field offsets.

ImageStack paint `0x1b47090` uses the image array at child offset `0xd0`, selected
index at `0xc8`, count at `0xe0`, and fallback image at `0xe8`. These are shared
native widget internals; replacing their global resources is unnecessary.

## Candidate custom image adapter

Native PNG-memory decoder `0x255ac60` takes PNG bytes in `x0`, a signed integer
length in `w1`, and the indirect C++ return image address in `x8`. This is verified
from both constructor call register setup and the callee's `x8` save/store path;
the decompiler's apparent first output parameter is not the machine ABI. A JUCE
image handle occupies eight bytes. Native image operations are:

| Operation | Address |
|---|---|
| Copy construction | `0x251e8b0` |
| Copy assignment | `0x251e8e0` |
| Swap | `0x251e970` |
| Destruction | `0x2511a90` |

Copy construction and assignment take destination in `x0` and source in `x1`.
Instruction inspection confirms that copy assignment increments the new image's
reference count and releases the previous destination. Swap only exchanges the
two handle words and must not be used directly against an immutable cached asset.
The destructor takes the image address in `x0`, accepts a null handle, and does
not clear the handle. Clear it explicitly after destruction to prevent reuse.

Initialize decoder output storage to zero, check its handle after return, and
destroy each returned image exactly once, including null failure results. The
decoder writes a null handle for unsupported/invalid input. Its native cache
uses the PNG byte address as a key, so asset bytes must have a stable lifetime.

A candidate adapter can decode original, self-drawn PNG assets once, scoped to
the exact firmware. For an owned incoming identity, copy the native description,
hide LINK/player children, invoke the normal updater, then assign the matching
custom image to widget `+0xd0`. The wrapper's second record also needs suppression
for waveform/status-area root markers. Non-owned updates invoke native behavior,
which assigns the stock image again when the identity changes. This avoids a
screen-position overlay and does not alter transport identities or library slots.

The [candidate ledger](acceptance/hub-source-ui-candidate.json) records emulator
source selection, Browse headers, Waveform layouts, native/hub replacement and
removed-but-loaded sources. Refcount/failure and unrelated-identity controls also
pass host-adapter tests. Actual external-CDJ UI and physical AZ acceptance remain
separate; the synthetic external media fixture did not create a Source row.

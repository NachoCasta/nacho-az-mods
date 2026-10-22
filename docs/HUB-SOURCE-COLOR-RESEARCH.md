# Hub source color research

Scope: exact supported EP147 firmware only. The static findings below are
followed by scoped emulator observations. Current configured initial Source
colors, live native color changes and Browse stripe controls pass; Waveform
preserves its native neutral H/USB treatment. See CURRENT-STATE.md for candidate
hashes and evidence boundaries. No physical color acceptance is claimed here.

## Native widget color path

- Source paint at `0x1c83888` reads a four-byte color at widget offset `0xc5`
  and fills its background before drawing the image at offset `0xd0`.
- Browse paint at `0x1c82a68` reads the same color offset for its five-pixel
  stripe. Its image is a separate presentation layer.
- Color assignment helper `0x25124c0` copies one four-byte word.
- Incoming MediaID callback `0x1c88618` invokes the controller provider's virtual
  method at offset `0x10`, then assigns the returned color to widget `0xc5`.
  This happens outside the branch that updates the image for a changed MediaID:
  an unchanged identity can still receive a new color.
- Callback `0x1c88738` similarly obtains color through provider virtual offset
  `0x18`. Setter `0x1c89070` assigns a supplied color to the widget.
- Update wrapper `0x1c890e0` snapshots color and image, executes its callback,
  and requests repaint when either changes.

The existing badge callback calls the original native callback, so it does not
itself remove the native color assignment. A fixed composite image is therefore
not sufficient evidence that the cache key is the cause of a blue hub badge.
The provider may return blue for the synthetic identity, or the image may mask
the native background. Resolve those alternatives before changing cache keys or
adding a color-setting parser. DEVSETTING waveform color is not evidence of the
USB device color.

## Emulator live-color control

The ordering candidate with SHA256
`f4ae81d71f338c092617d9322646d55daa48a3ca4b3661d1828f928b2397ab62`
now has a bounded Source-view live-color observation:

- Native USB2 initially shows Aqua in its Background Color settings.
- Native UI selection changes it to Yellow; its Source icon changes accordingly.
- Hub USB4 initially shows Default. Selecting Yellow through its own native
  settings changes its Source icon while retaining HUB, the USB artwork and USB4.
- In the same captured frame, sampled native USB2 and hub USB4 background pixels
  are exactly RGBA `(135,120,10,255)`. Neighbor hub USB3 remains
  `(15,50,95,255)`.
- Both settings were restored through the same native UI: USB2 Aqua, hub USB4
  Default. The restored screenshot was inspected.

This demonstrates that the current Source composite does not force a fixed-blue
background or prevent a live device-color update. Do not change the composite
cache key based on that disproved hypothesis. Initial metadata acquisition,
Browse and Waveform behavior still require separate controls.

Hashes of `/media/usb/sdb1/PIONEER`'s bounded settings/database file inventory
were unchanged across the Yellow observation. Binding that fixture to the
displayed native USB2 was not instrumented, so this is only a file comparison,
not evidence that the changed source performed no writes. Persistence and
deferred writes remain separate research questions. No raw
settings, databases or screenshots are published. See the
[scoped observation](acceptance/usb-hub-support-source-color-live-emulator.json).

## Provider metadata boundary

The Source color provider vtable begins at `0x2e39a70`. Its MediaID method
`0x1c84d18` queries `mount_device::IMountStorageInfo` through virtual offset
`0x40` for a color index, applies fallback rules, and converts the index through
`0x1fa3590` / `0x1fa35a0`. This identifies a native metadata boundary to inspect;
it does not yet establish the on-disk color format or prove synthetic hub
identities are supported by that lookup.

The Waveform loaded-icon provider vtable at `0x2e3a828` uses `0x1c91928` for
both color methods. That method returns the color at `0x3ff2778` without a
device lookup. Consequently, requiring every view to use the device color
cannot be justified simply by assuming every native provider already does so.

The primary `MountStorageInfoManager` vtable at `0x2b7b538` has color lookup
`0x111beb8` at virtual offset `0x40`. It locks the manager, resolves the media
record and reads its color index from the retained information object at offset
`0x25`, or an override at record offset `0x2a` when enabled. This is metadata
state, not an image-cache lookup. Unknown identities can fail the lookup and
reach the Source provider's fallback. The synthetic media response currently
does not populate any researched device-color field; its remaining zero bytes
must not be assigned guessed meanings.

An independent protocol reader is available locally in the Beat Link 8.0.0
source archive. `MediaDetails.java` reads the media-response color ID at `0xa8`
through `ColorItem.colorForId`; the existing `send_media` leaves that byte zero.
This is a concrete missing metadata field to validate against the native receive
path and the source's real color. Populate it only after establishing the color
source and supported index mapping. The same reader decodes the supplied name
at `0x2c` as UTF-16BE; the current sender already fills that name from discovery,
whose input is presently the volume label rather than a proven rekordbox name.

Native DeviceInfo comparison/debug function `0x1c0ed88` independently compares
the two string fields at offsets `0x08` and `0x10`, counts at `0x1c` / `0x20`,
and the byte at `0x25` already identified by the color lookup. This supplies a
bounded native record to trace upstream for name and color together. The string
semantics and ownership must be established before using those offsets in code.

Identify the provider implementation and its color metadata source. Inspect
stock-image alpha and observe the native color word during an emulator color
change. Verify both provider output and the resulting pixels, including an
unchanged MediaID. Preserve the native repaint path and artwork.

Raw headless analysis output remains ignored under
`local/diagnostics/source-presentation-research-v1/`. Analysis used the existing
local database in read-only mode with analysis disabled. No real AZ operation
was performed for these findings.

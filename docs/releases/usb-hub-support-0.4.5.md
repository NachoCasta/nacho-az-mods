# USB Hub Support 0.4.5

Hub USB sources now use port-based numbers, appear in numeric order after native USB sources, and show rekordbox device names and colors.

## Changes

- USB1-8 follows hub-port topology rather than connection order. Nested hubs flatten into ordinary numbers; only eight leaf positions are supported.
- Preserve native badge artwork and align Source numerals3-8 with the USB text. Browse and Waveform retain their HUB/H labels.
- Prefer rekordbox-configured device names and colors, with volume-name/default-color fallback.
- Preserve analyzed Browse previews, scrolling waveforms and full-track overviews. Retire the initial metadata mount before mounting the registered media for analysis; keep captured metadata available for the first response.
- Include two emulator screenshots for the mod details page in AZ-MODS1.2.5 and newer. Older clients ignore the screenshot URLs; the mod still requires core1.2.3 or newer.

## Validation

The owner confirmed source presentation and subsequently confirmed waveform restoration on the physical AZ after the exact corrected runtime was installed, approved and reloaded through AZ-MODS. This is manual user-reported acceptance, not agent-instrumented rendering/audio.

The shipped executable is byte-identical to that accepted candidate: SHA256 `6ec384330f43f5126e7d70492727f4d42c6e120dd55a9c856f5226c49812c904`. Release changes to the accepted package are manifest text and screenshots; the helper and kernel assets are unchanged.

Emulator controls cover native-positive/blank-hub-negative waveform detection, configured metadata/order, loaded-source safe eject/cache retention, reconnect/fresh waveform load, and native Link On/Off/On with continued engine advancement. Five independent software-client libraries and12 exact file reads passed. All103 portable components passed plain, ASan/UBSan and TSan checks; actual announcement/mount-body tests cover retirement ordering and metadata retention.

One emulator selector attempt was refused after the core's cumulative200us input limit; normal reload with empty decks restored input and the loaded-eject repeat passed. Core limits were unchanged. The network trial required the existing6.6 emulator macvlan fixture; that fixture is not shipped for the AZ.

## Compatibility and limits

Firmware2.00, exact tag `ep147-9e9454e357389ece`; kernel4.4.194; core>=1.2.3. No broader compatibility is claimed. Includes self-built GPL kernel modules and source/build materials.

Experimental Ethernet sharing is retained. Actual-CDJ interoperability, Wi-Fi sharing, audible phase alignment, eight-source hardware operation, long-session timing and cache capacity remain unvalidated. Native Link On disables local decks3/4. Wait for safe-to-remove confirmation; external players handle source loss according to their own cache. AZ-MODS stages executable files internally; persistence is not RAM-only or USB-only.

## Screenshots

Emulator previews, not photographs or evidence of five-source hardware acceptance:

![Ordered hub USB sources with configured names and colors](https://github.com/NachoCasta/nacho-az-mods/releases/download/usb-hub-support-0.4.5/usb-hub-support-0.4.5-shot1.png)

![Individual USB safe-eject selector](https://github.com/NachoCasta/nacho-az-mods/releases/download/usb-hub-support-0.4.5/usb-hub-support-0.4.5-shot2.png)

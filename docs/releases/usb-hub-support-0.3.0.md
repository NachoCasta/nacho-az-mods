# USB Hub Support 0.3.0

USB Hub Support can now participate in PRO DJ LINK while keeping hub libraries
available locally. With Link Off, the AZ retains four local decks. With Link On,
it uses the native two-local-deck mode alongside external players. Each hub source
keeps independent library servers and caches; native USB library slots are not
borrowed. Hub libraries are still **not shared with external players**.

## Known issue: changing Link mode with a track loaded

Changing PRO DJ LINK On/Off briefly disconnects and reannounces hub sources.
Previously loaded hub tracks can retain the native source-unavailable warning
triangle and rely on cached playback. A simultaneously loaded native USB source
does not show this disconnection in the user's comparison. This issue remains
unresolved; select the intended Link mode before loading hub tracks. Do not assume
seamless source continuity or unlimited cached playback across a mode change.

## Validation

- Real AZ, exact supported executable: the user confirmed hub browsing, loading
  and playback with Link Off and On. Read-only SSH verified normal framework
  activation with 54 hooks on firmware2.00 and kernel4.4.194.
- A Beat Link8.0.0 software peer communicated with the physical AZ over the
  user-selected Wi-Fi topology. Tests covered discovery/status, negotiated master
  transfer, native SYNC following126/128BPM and same-player process reconnect.
  The user confirmed a fresh CAMI hub-track load/playback while the peer was master;
  both local decks reported SYNC at128BPM. Audio is user-reported, not captured or
  measured for phase alignment. This does not establish actual-CDJ compatibility.
- Exact-artifact emulator tests covered five hub sources plus both native USBs,
  four/two-deck modes, cold On/Off startup, loaded transitions, individual/whole-hub
  loaded eject, partial-eject mode transitions, virtual-USB replug and fresh retry.
  Full1152-byte status captures checked known private identity fields. Host and
  separately identified diagnostic tests support request/socket lifetime handling.
  No complete semantic audit of unknown packet fields or every async race is claimed.
- Portable plain/ASan+UBSan and pinned SDK UI checks passed during candidate
  preparation. Release source CI and anonymous package-download checks are tracked
  separately in the publication record.

See the [hardware records](../acceptance/usb-hub-support-0.3.0-link-modes-hardware.json),
[master/SYNC trial](../acceptance/usb-hub-support-0.3.0-wifi-master-sync-hardware.json),
[fresh-load trial](../acceptance/usb-hub-support-0.3.0-wifi-fresh-load-hardware.json)
and [emulator evidence ledger](../acceptance/usb-hub-support-link-goal-evidence.json).

## Compatibility and operation

Firmware2.00, exact tag `ep147-9e9454e357389ece`, kernel4.4.194 and Community Mods
core>=1.2.3 only. Displayed2.01 on the tested unit denotes the AZ-MODS-patched2.00
installation; it does not broaden firmware support. The checked GPL kernel module
and USB STOP safe-eject controls remain included. Short-press USB STOP to select
one hub stick; hold it to eject the hub. Wait for final safe-to-remove confirmation.

Eight sources are implemented; five were tested together in the emulator. There is
no eight-source hardware, long-session, all-format or cache-capacity acceptance.
The slot8 source caption/icon limitation remains. Actual-CDJ interoperability and
audible phase synchronization need further validation. AZ-MODS stages files in
persistent internal storage; removing the USB does not unload a running mod.

This release publishes the exact tested candidate package, including its original
experimental manifest wording. The catalog and these notes provide the updated
validation and known-issue information. No executable or package member was changed
after acceptance.

Package SHA256: `38d70598ba8103d970f1b87a651c1e9268affbf4784e221bfb50ae87b0802eee`.
Executable SHA256: `5e8ecd3ee0426bec83d5fc0f73d7f7327aa982c89206a67c97d6fc5634be26bc`.

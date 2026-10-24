# Native USB Names 0.1.0

The XDJ-AZ can display only USB1 or USB2 when a drive has no device name configured in rekordbox. Native USB Names displays the USB filesystem volume label in that case, making drives easier to identify.

- A valid rekordbox device name always takes priority, preserving its case and Unicode.
- If the configured name is unavailable, empty or invalid, a usable volume label is displayed instead. If neither is usable, the native generic name remains.
- Native USB identity, icons, colors, browsing and library routing are preserved. No disk renaming, metadata rewriting, extra library mounts or USB Hub Support dependency.
- Separate USB1/USB2 insertion state prevents stale labels after removal and source reuse.

Compatibility is limited to firmware 2.00 tag `ep147-9e9454e357389ece`, kernel `4.4.194`, and AZ-MODS core 1.2.3 or newer. Built with SDK 1.2.3 at commit `62a156abf3a4789e9e3cdd610fe106227a3a8967`.

## Validation and limits

Five host regression suites passed plain, ASan/UBSan and TSan. Exact-runtime emulator tests covered standalone operation, coexistence with USB Hub Support 0.4.5, configured and fallback names, Unicode/bounds, removal/source reuse, native presentation, browsing, track loading and visible detailed/overview waveforms.

The exact hardware executable was verified as loaded on the physical XDJ-AZ alongside USB Hub Support 0.4.5. The user reported that it works well. This general user report does not establish separate instrumented hardware waveform, audio, Unicode, standalone, reinsertion, rollback or long-session acceptance.

The release changes the candidate's description only; the executable is byte-identical to the physically tested candidate. No screenshots or raw research are included.

## Installation and rollback

Refresh the Nacho AZ Mods repository in AZ-MODS and install Native USB Names. Use the normal installation/Allow controls and Reload with all decks empty. AZ-MODS stages mods in persistent internal storage.

To roll back, empty the decks, turn Native USB Names off under Community Mods and Reload. Use Remove from USB and Reload to uninstall. Unplugging the USB alone does not unload a running mod. Disk labels and rekordbox metadata need no restoration.

## Checksums

- Package SHA256: `ef160aca01418ce84af3e21a5d7502c761f9d9faf998134b3e052f1c975cece4` (61,577 bytes).
- Hardware executable SHA256: `fe8d50ead1488a7f72211042bd119390827b364a8eee541efe024444ea06d4c4`.

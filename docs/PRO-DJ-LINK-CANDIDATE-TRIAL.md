# USB Hub Support 0.3.0 candidate trial

Local preparation is complete within the bounded scope of the
[final audit](acceptance/usb-hub-support-link-final-preparation-audit-v5.json).
Use the normal AZ-MODS workflow under the [testing policy](AZ-TESTING-POLICY.md).
The user removed the separate physical recovery prerequisite; recovery remains
unverified and must not be described as a passing test.
Published 0.2.1 remains unchanged. This is not a hardware-ready or release claim.

Candidate: `local/integration/link-coexistence-v5/dist-candidate/usb-hub-support-0.3.0.azmod`.
No public package URL exists.

| Artifact | SHA256 |
|---|---|
| Package | `38d70598ba8103d970f1b87a651c1e9268affbf4784e221bfb50ae87b0802eee` |
| Packaged ELF | `5e8ecd3ee0426bec83d5fc0f73d7f7327aa982c89206a67c97d6fc5634be26bc` |

## Completed local checks

The [build record](acceptance/usb-hub-support-0.3.0-all-slot-replay-candidate-build.json)
and [requirement ledger](acceptance/usb-hub-support-link-goal-evidence.json) separate
supported runtime, host and diagnostic evidence. Bounded supported emulator tests
cover five hub sources plus both native USB ports, four local decks with Link Off,
two with Link On, both cold modes, loaded mode changes, individual/whole-hub loaded
eject, partial-eject mode changes and physical virtual-USB reconnect/fresh retry.

An isolated software peer exercises discovery, master transfer, tempo following,
loss/reconnect and selected hostile admission controls. Full 1152-byte statuses
pass known private-field checks. This is not a complete protocol-tail audit.
A separately identified derived diagnostic exercises pending normal/Instant Doubles
stale-source rejection at a controlled common-stage pause; it does not force all
asynchronous scheduling or allocation-failure boundaries. The supported package
was restored afterward. Emulator USB-absent recovery passes on kernel6.6, which
skips the hardware module; physical kernel4.4.194 recovery remains unverified.

## Prerequisites with the user present

1. Verify firmware `ep147-9e9454e357389ece`, kernel `4.4.194`, core at least1.2.3,
   actual loader identity and candidate hashes. Retain the known0.2.1 rollback
   package/configuration on USB.
2. Verify stopped playback and no in-flight writes before activation/reload.
   A separate remove-mods-USB/full-restart recovery test is not required.
3. Use supported Community Mods Install/Allow/enable/Reload controls. Uninstall
   legacy `hub-libraries` first if present. Never manually edit internal firmware,
   staging, settings, approvals or process memory.

Framework-managed staging persists internally; it is not RAM or USB-only storage.
Removing USB does not unload a running mod. Application restart does not reset the
kernel patch. Stop playback and wait for writes before a full restart.

## Physical trial through AZ-MODS

1. With Link Off, browse hub/native sources and load distinct tracks from different
   sticks. Verify four local decks; record source/title and warnings.
2. Enable Link through native settings. Expect two local decks; native decks3/4
   unload. Confirm decks1/2 continue and hub rows return without duplicates.
   Repeat Off/On. Hub libraries remain local, unavailable to external players.
3. Short USB STOP opens the selector: eject one loaded hub stick, wait for safe
   confirmation and check cached playback/unrelated/native sources. Change Link
   mode and verify the ejected source stays absent. Reconnect it and verify return.
4. Hold USB STOP to eject remaining hub sources; check cached playback and native
   USB operation. Record user-heard audio separately from SSH/UI observations.
5. With a reviewed isolated physical network setup, repeat software-peer discovery,
   master/tempo/loss/reconnect while using hub tracks. The fixed localhost emulator
   harness is not a physical-device deployment tool.
6. Restore settings through supported controls. Recovery testing is optional;
   record it only if actually performed without active playback/writes.

Private tracks carried across mode changes/eject can retain yellow native
source-unavailable warnings while cached playback continues; cache capacity is
finite. A native metadata startup prefix also occurs without candidate activation;
latency equivalence is unproven. Audible phase sync, actual-CDJ interoperability,
eight-source hardware, all formats and long sessions remain unvalidated. External
hub-library sharing is out of scope. Keep screenshots/captures local.

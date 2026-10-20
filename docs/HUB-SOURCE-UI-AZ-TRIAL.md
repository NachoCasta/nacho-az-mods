# Hub source UI: final AZ trial

Revision notice: the package and trial below describe the historical custom
badge v3 candidate. The current native-body v14 candidate is documented in
[the native candidate record](acceptance/hub-source-ui-native-candidate.json).
Its representative additional-numeral screenshots await user visual review.
Do not install this historical candidate or deliver the new one as a side
effect of following these notes; no new physical delivery is in the current goal.

This trial checks the local UI candidate described in the
[implementation plan](HUB-SOURCE-UI-PLAN.md). The exact package, ELF hashes and
bounded emulator results are recorded in the
[candidate evidence ledger](acceptance/hub-source-ui-candidate.json).
The unpublished 0.3.1 package is available locally at
`local/integration/hub-source-ui-v3/dist/usb-hub-support-0.3.1.azmod`.
Do not substitute the published 0.3.0 package or infer that these steps have
already passed on hardware. Actual external-CDJ rendering remains unvalidated;
the synthetic peer fixture did not produce a Source row.

Use the normal AZ-MODS workflow under the
[current testing policy](AZ-TESTING-POLICY.md). This document does not install or
activate a package. Keep screenshots local and ignored.

## Setup

Use two hub sticks with distinct volume names and one library partition each,
plus a native USB stick as the control. Record the candidate hash, AZ firmware,
Link setting, volume names and observed hub numbers. Use a normal unloaded state
for candidate activation. Leave Link in its starting mode for the first pass.

## Manual checks

1. Open Source. Each hub library should show a light HUB header, USB-stick icon,
   USB number and its original volume name. Record the number for each stick.
   The native USB control should retain its normal native icon and label.
2. Browse each hub library and then the native control. The hub badge should
   retain the same number in Browse; names, playlists and track selection should
   remain usable. Native Browse presentation should remain unchanged.
3. Load one hub track on a deck and a native-control track on another deck. In
   Waveform, the hub source should show `[H] USB n` at the same number recorded
   in Source; the native deck should retain its native USB identity. Compare
   clipping, alignment and readability. Then replace the hub deck with the other
   hub source, and finally with a native track: each source change should refresh
   the badge without leaving HUB, LINK or member-number remnants.
4. Short-press the hub's USB STOP button. Confirm the dark **Eject USB** selector
   lists **HUB / USB n** and each matching volume name in number order, without
   physical-port labels. Turn the encoder, tap a row, and cancel using BACK;
   reopen and cancel using **Cancel / BACK**. No cancellation should eject a disk.
5. Reopen the selector and eject one hub stick. Wait for the actual completion
   notification before removing it; closing the dialog is not completion.
   Confirm the other hub stick keeps its original number in Source, Browse,
   Waveform and the selector, and remains available to browse/load. Confirm the
   native control still works. Record any failure or retry message exactly.
6. Reconnect the removed stick and let discovery settle. Record its resulting
   number; only the continuously connected stick must retain its earlier number.
   Hold the hub's USB STOP button for the existing whole-hub gesture. Confirm
   completion for the remaining hub disks and continued native-control operation.
   A hold must not also open or submit an extra individual selector action.
7. Reconnect the hub sticks. Change Link Off/On and revisit Source, Browse and
   Waveform with both a hub and native source. Confirm hub badges and native
   controls remain correctly scoped. Record brief source interruption or loaded
   warning triangles separately: fixing that existing issue is outside this UI
   candidate, and its disappearance must not be assumed.

## Record the result

For each step, record pass/fail, volume-to-number mapping and any unexpected
native presentation, wrong ejection target, stale badge, clipping, warning or
input behavior. Stop the trial if the selected ejection target differs from the
volume shown. Distinguish visual/protocol observations from user-reported audio;
this bounded trial does not establish long-session, eight-source or phase-sync
acceptance. Publication remains a separate decision.

# Link mode continuity: physical AZ trial

This is a trial plan, not physical acceptance or permission to deploy. Ask the
user before each real-AZ action under the current action-specific authorization.
Use normal AZ-MODS installation/activation; never patch controller files or
memory directly. Publication requires a separate decision.

## Exact candidate

- Mod: USB Hub Support, stable ID `usb-hub-support`, version0.3.2.
- Firmware: exact `ep147-9e9454e357389ece`; kernel4.4.194; core>=1.2.3.
- Package: `local/integration/link-mode-continuity-integrated-v5/dist/usb-hub-support-0.3.2.azmod`.
- Package SHA256: `d461a7791ea0977eda3daf776933567197f19461b6abc73e4de370646b9caa81`.
- ELF SHA256: `a3d6c7ba438494008edf66fdda43d4be5ae702ed3afa4cdcb57791b9964d1add`.
-59 registered hooks. GPL kernel module and firmware support are unchanged.

Confirm the actual package path and both hashes before installation. Keep the
published0.3.1 package available for normal rollback. This candidate has not
been published or installed on the physical AZ by this work.

## User-assisted checks

1. With Link Off, connect native USB and at least two hub sticks. Record labels,
   source count and package identity. Browse and load each hub source freshly.
2. With decks empty, toggle On then Off. Sources must not vanish or duplicate;
   browse and load after each settled transition. Verify ordinary four-local
   deck availability Off and two-local availability On.
3. Load a native track on one of decks1/2 and a hub track on the other. Play
   both, toggle On then Off, and listen for interruptions. Neither active deck
   may receive a new source-unavailable warning. Capture source labels and
   playback state before/after. Native disabling of decks3/4 on On is expected.
4. Repeat with both decks1/2 loaded from different hub sticks; also repeat while
   paused. A fresh post-transition track load is required, not just cached play.
5. If an isolated software peer is available and authorized, verify external
   master/SYNC, loss and same-player reconnect while a hub track plays. Audio
   and phase behavior require user observation; emulator packet evidence alone
   does not establish them. Do not advertise hub sources to the peer.
6. Test individual safe eject while another hub source remains, then long-STOP
   whole-hub eject with multiple sources. Confirm native USB stays available.
   Intentional eject may show native warning triangles while cached tracks play;
   this differs from an unsolicited warning during a mode change.

If a continuity failure occurs, retain exact steps/logs and stop claiming success.
Do not substitute faster reannouncement or hide warnings. A selector refusal
from the existing core timing guard is a separate documented limit: preserve the
guard and arrange an authorized normal restart rather than bypassing it.

## Limits

Emulator6.6 results do not replace physical4.4.194 audio, actual-CDJ or long-session
acceptance. Five virtual hub sticks cover both private member identities; no
eight-source hardware acceptance is claimed. External hub-library sharing,
broader firmware support and memory-segment expansion are outside this trial.

# USB Hub Support 0.4.12

Fix a startup crash that could stop the deck while USB Hub Support was starting.

During startup the mod installs its hooks while the deck program is already
running. In 0.4.11, each hook's pointer to the original native function was saved
only after the AZ-MODS core had made the hook live. If the UI thread reached the
task-queue hook (`0x237f900`) in that short window, it called a null pointer. The
deck stopped and AZ-MODS suspended the mod with "the deck stopped while this mod
was starting". 0.4.12 lets the core store each original directly into the
storage the hook reads, before the hook can run. No feature behavior changes.

Validation:

- The physical crash record (SIGSEGV on `HuiProcessor`, PC 0, return offset
  `0xddac` in `probe_queue_pop`) matches static analysis of the exact 0.4.11
  executable and AZ-MODS core 1.2.9.
- An emulator-only diagnostic holds the same window open for 5 s. Built from
  0.4.11 sources, it reproduced the field failure 2/2 (null original, same
  suspension message, one service restart). Built from the corrected sources, it
  passed 2/2 (valid original, all 64 hooks loaded, no restart).
- Every production hook claim in the shipped executable passes its callback
  storage directly. A new host test exercises a hook entered before
  `claim_hook` returns. The full portable host suite (plain, ASan, TSan) passes.
- The exact shipped executable passed an emulator smoke pass: hub Sources, hub
  browsing, track load, both waveform views, playback, PRO DJ LINK On/Off with
  the deck playing, and knob-selected individual eject ("Safe to remove").
  Ethernet sharing helper startup behaves the same as published 0.4.11 in that
  emulator fixture.

Runtime SHA256: `d23170c5eca4a637a84f031189716942ce317a4f8c5521343ab2a16e40cb2167`.
Package SHA256: `fa559c1847f2316b05e107660c3d1c0358018f640f55a6df03f5296763e3a5a3`.
Package size: 711704 bytes. All helper/kernel assets are byte-identical to 0.4.11.
Publication is authorized by the owner. Physical acceptance of 0.4.12, actual-CDJ
interoperability and prolonged-session behavior are not claimed. The cause of the
earlier prolonged-session report is not established; this release fixes the
retained startup crash. See the [investigation](../HUB-STARTUP-CRASH-2026-11-21.md).

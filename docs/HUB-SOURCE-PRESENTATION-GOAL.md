# USB hub source presentation goal

This user-requested scope replaces the earlier physical-port numbering goal and
is the active goal. It retains that work and adds ordering, device colors and
rekordbox device names. Completion remains subject to the validation below.

- Retain physical-port USB 1-8 numbering, empty-port reservations, safe nested
  topology flattening and the measured Source numeral alignment correction.
- Show native sources first, followed by hub sources in ascending displayed USB
  number regardless of insertion order. Order the eject selector consistently.
- Reuse each device's native USB color in Source, Browse and Waveform, preserving
  native artwork and HUB/H markers. Reflect color changes made through the AZ's
  source settings during the session. Investigate native state and composite-cache
  behavior before choosing the implementation; do not assume the waveform-color
  setting is the USB device color.
- Investigate naming metadata and prefer the rekordbox-configured device name,
  preserving case and accents. Fall back to the volume label, then USB N. Leave
  native-source naming behavior unchanged.
- Preserve independent servers/caches, active-source and loaded-track continuity,
  safe eject and existing PRO DJ LINK sharing. Keep the stable mod ID, exact
  firmware/kernel/core compatibility, eight-source limit and documented topology
  constraints.
- Validate locally and emulator-first: insertion order, reconnects, nested hubs,
  colors and live color changes, naming fallbacks, playback, eject and shared
  services. Save ignored local screenshots for user review before physical trials.
  Obtain action-specific real-AZ authorization as required by current instructions.
- Finish with English documentation, scoped evidence and reviewed commits pushed.
  Resolve the existing public-push approval rejection through user approval; do
  not bypass it. Publishing a release or changing the catalog requires a separate
  user decision.

The user's observation that the hub source already exposes a working color
setting is research evidence, not proof of which native field drives its icon.

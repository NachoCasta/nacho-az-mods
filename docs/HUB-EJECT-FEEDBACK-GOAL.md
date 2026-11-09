# Persistent USB eject feedback goal

The owner reports that the two-second preparation banner disappears before the
later safe-removal banner, creating an unsafe impression that ejection is done.
The requested result is continuous floating feedback until verified completion,
with cleaner English labels and the USB name separate from its removal status.

## Implementation and required evidence

- Use a supported SDK top-layer surface, independent of both selector presentations.
  Keep the same surface visible from accepted submission to safe/failure completion.
  Do not use a longer guessed banner timeout or periodic native caution reposts.
- Show Ejecting USB/Hub and an explicit keep-connected instruction. Terminal success
  uses Safe to remove and a separate target name; failure keeps the USB connected.
  Only terminal states offer Done. The panel never owns knob/BACK focus and cannot
  cancel an active worker. Opening a new selector can dismiss a completed result.
- Preserve all existing reservations, detach waits, filesystem unmounts, persistence
  and final whole-hub inventory checks. Publish success only after they pass.
  A later observed remount invalidates an existing success notice.
- Cover real publication/render/input callbacks, no progress timeout or show/hide
  gap, guarded dismissal, long labels, failure and concurrent worker completion.
  Run relevant portable and SDK UI gates under plain, ASan/UBSan and TSan.
- Exercise the exact candidate in the owned emulator: progress visibly persists
  beyond the old two-second window, then transitions in place; individual/whole-hub
  and touch fallback, failure/retry, loaded-cache/native USB retention and reopening.
- Build and identify the exact unpublished 0.4.10 candidate with unchanged helper
  and kernel assets, validate its package and prepare it for owner AZ testing using
  the authorized mods-USB/framework workflow. Physical UI acceptance is the owner's
  next trial; do not infer it from emulator evidence or publish without permission.

Published 0.4.9 and its assets remain immutable. Screenshots and raw captures stay
local and ignored. Host and emulator gates now pass with the documented exact-runtime versus
diagnostic-control scopes. The final emulator runtime is restored without pause
or overrun fixtures. The exact candidate is active on the physical AZ (PID810530), with EEEE before
and after managed apply, ready for the owner to connect the hub and validate the
UX. This candidate-preparation goal is complete; physical UX acceptance remains
unclaimed. See the
[candidate evidence](acceptance/usb-hub-support-eject-feedback-candidate.json).

## Owner follow-up

The owner confirms the new eject UI looks good and works, then reports hub USBs
missing from Sources again. Read-only inspection finds the current connection
already safely ejected and unmounted, plus template0 and fallback inactive in the
same process. Sources startup remains a separate unresolved issue; do not treat
this UX acceptance as proof of source recovery or infer corruption. The owner
indicates the source investigation can be revisited later. Detailed facts are in
the candidate evidence. No device changes or restart were performed.

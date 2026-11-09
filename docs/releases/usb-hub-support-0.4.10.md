# USB Hub Support 0.4.10 candidate

Unpublished candidate: persistent USB eject feedback.

The eject panel stays visible while the USB is being prepared and changes in place
to Safe to remove only after the existing safety checks complete. The USB name is
shown separately. Failures explicitly keep the USB connected; terminal results
remain until Done or the next selector opening. Normal and fallback touch ejection
share the same panel without taking knob/BACK focus during background work.

Portable and actual-panel controls pass plain, ASan/UBSan and TSan, and the SDK
UI harness passes all measured checks. The exact runtime passes delayed individual
and whole-hub eject, loaded-cache/native USB retention, and genuine busy failure
with retry in the owned emulator. A separate emulator-only overrun control confirms
the same feedback after real SDK filter quarantine; it is not the release runtime.
No audible playback is inferred; the owner subsequently accepts the physical UI.

Runtime SHA256: `7c78f096684fa772e5e7193c500fd3f9f73080fade5073ca29ece9b00a64754e`.
Package SHA256: `41ecfed2f85f742844ed5b8d3b629fd4d1a08838db185d27bb86cff6c5d844ff`.
Existing helper/kernel assets are byte-identical to0.4.9.
The exact candidate is active on the physical AZ (core1.2.8), with owner-reported eject UI acceptance. USB rollback/data are retained; managed apply
completed with empty decks and the mapped hash matched. The separate recurring Sources failure is under investigation for0.4.11.
See the [candidate evidence](../acceptance/usb-hub-support-eject-feedback-candidate.json). See the
[feedback goal](../HUB-EJECT-FEEDBACK-GOAL.md).

# Selector input lifetime research

## Confirmed boundaries

The exact unavailable banner is reproduced in the owned emulator after both
mod frame filters are quarantined. Core quarantine is cumulative and remains
until process restart. Callback source inspection alone cannot attribute an
elapsed-time overrun to CPU work, page faults or scheduling. The original
physical incident has no recovered instrumented trace.

The baseline mod also latches any missing raw/edit observation permanently.
The current development change instead requires eight consecutive valid reads
with both callbacks advancing before recovery. Any missing callback resets that
streak; idle cannot heal a fault and a quarantined filter remains unavailable.
Fault reasons distinguish missing raw/edit observations from a missing panel FD.
This partial fix does not solve permanent SDK quarantine.

## SDK-only negative control

Removing the filters and trusting surface events opens and navigates the selector,
but immediate focus release leaks held BACK into native navigation. Core focus
also freezes the encoder counter; the old editing filter restored its physical
value to avoid a jump when ownership ended. A replacement must solve both.

Current upstream source was inspected at commit
`214a058b94120ec6c15070abddd6d04bbffef230`. The API exposes knob/click/BACK events
but no physical button-release state. Surface DOWN/UP/LONG refer to touch.
Frame-filter registration is init-only. Re-registering, resetting strike counters
or moving frame rewrites into an untimed path is not an acceptable recovery.

The guarded native GuiButton observer is at0x23606f0, vtable slot0x3216798;
the exact first instruction is0xa9b97bfd. Its owned input key is at object+0xd0
and the notification points to a boolean value. An emulator-only observation
identifies BACK as EasyID0x009fc1f55209ac55. The existing encoder key is
0x01d690928446227a: notification words0/1 carry signed delta and position,
bytes8/9 carry push value/change. Multiple observers see the same notification;
a one-shot atomic exchange must not consume a correction for only the first
observer. These are scoped research facts, not new production hooks.

A native tail guard could suppress residual button notifications, but the
SDK-only input loses information: a held key cleared by core focus and an
already-released key can produce the same returned frame. A guard must not
silently eat a genuine later press or release a still-held button on a timer.
That ambiguity remains an explicit gate for a mod-only replacement.

## Local framework API proposal

A core-owned atomic snapshot, published only after the existing CRC validation
and before input edits, could expose serial, physical key bits and encoder
counter to UI-thread consumers. No per-frame mod callback, extra device read,
allocation or core-quarantine reset is needed. Invalid CRC must not update it;
device close must invalidate it. Idle must retain state without fabricating a
heartbeat. A size-checked service would be appended to the SDK ABI.

An opt-in focus mode would also be needed to preserve the physical encoder
counter for this mod's already-guarded native observer. Ordinary surface focus
must retain its current freeze behavior. Focus refusal, competing dialogs,
ownership restoration and overlay watchdog behavior must remain unchanged.
This is an API design proposal, not functionality in the installed core.

A private snapshot component passes two million concurrent publications with
coherent serial/key/counter fields, idle, invalidation and wrap checks. A test
using the actual upstream SDK filter implementation retains its original200us/
eight-strike shutdown, verifies that the proposed snapshot survives quarantine,
and proves the filter remains off. All31 input checks pass. These components
also pass ASan/UBSan and TSan. Publication is modeled separately from the current
bus dispatcher: full core and mod integration have not been validated.

No custom core has been built or installed. Core replacement on the physical AZ
is outside the existing USB Hub Support candidate-install authorization. Any
such proposal needs a concrete reviewed build and a separate owner decision.

## Mod-only touch fallback experiment

A separate private emulator prototype retains the normal filters but forces
eight overruns in each callback. The unmodified SDK quarantines both. Short USB
STOP then opens a touch-only surface below core dialogs. This surface never
requests SDK focus or captures encoder/BACK; its text explains that those controls
keep their native functions. Native view changes close the panel. This avoids
the held-release and encoder-freeze boundary by leaving those inputs unowned,
rather than claiming exclusive ownership after the filters have failed.

With the same process,50 open/cancel cycles pass in72.14 seconds without new
removal requests. A populated FAT sample, Algo más/content495, loads with both
waveforms and ejects through touch. Its backing and private alias disappear;
the native mods USB and HFS+ neighbor remain. The exact-core observer sees the
retained own-source cache ticket at phase2 and advancing deck1 afterward.
No audio was captured. An earlier absent sparse-fixture Abcs file produced the
expected no-file error and is not a valid load regression.

This prototype is historical evidence, not a full-encoder replacement. The
fallback is now integrated into an unpublished0.4.8 candidate. An optional owner
question about the interaction remains unanswered; integration does not claim
owner UI approval. Persistent surfaces reject foreign and late events, normal
healthy input retains the existing encoder modal, and touch mode requests no
physical focus. Source/sanitizer checks and package validation pass. An
after-open injection exercises actual SDK quarantine and recovery through touch
without process restart. A subsequent unforced quarantine on the exact normal
runtime also opens the touch selector with both cached tracks still loaded.
Fresh-load/individual eject, startup and extended-duration controls remain
pending; no physical candidate was installed and no release was published.

# USB Hub Support 0.4.8

Individual USB selection remains available if the SDK disables an encoder-input
filter during a session. Short USB STOP then opens touch controls: tap a USB and
Eject, or Cancel. The encoder and BACK keep their native functions in this mode.
Healthy input retains the usual encoder selector. A transient observation fault
recovers after eight consecutive valid healthy reads instead of remaining latched.

If input stops while the encoder selector is already open, it releases ownership
and asks you to press USB STOP again for touch controls. No filter timing limit is
raised or reset, and no framework/core replacement is required. Late events from
hidden or other surfaces cannot submit another removal request. Hold USB STOP
to eject the whole hub; wait for safe-to-remove confirmation before unplugging.

## Validation

The baseline unavailable banner is reproduced after SDK filter quarantine, and a
separate component control reproduces the permanent health latch. Emulator-only
fault injection exercises actual SDK shutdown after the selector opens. The exact
normal candidate also encounters an unforced shutdown and then passes61 open/
cancel cycles over30 minutes without restart or unexpected removal requests.
The cause of the owner's historical physical incident cannot be established
without its original logs; elapsed overruns do not identify scheduling versus
callback CPU work.

All106 portable components pass plain, ASan/UBSan and TSan checks, along with both
SDK UI presentations and10000 actual-selector-open retries per sanitizer mode.
Exact-runtime controls cover FAT/HFS+ waveforms, native USB loading and advancing
engines, individual and whole-hub loaded eject with retained independent caches,
reconnect, native PRO DJ LINK transitions and modeled automatic hub recovery.
The startup model uses an explicitly unbound QEMU hub, not actual kernel4.4 depth
rejection. Emulator audio was not captured.

The exact hardware executable was installed, approved and activated through the
normal framework workflow. The owner reported "it works" and requested release.
This is general user-reported physical success, not separate instrumented audio,
physical filter-quarantine or prolonged-session acceptance.

## Compatibility and limits

Exact firmware `ep147-9e9454e357389ece`, kernel4.4.194, core>=1.2.3.
Stable ID: `usb-hub-support`. Kernel/helper/assets are unchanged from0.4.7.

Actual-CDJ interoperability, Wi-Fi hub-source sharing, audible phase alignment,
eight-source hardware operation and prolonged physical sessions remain
unvalidated. Large libraries can exceed sharing catalog capacity. The original
HFS+ USB physical trial and second physical cold-boot repetition remain deferred.
Framework staging is persistent. No new screenshots or proprietary files are
included; existing approved0.4.5 preview URLs remain in the catalog.

# USB Hub Support 0.4.7

USB sticks already connected through a hub are now recovered automatically at
power-on, without unplugging and reconnecting the hub.

The stock kernel can reject the hub before the mod's guarded hub-depth patch
loads. A bounded startup worker retries unbound external hub interfaces on the
known USB controllers after that patch is available. Already-working devices
are skipped; the helper never unbinds or resets them. The existing kernel
module, mount helper and other previous assets remain unchanged.

## Validation

The emulator explicitly models the observed unbound-hub startup condition:
the previous runtime leaves the hub unavailable, while this candidate restores
FAT and HFS+ sources without hub reconnection. Fresh loads render both waveform
views. Mixed-source operation, Link transitions, loaded eject/cache continuity
and child reconnect controls pass. Emulator audio was not captured.

All105 portable components pass plain, ASan/UBSan and TSan checks, along with
the SDK UI contract harness and17 targeted startup-helper cases.

On the physical AZ, a new kernel boot ID and logs confirm initial hub rejection,
the guarded patch at10.05 seconds and successful hub discovery at13.19 seconds,
without an intervening USB disconnect. Native USB1 and both FAT/exFAT hub sources
are mounted. The exact loaded executable matches this package. The owner reports
that it worked and requested publication. A second physical cold-boot repetition
remains pending; detailed audio/UI acceptance is not inferred from that general
success report.

## Compatibility and limits

Exact firmware `ep147-9e9454e357389ece`, kernel4.4.194, core>=1.2.3.
Physical testing used core1.2.5. Stable ID: `usb-hub-support`.

Existing limits remain: actual-CDJ interoperability, Wi-Fi source sharing,
audible phase alignment, eight-source hardware operation and long sessions
are not validated. Large libraries may exceed sharing catalog capacity.
The original HFS+ USB physical trial remains deferred. Wait for safe-to-remove
confirmation before unplugging a stick. Framework staging is persistent.

No new screenshots, music, USB images or proprietary files are included.

# USB Hub Support 0.4.9

Fix hub libraries missing from Sources when Ethernet has no IPv4 address, even
though the same drives are already visible in the eject selector. Publication
authorized by the owner on 2026-11-08; physical screen/load acceptance remains
explicitly pending.

Hub libraries can be detected and listed by the eject selector while missing
from Sources when native Ethernet has no IPv4 address. The private local Link
transport now starts independently of that external connection. Its temporary
loopback bootstrap does not assign interface addresses or change saved native
settings. Native Link packets are withheld from external destinations while
that fallback is active. Private source labels refresh atomically as network
availability changes, with loopback labels available when no external IPv4 exists.

The stable mod ID, firmware/kernel compatibility, minimum core1.2.3, existing
helper and kernel assets, independent library servers and caches are preserved.

Full host components and the SDK UI harness pass plain, ASan/UBSan and TSan
checks. Focused actual-adapter tests cover native call-site scope, unchanged
observed properties, inactive and IPv6 forwarding, recursive entry and C++
unwind cleanup. Cached peer refresh/ingress and fallback transmission-policy
controls pass all three sanitizer modes with10000 concurrent refreshes.

The exact final runtime passes Sources, populated browsing, fresh deck loads, both
waveform views, completed native Link On/Off transitions and loaded eject/reconnect
with the application network namespace containing only loopback. A separate negative
native-packet ingress control is rejected while private announcement succeeds.
The exact runtime is active on the physical AZ through managed framework apply.
Private member readiness, source mounting and native announcement/query acknowledgment
now succeed without assigning an Ethernet IPv4. Physical UI acceptance remains pending.
No audio, actual-CDJ, eight-source or prolonged-session hardware acceptance is claimed.

Runtime SHA256: `2c45d96a1bd160fe00fbe516edebbf135841952b68a6dd341ba68cc2a38ae48f`.
Package SHA256: `d73f51370565563483c051ac19378abe749030314e4f6e8d8cbe2ab5b57e9a57`.

See [the active goal](../HUB-SOURCE-VISIBILITY-GOAL.md) and
[progress evidence](../acceptance/usb-hub-support-source-visibility-progress.json).

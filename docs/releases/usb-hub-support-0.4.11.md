# USB Hub Support 0.4.11

Recover local hub sources when Wi-Fi has an address but
Ethernet is unavailable and native transport startup has not completed.

A management/Wi-Fi IPv4 address no longer prevents the native local bootstrap.
Fallback peers retain their loopback identities, and only the exact native local
configuration call receives the matching127/8 subnet. Native observed addresses,
interface configuration and saved network settings remain unchanged. Usable
wired Ethernet retires fallback and retains its original address/subnet.

The persistent eject feedback accepted by the owner in0.4.10 is retained.
Independent source servers, SQL caches and tagged caches remain independent.

A hardware-refusing emulator-only missed-event control distinguishes the old
persistent pending state from successful readiness/announcement/query with the
complete fix. It models a missing native event; natural QEMU wireless event timing
did not reproduce the persistent physical incident. The ordinary candidate has
no diagnostic event suppression. Full host/sanitizer and pinned SDK validation
pass; exact-runtime controls cover Wi-Fi startup, late USB arrival, Sources,
browsing/loading, both waveform views, loaded Link On/Off and individual/whole-hub eject and instrumented no-network startup.
A wired fixture also confirms source readiness with fallback retired and the
native192.0.2.2/24 transport unchanged. Exact0.4.11 is active on the physical AZ through managed apply, with CAMI
readiness and announcement/query acknowledgment verified. The owner confirms physical Sources and fresh loading work.

Runtime SHA256: `02fd149b23a13a1d8a32383694b24b8137ead89ca6d5675edb644b795f9778c2`.
Package SHA256: `7039202d06ea4a2499ee6d240fc998c8ca085dd9308fdcaf51c4153c8379060f`.
Package size:711728 bytes. All14 helper/kernel assets are byte-identical to0.4.10.
Publication is authorized by the owner. No actual-CDJ or prolonged-session acceptance is claimed; the
earlier suspected crash remains unexplained. See the [investigation](../HUB-SOURCE-STARTUP-RECOVERY-GOAL.md) and
[candidate evidence](../acceptance/usb-hub-support-source-startup-recovery-candidate.json).

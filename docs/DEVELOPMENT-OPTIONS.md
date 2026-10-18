# USB Hub Support development options

Research review: 2026-10-17. These are investigation and design proposals, not
implemented features or new acceptance results. No real-AZ access was performed.

## Rear USB-C connector

The [official rear-panel manual](https://downloads.support.alphatheta.com/manuals/all-in-one-dj-systems/XDJ-AZ/html/en/000COV_en/Part_names/Part_names.htm)
describes this connector as a PC/Mac connection. The
[product page](https://alphatheta.com/en/product/all-in-one-dj-system/xdj-az/black/)
distinguishes two top USB-device ports from one laptop connection. The
[connection guide](https://downloads.support.alphatheta.com/manuals/all-in-one-dj-systems/XDJ-AZ/html/en/000COV_en/Connections_%28basic_styles%29/Connections_%28basic_styles%29.htm)
also documents a computer connection for Link Export.

The supported role is therefore a computer connection; a hub of storage devices
is not documented for this connector. USB-C connector shape and USB bus speed do
not establish host capability. The observed xHCI 5000 Mb/s controller has not been
mapped to the rear connector. Internal firmware media-slot numbers also do not
establish physical connector identities.

Before proposing support, inspect local board/device-tree and kernel configuration
for connector routing, host/device or dual-role support, and VBUS power control.
If the hardware is device-only, a software change or passive adapter cannot turn
it into a host. If dual-role hardware exists, driver and role-switch validation
would still be required. No rear-port support is currently claimed.

The ignored machine configuration and preserved SDK/research directories described
in the handoff are absent from this worktree. Their contents were not inspected
as part of this review.

### User-connected rear-hub inspection

On 2026-10-17, after the user connected a USB-C hub and stick and acknowledged
the proposed read-only SSH inspection, the running 4.4.194 kernel showed no new
hub or storage enumeration on xHCI buses 5/6. Existing storage and the visible
VIA hub were on EHCI buses 1/2; their physical connector mapping was not reverified.
No filesystem was mounted or written by the inspection commands.

The live device tree reports `usb0/dwc3@fe800000` as `dr_mode=peripheral`,
`maximum-speed=high-speed`, and `usb1/dwc3@fe900000` as `dr_mode=host`.
The UDC `fe800000.dwc3` reported `not attached`. The latter host controller owns
the empty xHCI buses. This supports a peripheral-role explanation for the rear
connection, but does not independently prove connector wiring or inability to
change roles. No role switch, power change, driver reload or restart was attempted.
The rear-connected stick was not detected in this configuration; no file-read or
playback acceptance is claimed. Electrical host capability remains unestablished.

## Safe ejection



Current removal handling reacts to a missing source; it is not a user-requested
safe-eject workflow. The fallback mount helper normally mounts read/write.
`hub_mount_poll()` in `src/azhub/linkfake/hubservers.h` queues native library
unmount callbacks, withdraws the alias through `/proc/udev_nfs`, and later attempts
ordinary alias unmount. The delay is not proof that every operation completed.
`hub_cleanup_missing_mounts()` only cleans fallback mounts after the block device
has disappeared from sysfs.

A proposed eject operation should:

1. Resolve the selected source to its physical block device and all affected
   partitions. Keep native USB devices outside the operation's scope.
2. Block new requests and rediscovery/remount for that device before withdrawal.
   The mount worker otherwise remounts an unmounted fallback device on its next scan.
3. Determine whether any deck is using that device. Initially reject a busy eject
   rather than assume that a loaded or paused track no longer needs file access.
4. Withdraw affected sources and wait for native server, cache and alias release.
   Retain independent caches and handles for unaffected sources. Reuse of existing
   removal callbacks requires completion checks and a synchronization review.
5. Flush pending writes, then perform ordinary filesystem unmount for every affected
   mount. Coordinate stock-mounted devices as well as fallback-mounted devices.
   A successful flush alone is insufficient; do not report success after a busy or
   failed unmount, or use forced/lazy unmount as evidence of safety.
6. Confirm completion before showing a safe-to-remove result. Preserve the ejected
   state until observed physical removal, so discovery does not reannounce the stick.

No native per-hub-source eject UI hook or validated deck-source busy detector has
been established. Investigate the native eject action first; a separate mod control
is another option if the core supports it. UI selection must identify one physical
stick accurately, including its partitions, without borrowing native library slots.

Local tests should cover browsing and in-flight requests, loaded/paused/playing
tracks, multiple partitions, native USB alongside hub sources, stock and fallback
mounts, unmount failure, simultaneous unplug, repeated requests, and removal then
reinsertion. Same-path replacement without observed absence remains an existing
unverified case. Emulator evidence must precede any separately authorized hardware
test. Runtime implementation has not started.

## PRO DJ LINK

Treat three goals separately: guarding against an incompatible user setting,
allowing external players while keeping hub libraries local, and exporting hub
libraries to external players. The existing warning addresses the current limitation;
it does not implement any of these features.

See [PRO DJ LINK research](PRO-DJ-LINK.md) for setting detection and lifecycle work.
Coexistence additionally requires proving separation of internal identities 3/4,
physical player discovery and routing. Removing the physical packet filter alone
does not establish that separation. Source sharing would additionally require a
network-visible library design and independent validation; it is not promised.

Current routing details reinforce that boundary: filtering is bidirectional,
connections to synthetic source addresses are redirected to loopback, and database
requests translate private source codes `e0..e7` and requester identity 5. The peer
selection in `network-identity.h` chooses high subnet addresses without external
occupancy discovery. That is an internal labeling scheme, not a collision-safe
network allocation scheme. Native Link mode also disables internal decks 3/4.
An emulator protocol harness should establish player-number and address separation,
routing and deck behavior before changing packet-filter policy.

## Suggested order

Prioritize safe ejection because it affects normal hub use. Investigate its native
UI action and busy/completion signals before changing runtime code. In parallel,
research the Link user-setting accessor and identity/routing boundaries locally.
Keep rear USB-C support exploratory until connector role and electrical routing
are established.

# Hub source visibility recovery goal

## Required outcome

Fix USB Hub Support sources missing from the physical AZ Sources view despite
appearing in the eject selector. Preserve the stable mod ID, independent source
servers and caches, native USB operation, both native Link modes, and managed
activation through AZ-MODS. Validate locally and in the owned emulator before
installing an exact candidate on the mods USB. Physical installation requires
safe deck state. The owner separately authorized pushing and publishing 0.4.9 on 2026-11-08.

## Current physical evidence

Read-only inspection on 2026-11-08 finds the published 0.4.8 runtime, SHA256
`c35df90775b60c4bff2dbb70bc3606efd82825595bce8e1151152ce2c6953ff2`.
CAMI is mounted with a readable SQL library and published in the mod's discovery
file. The earlier classic-PDB-only hypothesis does not explain this session.
The source is present, notification is pending, no unmount is pending, and no
announcement has occurred. The runtime has no status template and its private
member is not ready. UDP50002 has no listener. Ethernet has no carrier or IPv4;
Wi-Fi management works. The owner reports the source absent with native Link On
and Off. Read-only mod-owned state confirms three completed native mode epochs,
so the UI toggle was processed and did not recover the source.

No physical files, settings, network configuration or process memory were changed.
Raw local diagnostics and the exact USB package remain ignored under
`local/source-visibility/`.

## Validated correction

The final 0.4.9 runtime is
`2c45d96a1bd160fe00fbe516edebbf135841952b68a6dd341ba68cc2a38ae48f`.
The published package SHA256 is
`d73f51370565563483c051ac19378abe749030314e4f6e8d8cbe2ab5b57e9a57`.
The native completed mode task bootstraps the existing network-change operation
when its observed IPv4 is zero and no status template exists. Only that operation's
temporary IPv4 constructor receives loopback. Observed properties, interface
addresses and saved native settings remain unchanged. Fallback native Link TX/RX
is restricted to local transport; private member provenance checks remain enforced.

The exact final executable passes all portable components and SDK UI controls in
plain, ASan/UBSan and TSan modes, including actual-adapter forwarding, recursion,
C++ unwind cleanup and 10000 concurrent peer-label refreshes. Preserved SDK1.2.3
validation accepts the directory and package. Existing helper/kernel assets match
0.4.8. The compiler remains GCC7.5 with the preserved ARM64 SDK sysroot.

## Emulator evidence and limitations

Exact published 0.4.8 reproduces the pending state with unaddressed dummy Ethernet
and separate management. Restoring the original addressed Ethernet interface and
restarting empty decks restores readiness and acknowledgment. The dummy does not
model physical carrier or driver behavior.

Final 0.4.9 was tested with the application in an isolated network namespace having
only loopback: initial discovery is empty, then late library discovery, template1152,
member readiness, announcement acknowledgment, rendered Sources, populated browsing,
a fresh deck1 load and both waveform views succeed. Native Link On and Off complete
in the same PID152828 while retaining the loaded deck. Loaded touch eject succeeds,
removes backing and alias mounts while retaining the native mods USB and loaded
cache. QMP reconnection restores the source and permits a fresh deck2 load with both
waveform views. Decks were unloaded through native controls afterward.

After restoring the emulator application namespace (dummy Ethernet still without
IPv4), PID170503 again announces the source. One native-format member3 packet from
the management address is rejected by kernel-derived ingress metadata; real private
member7 announcement remains acknowledged. This is a bounded emulator ingress
control, not physical on-wire or actual-CDJ acceptance. Audio advance is unclaimed.

The emulator-only legacy renotifier was corrected to skip nested hub topology;
no duplicate native USB2 slot is used. Original USB images remain untouched under
snapshot/overlay storage. Screenshots and raw diagnostics remain ignored locally.

## Instrumented physical recovery

The normal framework loader staged the exact runtime on core1.2.8 without restarting
PID3033. A complete old-package/config backup is on the mods USB. The exact USB and
staged SHA256 values matched before pre-approval. Only the usb-hub-support approval
entry was replaced under the framework lock, with a USB backup and atomic rename;
unrelated approvals were preserved. BusyBox lacks chmod --reference and stat, so
those failed attempts left the original list unchanged before compatible completion.
Framework managed staging persists internally; it is not RAM or USB-only storage.

The supported Community Mods apply request completed with four empty decks.
PID311725 maps the exact reviewed runtime and reports 0.4.9 with63 hooks. Read-only
mod-owned state confirms template1152, private member ready, mounted/present source,
notification cleared, announcement timestamp1794174456 and query acknowledgment
1794174457. CAMI's independent /media/player07/usb mount exists. Fallback is active,
native mode remains Off and eth0 still has no IPv4. The original pending transport
state is corrected on the physical controller. Framework apply status is done.
Owner confirmation of rendered Sources, browsing and fresh loading is pending.
No public catalog, release or repository publication was changed.

## Publication authorization

The owner requested push and publication after the instrumented physical recovery.
This authorizes the release and catalog update, without constituting a report of
physical screen/load acceptance. The exact tested package remains unchanged.

The release and catalog are now published as [0.4.9](https://github.com/NachoCasta/nacho-az-mods/releases/tag/usb-hub-support-0.4.9).
See the [publication record](acceptance/usb-hub-support-0.4.9-publication.json) for
public asset/source checks and CI. Physical screen/load confirmation remains open.

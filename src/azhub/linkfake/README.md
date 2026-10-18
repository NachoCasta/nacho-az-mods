# USB Hub Support

Version 0.2.0 adds safe eject through short/long USB STOP. The exact executable
passed user-operated tests with two hub sticks plus native USB1 and SSH mount
verification. Unload tracks first and wait for final safe-to-remove confirmation;
loaded-track eject is not supported and its refusal message was not observed.
See [release notes](../../../docs/releases/usb-hub-support-0.2.0.md).
Read [repository state](../../../docs/CURRENT-STATE.md) and
[publishing notes](../../../docs/PUBLISHING.md) before building/deploying.

The material below is retained as **historical experiment notes**, not current status.

Current community implementation (0.1.3, subnet fix; manual AZ retest pending): see [AZ test instructions](AZ-TEST.md) and
[port notes](COMMUNITY-PORT.md). `community.c` builds `hub-libraries.so` through
the normal Community Mods SDK; `linkfake.so` is the retained research prototype.
The historical prototype notes below do not describe the latest packaging.

# Independent library server experiment (EP147 2.00)

This is an emulator-only LD_PRELOAD prototype, not a hardware-ready mod.
`hubservers.h` is enabled only when both LINKFAKE_PROBE=1 and LINKFAKE_HUBSRV=1.
Do not set LINKFAKE_SLOT: that option overrides the private request-slot mapping.

The experiment constructs a fresh MusicLibraryServer and PersistentStorageServer
for each configured hub stick (player and slot). Their database state, request dispatchers,
queues, threads, mutable mount caches and mounted media IDs are separate from local USB1/2/3. Shared
system-service references are copied from the initialized stock USB1 servers.
Requests for each fake player's USB are mapped to private protocol codes e0..e7;
per-object vtable matchers claim those requests. The original firmware vtables
are not modified. The copied vtables preserve the ABI's RTTI prefix.

Current limitations: fixed 20-second delayed mount, no removal handling, startup requires restart, fixed firmware addresses, shared-service wiring under
investigation, and incomplete removal/teardown support. Four configured sources are now shown
in the emulator; current acceptance evidence is tracked in
`../research/GHIDRA-FINDINGS.md`. Do not package for hardware until the remaining
playback and lifecycle checks and the SDK port are complete.

Build in WSL using the GPL tree's Linaro aarch64-linux-gnu-gcc:

    aarch64-linux-gnu-gcc -O2 -Wall -shared -fPIC -o linkfake.so linkfake.c -ldl -lpthread

The factory uses an AAPCS64 indirect C++ result in x8, not an output pointer in x0.
The C 24-byte aggregate deliberately selects x8 when calling the original.
The assembly hook entry captures the stock caller's x8 and writes only its
8-byte result. Each stick also owns a private mutable factory cache.

Emulator rollback: stop EP147, restore /home/root/linkfake.so.before-hubsrv and
/home/root/settings/az-usbload/az-preload.env.before-hubsrv to their original
names, then start EP147. Those backups were made before enabling this experiment.



Playback fixture: the four current emulator images each contain the full DB but
only 17 audio files. Verify file existence before load/playback tests; �Algo m�s�
is present on all four. Missing audio does not invalidate DB browsing.

Browser testing: selecting a playlist can leave its side preview empty. Tap the
selected playlist again to enter its full track list, then verify the count,
track metadata and navigation. The full list is the direct browsing check;
side-preview behavior is a separate outstanding issue.

`LINKFAKE_LATE_SERVERS=1` skips the factory/holder startup hooks and constructs
server pairs on the initialized dispenser thread. It extracts only the native
common service handle and timer from the stock server's initialized command
component. Each new server retains its own database, cache and task thread.
This path prepares for community loader initialization after EP147 startup;
it is still an LD_PRELOAD experiment, not a packaged community mod.

# Wi-Fi-present hub source startup recovery

## Confirmed physical failure

The owner accepts 0.4.10's persistent eject UX, but reports USBs missing from
Sources. A first read-only snapshot found an already-ejected connection. The
owner then physically disconnected/reconnected the stick and the next snapshot
confirms a fresh connection: NACHO CHIPS on2-1.1.3, USB device8, sysfs inode18074,
mounted at /media/usb/sda1 with readable SQL and an empty eject journal. PID810530
still runs exact 0.4.10 (7c78f096684fa772e5e7193c500fd3f9f73080fade5073ca29ece9b00a64754e).
Its new active source is present/pending, not mounted as a private source, and
has no announcement/query acknowledgment. Template0, private readiness0, fallback0,
and one completed native Link Off epoch rule out an unprocessed mode selection.

Read-only inspection of the existing typed native NetworkManager registry finds
observed IPv4 192.168.1.115 despite eth0 having no IPv4 or carrier; it is the Wi-Fi
address. Mode is initialized and settled Off. The old bootstrap returns on any
nonzero observed IPv4, so it cannot recover this session. No manual controller
settings, interfaces, services, firmware or memory were changed during diagnosis.

## Reproduction controls and limits

The owned QEMU app initially had a separate management IPv4, dummy eth0 and down
mlan0. Adding Wi-Fi IPv4 and disabling Ethernet carrier did not reproduce a
persistent fault: native startup eventually announces the source. An isolated app
network namespace with only lo, unaddressed/no-carrier eth0 and addressed mlan0
also auto-starts the native transport. Initial template0 snapshots in those
controls are transient and must not be reported as stable reproductions.

A separate explicitly diagnostic build refuses hardware kernels and withholds
only the automatic native network-change operation, while allowing the mod's
explicit bootstrap. This models the missing native startup event observed on
hardware without changing core safeguards. The original 0.4.10 guard remains
persistently stuck with Wi-Fi IPv4 already observed: template0, present/pending,
no readiness/announcement, one settled Off epoch. Raw controls stay ignored in
local/source-recovery; this diagnostic is never a distributable candidate.

The first correction distinguishes usable wired Ethernet (IPv4, admin up,
carrier) from Wi-Fi/management IPv4 using a read-only lifecycle observation. It
successfully enters fallback with nonzero Wi-Fi IPv4, but that alone does not
prove recovery: a strict nonzero-address startup control still lacks readiness.
The next correction keeps peer identities on127.77.0.7/8 whenever fallback is
active, instead of deriving those loopback peers' transport labels from Wi-Fi.
That pair alone still fails the strict control: native local IPv4 is127.0.0.1
but its independently copied Wi-Fi mask remains255.255.255.0, excluding127.77 peers.
The complete correction supplies255.0.0.0 only to the exact native local-config
call at return site0x140693c when its address is127.0.0.1 and fallback is active.
Native observed properties and interface addresses remain untouched. Failed interface observation preserves
an existing fallback rather than promoting external Link.

## Current candidate evidence

The full emulator-only missed-event control succeeds with observed Wi-Fi IPv4
already nonzero: template1152, private readiness1, pending0 and announcement/query
acknowledgment. Observed Wi-Fi192.168.1.115/24 remains unchanged; the native local
transport uses127.0.0.1/8. This is a bounded event model, not natural reproduction
of wireless driver timing.

Ordinary0.4.11 runtime SHA256:
`02fd149b23a13a1d8a32383694b24b8137ead89ca6d5675edb644b795f9778c2`.
Package SHA256:
`7039202d06ea4a2499ee6d240fc998c8ca085dd9308fdcaf51c4153c8379060f`.
The pinned SDK validator and full portable/SDK plain, ASan/UBSan and TSan gates
pass. The ordinary runtime has64 production hooks and no missed-event control.

Exact ordinary-runtime emulator controls pass Wi-Fi/no-Ethernet startup,
rendered Sources/browser, fresh track loading and both waveform views. Native
Link On/Off preserves the loaded deck and process. Individual eject reaches the
persistent Safe to remove panel, removes the backing mount, retains native USB
and the loaded cache. Native unloading settles toEEEE before restart. Starting
without the hub and attaching it afterward reaches readiness and announcement/
query acknowledgment, with the source rendered alongside native USB. The settled no-network control also reaches announcement/query acknowledgment
with observed0.0.0.0 and local127/8. Usable wired Ethernet retires fallback and
preserves native192.0.2.2/24 while Sources works. Whole-hub eject in the offline
fixture reaches persistent Safe to remove, removes backing/player aliases and
retains native USB. Rapid restart testing triggered the framework safety interlock;
supported managed apply restored the candidate, without manual internal edits.
After whole-hub eject/reconnect, the restored Wi-Fi fixture also passes fresh
track loading and both waveform views onPID983207. Native unload leavesEEEE
and managed apply is done. No audible advance or actual-CDJ interoperability
is claimed.

## Physical activation after owner restart

SSH returned after the owner restarted the controller. Boot ID
`aad2e521-1b2a-4569-b2bd-52f5dd28c12c`, PID5469, firmware/core/kernel unchanged,
empty decks. The same0.4.10 failure recurs with freshly mounted CAMI on2-1.1.3:
template0, readiness0, present/pending, observed Wi-Fi192.168.1.115/24. The available
crash log contains no record and pstore is empty; the previous outage's cause is
unproven. Do not describe the lost SSH connection as a confirmed crash.

The exact0.4.11 candidate was installed on the mods USB with full0.4.10 rollback,
configuration and data retained underMODS/backups/source-recovery-0411. Normal
az-usbload staging matched USB/runtime hashes. The target-only preapproval used
the authorized lock/USB-backup/atomic replacement procedure. Supported managed
apply completed with empty decks, PID20044,64 hooks, native USB naming retained.

CAMI now has template1152, readiness1, mounted/present1, pending0, and matching
announcement/query acknowledgment1794207967. OS Wi-Fi remains192.168.1.115/24,
Ethernet unaddressed. Native observed IP/mask at this snapshot are0.0.0.0 and local
transport is127.0.0.1/8: this physical apply does not replace the emulator's strict
already-observed-Wi-Fi timing control. The owner explicitly confirms Sources/browsing/fresh loading worked. Final
read-only audit retains PID20044, exact USB/staged/mapped SHA and managed apply
done; decks areEEEE. No publication has been performed.

## Completion audit

The goal is complete for the confirmed incident: causal control, full correction,
exact ordinary-runtime regressions and authorized physical activation with owner
Sources/loading acceptance are recorded in the
[candidate evidence](acceptance/usb-hub-support-source-startup-recovery-candidate.json).
The gates below retain the original scope and are satisfied by that evidence;
unperformed broader acceptance is explicitly excluded.

## Original completion gates

- Demonstrate old/fixed behavior under the same missed-event control with observed
  Wi-Fi IPv4 already nonzero; require private readiness and announcement/query,
  rather than fallback-enabled alone. Distinguish this bounded event model from
  actual wireless hardware or driver acceptance.
- Test the ordinary exact 0.4.11 runtime without the diagnostic hook: startup with
  Wi-Fi and no Ethernet, no-network/late USB arrival, reconnect, native Link modes,
  source browsing/loading/waveforms and persistent individual/whole-hub eject.
- Preserve independent libraries/caches, the accepted eject UX, native settings,
  native USB operation and wired Link policy. Run relevant sanitizer/SDK gates;
  validate and identify the exact package with unchanged helper/kernel assets.
- Through the authorized mods-USB/framework workflow, retain rollback and existing
  mod data, verify USB/staged/approval/mapped hashes, activate only with empty decks
  and verify the physical fresh-source recovery. Owner UI acceptance remains
  necessary to claim physical Sources behavior; do not infer it from instrumentation.
- Do not publish 0.4.11 or modify published 0.4.9 artifacts without authorization.

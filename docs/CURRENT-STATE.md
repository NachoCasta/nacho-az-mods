# Current state

## Active development: automatic hub recovery after cold boot

The owner reports that a hub connected before power-on needs reconnection.
Read-only physical boot logs confirm the boundary: the4.4 kernel rejects the
external hub before azhub loads its guarded depth patch; the later hotplug
succeeds. Existing block-device scans cannot recover a hub that has no driver.
A startup helper now retries only unbound external hub interfaces on the known
USB controllers, with three bounded SDK worker passes and the existing helper
timeout/fault policy. It never unbinds or resets an already-working device.
All105 portable components pass plain/ASan+UBSan/TSan, the SDK UI harness passes,
and17 private-sysfs helper cases cover the target/skip policy. The model proves
baseline failure with an unbound hub and candidate recovery without hub hotplug.
A fresh candidate cold boot restores FAT/HFS+ sources, waveforms and loaded
eject/cache continuity; child reconnect and Link On/Off/On controls pass.
The unpublished0.4.7 hardware package validates and is installed, exactly approved
and active through the normal framework workflow on the physical AZ. Its mapped
runtime hash matches the candidate; already-bound hubs and Native USB Names remain
available after the warm reload. Two real cold-boot acceptance cycles remain
planned. The first real cold boot now passes: rejection at0.75s, guarded patch
at10.05s and hub discovery at13.19s without USB disconnect; native USB1 and both
FAT/exFAT hub sources are mounted. The owner reports success and authorized
publication after this first cycle; the second repetition remains pending. See the
[goal](HUB-COLD-BOOT-GOAL.md) and
[progress evidence](acceptance/usb-hub-support-cold-boot-progress.json), plus the
[candidate package](acceptance/usb-hub-support-cold-boot-package.json) and
[physical installation](acceptance/usb-hub-support-cold-boot-physical.json).

## Published: USB Hub Support 0.4.6 (HFS+)

The private reduced HFS+ fixture uses `.PIONEER`. Filesystem-aware discovery,
settings and public PDB paths restore admission. Native private mounts also need
the Mac-layout flag; waveform analysis needs a guarded per-owned-repository root
adapter rather than the native USB1 filesystem reference. The current exact
emulator candidate loads populated samples and renders detailed/overview graphs
alongside a FAT hub neighbor. Native Link On/Off/On retains both loaded sources,
both graphs and advancing HFS playback state with the same process. No audio
signal was captured. Complete plain/ASan+UBSan/TSan component checks and the SDK
UI contract harness pass. A fresh framework session passes loaded
HFS+ eject, backing/alias mount removal, retained cache/engine advance, reconnect
and fresh deck2 waveforms. The initial long-session selector refusal remains
recorded. A native FAT load/playback control also passes alongside HFS+ hub
waveforms. Cold startup, nested playlists and populated playlist tracks pass;
the native HFS+ checker delayed startup until it declared the dirty overlay OK.
An exFAT-FUSE control also loads a populated sample and renders both graphs while
HFS+ remains loaded; its playhead advances with the same process. That small
whole-device emulator fixture was manually mounted, not an automatic-mount test.
The owner no longer has favlab and explicitly accepts the emulator evidence for
this stage, deferring original-USB physical acceptance. After the owner powered
on the AZ, candidate0.4.6 was installed on the mods USB, approved under the exact
candidate exception and activated through the framework. Mapped runtime matches
the validated hardware candidate; PID20167, firmware/kernel unchanged, core1.2.5,
empty decks and loaded status1. The owner confirmed ordinary USB operation
and authorized publication. Original HFS+ physical acceptance remains deferred. See [physical installation](acceptance/usb-hub-support-hfsplus-physical-install.json).
The large public PDB reaches the existing
catalog capacity; external sharing is not validated for this fixture.
See the [goal](HUB-HFSPLUS-SUPPORT-GOAL.md) and
[progress evidence](acceptance/usb-hub-support-hfsplus-progress.json).
[Release0.4.6](https://github.com/NachoCasta/nacho-az-mods/releases/tag/usb-hub-support-0.4.6)
is published and the root catalog now lists it. The exact accepted hardware-target
package validates with pinned SDK1.2.3;
helpers/kernel assets remain byte-identical to0.4.5. See the
[candidate package receipt](acceptance/usb-hub-support-hfsplus-package.json).
Source CI passed both jobs. See the
[publication record](acceptance/usb-hub-support-0.4.6-publication.json) for exact
hashes and anonymous-download verification. Previous releases and tags were not changed.

## Published: Native USB Names 0.1.0

Independent mod `nachocasta-native-usb-names` preserves valid rekordbox device names
exactly and uses the filesystem volume label when the configured name is absent,
empty or invalid. Otherwise it preserves the native generic fallback. Presentation
only; no disk renaming, library mounts, routing changes or hub dependency.
Exact firmware tag `ep147-9e9454e357389ece`, kernel 4.4.194, pinned SDK 1.2.3.

[Release 0.1.0](https://github.com/NachoCasta/nacho-az-mods/releases/tag/nachocasta-native-usb-names-0.1.0)
is published and listed in the root catalog after user-reported physical success.
The loaded hardware executable hash was verified; the release preserves those exact
bytes and expands the description. Host sanitizer suites and exact-runtime emulator
standalone/coexistence waveform, naming and source-reuse controls passed. Detailed
physical waveform/audio, Unicode, standalone, reinsertion and rollback acceptance
remain unclaimed. See [release notes](releases/nachocasta-native-usb-names-0.1.0.md),
[acceptance](NATIVE-USB-NAMES-ACCEPTANCE.md), [goal](NATIVE-USB-NAMES-GOAL.md)
and [native hook research](NATIVE-USB-NAMES-RESEARCH.md).

## Published: USB Hub Support 0.4.5

[Release 0.4.5](https://github.com/NachoCasta/nacho-az-mods/releases/tag/usb-hub-support-0.4.5)
is available through the root catalog. It includes port-based USB numbering,
numeric source ordering, rekordbox names/colors, native badge alignment and the
accepted waveform restoration. Two selected emulator Source/eject previews are
published as release assets for screenshot-aware AZ-MODS clients.

The release executable matches the physically accepted candidate byte for byte.
The owner confirmed presentation and restored waveforms; audio/rendering remain
user-reported. Both CI jobs passed after a test-only GCC compiler selection fix;
the release tag was not moved. Anonymous catalog/package/image downloads passed.
See the [publication record](acceptance/usb-hub-support-0.4.5-publication.json)
and [release notes](releases/usb-hub-support-0.4.5.md) for hashes and limitations.
Actual-CDJ interoperability, Wi-Fi sharing, phase alignment, eight-source hardware
and long-session acceptance remain open. Earlier unpublished-candidate entries
below describe their historical state, not current catalog availability.

## Completed development: hub waveform regression

The user confirmed that the corrected physical0.4.5 candidate restores waveforms
after the authorized USB install, exact approval and normal framework reload.
This is user-reported manual acceptance, not agent-instrumented rendering/audio.
The original presentation candidate loaded/played tracks but lacked waveforms.
Previous source-presentation checks validated labels and playback state, not
rendered waveform content; they do not prove waveform visibility. The completed
[waveform goal](HUB-WAVEFORM-REGRESSION-GOAL.md) adds actual graph assertions and
preserves its native positive/blank-hub negative controls. See the
[completion audit](acceptance/usb-hub-support-waveform-completion.json).
Earlier presentation evidence below retains its original, narrower scope.

Diagnostic A/B now isolates premature native mount initialization: preparing the
alias alone does not help, while preserving the media-query scheduled mount
restores all five native analysis-path resolutions and both graphs for Algo más.
Exact emulator diagnostic `4f56bb1b24ff38218355c1f6b1ed996567778727404ff3dc8092e2fa6d54adb8`
passes the pixel visibility check and restores Browse previews. It still exposes
fallback names/default colors in Source, so it is not the final fix. Preserve
configured metadata while separating its acquisition from early analysis setup.
See [diagnostic comparison](acceptance/usb-hub-support-waveform-alias-ab.json).

The subsequent two-phase diagnostic preserves configured names/colors and source
order while restoring Browse previews and both deck graphs. It retires the
pre-announcement metadata mount and mounts again after native registration;
the comparison is recorded in [retirement evidence](acceptance/usb-hub-support-waveform-metadata-retirement.json).
Production code now includes admission-protected retirement and lifecycle resets;
the first non-diagnostic build restores both graphs but exposed a first-response
metadata race. Remount preparation now preserves complete same-source metadata;
the corrected exact runtime now passes the waveform and presentation controls. All103 portable components
pass in plain, ASan/UBSan and TSan modes, with separate actual queue/announcement
controls and preserved environment failures. See [host checks](acceptance/usb-hub-support-waveform-host-checks.json)
and [production progress](acceptance/usb-hub-support-waveform-production-progress.json).

Corrected non-diagnostic emulator runtime
`b98896db19708b4a3eddecbab51555ae15a7dbad223b11009e07de65a26b672c`
shows both graphs and configured names/colors in native-first numeric order.
Native Link On/Off/On retains the loaded source and both graphs with the same
process and advancing engine. A fresh session passes own-source loaded eject,
mount/alias removal, retained completed cache ticket and continued engine advance;
reconnection restores USB1 and configured metadata. A new load after reconnect
into deck2 again passes both graph assertions. These are emulator observations,
without captured audio or actual CDJs. The first eject attempt refused input
after the core's cumulative200us frame-filter limit; normal reload with empty
decks restored input. Core limits were not changed.

The unpublished0.4.5 hardware-target package validates; only its manifest and
runtime differ from the preceding presentation package. Kernel assets/helper,
stable ID and exact compatibility remain unchanged. See
[package receipt](acceptance/usb-hub-support-waveform-package.json).
Network-client regression passes five complete libraries and12 exact file reads,
with unchanged process, clean transport and namespace removal. The existing6.6
emulator macvlan test fixture was restored after the initial attempt stopped
before clients because that fixture was absent. This fixture is not part of the4.4
hardware package. See [network evidence](acceptance/usb-hub-support-waveform-network.json).
After the user's reload instruction, physical preflight confirmed empty decks.
The corrected0.4.5 package is installed and approved, and the normal framework
reload completed: PID3412453, exact mapped hardware runtime
`6ec384330f43f5126e7d70492727f4d42c6e120dd55a9c856f5226c49812c904`.
USB rollback and approval backups exist; no other manual internal edits occurred.
See [installation evidence](acceptance/usb-hub-support-waveform-physical-install.json).
The user replied "it works" to the request to check both physical waveform views.
No eight-source, long-session or actual-CDJ hardware acceptance is inferred.
Published0.4.4 and catalog remain unchanged; public push approval remains separate.

## Active development: hub source presentation

The user approved the emulator presentation and explicitly requested physical
installation, approval and reload. The local0.4.5 candidate is now active on the
real AZ: exact mapped hardware runtime SHA256
`91b470fbd3cd8458a83ea5dedda3b0662fdf45525105ec577981ac1884af968e`,
EP147 PID3139484, firmware tag/kernel/core unchanged and decks empty. USB rollback
and approval backups exist; approval preceded the application restart. This is
installation evidence, not functional hardware acceptance. User testing is pending.
See [physical installation](acceptance/usb-hub-support-source-presentation-045-physical-install.json).
Published0.4.4 and catalog remain unchanged.

The current emulator candidate is
`6cc52cacba0861282e37807297c993f61e708d282c399e0e094f1e7bce50d270`.
It retains physical-port numbering and Source numeral alignment, orders hub rows
numerically after native sources, and advertises each library's native configured
name and device color. Names fall back to volume labels and then USB N. Native
Waveform uses its existing neutral H/USB label; device color is distinct from the
waveform setting. See [goal](HUB-SOURCE-PRESENTATION-GOAL.md),
[ordering](HUB-SOURCE-ORDER-RESEARCH.md), [colors](HUB-SOURCE-COLOR-RESEARCH.md)
and [metadata research](HUB-SOURCE-PROPERTIES-RESEARCH.md).

Eight independent native server pairs are prepared during initialization; dormant
slots open no database. Initial five-source names/colors and a valid USB arriving
later into previously unused nested USB8 pass. Source settings live-color and
Browse stripe controls pass on the preceding metadata candidate, which shares
the presentation code. Portable configured-name case/accent/fallback checks pass;
earlier emulator fixtures cover volume and unnamed fallbacks. All 103 portable
components pass in plain, ASan/UBSan and TSan modes.

The current candidate passes five independent complete libraries plus 12 exact
native/hub file reads. Own-source loaded eject removes backing and alias mounts,
retains four neighbors and the loaded cache ticket, and keeps the core engine
advancing. Replug restores number/name/color. Native Link On/Off/On retains the
cache-backed track and Source presentation; post-cycle complete menus and six
exact files pass. A native USB safely removed and reinserted after all five hub
libraries returns ahead of the hub group, with the same PID and advancing deck.
These are emulator/software-client results, without audio capture or actual CDJs.
See [late-slot](acceptance/usb-hub-support-source-presentation-late-slot-emulator.json),
[network](acceptance/usb-hub-support-source-presentation-capacity-network-emulator.json),
[eject](acceptance/usb-hub-support-source-presentation-capacity-eject-emulator.json)
and [mode-cycle evidence](acceptance/usb-hub-support-source-presentation-capacity-mode-emulator.json).

Failed attempts remain recorded. The independent network checker now indexes
its local PDB oracle before opening a connection with a bounded handshake.
An earlier emulator session disabled its frame filter after eight calls exceeded
the core's 200 us limit. The selector refused unavailable input; a normal native
reload restored it and the eject repeat passed. The two mixer filter callbacks
perform bounded arithmetic/atomic edits without metadata, I/O, locks or allocation.
The core measures elapsed counter time with cumulative strikes; host scheduling
is a possible contributor, not a proven cause or a physical timing pass.

The hardware-target local0.4.5 package validates, retaining exact ID, firmware,
kernel and core requirements and unchanged kernel assets. Its ELF differs from
the tested emulator build by explicit target flags; it is not physically accepted.
See [package receipt](acceptance/usb-hub-support-source-presentation-capacity-package.json)
and [remaining-work audit](acceptance/usb-hub-support-source-presentation-progress.json).
Local code, controls and documentation are committed; approved public push remains
pending. The user subsequently approved screenshots and installed the physical
candidate as recorded above. Published0.4.4, release tags/assets and catalog are unchanged.
The current user instructions authorize the bounded emulator-first USB/framework
testing workflow in AZ-TESTING-POLICY.md without repeated per-action questions.
Screenshot review still precedes physical validation for this goal. No release
publication decision covers this work.

The earlier numbering-only candidate below is superseded by the combined source
presentation candidate above; its evidence remains scoped to its recorded bytes.

Unpublished changes assign USB 1-8 from hub-port topology and move Source-only
numerals 3-8 down one pixel. Real emulator detach/move/return and fresh nested-hub
UI controls, final own-source loaded eject/public clients and cross-view callback
controls pass. The local0.4.5 package validates; visual review, push approval and
separately authorized physical acceptance remain open.
Published 0.4.4 and the physical AZ are unchanged. See
[implementation notes](HUB-PORT-NUMBERING.md) and
[scoped emulator progress](acceptance/usb-hub-support-port-numbering-emulator-progress.json).


## Published release0.4.4

[USB Hub Support0.4.4](https://github.com/NachoCasta/nacho-az-mods/releases/tag/usb-hub-support-0.4.4)
is published and the root catalog points to its exact accepted package.
Source CI passed both jobs; anonymous catalog/package download validation and
source-manifest access passed. Source commit: `22f6b2ff1f416fa90146fc1a7441649b767b17b2`.
See [publication record](acceptance/usb-hub-support-0.4.4-publication.json),
[release notes](releases/usb-hub-support-0.4.4.md) and the completed
[goal audit](acceptance/usb-hub-support-sharing-044-completion-audit.json).
The physical native-GUI Off/On cycle passes. Pending-GUI/unpublished statements
in the frozen candidate reports below describe their earlier recording time.
Actual-CDJ interoperability and Wi-Fi sharing remain unvalidated.

## Physical Ethernet candidate0.4.4

The real AZ now runs the unpublished0.4.4 candidate without suspension. The user
reported local hub operation on the earlier0.4.0 runtime;0.4.4 retains the exact
same runtime ELF (`ccf15593...`). Current instrumented Ethernet controls cover one
hub stick: unchanged Beat Link DeviceFinder discovery, complete library menus
(3172 tracks,166 ordered nodes,11022 memberships), exact complete PDB and one real
MP3 download with unchanged Crate Digger. Native USB1 file access also passes.
These are software-client/file-transfer results, not actual-CDJ or rendered-audio
acceptance. See [physical evidence](acceptance/usb-hub-support-sharing-044-physical-ethernet.json).

Two physical startup/transport issues were isolated: IPv4 labels such as
eth0:avahi were excluded by exact-name enumeration; and secondary-MAC TCP delivery
needed an owned parent receive-mode reference on the tested AZ. A bounded physical
A/B control confirmed TCP works with that reference and fails again after release.
The helper now holds a protocol-zero packet-socket membership for its session;
normal close or process exit releases its own reference without overwriting NIC
flags. No manual driver/OS/settings patch is involved. Java clients on this
multi-interface Windows host require preferIPv4Stack=true for discovery. No
firewall rules were changed. Wi-Fi remains a separate unsupported candidate path.

Current-helper loaded-eject regression passes, as do97 portable tests per plain,
ASan/UBSan and TSan mode. The final current-helper mode cycle reached Off withdrawal
and fresh On readiness but recovered menus hit EOF and one MP3 handle was stale;
that first run remains failed. Both a validation-trace repeat and an exact normal
helper repeat now pass completely. An isolated VFAT inode-cache control does not
reproduce the failure; no speculative identity-guard change was made. Final physical
GUI Off/On observation now passes: same EP147 PID and retained native/private hub
mounts, public withdrawal and rediscovery, complete3172-track menus and exact
PDB/MP3 reads. This cycle does not assert physical loaded-deck playback. See
[mode-cycle evidence](acceptance/usb-hub-support-sharing-044-physical-mode-cycle.json). Exact hashes and rollback are recorded in the physical evidence. Builds
do not deploy implicitly; diagnostic candidate updates and normal framework
reloads were explicitly authorized, and every activation checked fresh empty decks.
No candidate release, tag or catalog update has been published.

## Historical software-candidate completion and research

## Active goal: share hub USBs with external Link players

## Authoritative candidate status (latest integrated evidence)

Current integrated automatic-controller network recovery now passes: unchanged
EP147 PID, fresh helper/token, complete independent menus and four exact PDB/MP3
reads after recovery, second graceful retirement, exact restoration and cleanup.
See [integrated network result](acceptance/usb-hub-support-sharing-final-integrated-network-recovery.json)
and [candidate handoff](HUB-LINK-CANDIDATE-HANDOFF.md). Physical trials remain separate.

The experimental sharing candidate passes the software/emulator controls below.
This supersedes pending statements in the chronological research notes below;
those notes retain failures and the reasons controls were corrected.

- Eight concurrent independent discovery/menu clients and exact PDB/MP3 files:
  [capacity result](acceptance/usb-hub-support-sharing-capacity-eight-final.json).
- Native shared-USB loaded withdrawal, with matching source/slot/track/file:
  [native reference](acceptance/usb-hub-support-sharing-native-paired-reference.json).
- Individual public withdrawal/reconnect with continuing synthetic external loaded
  status, retained neighbor sessions and stale old handles:
  [individual result](acceptance/usb-hub-support-sharing-individual-full-tuple.json).
- Whole-hub withdrawal during native USB2 playback and eight complete clients:
  [combined result](acceptance/usb-hub-support-sharing-whole-hub-full-tuple.json).
  Source41/COLLECTION4/track1884 matches the retained MP3; native admission,
  56 foreign-handle checks and native reads after withdrawal pass.
- Native two-On/four-Off/returned-two-On, continuity and log retention:
  [topology result](acceptance/usb-hub-support-sharing-topology-final.json).
- Portable/sanitizer tests, SDK UI checks, hooks and byte-identical target rebuild:
  [source checks](acceptance/usb-hub-support-sharing-final-source-checks.json).

These are independent software-client protocol and emulator engine observations,
not actual-CDJ interoperability or captured-audio acceptance. Target kernel4.4.194
MACVLAN loading and model-specific CDJ behavior require separately authorized
physical trials. SDK input-filter guards remain unchanged; emulator scheduling
can quarantine the short selector, while the separately hooked long STOP passes.
Outstanding-generation IPC/process faults refuse eject rather than infer safety.
Local private hub track identities remain redacted from outgoing deck status;
translation for instant doubles/current-deck metadata is not implemented.
No physical AZ was touched and no candidate was published during this work.

## Historical research updates (superseded where noted above)

Bounded whole-hub paired withdrawal now passes with diagnostic admission of the
correct track1884: eight independent discoveries/retained sessions, unique roots,
56 foreign-handle STALE checks, exact first MP3 block and continued loaded status.
Long STOP unmounts all eight hub roots and withdraws their services/discovery;
five exact native USB1/2 file reads still pass, with unchanged EP147 PID. Exact
restoration/owned cleanup pass. Full-capacity/native-USB2 late-selector combined
case remains failed: the SDK quarantines raw input after eight over200us calls.
A standalone real-callback timing control also measures overruns in an empty
callback (11 across two500000-call batches); raw/edit medians96ns and p99<1us
support an emulation timing/scheduling contribution, not physical timing proof.
No SDK guard is changed. Paired native reference and individual hub repeat remain
open. See the [whole-hub result](acceptance/usb-hub-support-sharing-whole-hub-paired.json),
[combined failure](acceptance/usb-hub-support-sharing-whole-hub-combined-partial.json)
and [timing control](acceptance/usb-hub-support-sharing-input-timing-control.json).

Loaded-file binding audit found a test mismatch: independent Crate Digger maps
the retained Aaliyah MP3 to rekordbox ID1884, while prior synthetic status used495
(the different Algo mas M4A). Prior controls prove source withdrawal/file closure
and status admission separately, not same-track pairing; five records now state
that boundary explicitly. Status clients and wire/store assertions now use1884;
an independent PDB-row/path verifier runs before future loaded-file controls.
Parser self-test, binding verification and syntax/Java compilation pass. The
running whole-hub case will load corrected classes before its status phase;
paired native baseline and individual hub repeats remain required. Local native
engine content495 controls are separate and unchanged. See the
[binding audit](acceptance/usb-hub-support-sharing-loaded-file-binding.json).

Latest non-diagnostic topology/mode/retention regression now passes with runtime
a036b5ae/helper b5ff9ab7: all-four sdc1 load proofs, eight samples per topology
phase,90/80 concurrent primary-engine observations during complete independent
two-source menus/files, reviewed returned-On secondary rejection and native
unload. Seeded old log families are pruned; exact restoration and owned cleanup
pass. Whole-hub/native-USB2 control is now running as the remaining live gate.
See the [final topology record](acceptance/usb-hub-support-sharing-topology-final.json).

Whole-hub sharing withdrawal is an additional open gate from the original plan.
`--integrated-whole-hub` is prepared with eight-source complete clients, retained
metadata/root sessions, unique handles and56 cross-source STALE checks. The
independent DeviceFinder watcher retains a real MP3 handle and continuing
synthetic source41 loaded status across long STOP; the parent requires all eight
hub mounts to vanish with native mounts/PID unchanged, then repeats exact native
USB1/2 file reads. Short-selector ownership is checked before long STOP without
bypassing SDK quarantine. Java compilation/Python syntax pass; no live whole-hub
sharing result is claimed. Run only after the current final topology control.

Latest non-diagnostic runtime/helper loaded-eject regression passes. Real MP3
handle and loaded-status packets precede native eject, continue through withdrawal
and reconnect (188 packets per destination), and require no manual external
unload. Original neighbor sessions survive; old MP3 READ handles are STALE70 on
both aliases. Exact restoration/owned cleanup pass. Native-store admission is
separately proven by the final diagnostic case, not claimed from this wire-only
control. Final latest-runtime topology/mode/retention repeat is now running.
See the [non-diagnostic record](acceptance/usb-hub-support-sharing-loaded-eject-final-runtime.json).

Latest helper/diagnostic runtime loaded-eject regression passes: native status
store accepts source41/track495 while a real MP3 handle is retained; native
selector withdraws and unmounts without manual external unload. Original neighbor
services remain available; reconnect rejects old MP3 READ handles as STALE70 on
both aliases.191 status packets per destination continue through the lifecycle.
Exact restoration/owned cleanup pass. Non-diagnostic runtime repeat is running;
no actual-CDJ/audio acceptance follows. See the
[final diagnostic record](acceptance/usb-hub-support-sharing-loaded-eject-final-diagnostic.json).

Final executable-asset helper b5ff9ab7/runtime a036b5ae now passes the complete
eight-source/native control: all eight complete independent menus, nineteen exact
hub/native file fetches, all-source renewals and117 concurrent native engine
observations, with fresh USB1 load/backing proof, native unload and exact
restoration/owned cleanup. This closes the packaged-helper capacity repeat;
loaded-eject and final topology regressions remain separate gates. See the
[final capacity record](acceptance/usb-hub-support-sharing-capacity-eight-final.json).

Current-source final portable checks pass96 tests each in plain/ASan+UBSan/TSan.
SDK UI passes10/8/8 checks (functional scenarios not measured), hook registration
passes, and target runtime/helper rebuilt from the source snapshot are byte
identical to the draft package inputs. See the
[source-check record](acceptance/usb-hub-support-sharing-final-source-checks.json).

Eight-source integrated capacity plus native fresh loading now passes in the
emulator: eight independent complete menu clients, sixteen exact hub PDB/MP3
downloads, three native USB file downloads and accepted renewals for all eight
sources. A fresh native USB1 UI load proves the native request identity and /C/
partition backing; engine positions advance before/during/after clients. Native
unload, global retirement, exact restoration and owned network cleanup pass.
Runtime a036b5ae is tested, but helper ab8773de predates the DHCP executable-asset
suffix fix; repeat against the final helper is still required. Additional owned
virtual media remain attached for subsequent controls. No captured audio or
physical acceptance follows. See the
[eight-source/native record](acceptance/usb-hub-support-sharing-capacity-eight-native.json).

A local draft0.4.0 package now validates with the pinned preserved SDK packer:
682234bytes, SHA2568edd7f78c89ca2aa75a0127c496e63acbfaf418bce0115f463da18d4607bd4c0.
It stages target runtime ccf15593, helper2efe0ca3, the existing hub-depth module,
target MACVLAN module and source/license/build material. Both executable assets
(`usb-hub-support-network-helper.so` and shell `public-dhcp-event-hook.so`) have
verified0755 archive modes without patching the SDK. The validator's existing
settings-partition warning resolves to the optional azhub.conf read. This is
draft packaging evidence only: latest helper, final integrated regressions,
target transport runtime and source snapshot/reproduction remain pending. No
catalog, published asset/tag or physical AZ change occurred. See the
[draft package record](acceptance/usb-hub-support-sharing-package-draft.json).

Candidate packaging review found an executable-mode mismatch: pinned azmod-pack
stores non-.so assets as0644, while SDK launch requires X_OK. The runtime and
managed harness now use `assets/usb-hub-support-network-helper.so`; the helper
is still the same executable, with no core/packer workaround or production asset chmod.
Emulator/target builds pass with hashes a036b5ae/ccf15593 respectively; earlier
b2a3ce43 runtime evidence predates this path-only change and final regressions
remain required. Eight virtual sources are now attached with owned receipts.
The first combined eight-source/native-load control stops before remote clients:
native LOAD succeeds but the harness incorrectly demands a synthetic player
alias for USB1. Its fixed control requires the exact native request identity,
`/C/` resolution and actual partition device identity. Syntax passes; repeat
with the new runtime is running. Package restoration/owned network cleanup pass
for the failed attempt. No eight-source acceptance is claimed yet.

Native file preservation now passes with the integrated emulator runtime:
independent Crate Digger clients fetch exact complete USB1 PDB/MP3 and the exact
candidate mod file from USB2 while hub clients browse/fetch. Supported post-start
re-announcement registers EP147 native exports; global retirement, restoration
and owned cleanup pass. Fresh native UI load/playback remains a separate pending
control. See the [native file record](acceptance/usb-hub-support-sharing-native-files.json).

Five-source integrated capacity now passes: one independent DeviceFinder and
five concurrent complete menu clients, ten exact PDB/MP3 downloads, accepted
same-generation renewal for all five sources, global retirement and exact
restoration/cleanup (exit0). This supersedes the earlier two-source full-client
limit, but is not eight-source or actual-CDJ acceptance. See the
[five-source record](acceptance/usb-hub-support-sharing-capacity-five.json).

The complete non-diagnostic native topology control now passes. One EP147 PID
retains primary playback through On/two decks, Off/four decks and returned On;
all four loads prove sdc1 backing. Eight advancing engine samples pass for each
topology phase, with80/75 additional observations during independent complete
two-source menus and exact PDB/MP3 clients before/after. A reviewed retained
Browse screenshot precedes rejected LOAD3/4 attempts in returned Link On. Native
unload assertions, old-log retention, global retirement, exact package restoration
and owned cleanup pass (runner exit0). No captured audio, native fresh-source,
eight-source or actual-CDJ acceptance follows. See the
[complete topology record](acceptance/usb-hub-support-sharing-local-deck-topology.json).
The earlier attempts below remain historical evidence, not current completion gaps.

The extended native topology run now proves engine continuity through returned
Link On: eight samples each for initial two-On, four-Off, secondary native unload
and returned two-On;81/75 additional samples keep primary decks loaded/playing
while initial/recovered complete two-source clients pass. All four loads verify
sdc1 and one EP147 PID is retained. Complete control still stops before final
secondary-load rejection/unload: SDK quarantines the edited frame filter after
eight over200us calls under emulation, and the eject selector correctly refuses
encoder-calibration ownership. No guard/core change is made. Final control now
reopens retained native Browse without selector calibration and requires a saved
selected-track review. Syntax passes; live repeat pending. Package restoration/
cleanup pass. Physical timing and long-session eject acceptance remain unproven.
See the [returned-mode evidence](acceptance/usb-hub-support-sharing-local-deck-topology-return.json).


The second native topology attempt proves initial two-On and four-Off engine
operation: all four actual native requests resolve to sdc1; each tested phase
has eight loaded/playing/content495 samples with advancing positions. Native
secondary unload clears3/4 while1/2 continue. Initial independent full two-source
menus/files also pass. The complete control still fails: recovered metadata
clients time out when the original600-second TAP/DHCP fixture lifetime ends,
before final returned-On secondary rejection/unload checks. Exact restoration/
cleanup pass. The topology-only fixture window is now1200seconds and primary
engine observation is added during remote client waits; individual operation
limits and mod behavior are unchanged. Syntax passes, live repeat pending. See
[partial topology evidence](acceptance/usb-hub-support-sharing-local-deck-topology-partial.json).


A native four-Off/two-On control is prepared for the non-diagnostic integrated
runtime, using real LOAD1..4, physical bank selection, native unload dialogs,
per-deck content/playing/position telemetry and actual request-to-sdc1 bind proof.
The first live attempt verifies deck1 content495/sdc1, but deck2 remains unloaded
and no second native load request appears. It stops before Link Off/four-deck
operation; exact package restoration and owned cleanup pass. The control now
re-enters source/track selection between loads and saves per-load states/screens;
Python syntax passes and a live repeat is pending. This is an automation finding,
not proof of four-deck failure or acceptance. See the
[topology preparation record](acceptance/usb-hub-support-sharing-local-deck-topology-preparation.json).


Non-diagnostic runtime with log retention now passes integrated native Link
On/Off/On. Native UI confirmations withdraw old endpoints, retire the old helper
and restart sharing with a new PID/IPC session while EP147 and native/hub mounts
remain. Complete independent two-source menus and exact PDB/MP3 pass before and
after. Two old test log families (four files, year2000 mtime) are seeded in the
emulator mod data folder; next launch removes all four and retains exactly the
previous/current families. Final global retirement, package restoration and
owned cleanup pass. Runtime SHA256 starts b2a3ce437cddee95. No loaded local-deck,
native fresh-file or actual-CDJ acceptance is implied. See the
[live retention/mode record](acceptance/usb-hub-support-sharing-log-retention-mode-cycle.json).


Public helper logs now have session retention: before launching a new helper,
keep the newest existing owned log family and remove older strict-name regular
single-link families through a pinned SDK data-directory FD. The SDK caps and
rotates each family once; normal cycles retain only previous/current families.
Unknown files, links and directories are preserved; cleanup failure defers the
public launch. Forty simulated cycles and boundary/error controls pass. Full
portable suite passes96 tests per normal/ASan+UBSan/TSan mode, hook registration
passes under WSL, and AArch64 emulator/target runtime builds pass. These binaries
supersede prior runtime hashes; live retention/mode-cycle now passes as recorded above; final candidate
regressions remain pending. No physical AZ or publication occurred. See the
[retention record](acceptance/usb-hub-support-sharing-helper-log-retention.json).


Integrated public-hub loaded withdrawal now passes. Shared-listener software
client retains an actual MP3 handle with an exact1024-byte first READ while
player3 sends180 source41/slot3/track495 loaded/playing frames to each of the
provider and native AZ; the native store returns accepted=1. Eject retires before
source close/unmount, drains RPC/metadata/query endpoints and expires discovery.
Neighbor original RPC/TCP menus and native/other hub mounts remain available.
Replug yields generation2/new root, STALE old root and MP3 READ handles on both
RPC aliases and an exact fresh PDB. Final global retirement/helper exit, package
restoration and owned cleanup pass. No manual external stop/unload is required.
This is independent protocol evidence with a diagnostic emulator mod, not real
external audio/cache, actual-CDJ interoperability or final package acceptance.
The exact reference MP3 also passes strict FFmpeg decode to null; its hash equals
the completed client downloads, but no direct external audio output is claimed.
See the [loaded hub eject record](acceptance/usb-hub-support-sharing-integrated-remote-loaded-eject.json).
Next close final candidate topology/capacity/resource/artifact gates in the
[readiness checklist](HUB-LINK-CANDIDATE-READINESS.md). Earlier harness failures
are preserved in the preparation record and superseded by this passing control.

Native shared-USB loaded withdrawal now passes in the emulator. Verified fixture
USB1/sdb1 receives ordinary STOP1 while external player3 continues announcing
firmware-accepted source1/slot3/track495 loaded/playing status. Backing unmount is
observed about2.87seconds after the request, the native first export is cleared
while the mods export remains, and the retained NFS handle returns eight STALE70
responses after an exact1024-byte initial read. Loaded status continues beyond
the final STALE. Mods USB and all five hub mounts stay intact; the owned virtual
fixture is replugged, database hash/mount snapshot return, and native PID/modules/
network plus exact package restoration/cleanup pass. This establishes the native
reference without manual external unload, not external audio/cache or actual-CDJ
acceptance. The integrated public-hub comparison is recorded above. See the
[native loaded-eject record](acceptance/usb-hub-support-sharing-native-loaded-eject.json).


The corrected native joint control now passes. Supported AZ-MODS post-start
re-announcement registers both native roots;300 synthetic player3 loaded/playing
frames reach the firmware, and its actual store accepts source1/slot3/track495.
A separate NFS process retains one/C/ handle and reads nine1024-byte blocks while
status continues; all9216 bytes match the exact reference by per-block hash.
Native snapshot, exact original package restoration and owned cleanup pass.
No eject/audio renderer/physical AZ or actual-CDJ acceptance is claimed. Next
perform verified native STOP1 with continuing status/held handle, preserve mods
sda1 and hub roots, observe withdrawal/STALE and restore the owned native fixture.
See the [joint control record](acceptance/usb-hub-support-sharing-native-loaded-handle.json).
The post-start timeout and empty-root findings below are superseded by this
successful control; older integrated runs still do not establish native file
export preservation through their custom startup path.


The joint native loaded-status/file-handle control exposed a harness startup
omission: the normal EP147 unit invokes `az-usbload --post-start` to re-announce
already-mounted native media, but the custom diagnostic unit did not. Read-only
inspection finds native NFS root lengths0/0/0 there, versus15/15/0 and sdb1/sda1
roots in the stable normal process. Bounded MOUNT retries still return13; no
eject occurs. A separate stable-process control accepts/C/ UTF16LE unchanged,
refuses alternative encodings/paths, and preserves its native snapshot/cleanup.
The diagnostic harness now invokes the supported post-start operation and saves
its log; live validation of restored roots and the joint handle remains pending.
No export permissions, controller files or native tables were manually changed.
The first corrected run timed out at the harness's20-second SSH limit while the
official post-start operation was still executing; package/scope restoration
completed. Subsequent process inspection found no remaining post-start process
and its log ended with done. The harness now allows90seconds for that specific
operation and asserts both native root registrations before a held-file client;
that revision still needs a live run. Read-only topology inspection identifies
native fixture sdb1 on usb1/1-1 (QEMU nativecontrol2, ehci.0 port1) and mods sda1
on usb2/2-1. Do not infer physical native port from block-device enumeration.

See the [observer/investigation record](acceptance/usb-hub-support-sharing-native-status-observer.json).


Native loaded-status admission is now proven in the emulator for the synthetic
external player3: the native connecting event identifies192.0.2.2, receiver fields
identify USB source1/slot3/track495, and the actual store returns accepted=1.
The diagnostic hooks the validated body at0x10d324c before its original STP frame;
early null/key rejection stays native and all eight argument/live temporary
registers are forwarded. Portable controls pass normal/ASan+UBSan/TSan and the
isolated live run sends300 matching frames. Original bytes, native snapshot and
owned cleanup pass; no physical AZ was touched. Earlier failed entry-hook and
unobserved wrapper trials remain explicitly invalid/inconclusive in the
[observer record](acceptance/usb-hub-support-sharing-native-status-observer.json).
The first joint loaded-status/held-handle attempt fails before obtaining the
handle: native MOUNT returns13 for/B/,/C/,/D/. Guest aliases resolve B/D to
sda1 and C to sdb1; alias existence does not establish export authorization.
No eject was attempted and exact normal restoration/owned cleanup pass. The
held-handle control now accepts bounded sample/interval options and preserves
explicit MOUNT status in future failure logs. Resolve native mount admission
before loaded shared-USB withdrawal, then repeat for a public hub source. This is not an audio renderer,
external-loaded eject or actual-CDJ acceptance. The
[physical trial plan](HUB-LINK-PHYSICAL-TRIALS.md) defines per-model gates and
explicitly treats all proposed model/firmware combinations as unconfirmed.

A bounded synthetic loaded-deck control now builds status frames from unchanged
Beat Link8.0.0 framing/template and validates loaded/playing USB track495 with its
independent parser for native1/2 and public41..48. Initial namespace execution is
refused. Isolated live controls emit300 matching player3/source1 packets per60s;
native PID/modules/mounts/network/loaded-mod snapshot remain unchanged and cleanup
passes. This is wire stimulus only: waveform screenshots retain LINK PLAYER
placeholders, so native loaded-track admission remains unproven. No NFS read,
eject, audio renderer or real CDJ was involved. The next external-loaded eject
control must prove native admission/source mapping and pair this stimulus with a
real held file handle; the older native held-handle baseline is separate. See the
[wire control record](acceptance/usb-hub-support-sharing-loaded-status-wire.json).

Target-helper MACVLAN asset loading is now integrated after parent IPC
authentication and before any allocation. It independently gates kernel4.4.194,
uses only sibling `assets/usb_hub_macvlan.ko` from the actual executable directory,
refuses symlink/nonregular files, uses no force/modprobe/system installation and
requires expected registration. The full portable suite passes95 tests per
normal/ASan+UBSan/TSan mode; AArch64 target/emulator helper builds pass. The target
helper exits126 on the6.6 emulator without module/namespace changes, and the
emulator helper hash is unchanged. No target module was loaded: kernel symbol/
ABI/runtime acceptance remains open for separately authorized physical trials.
No package or deployment occurred. See the
[asset component record](acceptance/usb-hub-support-sharing-macvlan-asset-component.json).

Integrated local/remote concurrency now passes with native Link On: verified
source0 backing, deck1 content495 four-beat loop, full independent two-source
menus/exact PDB/audio and real refresh.75 samples retain one EP147/content ID,
loaded/playing and advancing positions through native loaded-source eject/replug.
Neighbor original RPC/TCP sessions, generation2 rediscovery/fresh root/STALE old
handles/exact PDB, final retirement and complete restoration/cleanup pass. This
is native engine telemetry, not captured audio, full-track playback or an external
loaded deck. Four/two-deck topology, target4.4/module packaging, final capacity/
compatibility and actual-CDJ trials remain open. See the
[concurrency record](acceptance/usb-hub-support-sharing-integrated-local-remote.json).

Integrated native Link On→Off→On now passes in the emulator through Utility and
its real confirmation dialogs. Off persists0, native completion gates graceful
public withdrawal, retained external RPC/metadata/query endpoints close and
DeviceFinder expires the old source. Helper/namespace retirement completes while
EP147 PID and native/hub mounts remain. On persists1, starts a different helper
with fresh authenticated IPC, and both sources pass complete independent menus/
exact PDB/audio again. Final global retirement, package/network restoration and
cleanup pass. Full portable suite:94 tests per normal/ASan+UBSan/TSan. No loaded
local four/two-deck regression, old-handle recheck across helper restart, external
loaded deck, target4.4 or physical CDJ acceptance. See the
[mode-cycle record](acceptance/usb-hub-support-sharing-integrated-mode-cycle.json).

Integrated native eject/replug now passes without SDK probe activation: complete
initial two-source clients/refresh, native selector withdrawal/backing unmount,
original neighbor RPC/TCP continuity, generation2 rediscovery, fresh root handle,
old handles STALE on both aliases and exact fresh PDB. Final global retirement,
helper exit, network/package restoration and cleanup pass. The first attempt
stopped before clients with framework suspension and a stale-marker verifier
bug; normal Resume/Reload restored0.3.2, marker assignment now follows current
helper/parent verification, and the harness waits for a stable normal startup.
See the [integrated eject record](acceptance/usb-hub-support-sharing-integrated-eject.json).

Native Link-mode gating is now wired to confirmed mode-completion callbacks,
not the internal cache. Unknown/Off cannot start public sharing; Off latches a
single graceful STOP, including transport backpressure and On-during-drain.
The full portable suite passes94 tests per normal/ASan+UBSan/TSan mode, including
STOP backpressure/On-during-drain controls. Explicit AArch64 builds pass. This
change postdates the integrated eject binary and still needs live mode/startup/
regression validation. See the [mode component record](acceptance/usb-hub-support-sharing-runtime-mode-component.json).

The integrated controller now passes its first live emulator trial without SDK
probe variables, masks or completion markers. Native inventory starts all five
present hub roots; complete independent discovery/full menus/exact PDB/audio and
same-generation DHCP refresh are validated for slots0/4. The other three log
ACTIVE through emulator NAT DHCP but are not fully client-validated. A controlled
native network change retires all five, reports EMPTY and exits the helper before
scheduling restart; the test stops the unit before that restart. Namespace cleanup,
network/package restoration pass. No integrated native eject, local playback,
external loaded deck, Link-mode policy or target4.4 runtime acceptance yet. See the
[integrated runtime record](acceptance/usb-hub-support-sharing-integrated-runtime.json).

An integrated sharing controller now builds as explicit local runtime candidates
for emulator and target helper modes. It starts from native inventory without
probe environment variables, fixture masks, result markers or a service deadline;
authenticates helper PID/UID/executable/token; waits for SDK-confirmed helper exit
after graceful retirement before retrying; and retains outstanding native eject
gates on channel faults. Portable startup/resource deferral, missing-source0
allocation, authenticated eject retirement, SDK-exit ordering and broken-channel
controls pass normal/ASan+UBSan/TSan. This is not live acceptance: native Link-mode
policy, target MACVLAN module packaging, bounded recovery of outstanding crashed
sessions, user-visible failure state and final runtime regressions remain open.
The full portable suite passes94 tests per normal/ASan+UBSan/TSan mode. Exact
build hashes and scope are in the [component record](acceptance/usb-hub-support-sharing-runtime-component.json).
Default release builds are unchanged. No physical deployment is authorized by
these explicit build targets.

Managed IPv4LL-to-DHCP recovery now passes: after a fixture marker enables DHCP
for source0 only, periodic acquisition accepts a grant, retires generation1 with
authenticated EMPTY/ENETRESET and starts generation2 at192.0.2.3. Old RPC/metadata/
query endpoints withdraw and DeviceFinder expires the old address. Original
source4 RPC/TCP sessions survive while its DHCP remains withheld. Fresh complete
independent menus/exact PDB/audio, final STOP, network/package restoration and
owned cleanup pass. Together with the following conflict/eject records this
closes the three primary isolated IPv4LL lifecycle controls, not physical native
autoconfiguration, local playback or external loaded-deck acceptance. General
production activation, final regressions and packaging remain open. See the
[DHCP-return record](acceptance/usb-hub-support-sharing-sdk-link-local-dhcp-return.json).

Managed IPv4LL native eject/replug now passes: the native selector withdraws one
source, drains retained external RPC/metadata sessions, expires independent
DeviceFinder discovery and unmounts after managed retirement. Neighbor mounts
and original source4 RPC/TCP sessions survive. Virtual unplug/replug republishes
player41 at a new address; complete independent menus/exact PDB/audio pass again.
Final STOP, network/package restoration and owned cleanup pass. No external
loaded-deck or physical-CDJ acceptance is claimed; DHCP return remains open.
See the [eject record](acceptance/usb-hub-support-sharing-sdk-link-local-eject.json).

Managed IPv4LL conflict recovery now passes in the emulator: a foreign ARP claim
retires source0 with authenticated EMPTY/EADDRINUSE before generation2 receives
a different address. Source4 keeps its original RPC root/TCP menu session;
independent discovery/full menus/exact PDB/audio pass at the replacement address.
No DHCP DECLINE is observed by the isolated fixture. Final STOP, native network,
package restoration and owned cleanup pass. Retained retirement evidence avoids
losing intermediate statuses to receive batching; targeted inventory controls
pass normal/ASan+UBSan/TSan. Native eject/replug, old-address withdrawal observer,
DHCP return, external loaded deck and physical CDJ controls remain open. See the
[conflict recovery record](acceptance/usb-hub-support-sharing-sdk-link-local-conflict.json).

Two managed IPv4LL libraries now pass in the emulator without DHCP replies:
derived identities/native roots, independent discovery/full menus and exact
PDB/audio downloads from both sources. Each client performs a second distinct
DHCP transaction while its source stays published. Final STOP, helper/namespace
cleanup, removal of the owned temporary native link-local alias, native network
restoration and exact package restoration pass. The portable suite passes93
tests per normal/ASan+UBSan/TSan mode. Native eject/replug, IPv4LL conflict and
DHCP-return controls remain pending. This uses an explicit secondary link-local
test domain, not physical native autoconfiguration acceptance. See the
[managed link-local record](acceptance/usb-hub-support-sharing-sdk-link-local.json).

Live standalone IPv4LL transport now passes in the emulator: anonymous owned
namespace/MACVLAN, native no-offer, random candidate, ARP/configuration/readiness,
direct UDP roundtrip over169.254/16 without default route/gateway, address
retirement and explicit link deletion. Native PID/DNS/routes/DHCP-process state
and owned cleanup pass. Route inspection uses `/proc/thread-self/net/route`
inside the namespace-entered thread; the first attempt incorrectly inspected
process-self procfs and failed, then cleaned up. Managed broker/library/client
integration remains pending. See the
[link-local transport record](acceptance/usb-hub-support-sharing-link-local-transport.json).

IPv4LL allocator wiring is implemented but not live-validated: a candidate helper
opts in only when native eth0 already has an eligible link-local /16 address.
Qualified no-offer retains namespace ownership; unbiased sampling excludes native/
other-source reservations, then existing ARP/configuration gates apply. Link-local
conflicts use retained ACD backoff without DHCP DECLINE. Periodic supervised DHCP
attempts continue every60 seconds; a valid ACK requests ENETRESET retirement,
never live replacement. Candidate exclusion and periodic-attempt/transition
component controls pass all three host modes; AArch64 helper builds pass.
No live fallback, routing or conflict/recovery acceptance is claimed yet.

Native udhcpc no-offer admission now passes on the isolated emulator: three
DISCOVER packets, no server replies, deconfig→leasefail, normal exit1, child
reaped and pipe EOF. No address is applied. Native PID/DNS/routes/DHCP-process
snapshot and owned cleanup pass. See the
[native no-offer record](acceptance/usb-hub-support-sharing-native-no-offer.json).

The allocation supervisor can now explicitly retain its owned namespace/link
after that qualified terminal result and returns2, distinct from grant success1.
Default policy still rejects no-offer and closes ownership. Cancellation never
permits fallback. Synthetic terminal-boundary ownership controls pass all three
host modes; the allocator has not enabled this branch or configured IPv4LL yet.

DHCP no-offer admission is now distinct from generic failure: only complete
deconfig→leasefail records, normal exit1, reaped child and pipe EOF yield ENODATA
and the explicit no-offer predicate. Exit1 alone, missing/out-of-order records,
malformed output, exit0, cancellation and timeout cannot authorize fallback.
Real spawned-child component controls pass normal/ASan+UBSan/TSan. Confirmation
with the emulator's native udhcpc and allocator IPv4LL integration remain pending.

No-DHCP groundwork: explicit LINK_LOCAL ownership is distinct from DHCP grants
and has no synthetic expiry. Uniform candidate sampling, mandatory readiness and
retired-generation transitions pass normal/ASan+UBSan/TSan, alongside DHCP lease/
refresh regressions. Fallback admission, ongoing DHCP attempts, on-link routing
and live isolated-network tests are not wired yet. See the latest section of
`HUB-LINK-ADDRESS-OWNERSHIP.md`.

Graceful native-network recovery now passes in the explicit emulator SDK path:
old services withdraw and the old helper exits before a15-second configured
backoff and SDK re-launch with fresh PID/token/log. Both sources rediscover with
stable identities; independent complete menus and exact PDB/audio downloads pass
again for both. Final STOP, native network equality, exact restoration and
cleanup pass. A pending test STOP suppresses recovery. This is one graceful
network-change cycle, not crash/start-failure recovery or eject-during-backoff
runtime acceptance. Production activation remains pending. See the
[network recovery record](acceptance/usb-hub-support-sharing-sdk-network-recovery.json).

Controlled native-network change now passes in the emulator: adding an owned
secondary address causes graceful public-source retirement while keeping the
authenticated channel open through per-source EMPTY/global completion. The
controller waits for SDK child exit instead of stopping a helper mid-cleanup.
External old RPC/metadata/discovery withdrawal passes; native USB mounts survive,
owned namespaces/processes disappear and the temporary address is removed.
Native network equality, exact restoration and cleanup pass. V1 queried before
retirement confirmation and failed; V2 corrects that verifier ordering. Global
terminal-before-drain rejection and quiesce/no-republish controls pass all three
host modes. Automatic recovery after the change remains pending. See the
[network-change record](acceptance/usb-hub-support-sharing-sdk-native-network-change.json).

Candidate runtime positive regression passes through the separate exact-emulator
entry: native-subnet peer admission, immutable native-network monitoring,
urandom ACD jitter and SDK-owned helper lifetime. Reusable inventory/derived
identities, independent complete menus/exact PDB/audio, refresh, native eject/
replug, original neighbor continuity and selected rediscovery/STALE/fresh PDB
pass. Global STOP, native network equality, exact restoration and cleanup pass.
The portable suite passes92 tests per normal/ASan+UBSan/TSan mode. No native
address change was injected; physical4.4.194 execution, general activation and
actual-CDJ acceptance remain pending. See the
[candidate runtime record](acceptance/usb-hub-support-sharing-sdk-candidate-runtime.json).

Candidate helper development: a fresh native subnet snapshot is compared once
per second with the immutable service snapshot. A read failure or address/mask
change requests managed session loss and complete owned-source retirement;
worker peer policy is never mutated concurrently. Candidate ACD timing now uses
unbiased16-bit rejection sampling from an owned urandom descriptor; missing
entropy returns an invalid readiness sentinel and fails publication closed.
Snapshot-order/change and jitter-bound/error controls pass all three host modes,
and candidate/emulator AArch64 helper builds pass. Runtime network-change drain,
automatic controller recovery and candidate random-timing integration still
require emulator evidence. No physical AZ operation occurred.

An explicit local `sharing-broker-candidate` helper build now provides
`--broker-share` guarded by exact kernel4.4.194. It reuses the managed allocator
with dynamic root/identity admission, independently snapshots native eth0 IPv4
subnets for peer admission and removes the emulator's600-second lifetime limit.
SDK session loss/STOP still drains ownership. Emulator wrappers retain exact
6.6.138-dirty gating and bounded fixture lifetime. Peer subnet/native-address/
broadcast/multicast/link-local controls pass normal/ASan+UBSan/TSan and both
AArch64 helper variants build. No target execution or installation occurred.
Native-address change handling, ACD jitter, no-DHCP policy, target transport/module
packaging and production controller activation remain open; this build is not
yet a hardware-ready candidate.

The reusable inventory engine now passes through explicit emulator SDK launch/
IPC/shutdown and the native unmount gate. Derived identities/native roots,
independent helper admission, full independent menus/exact PDB/audio, refresh,
native eject/replug, original neighbor RPC/TCP continuity and selected
rediscovery/STALE/fresh PDB pass. Global STOP, native network equality, exact
restoration and cleanup pass. The portable suite passes90 tests per normal/
ASan+UBSan/TSan mode. Source0-absent startup remains component-level evidence;
the live runner still uses its explicit mask and bounded lifetime. General
production activation/target helper lifetime remain pending. See the
[inventory engine integration record](acceptance/usb-hub-support-sharing-sdk-inventory-engine.json).

Reusable controller inventory is implemented in `public-sharing-inventory.h`:
after authenticated HELLO it receives bounded status batches, withdraws absent/
reserved roots before allocation, derives declarations for any subset of eight
sources and isolates per-root/capacity failures. Source0 is not required. Failed
never-published roots release unused number reservations; active reservations
wait for EMPTY. Real seqpacket controls cover source0-absent startup, root
failure/neighbor isolation, eject/retirement/replug and lost-channel gate
retention in plain/ASan+UBSan/TSan. This component still needs SDK/native adapter
integration; the live native-inventory probe below is not using it yet.

Native-inventory root selection now passes the two-source managed emulator
runner with derived MACs and independent helper admission. The controller uses
existing local source slots/roots instead of selected fixture disk-letter roots.
Independent complete menus/exact files, refresh, native eject/replug, original
neighbor RPC/TCP continuity and rediscovery/STALE/fresh PDB pass. Global STOP,
native network equality, exact restoration and cleanup pass. The selected mask,
initial source0 requirement and bounded probe lifetime remain explicit test
constraints; arbitrary missing-source startup/production activation are not
established. See the
[native inventory record](acceptance/usb-hub-support-sharing-sdk-native-inventory.json).

Dynamic identities now pass the managed two-source emulator runner: controller
and helper independently read the parent MAC; helper enforces derived source
MAC/interface/label/player bounds plus live root/mount/hub topology. Isolated
DHCP serves the declared derived MACs. Independent full menus/exact files,
refresh, native eject/replug, original neighbor RPC/TCP continuity and selected
rediscovery/STALE/fresh PDB pass. Global STOP, native network equality, exact
restoration and cleanup pass. Source paths/selection remain explicit fixtures;
production activation and actual-CDJ acceptance remain open. Shared fixed C/
Python MAC vectors and transport/network controls also pass. See the
[dynamic identity record](acceptance/usb-hub-support-sharing-sdk-dynamic-identity.json).

Dynamic helper identity policy is available in the separate explicit
`sharing-broker-policy-emulator` build: the helper independently reads eth0's
Ethernet MAC, derives the source MAC, checks source interface/label and bounds
the public player to41..48, then applies live root/mount/topology admission.
It does not use fixed disk letters or accept the legacy bind alias. Portable
eight-source identity controls pass all three modes; AArch64 compilation passes.
The controller/DHCP fixture still need matching dynamic-identity wiring before
live acceptance. This remains exact-emulator-only and does not activate sharing
on the physical4.4.194 target or alter normal builds.

The generalized reconciliation now passes managed slot0 collision/recovery:
generation1 EMPTY/EEXIST precedes player42/generation2; original neighbor RPC/TCP
state survives, and independent full menus plus exact PDB/audio downloads pass
after reselection. Global STOP, native network equality, original-byte restoration
and cleanup pass. The complete portable suite passes88 tests in each normal/
ASan+UBSan/TSan mode. Eight-slot/exhaustion/eject precedence evidence remains
component-level, not all-slot runtime acceptance. See the
[generalized reconciliation record](acceptance/usb-hub-support-sharing-sdk-player-reconcile.json).

Post-recovery-trial generalization: the explicit SDK controller now uses
`public-player-reconcile.h` for all eight selected source slots. Reservation
release requires authenticated EMPTY, duplicate collision statuses are idempotent,
and ENOSPC leaves only that source waiting instead of stopping the helper.
Absence/eject takes precedence and retains a live generation's reservation until
retirement. All-slot selection, duplicate status, exhaustion/neighbor isolation
and release-order controls pass plain/ASan+UBSan/TSan; pinned AArch64 builds pass.
The managed41→42 trial below predates this refactor. Generalized runtime recovery
and exhausted-pool runtime controls remain pending.

Explicit managed slot0 player recovery now passes: foreign41 claim retires
generation1 to authenticated EMPTY/EEXIST before the controller reserves42 and
publishes generation2 with the same root/MAC/IP/label. Original neighbor RPC/TCP
state survives; unchanged Beat Link discovers42 and completes full menus, and
Crate Digger fetches the exact PDB. Global STOP, native network equality, exact
restoration and cleanup pass. This fixture path learns the occupied number from
the helper's confirmed collision, not a complete initial peer inventory. General
all-slot reselection, native eject precedence runtime, post-reselection audio
fetch and actual-CDJ acceptance remain open. See the
[player-recovery record](acceptance/usb-hub-support-sharing-sdk-player-recovery.json).

The allocator now refuses duplicate public player numbers among its pending/
active sources. Portable allocator controls pass all three modes; pinned
AArch64 mod/helper builds pass. The emulator helper accepts41..48 alternatives
while retaining the fixed fixture root/MAC/interface/label checks. Normal
activation remains unchanged; recovery requires AZHUB_SDK_PLAYER_RECOVERY=1.

Player discovery parsing now shares one implementation between the running UDP
provider and reservation observations. Complete kind2/6 claims require exact
datagram/embedded lengths, nonzero player and nonzero unicast MAC. The observer
excludes controller-owned MACs before updating foreign reservations. Parser,
provider loopback and actual source-worker collision controls pass in all three
normal/ASan+UBSan/TSan modes. Controller discovery reception, confirmed-retirement
release and replacement publication are still pending; this is not end-to-end
reselection acceptance. The preceding managed collision hashes predate this parser.

Public player reservation groundwork: `public-player-pool.h` tracks foreign
claims and eight controller-owned reservations in the emulator-exercised41..48
computer-identity range. Preferred slot numbers remain stable; an active
reservation cannot move or be stolen. After caller-confirmed retirement/release,
a free alternative may be reserved; exhaustion returns ENOSPC. Foreign claims
remain excluded for60 seconds and backward clocks fail closed. Portable
normal/ASan+UBSan/TSan controls and AArch64 compilation pass. Discovery parsing,
own-packet exclusion, authenticated retirement/release and inventory updates are
not yet wired to this component. This is not actual-CDJ range acceptance.

Managed player-number collision now passes: after both sources complete
independent full menus/exact files, an isolated TAP claim for player41 with a
foreign payload MAC retires source0 to generation1 EMPTY/EEXIST. No newer
generation status appears in the following two-second observation; component
controls establish the no-retry policy. The neighbor retains original RPC/TCP
state. Global STOP, native network equality, exact restoration and cleanup pass.
No replacement-number selection, native eject/replug or external loaded deck
was tested in this mode. See the
[player-collision record](acceptance/usb-hub-support-sharing-sdk-player-collision.json).

Player collision correction after the root-policy trial: the source worker now
reports EEXIST for a foreign MAC claiming its public player number, separating
that failure from allocator ACD/EADDRINUSE recovery. The inventory client leaves
that identity retired instead of reacquiring the same occupied number. A real
loopback discovery datagram through the running source worker, plus client
no-retry controls, pass plain/ASan+UBSan/TSan. Dynamic number selection/reselection
and managed multi-source collision injection remain pending; the prior runtime
hashes do not include this correction.

Dynamic root admission is now exercised inside the managed emulator helper,
in addition to fixture MAC/player policy: two mounted roots pass independent
complete menus/exact files, refresh, native eject/replug, neighbor original
RPC/TCP continuity and selected rediscovery/STALE/fresh PDB. Global STOP,
native network equality, exact restoration and cleanup pass. The legacy
single-source bind-alias fixture retains its separate verification path.
Production activation and dynamic public identity policy remain open. See the
[root-policy integration record](acceptance/usb-hub-support-sharing-sdk-root-policy.json).

The post-integration portable suite passes85 tests per plain/ASan+UBSan/TSan
mode, including source MAC and dynamic root policy. The separate pinned-SDK
hook-registration control passes. These checks do not replace final candidate
mode/loaded-deck/target-kernel regressions.

Dynamic root admission groundwork: `public-broker-root-policy.h` independently
reopens the declared root, compares its pinned device/inode, requires one exact
matching partition mount and checks nested USB sysfs topology using the existing
discovery predicate. A second reopen rejects path replacement during inspection.
Trusted helper policy supplies filesystem inventory paths; IPC does not. Mounted
hub root, wrong capability, ambiguous mount and direct topology controls pass
plain/ASan+UBSan/TSan. This is not yet wired into activation, does not authorize
public MAC/player identities and is not a general mount-race security proof.

Production identity groundwork: `public-source-mac.h` derives eight stable local
unicast MAC candidates from the controller parent MAC and a versioned domain.
Candidates do not depend on disk letters, mount order or a common fixture MAC;
their source bits are distinct. Portable normal/ASan+UBSan/TSan controls pass.
This utility is not yet wired into activation and does not prove global MAC
uniqueness; native/local admission and on-wire conflict handling remain required.
Public player-number policy, dynamic inventory authorization and target helper
lifetime/activation remain separate integration work.

Managed changed-address recovery now passes: slot0 retires generation1 with
ENETRESET and publishes generation2 at192.0.2.4 after an initial192.0.2.3 grant.
An independent old-address withdrawal observer passes, the neighbor's original
RPC/TCP session survives the transition, and independent complete menus plus
exact PDB/audio downloads pass at the new address. Global STOP, native network
equality, exact restoration and cleanup pass. No native eject/replug or external
loaded deck was exercised in this mode. See the
[changed-refresh record](acceptance/usb-hub-support-sharing-sdk-changed-refresh.json).

Post-trial development: changed DHCP configuration now retires with ENETRESET,
distinct from hard root/identity ESTALE. The inventory client may reacquire the
same pinned desired root only after authenticated EMPTY; a withdrawn desire
still suppresses recovery, including repeated EMPTY notifications. Component
ordering, hard-error refusal and eject precedence pass plain/ASan+UBSan/TSan;
the pinned AArch64 mod/helper build passes. Real changed-address recovery is
not yet validated. These new binaries postdate the pending-refresh trial hashes.

Native selected eject now passes while a real refresh DHCP client is pending:
the owned slot0 client is observed alive before eject and absent after backing
unmount. Slot4 retains its original RPC/TCP session; source0 rediscovery, stale
old handles and a fresh exact PDB download pass after virtual replug. Global STOP,
native network equality, exact restoration and cleanup pass. This does not
independently establish the client's terminating signal or external loaded-deck/
actual-CDJ behavior. See the
[pending-refresh eject record](acceptance/usb-hub-support-sharing-sdk-pending-refresh.json).

The refresh host suite completed83 tests in each plain/ASan+UBSan/TSan mode.
An additional targeted control passes in all three modes: a completed failed
refresh caps its retry at the original lease expiry, cannot extend ownership,
retires exactly at expiry and leaves an independent neighboring lease intact.
These are component controls, not managed DHCP-outage or pending-refresh eject
acceptance. The managed pending-refresh eject trial is recorded above.

Real same-configuration refresh now passes with60-second grants on two managed
emulator sources: accepted counts3/4 retain public/lease generation1 while independent
complete TCP menus and exact files continue. Native selected eject/replug and
original neighbor RPC/TCP continuity, fresh PDB, global STOP, native network checks,
exact restoration and cleanup pass. This uses fresh supervised same-MAC acquisition,
not RFC2131 unicast RENEWING. Changed-address recovery/expiry negatives remain open.
See the [managed refresh record](acceptance/usb-hub-support-sharing-sdk-refresh.json).

Positive two-source regression now passes on the same helper hash as both initial
IP exclusion negatives: independent parallel complete menus/exact PDB/audio,
neighbor session/handle survives native source eject/replug, selected rediscovery/
STALE/fresh PDB and global STOP pass. Native address/routes/DNS/namespace snapshots
match, exact restoration and cleanup complete. No local playback in this particular
run. See the [address-guard positive record](acceptance/usb-hub-support-sharing-sdk-address-guard-positive.json).

The managed native-IP grant rejection now passes: DHCP proposes the emulator's
native192.0.2.15, helper reports EADDRNOTAVAIL without ACTIVE/readiness marker,
then global STOP/managed exit and exact restoration/cleanup pass. Native address,
routes, DNS hash and namespace snapshots match before/after. Continuous native
address changes and the positive two-source regression remain pending. See the
[native grant record](acceptance/usb-hub-support-sharing-sdk-native-grant.json).

The managed duplicate-grant negative control passes: isolated DHCP grants the same
IP to distinct source0/4 MACs; source4 reaches EMPTY/EADDRNOTAVAIL without ACTIVE,
while source0 passes independent discovery/full menus/exact files. Global STOP,
original-byte restoration and owned cleanup pass. Native-IP rejection and the
positive two-source regression on this helper remain pending. See the
[duplicate grant record](acceptance/usb-hub-support-sharing-sdk-duplicate-grant.json).

Initial allocator grants now exclude native-controller IPv4 addresses and other
source reservations before ARP/configuration. Bounded native snapshot and duplicate
candidate controls pass plain/ASan+UBSan/TSan; AArch64 builds. This postdates the
successful two-source/local-loop runtime hashes and needs managed negative/positive
trials. Continuous native address changes and renewal remain pending. See
`HUB-LINK-ADDRESS-OWNERSHIP.md`.

Simultaneous native-deck and external-client use now passes the two-source emulator
runner:73 samples retain the same native PID/content495, loaded/playing flags and
advancing four-beat-loop position through remote menus/files and native loaded-source
eject/replug. Backing identity is independently verified as `sdc`; external source
and neighbor lifecycle checks, global STOP, exact restoration and cleanup pass.
This is engine-state telemetry, not audio-signal capture or external loaded-deck/
actual-CDJ acceptance. See the
[local/remote record](acceptance/usb-hub-support-sharing-sdk-local-remote.json).

Two managed native sources now pass the complete gated emulator runner: distinct
owned networks, independent concurrent full menus and exact PDB/audio downloads
from both. Native selector eject/replug of source0 preserves source4's original
RPC handle/TCP menu session. Source0 rediscovery, old-handle STALE and fresh exact
PDB fetch pass. Current-run predicates, per-slot/global STOP, exact restoration
and all cleanup pass. No local/remote simultaneous playback, external loaded deck,
production activation, target4.4 runtime or actual-CDJ claim. See the
[two-source record](acceptance/usb-hub-support-sharing-sdk-two-sources.json).

Managed native eject/replug now passes for one direct native root: independent
complete menus/exact files, external reference withdrawal, backing unmount and
neighbor mounts preserved, then generation2 with the same MAC/player/IP. Unchanged
Beat Link rediscovers it, repository RPC controls reject old handles as STALE on
both aliases and Crate Digger fetches the exact PDB again. Explicit link deletion
and address-finalization reentry fix are included. Exact rollback and cleanup pass.
Not production activation, multisource, external loaded deck or actual CDJ proof.
See the [managed replug record](acceptance/usb-hub-support-sharing-sdk-native-reconnect.json).

The corrected native-eject runner now passes end to end with an external metadata
session and RPC root handle retained during selector eject. Both RPC aliases
become unavailable, metadata/query closes and unchanged Beat Link DeviceFinder
expires the source. Helper retirement precedes native closure/backing unmount;
neighbor mounts survive. Exact original restoration and owned-scope cleanup pass
without recovery. No remote loaded deck was tested. See the
[observed eject record](acceptance/usb-hub-support-sharing-sdk-native-eject-observed.json).

The single-source managed native-eject trial now confirms helper retirement
before native source closure, alias/backing unmount and preservation of other
USB mounts, after independent full menus/exact files. No external loaded deck
was tested. Runner cleanup tripped on the already-unmounted alias; exact originals
were restored and a separate scoped recovery completed/verified remaining cleanup.
The runner fix awaits a fresh complete trial. See the
[native-eject record](acceptance/usb-hub-support-sharing-sdk-native-eject.json).

The inventory IPC client now retains desired root capabilities through channel
backpressure, consumes generations only after sending, orders replacement after
confirmed retirement and suppresses recovery when eject withdraws a source.
Stale statuses and channel loss cannot open an outstanding unmount gate.
Actual broker-session rejection/duplicate-EMPTY controls pass plain/ASan+UBSan/
TSan; pinned AArch64 build passes. SDK probe ALLOCATE now uses this component;
native inventory/eject wiring remains pending. See `HUB-LINK-BROKER.md`.

Managed initial-conflict recovery now passes in the emulator: correct-source
DECLINE, retained backoff (independently observed10.126 seconds), then a new
address with the same client/public identity. Independent Beat Link completes
2546 tracks,185 ordered nodes and8359 memberships; concurrent database/audio
downloads match exact hashes. Managed retirement, original-byte restoration and
owned-scope cleanup pass. This is one gated SDK fixture, not production activation,
ongoing conflict/renewal or physical-CDJ acceptance. See the
[recovery record](acceptance/usb-hub-support-sharing-sdk-conflict-recovery.json).

Managed initial ARP conflict now retires without publication and sends DECLINE
from the correct source namespace. Independent fixture receives zero-source
DECLINE with matching grant identity and observes no announcement/IPv4 use of
the disputed address. Managed exit/rollback/cleanup pass; targeted namespace/
readiness/allocator controls pass all host sanitizer modes. Automatic retry and
ongoing conflict/renewal remain pending. See the [managed conflict record](acceptance/usb-hub-support-sharing-sdk-arp-conflict.json).

Managed helper IPC loss during pending native DHCP now passes: fixture observes
DISCOVER without OFFER, runner identifies the live owned client, then closes
the peer. Client and managed/peer helper PIDs disappear without ACTIVE/library
publication. Exact originals are restored and owned network/module/bind/TAP/temp
state cleans up; normal EP147 is active. Not native eject/unmount or a loaded
remote/neighbor/renewal test. See the [pending DHCP cancellation record](acceptance/usb-hub-support-sharing-sdk-pending-dhcp-cancel.json).

Actual SDK-managed ALLOCATE over IPC now passes with helper-owned namespace/
MACVLAN/native DHCP/ARP/configuration and a real library: independent complete
menus and exact concurrent PDB/MP3 downloads, then real retirement and managed
exit. Runner supplies no guest network. Exact original files/approval/marker and
absent helper/hook assets are restored; bind/module/TAP/fixtures/temp state is
cleaned and normal EP147 active. One gated fixture with fixed identities/jitter;
production policy, refresh/conflicts, concurrency and loaded native eject remain
pending. See the [SDK owned-allocation record](acceptance/usb-hub-support-sharing-sdk-allocated-library.json).

The positive ALLOCATE callback path now passes with real owned allocation and
a hub library: native DHCP/ARP/configuration before ACTIVE, independent complete
menus and exact PDB/MP3 downloads, then real callback retirement/namespace release.
Native state and cleanup pass; portable77/77/77 passes. This is a direct dispatcher
call in standalone research, not SDK helper IPC. One fixture; SDK executable
integration, concurrency/loaded eject and network-policy gates remain pending.
See the [real callback library record](acceptance/usb-hub-support-sharing-broker-allocated-library.json).

Actual ALLOCATE callbacks now connect owned acquisition/readiness and endpoint/
address retirement. Eight pending cancellations pass targeted sanitizers and
pinned AArch64: creator threads join, no service publishes and FD count returns
to baseline. This cancels before DHCP/MACVLAN; positive callback readiness and
SDK executable wiring remain unvalidated. Removal faults retain ownership.
See the [allocator callback record](acceptance/usb-hub-support-sharing-broker-allocator.json).

A distinct ALLOCATE/root-FD command now defers publication until asynchronous
network readiness and polls readiness through ACTIVE. Pending detach releases
the root and waits for network cleanup; path replacement and later readiness
loss controls pass targeted sanitizers/pinned AArch64. Mod/helper builds pass.
Callbacks remain fixtures here; real DHCP/ARP executable wiring and SDK ALLOCATE
trial remain pending. See the [allocation command record](acceptance/usb-hub-support-sharing-broker-allocate.json).

The automatically allocated network now serves one real mounted hub library:
native DHCP/ARP/configured READY precedes independent discovery, all2546 tracks/
185 nodes/8359 memberships and concurrent exact PDB/MP3 downloads. Withdrawal/
join/endpoint closure precedes quiesce and exact address retirement; native state
and complete cleanup pass. Portable76/76/76 passes. The first marker-format
attempt cleaned up; corrected v2 passes. This remains standalone research, with
SDK allocation operation, concurrency, conflict/refresh and remote-loaded eject
pending. See the [allocated real-library record](acceptance/usb-hub-support-sharing-allocated-library.json).

Automatic allocation→native DHCP→controller ARP→async configuration→READY now
passes in the emulator, including independent UDP reachability and exact address
retirement. Native state and owned-resource cleanup pass. Host sanitizer controls
cover wrong-namespace/backward-clock retirement and joining pending factory.
Libraries/SDK allocator integration, integrated conflict/retry and refresh remain
pending; the fixture uses fixed jitter. See the [automatic readiness record](acceptance/usb-hub-support-sharing-allocated-ready.json).

The namespace-bound ARP factory now passes a pinned AArch64 emulator control:
controller takes a validated nonblocking CLOEXEC source socket and submits a
candidate probe while remaining in its namespace. Configure/remove regression
and native/module cleanup pass. Targeted host sanitizers cover refusal/failure
cleanup. This is one kernel-accepted send, not full ARP/DHCP readiness or wire
observation. See the [ARP factory record](acceptance/usb-hub-support-sharing-arp-namespace-factory.json).

Exclusive async IPv4 configuration now passes a real allocated-namespace/MACVLAN
control: kernel ACK and address observation, EADDRINUSE on reconfigure, then exact
removal and absent-IP observation. Native state survives and owned scope/module
cleans up. Full portable74/74/74 passes. This operation test does not run DHCP/
ARP/services; acquisition→ARP→configuration and SDK broker readiness integration
remain pending. See the [configure record](acceptance/usb-hub-support-sharing-network-configure.json).

A standalone controller now creates its own anonymous namespace/MACVLAN and
obtains a real supervised native DHCP OFFER/ACK in the emulator, without guest
network preparation by the runner. Native state survives and all owned network/
module/TAP/temp resources clean up. Cancellation controls pass targeted host
sanitizers. The proposal is not applied/public-ready; ARP/configuration, refresh
and SDK broker allocator integration remain pending. See the [allocated DHCP record](acceptance/usb-hub-support-sharing-allocated-dhcp.json).

Direct MACVLAN allocation into an anonymous owned namespace now passes a
hash-verified pinned AArch64 emulator control: correct name/MAC, UP and no IPv4.
Native PID/address/routes/DNS/controller namespace remain unchanged; closing
namespace references permits module unload, with temporary files removed.
Targeted host sanitizers cover request/native-namespace refusal. DHCP, broker
allocator integration and target4.4 runtime remain pending. See the [link allocation record](acceptance/usb-hub-support-sharing-namespace-link.json).

Anonymous namespace allocation now passes eight sequential pinned AArch64
creations in the localhost emulator, retaining typed CLOEXEC FDs after creator
thread exit without changing the controller namespace or native PID. Host
sanitizers cover privilege-denial cleanup. Interface creation/configuration,
DHCP and broker allocation integration remain pending; vendor4.4 source facts
do not establish target runtime acceptance. See the [namespace creation record](acceptance/usb-hub-support-sharing-namespace-create.json).

The SDK-managed helper now exports a real mounted emulator hub library to
unchanged independent clients: discovery, all2546 tracks/185 nodes/8359 ordered
memberships and concurrent exact-hash PDB/MP3 downloads pass. Held ACTIVE then
STOP completes managed exit; exact original files are restored and owned bind/
network/TAP/module/files are cleaned. Single fixture with external IP/interface
setup; production allocation/DHCP, multisource/local playback and native/remote
loaded eject remain pending. See the [SDK real-library record](acceptance/usb-hub-support-sharing-sdk-broker-library.json).

SDK-managed executable service integration now passes inside EP147: authenticated
SOURCE/namespace FD reaches the helper, actual endpoints open, synthetic library
stages to ACTIVE, STOP drains and both managed/peer processes exit. Exact original
files/approval/marker state and absent helper are restored; normal EP147 is active
and owned network/module/library/files are cleaned. This supersedes the launch
gate below. Address/interface preparation remains external, PDB is empty and
independent client/native eject/DHCP allocation tests remain pending. See the
[SDK service record](acceptance/usb-hub-support-sharing-sdk-broker-service.json).

An explicit emulator-only helper executable now connects the authenticated session
to dispatcher/verified-FD manager and real namespace endpoint factory, using one
externally prepared synthetic source. Both pinned helper builds pass; the reused
connection/probe regression passes plain/ASan+UBSan/TSan. The new service mode has
not yet been executed or launched by the SDK. Production allocation and full
source scope remain pending; this is an integration fixture, not a candidate.

Saturated helper IPC now has a targeted plain/ASan+UBSan/TSan control: after
parent disconnect an active worker still joins/closes, and pending network
release prevents premature terminal completion. Network release remains a
fixture; SDK executable-mode integration is still pending. See the
[backpressure record](acceptance/usb-hub-support-sharing-broker-backpressure.json).

Authenticated session polling now carries SCM_RIGHTS SOURCE commands through
actual manager/staging threads and reports STOP only after drain. Parent loss
while ACTIVE and wrong-token rejection pass71/71/71 and hash-verified pinned
AArch64 execution in the localhost emulator, preserving EP147 PID. Network
callbacks remain loopback fixtures; SDK-launched executable mode and real
allocator integration remain pending. See the [session record](acceptance/usb-hub-support-sharing-broker-session.json).

Real namespace service endpoints and a pinned synthetic library now stage through
the manager, withdraw/join/close, then retire the address through the asynchronous
netlink operation and lease acknowledgement. A neighboring address and native
state survive; owned scope is cleaned. Portable negative controls pass all three
modes. Lease/ARP proof and library are synthetic; independent client, native
eject/unmount and SDK/DHCP integration remain pending. See the
[live retirement record](acceptance/usb-hub-support-sharing-live-broker-retirement.json).

Real asynchronous netlink removal now passes in an isolated emulator namespace:
192.0.2.3/24 is removed with kernel ACK while192.0.2.4/24 remains on the same
interface. Uploaded probe/module hashes match; native PID/address/routes/DNS/
namespace are unchanged and owned scope/module/files are removed. Portable
negative controls repeat under all three sanitizer modes. No music endpoints
ran; lease-release/eject integration remains pending. See the [live removal record](acceptance/usb-hub-support-sharing-live-address-remove.json).

An asynchronous exact netlink IPv4-removal component now passes targeted
plain/ASan+UBSan/TSan negative controls and a pinned AArch64 build. It refuses
the native namespace and requires matching kernel ACK; no successful address
mutation is claimed yet. Live isolated removal and lease callback integration
remain pending. See the [removal record](acceptance/usb-hub-support-sharing-address-remove.json).

Lease-aware dispatch now retires an expired source through actual worker join
and endpoint close before address removal. A pending removal keeps that source
DRAINING while a longer-lived neighbor stays ACTIVE/READY. The combined synthetic
lease/loopback/removal control passes69/69/69 and pinned AArch64 execution in the
localhost emulator, preserving EP147 PID. Real DHCP refresh/interface mutation
and SDK-managed command-loop integration remain pending. See the [combined record](acceptance/usb-hub-support-sharing-broker-lease-dispatch.json).

A lease-owned endpoint context now gates preparation/open on exact READY lease
data, retains the namespace reference and orders removal acknowledgement before
queued replacement. Targeted plain/ASan+UBSan/TSan controls pass with synthetic
lease/ARP and removal fixtures; no actual endpoint open/address mutation is
claimed by this control. Periodic loss-to-dispatch retirement and SDK runtime
integration remain pending. See the [lease adapter record](acceptance/usb-hub-support-sharing-broker-lease.json).

Typed network-namespace validation and unexpected worker-loss retirement now
pass68/68/68 plus the pinned AArch64 control in the localhost emulator. Native
EP147 PID is unchanged and the temporary test executable is removed. Network
prepare/release are still loopback fixtures; live SDK broker and allocator
integration remain pending. See the [namespace record](acceptance/usb-hub-support-sharing-broker-namespace.json).

The post-authentication dispatch adapter now connects SOURCE parsing, generation
reservation, pinned root and actual manager/worker staging. Targeted plain,
ASan/UBSan and TSan controls pass: replay/old detach and preparation denial,
network-release wait before drained acknowledgement, surviving neighbor and
terminal STOP. Network preparation/release use loopback fixtures; SDK helper
and live allocator integration remain pending. See the [dispatch record](acceptance/usb-hub-support-sharing-broker-dispatch.json).

A per-slot asynchronous generation gate now rejects replayed SOURCE/old DETACH
and late readiness, requires drain before replacement and makes STOP terminal
for its session. Targeted plain/ASan+UBSan/TSan controls pass for eight slots and
UINT64_MAX. This is not yet connected to authenticated broker dispatch; real
worker/network drain evidence remains a caller obligation. The complete66/66/66
record below predates this extra component.

Verified directory ownership now extends through the source manager into worker
staging. Caller close, path replacement, same-identity object substitution and
eject-before-first-poll controls pass66/66/66, with a pinned AArch64 build.
Authenticated dispatch, external generation authorization and live SDK-managed
export remain pending. See the [manager record](acceptance/usb-hub-support-sharing-pinned-manager.json).

The SOURCE IPC codec and pinned directory identity component now pass the full
portable66/66/66 matrix. Replacement directories and leaf/intermediate symlinks
are rejected. An explicit worker start now carries the pinned FD through staging without
reopening its pathname; rename/replacement controls and66/66/66 pass, as does
the pinned AArch64 build. Broker dispatch still needs to invoke it and enforce
source/generation authorization. See the [pinned worker record](acceptance/usb-hub-support-sharing-pinned-worker.json).
See the [source component record](acceptance/usb-hub-support-sharing-broker-source.json).

The actual core-managed helper launch/IPC probe now passes inside EP147 in the
emulator: reviewed USB helper asset, real SDK launch, peer/token/executable checks,
namespace FD acknowledgement, STOP and managed liveness end. Managed and peer
PIDs are gone; exact0.3.2 package/stage/approval/start-marker state is restored and
the temporary helper asset/namespace/files are removed. This IPC-only probe
does not enter namespaces or enable public networking. An initial missing-stat
preflight and two pathname-validation failures were cleaned/restored; replacing
the namespace display-name assumption with NSFS/inode validation resolves them.
The bind-mounted descriptor regression passes separately. Production broker
dispatch and network/source lifecycle integration remain pending.
See the [SDK-managed runtime record](acceptance/usb-hub-support-sharing-sdk-broker.json).

Broker integration now follows the supported SDK boundary: core1.2.3 owns helper
launch/reaping and closes inherited FDs, so namespace references need a separate
local channel. Bounded Unix packet/FD transport and explicit credentials/executable/
token checks pass targeted sanitizers and pinned AArch64 emulator controls. A
separately executed standalone helper completes HELLO, namespace-inode acknowledgement
and STOP on an owned private socket, preserving native state and cleaning up.
This standalone probe does not enter namespaces or start libraries. The managed
launch result above supersedes its earlier launch gate; operation/generation
authorization and production broker dispatch remain pending.
See [broker integration](HUB-LINK-BROKER.md).
The complete portable matrix now passes65/65/65, with hook registration checked
separately. See the [IPC transport/probe record](acceptance/usb-hub-support-sharing-broker-wire.json).

One live hub library now passes end-to-end over its DHCP-acquired/ARP-validated
address: unchanged Beat Link discovery and full2546-track/185-node/8359-membership
menus, plus concurrent unchanged Crate Digger exact-hash PDB/MP3 downloads.
Source-owned endpoints use the actual grant and a fresh nonce; the worker is
withdrawn/joined and sockets closed before exact address removal. Native state
and parent namespace survive; all owned resources are cleaned and transport
reports no errors. This is a separate research process, not public networking
inside the mod. Production broker/namespace lifecycle, refresh/expiry, local
playback concurrency and multiple leased sources remain pending. The live branch
is not sanitizer-instrumented; acquisition unit controls pass all three modes.
See the [leased library record](acceptance/usb-hub-support-sharing-leased-library.json).

One bounded live post-DECLINE recovery now passes: the conflicting192.0.2.3
candidate is rejected without configuration, retained retry policy waits10001ms,
then a fresh native client obtains192.0.2.4 with the same explicit client identity.
The C lease controller advances generation after retirement/removal acknowledgements;
the new address completes ARP/configuration/UDP and exact removal. Host observation
sees about10.101 seconds between DECLINE and the next DISCOVER. Native processes,
DNS/routes and parent namespace survive, and owned resources are cleaned. This is
one research fixture retry without music services, not the production allocator
or continuous refresh/expiry acceptance. Acquisition unit sanitizer controls pass
again; the live recovery branch itself is not sanitizer-instrumented.
See the [bounded recovery record](acceptance/usb-hub-support-sharing-dhcp-reacquire.json).

Initial-conflict DHCPDECLINE now reaches the isolated fixture server's UDP
socket with the rejected IP/server and same explicitly pinned native client ID.
It uses source0.0.0.0 and valid IPv4/UDP checksums, and the disputed IP stays
unconfigured. Actual current-namespace interface/MAC validation gates sending.
The retained retry policy separately passes ten-second post-DECLINE quiet-period
and stricter repeated-conflict backoff controls. Production automatic reacquisition and
ongoing music-service conflict retirement remain pending. The full portable
matrix passed63/63/63 at that boundary; live branch evidence is not sanitizer-instrumented.
The same exact probe also passes the successful configure/announce/UDP/remove
control. Both scopes finish with cleanup and preserved native state. See the
[DECLINE record](acceptance/usb-hub-support-sharing-dhcp-decline.json).

An integrated initial-conflict negative control now passes: after native DHCP
ACK, the host observes the first ARP probe and injects a foreign claim. The client
detects conflict before applying the IP and retires its candidate; the host sees
no address announcement or IPv4 use in its bounded observation. Actual IPv4 stays
absent and native state survives. Production retry and conflict retirement while
music services are running remain pending; no library endpoint ran in this probe.
The same probe binary also repeats the positive configure/announce/UDP/remove
control successfully, and both runs finish with owned network/module cleanup.
See the [initial conflict record](acceptance/usb-hub-support-sharing-address-conflict.json).

An explicit integrated address-flow probe now passes in the emulator: supervised
native DHCP grant, three real ARP probe sends and quiet interval, exact IP/prefix
configuration, both announcements, lease readiness, host UDP roundtrip and exact
address retirement/removal. Native processes/DNS/routes and parent namespace
remain unchanged, and owned networks/module/files are cleaned. This is a research
harness with no music endpoints, not the mod's production allocator. Its guarded
single-threaded fork path must never run inside EP147. Asynchronous interface
operations, source endpoint coupling, refresh/conflict/expiry and full local MAC
inventory remain pending. See [address ownership](HUB-LINK-ADDRESS-OWNERSHIP.md).

Supervised one-shot DHCP acquisition now resolves the grant-time reference for
initial acquisition: the deadline is anchored before spawning a fresh native
client, not when an event is eventually read. A dedicated nonblocking pipe,
validated bound record, successful exit, EOF and reap are required. A separate
exec'd helper enters the owned namespace with clean environment and closes other
descriptors. Targeted plain/ASan+UBSan/TSan controls pass. Exact pinned AArch64
control plus standalone helper complete real emulator OFFER/ACK and feed the C
lease candidate using that conservative reference. The candidate remains unready
and no IP is applied. Native processes/DNS/routes and parent namespace survive.
Refresh scheduling, conflict/NAK handling and integration with owned interfaces
and source endpoints remain pending; child-reaping ownership must be preserved.
The preceding complete portable matrix passed62 cases each in plain/ASan+UBSan/TSan;
hook registration passes separately. See the
[supervised DHCP record](acceptance/usb-hub-support-sharing-supervised-dhcp.json).

A socket-free lease/address controller now gates readiness on confirmed
configuration and matching-address ARP readiness. Renewal retains its generation;
expiry/invalidation/replacement requires drain before address removal and removal
before a queued generation begins. Targeted plain/ASan+UBSan/TSan and pinned
AArch64 emulator controls pass, including the actual ACD state machine and
neighbor isolation. It is not wired into the mod. The v1 reporting channel alone
lacks a grant-time reference; the supervised acquisition above supplies one.
Do not infer live DHCP renewal/expiration or automatic networking from these
simulated controls. The latest63/63/63 result includes address, DHCP,
lease, acquisition and DECLINE components; earlier58/58/58 records retain their exact
historical artifact scope.

DHCP reuse research identifies native BusyBox1.27.2 `udhcpc`, but its default
hook modifies global DNS/routes and can kill other DHCP clients. A separate
reporting-only hook and bounded C event parser are now tested instead. Parser
plain/ASan+UBSan/TSan and pinned AArch64 execution pass; native shell hook events
also pass. An isolated emulator udhcpc process delivers deconfig/leasefail on
inherited fd3, without changing native DNS/routes or EP147 PID, and its namespace
is removed. A subsequent isolated live OFFER/ACK trial now delivers the exact
192.0.2.3/24,60-second lease and server address through the native reporting hook;
the proposal is not applied. Native EP147/DHCP process IDs, DNS and routes remain
unchanged. The server is a repository-authored fixed fixture, not general DHCP
interoperability acceptance. Refresh/expiration, production broker integration,
owned interfaces and allocator integration remain
pending; this code is not enabled in the mod or included in release assets.

The first address-allocation component is now implemented: a socket-free ARP
candidate detector with explicit probe/send completion, announcement readiness
and immediate retirement on foreign conflict. Targeted plain/ASan+UBSan/TSan and
pinned AArch64 emulator controls pass. It is not wired into the mod and performs
no networking. A separate nonblocking Ethernet ARP adapter now passes isolated
WSL veth controls plain and ASan/UBSan, including acquiring/active conflict
retirement and MAC validation. The retained per-interface retry context passes
plain/ASan+UBSan/TSan controls across candidate changes and neighbor isolation.
Its pinned AArch64 adapter build succeeds; live emulator ARP remains pending.
DHCP leases, allocator wiring, owned interface
lifecycle and public player-number collision handling remain pending. See
[address ownership](HUB-LINK-ADDRESS-OWNERSHIP.md). The portable runner now includes
this component; the previous58/58/58 result predates its addition.

Cold-start investigation has now located the recorded signal11 stack in native
image-file loading, called by eager HUB badge preparation during mod init.
The framework's `froze` marker alone did not establish this cause; the saved
crash registers and executable return addresses supplied the evidence.
The current candidate defers each badge composition until its first owned
native widget update, publishes completed images atomically and preserves the
original native badge on a failed or concurrent attempt. Targeted badge tests
pass plain, ASan/UBSan and TSan; both AArch64 candidate variants build and the
hook-registration control passes. The full portable component matrix also
passes in plain, ASan/UBSan and TSan modes after this change; its ignored log is
`local/integration/cold-badges-v1/host-suite.log`. The standard AZ candidate now
passes one new emulator cold boot: exact mapped hash, zero application restarts,
empty crash log and all eight persistent disk roots retained. SOURCE badges1–5
were visually inspected after that boot. Cold browse/waveform validation remains
pending. This does not establish a hardware boot fix.
The lazy-badge sharing build also passes the complete two-source integrated
discovery/menu/file and isolated eject/reconnect regression. Source badges1–5
compose on widget updates without fallback logs. The runner exits0, restores
normal0.3.2 and removes its networks/module; the transport reports no errors.
See the [lazy initialization record](acceptance/usb-hub-support-lazy-badge-init.json).
The cold runner's restore command used an unsupported guest `find` expression;
an explicit verified owned USB backup path restored the original package, stage
and approval bytes separately. Normal0.3.2 is loaded afterward with zero
restarts. This runner error does not invalidate the saved cold-boot observation.

Two live integrated public sources now pass independent concurrent discovery,
complete menus and exact PDB/MP3 downloads. Each exposes2546 tracks,185 playlist
nodes and8359 memberships from identical protected fixtures. Separate root
nonces reject cross-source handles on111/50111. Native STOP ejects source1;
source2's original TCP menu connection and root stay usable during eject and
reconnect, and its discovery remains present at lifecycle checkpoints. Source1
returns with a fresh root and its old handle is STALE. The application PID stays
unchanged. Both networks/module/local socket are removed and exact0.3.2 is
restored afterward. See the [two-source record](acceptance/usb-hub-support-sharing-integrated-two.json).

The first two-process discovery attempt failed because two DeviceFinders tried
to bind the same UDP port. The corrected observer uses one unchanged Finder
with separate concurrent Client sessions. Public socket endpoints themselves
were separate and delivered files in the failed observer trial.

Local resource cleanup reduced QEMU TCP listeners from77 to5 (SSH2222,
QMP4445/4446 and existing capture/injection15101/15102). Windows still listed
old research listeners although QMP exposed no corresponding dynamic channels.
A controlled emulator restart preserved all eight current persistent QCOW2 roots
and original USB topology. The first EP147 cold start exited139; the framework
quarantined the exact0.3.2 executable. Native Community Mods `Try again` followed
by a warm application restart restored it. The crash-stack investigation and
lazy-badge cold-boot control above supersede the original unresolved-cause note;
this published-binary restart itself is not cold-boot acceptance.
Port-cleanup research files remain ignored. See the
[resource cleanup record](acceptance/usb-hub-support-emulator-port-cleanup.json).

The integrated network runner now uses a temporary local socket for NAT capture
instead of allocating another TCP listener. The Windows Python build lacks the
AF_UNIX constant; a bounded native Winsock adapter supplies that local connection.
A connect/remove probe and the full two-source integrated stream/cleanup pass.
The QEMU listener count remains5 during and after that trial. Local sockets are
now the default; legacy TCP requires the explicit `--nat-tcp` diagnostic flag.

The integrated mod now passes native UI safe eject and reconnect for one public
hub source. An external observer retains its root handle and metadata session;
the native USB STOP selector ejects the stick, ordinary unmount removes its
backing mount and all other observed USB mounts survive. The observer confirms
both RPC endpoints drained, metadata/query closure and unchanged DeviceFinder
expiry. Replugging the protected virtual fixture produces incarnation2 and
rediscovery; the old root handle returns STALE on111 and50111, while the new
root handle differs. The application PID stays unchanged. Full browsing and
exact-hash PDB/MP3 transfers pass before eject. Cleanup restores0.3.2 and the
fixture and removes the isolated network/module. See the
[integrated eject record](acceptance/usb-hub-support-sharing-integrated-eject.json).
This does not simulate an externally loaded/playing deck or establish concurrent
local/remote playback, multiple live public exports or actual-CDJ acceptance.

Earlier integrated boundary: the exact AArch64 sharing-emulator mod runs inside
EP147 and exports one live hub source. Independent unchanged Beat Link discovers
it and browses2546 tracks,185 ordered playlist nodes and8359 memberships;
unchanged Crate Digger downloads the complete PDB and MP3 with exact hashes.
The mapped executable hash and unchanged application PID were verified. The
full portable suite passes58 cases each in plain/ASan+UBSan/TSan; hook
registration passes separately. Eight-source bridge lifecycle controls also
pass as an AArch64 synthetic fixture, not as eight live exports.

User-authorized elevated WslService restart resolved the VM startup timeout;
WSL and Docker responded afterward. The integrated trial completed and restored
the previous emulator mod/approval bytes, verified normal EP147 with0.3.2,
and removed its namespaces, MACVLAN test module and temporary files. No physical
AZ action or publication occurred. See the [integrated mod record](acceptance/usb-hub-support-sharing-integrated-mod.json).
`research/scripts/test-public-mod-network.py` preserves the reconstruction flow;
its guarded runner was derived from the successful ignored trial runner and
syntax/CLI checked after additional precondition checks.

Automatic LAN allocation, live capacity beyond two exports,
external loaded-deck removal, local/remote concurrency and target4.4 runtime
validation remain pending. The earlier entries below preserve the sequence of
intermediate builds and infrastructure failures; the latest record supersedes
their pending-build and standalone-only boundaries.

### Earlier integration steps

An explicit `sharing-emulator` build now connects the public manager to the
HubTransport presence poll and gates both natural alias removal and safe-eject
alias unmount on public worker join/endpoint release. During the native listener
shutdown it retains the withdrawn RPC service, then drains before unmount.
Public UDP calls bypass the private datagram hooks so advertisements cannot
overwrite the private observed address. This build requires the explicit
`AZHUB_PUBLIC_EMULATOR=1` setting, exact emulator kernel and an externally prepared
`azhub-pubtest` namespace; it exports only source0 for integration research.
It does not allocate LAN addresses/interfaces and is not a production candidate.

The complete AArch64 shared object compiles with the pinned compiler. A focused
bridge fixture passes plain/ASan+UBSan/TSan and AArch64 execution in the guarded
localhost emulator, covering presence, reservation, withdrawal, join-before-drain,
reconnect and natural removal. Its temporary executable is removed. These are
synthetic callback controls, not execution of the shared object inside EP147;
actual integrated discovery, file access and native eject remain unverified.
Artifacts stay in ignored `local/integration/sharing-mod-v1/`.

The first integrated EP147 network trial stopped during the initial host
namespace inventory: `wsl.exe -u root --exec ip netns list` exceeded its20-second
deadline. This occurred before module, namespace or candidate installation;
the runner completed its owned cleanup. The emulator still runs its previous
mod. A subsequent WSL inventory reported no running distributions. This is an
infrastructure prerequisite failure, not integrated sharing acceptance.
The minimal WSL startup check also terminated with
`Wsl/Service/CreateInstance/CreateVm/HCS_E_CONNECTION_TIMEOUT`. Automatic approval
review rejected the subsequent host-wide `wsl.exe --shutdown` recovery action;
it was not executed. Recovery requires explicit permission because it may stop
Docker or unrelated WSL workloads. No integrated candidate was installed.
Transition-only `public-sharing` diagnostics have since been added to the
emulator bridge; those source changes still require rebuilding and verification.
The recorded shared-object hash identifies the earlier compiled artifact.
The user subsequently authorized WSL shutdown/restart. `wsl.exe --shutdown`
completed successfully, but the following `wsl.exe --exec /bin/true` again
terminated with `HCS_E_CONNECTION_TIMEOUT`. Read-only service checks showed
WslService, vmcompute and hns running. No Windows restart or service restart
was performed; integrated runtime validation and rebuilding remain pending.

Current unbuilt source extends the emulator bridge to eight independently
observed source slots. `AZHUB_PUBLIC_EMULATOR_COUNT` accepts1..8 (default1).
Each configured slot requires its own externally prepared namespace/interface:
slot0 uses `azhub-pubtest`/`azpub0`; later slots use `azhub-pubtestN`/`azpubN`.
The isolated test addresses are192.0.2.(3+N), public identities41+N and distinct
local MACs. Initialization closes already opened namespace descriptors if any
later open fails. This remains a research configuration, not automatic LAN
allocation or an actual eight-source interoperability claim.

The expanded synthetic fixture now requests eight workers, fresh per-source
nonces, transition-only diagnostics and isolation of seven neighbors during one
source's withdrawal/drain/reconnect. These latest changes are uncompiled and
unexecuted because WSL remains unavailable. A read-only compiler inventory found
no gcc/clang/cc in the localhost emulator. `git diff --check` passes. Earlier
acceptance records apply only to their exact earlier source/binary hashes.

Automatic approval review also rejected `Restart-Service -Name WslService`:
the prior user authorization covered `wsl.exe --shutdown`, not restarting the
Windows service. The service restart was not executed. After repeated WSL VM
startup failures and the completed independent code work above, further
compilation/runtime evidence requires WSL recovery or another validated build
environment. Do not treat automatic goal continuations as service-restart or
Windows-restart approval.

The source manager now suppresses endpoint opening when withdrawal arrives
before its first poll, including a pending replacement after an older worker
joins. Repeated observations of the same withdrawn incarnation do not restart
it; a new incarnation can start normally. The focused manager regression passes
plain, ASan/UBSan and TSan in the local network-disabled test container. This
closes a pre-publication eject race; native mod callback integration remains open.

Production remains immutable0.3.2; the physical AZ is untouched by this goal.
Current sharing work combines host metadata prototypes and standalone AArch64
emulator file components on an isolated network, not a new mod candidate. The [sharing plan](HUB-LINK-SHARING-PLAN.md)
retains separate discoverable sticks, independent servers/caches, native sources,
local continuity and native eject behavior with external loaded tracks.
No sharing release or reduced scope is approved.

Current tested boundary: one standalone AArch64 worker exports a mounted emulator
hub source to independent discovery/menu/file clients, and external retirement
checks pass. A separate native-emulator client loads an MP3 from the host C
worker and advances playback position. Neither control is mod integration.
The next integration work must connect source presence/generations, per-source
network endpoints and native safe eject while preserving the private transport.
Multiple live sources, reconnect, loaded-deck eject and combined local/remote
operation remain required; the published mod and physical AZ are unchanged.

### Named-table parser work in progress

The C reader now traverses genres, artists, albums, labels, keys, playlist
nodes, memberships and artwork. A real local PDB exposed two reader defects:
the last row-index group can be partial, and the allocated offset count is a
13-bit field at byte24; byte34 is a transaction index. The latter interpretation
is documented in the current primary Crate Digger Kaitai schema:
https://github.com/Deep-Symmetry/crate-digger/blob/main/src/main/kaitai/rekordbox_pdb.ksy
Both C track and entity traversal now use these bounds/counts. Targeted controls
cover partial groups and284 allocated offsets with a distinct transaction index.
All44 portable cases pass in plain, ASan/UBSan and TSan; hook registration passes
separately. No new target execution or runtime integration is claimed.

The scan yields67 genres,2937 artists,1916 albums,175 labels,24 keys,185 playlist
nodes,8359 membership rows and2504 artwork rows. All12730 referenced name
comparisons and185 playlist-node comparisons match the previous independent
Crate Digger snapshot. A corrected-count Crate Digger adapter now independently
matches all16167 decoded records byte-for-byte, including8359 membership rows
and their positions. The old snapshot omitted2404 memberships;151 playlist
lengths changed. Earlier snapshot-backed browsing does not establish complete
playlist content. See the [named-entity record](acceptance/usb-hub-support-sharing-named-entities.json).

A new compact entity index preserves all16167 records after sorting, rejects
duplicate named IDs/playlist positions, and provides generation-gated named
lookups and ordered membership ranges. Retirement blocks new lookups without
holding filesystem leases; callers must join existing readers before destruction.
Measured buffers occupy720896 bytes for this fixture, excluding parser/allocator
and other library storage. Bounds are262144 rows and8MiB name storage per source;
eight-source resource limits are not validated. Joint lifecycle/cross-reference
validation, target execution and metadata TCP integration remain open. Local
raw comparison output stays ignored in local/diagnostics/sharing-pdb-tables-v1/.
Nothing was installed or published.

### Joint file, track and playlist staging

The staged C library now traverses all named/playlist tables before publishing
its catalog, track index and entity index together. Each worker step decodes
at most one page. Cancellation, source replacement and post-ready PDB mutation
retire all three views. Structural validation rejects missing playlist tracks,
invalid folder parents and hierarchy cycles. Optional missing name IDs remain
absent metadata so playable tracks survive; the real fixture has17 such refs.

All45 portable cases pass in plain/ASan+UBSan/TSan, and hook registration passes
separately. The pinned target-toolchain AArch64 harness passes synthetic controls
and stages the emulator's live hub library:2546 tracks,16167 entity rows and10329
file nodes, with2529 absent audio fixtures. No USB content is written. An unchanged
Crate Digger client still completes concurrent exact-hash PDB/audio downloads
from the integrated host service; hidden-file and post-mutation STALE controls
pass. Owned temporary emulator executables are removed. See the
[joint entity lifecycle record](acceptance/usb-hub-support-sharing-entity-lifecycle.json).
Metadata TCP, public runtime integration and actual-CDJ acceptance remain open.

### C metadata message codec

A bounded C codec now reads/writes dbserver messages with typed arguments,
UTF16BE text, binary payloads and native elided empty fields. It reports partial
frames without dispatch and consumes one frame from coalesced input. Limits
are16 arguments,64KiB per field and128KiB per frame; the future session adapter
must enforce buffering/deadline/ownership limits. All46 portable cases pass
in plain/ASan+UBSan/TSan. The pinned AArch64 synthetic harness passes in the
emulator and its owned executable is removed.100 Python-generated messages
round-trip through C; unchanged Beat Link8.0.0 also generates and decodes3
C-normalized controls with identical arguments. See the
[codec record](acceptance/usb-hub-support-sharing-db-wire.json).
This codec record establishes wire framing only. Subsequent session and menu
backend controls below extend it; native/actual-CDJ compatibility remains open.

### C metadata connection lifecycle

The C session component owns a nonblocking accepted socket, performs greeting
and setup, preserves fragmented/coalesced input, and streams one response
message at a time through bounded callbacks. Each session allocates128KiB input
and128KiB output; aggregate provider capacity is still an integration obligation.
Handshake/partial-frame deadlines are5s and idle timeout30s. Atomic cancellation
and a source-alive guard close the connection; worker/readers must join before
freeing session/source state. Backend callbacks still need DMST/source checks.

All47 portable cases pass in plain/ASan+UBSan/TSan. AArch64 socketpair controls
pass in the emulator. In an additional loopback TCP harness, the unchanged
Beat Link8.0.0 Client completes provider41/requester3 setup while a second socket
has only supplied one greeting byte; both connections close cleanly. See the
[session record](acceptance/usb-hub-support-sharing-db-session.json).
This harness rejects library requests: menu/metadata responses over the live
indexes, public runtime wiring and native browsing remain the next gates.

### C live-index menu backend

The C nonblocking sessions now connect to source-owned library indexes in a
standalone loopback-only service. Unchanged Beat Link8.0.0 retrieves all2546
tracks with exact IDs/titles,185 ordered folder/playlist nodes and8359 ordered
playlist memberships against the corrected independent Java oracle. Root menu
contains TRACK and PLAYLIST. The fixture is copied from the local PDB into an
owned temporary directory; this is not native emulator UI or live network-source
acceptance. The harness and temporary fixture exit/clean up normally.

Per-session menu descriptors isolate client selections. Responses stream one
item at a time with bounded1..64-item pagination; DMST requester/collection-slot
checks reject other identities and private/local slots. Track ordering folds
ASCII case and otherwise compares UTF8 bytes, not locale-aware Unicode collation.
Folder/PDB playlist ordering is retained. Eleven metadata rows, Unicode conversion,
source retirement, spoofed slots and pagination pass synthetic controls.
All48 portable cases pass in plain/ASan+UBSan/TSan; pinned AArch64 synthetic menu
controls also pass in the emulator. See the [menu record](acceptance/usb-hub-support-sharing-db-menus.json).
Load-info0x2102, analysis/cues/artwork, native UI controls, public runtime/file
service integration and concurrent local/remote acceptance remain open.

### C selected MP3 load-info response

The C menu backend now serves0x2102 with seven MP3 load-info rows, using the
previous native-emulator-confirmed decoder1 mapping. It requires the PDB audio
path to exist in the source's authorized file catalog. Generic path lookup was
moved unchanged into the catalog header so metadata and NFS use the same lookup.
An unchanged Beat Link handshake/message codec control over loopback TCP verifies
all seven rows against the independent PDB, including exact path/size, duration,
tempo, comment and key ID. Host48/48/48 and pinned AArch64 synthetic controls pass.
This is not a new native deck load/playback test. See the
[load-info record](acceptance/usb-hub-support-sharing-db-load-info.json).

Other formats remain an implementation/validation gate. The ignored local
vynull reference suggests AAC/M4A4, FLAC5, WAV11 and AIFF12; these are candidate
codes only, and its AIFF comment explicitly says inferred/unverified. Real
analysis/cue/beatgrid/waveform/artwork responses and combined native metadata/file
load controls are still required. The goal is not reduced to MP3-only support.

### Incremental analysis sections and response size correction

The new C ANLZ reader locates a requested section/occurrence and copies it in
at most8192-byte chunks through supplied source I/O. It rejects malformed bounds,
missing sections and read failures; it does not yet dispatch analysis responses.
Ten real DAT/EXT/2EX sections match unchanged Crate Digger section boundaries/raw
bodies byte-for-byte, including repeated cue tags and color/three-band waveforms.
PQT2 is absent from the old enum, so the oracle adapter identifies its raw fourcc
at the generated section boundary. This verifies bytes, not cue/tag semantics.
The pinned AArch64 reader also reads the live emulator USB PWV5 section with
exact79748-byte/hash equality. Source USB files are read only; temp executables
are removed. All49 portable cases pass in plain/ASan+UBSan/TSan.

Real PWV5/PWV7 sizes79748/119610 exceeded the initial64KiB codec limit. Fields now
allow4MiB and frames4MiB+4096; incoming session buffers remain128KiB. Output starts
at4KiB, grows on demand and shrinks after large sends. A119610-byte partial-write
control with a small send buffer and257-byte peer reads passes on host/AArch64.
Aggregate multi-provider memory quotas remain open. Reader bounds are64MiB/file,
4MiB/selected section and1024 tag headers. See the
[analysis reader record](acceptance/usb-hub-support-sharing-analysis-reader.json).
The next gate is the source-catalog adapter and real analysis/cue/beatgrid response
integration, followed by native rendering/load/playback and other-format controls.

### Source-backed analysis tag responses

The standalone C metadata backend now handles0x2c04 requests for registered DAT,
EXT and2EX analysis files. Paths come exclusively from the source's PDB/catalog;
generation, inode and file fingerprints guard incremental reads. The session
producer yields between read steps. Withdrawal or mutation terminates pending
work, and backend destruction releases pending allocations. A missing tag returns
an empty0x4f02 response. Requests never supply filesystem paths.

Unchanged Beat Link8.0.0 AnalysisTagFinder decodes six real sections over local TCP
and matches the independent raw-section oracle: PQTZ, PWV4, PWV5, PCO2, PWV6 and
PWV7, totaling212694 section bytes. The selected PCO2 fixture is empty; populated
cue semantics are not validated. All49 cases pass in plain, ASan/UBSan and TSan;
hook registration passes separately. The updated synthetic backend built with
the pinned AArch64 compiler also passes in the guarded6.6.138-dirty emulator;
its owned temporary executable was removed. Two concurrently launched unchanged
Beat Link controls each complete all six section queries while a third accepted
connection sends no greeting. This demonstrates service progress with a stalled
peer, not measured per-request overlap or multi-player hardware acceptance.
See the [analysis response record](acceptance/usb-hub-support-sharing-db-analysis.json).

This supersedes the preceding pending adapter/0x2c04 statements only. Legacy
analysis responses, populated cues, artwork, other audio formats, aggregate
memory quotas and native runtime/rendering/playback remain open. No physical AZ
operation, package change or publication occurred.

### Legacy beatgrid response

The C metadata backend now handles0x2204 using source-authorized DAT/PQTZ data.
It converts the exported eight-byte big-endian beat records into the legacy
sixteen-byte little-endian wire records, preserving each beat's position, tempo
and time. It does not synthesize a constant-tempo grid. Header/count/length and
output-size bounds reject malformed input before allocation.

Two concurrently launched unchanged Beat Link controls each decode all306 beats
from the real fixture and match Crate Digger's decoded PQTZ values. Targeted
backend tests pass plain, ASan/UBSan and TSan, including a variable-tempo fixture
and malformed lengths/counts. The pinned AArch64 synthetic harness also passes
in the guarded emulator; its temporary executable was removed. This is not a
new full49-case matrix run. Unknown wire fields follow a public-provider
convention and still need native-client validation. No physical AZ operation or
runtime integration is claimed. See the
[beatgrid record](acceptance/usb-hub-support-sharing-db-beatgrid.json).

### Legacy detailed waveform response

The standalone C backend now handles0x2904 with the original EXT/PWV3 samples.
It validates the tag envelope, one-byte entry size and exact sample count before
building the response. Reads retain the existing catalog/generation/fingerprint
guards and yield between chunks. The nineteen-byte legacy prefix is zero-filled;
Beat Link ignores it, but its native meaning and acceptance remain unverified.

Two concurrently launched Beat Link controls independently match all39862 samples
against Crate Digger's direct EXT decoding, while the six analysis-section queries
and306-beat conversion still pass. Targeted backend tests pass plain, ASan/UBSan,
TSan and pinned-toolchain AArch64 execution in the guarded emulator. Temporary
emulator executables were removed. This does not establish native waveform
rendering, actual-CDJ interoperability or a new full host-matrix result.
See the [detailed waveform record](acceptance/usb-hub-support-sharing-db-waveform-detail.json).
Legacy previews, populated cues, artwork and joint runtime integration remain open.

### Legacy waveform preview response

The standalone C backend now handles0x2004 by combining DAT/PWAV and DAT/PWV2.
It expands400 primary samples into height/color pairs and preserves the100-byte
tiny preview, yielding900 bytes. Both reads reuse the same captured file
fingerprint; mutation between sections fails, and a missing tiny section returns
an empty response rather than publishing partial data. Four numeric arguments
and the documented optional elided empty binary argument are accepted.

Two concurrently launched unchanged Beat Link controls match all400 primary
column heights/colors against direct Crate Digger decoding and all100 tiny bytes.
Beat Link8 counts the tiny suffix as50 additional columns; the oracle explicitly
compares the primary400 and tiny payload separately. This is a software-client
display limitation, not native rendering acceptance. Prior analysis, beatgrid and
detail controls still pass. Targeted backend tests pass plain, ASan/UBSan, TSan
and guarded emulator AArch64, including mutation between sections and missing
tiny-section cleanup. The temporary emulator executable was removed.
See the [preview record](acceptance/usb-hub-support-sharing-db-waveform-preview.json).
Populated cues, artwork, native rendering and joint runtime integration remain open.

### Populated cue input validation

A bounded C PCOB reader now preserves list kind, hot-cue ID, type, raw status,
start time and loop end time. It validates section/entry lengths, counts and
known cue types before publishing a decoded list. Status is retained verbatim;
deleted-cue semantics are unresolved and no rows are silently filtered.

Three populated DAT fixtures were copied read-only from the guarded emulator's
hub library and remain ignored locally. All eight real hot-cue entries match
unchanged Crate Digger's generated parser byte-for-byte in the numeric oracle.
These fixtures have no memory cues or loops; those paths have synthetic tests
only. Targeted plain, ASan/UBSan and TSan tests pass, including truncation,
oversized entry and mismatched count rejection. The host test list now includes
this component (50 cases); the full50-case matrix has not yet run.

This parser is not yet connected to0x2104/0x2b04 responses. Combining hot/memory
sections, extended comments/colors, populated loop fixtures and native acceptance
remain open. See the [cue input record](acceptance/usb-hub-support-sharing-cue-input.json).
No source USB writes, physical AZ operations or publication occurred.

### Legacy cue network response

The standalone backend now serves0x2104/0x4702 from the source's DAT/PCOB lists.
It combines the first two cue sections with one captured file fingerprint,
rejects duplicate list kinds, preserves cue/loop flags and hot IDs, and converts
milliseconds to legacy half-frame positions. Counts distinguish hot and memory
entries. Unknown entry fields and the undocumented auxiliary blob are empty;
their native meaning remains unvalidated. Raw exported status is retained by
the parser but is not interpreted as deletion by the wire converter.

Unchanged Beat Link Client/CueList decodes all eight hot cues across three real
fixtures, exactly matching direct Crate Digger decoding. Combined memory/hot
loops and mutation between section reads pass synthetic backend tests. Memory
cue decoding in Beat Link's legacy path is not covered by these real fixtures;
native acceptance and extended cues remain separate gates. The full50 cases
pass plain, ASan/UBSan and TSan; hook registration passes independently. Pinned
AArch64 synthetic backend controls pass in the guarded emulator, with temporary
executable cleanup. Analysis/beatgrid/detail/preview regression also passes with
two concurrently launched clients and a stalled greeting peer.
See the [legacy cue response record](acceptance/usb-hub-support-sharing-db-cues.json).
No mod runtime integration, physical controller operation or publication occurred.

### Extended cue network response

The C backend now serves0x2b04/0x4e02 from source-authorized EXT/PCO2 sections,
combining hot/memory lists under the same file fingerprint. It preserves
millisecond positions, cue/loop types, memory color IDs, hot-cue colors and
comments, converting UTF16BE exports to UTF16LE network text. Header/count/entry
and comment lengths are checked; invalid surrogates/terminators are rejected.
Missing EXT or absent extended tags reports unavailable so clients can fall back
to legacy cues; existing empty extended sections remain a valid empty list.

The corresponding real EXT fixtures contain12 hot cues, including IDs above C,
and six nonempty comments. Unchanged Beat Link decodes all positions/types,
comments and colors identically to direct Crate Digger decoding; eight legacy
cues still match. Synthetic memory-loop/Unicode/color, partial-entry, duplicate
kind, malformed comment and missing-EXT fallback controls pass plain, ASan/UBSan
and TSan. The full50-case matrix and AArch64 execution predate this change and
are not new acceptance for it. Unknown network fields remain zero-filled;
native interpretation, real memory/loop fixtures and rendering are unvalidated.
See the [extended cue record](acceptance/usb-hub-support-sharing-db-extended-cues.json).
Artwork, other audio formats and joint runtime integration remain open.

### Artwork catalog staging

Library staging now registers original artwork paths from PDB table13 alongside
the source-owned audio/analysis catalog, before publishing the combined views.
Missing artwork is counted and retains its metadata row; unrelated files remain
unregistered. Paths use the same confinement and generation rules as media.
Only explicitly indexed artwork paths are registered; high-resolution companion
naming remains a research gate.

Targeted plain, ASan/UBSan and TSan staging controls pass for present/missing
artwork and hidden unrelated files. The emulator's source fixture has no JPEG
files under PIONEER, so the selected real artwork could not be copied. A host
copy of the real PDB with one explicitly synthetic file at an independently
decoded artwork path stages2546 tracks and16167 entities, with2503 missing
artwork rows. This validates catalog staging, not image decoding or delivery.
No artwork request handler or physical acceptance is claimed. See the
[artwork staging record](acceptance/usb-hub-support-sharing-artwork-staging.json).

### Original artwork network response

The standalone C backend now serves two-argument0x2003/0x4002 artwork requests
by table13 ID, resolving only catalog-registered paths. A source-owned reader
captures inode/size/timestamps, checks them around each8192-byte read, yields
between chunks and revalidates the PDB before response publication. The cap is
4MiB; reads retain no filesystem lease between steps. Missing IDs return empty
artwork. High-resolution/unanalyzed selection arguments remain unsupported and
must be researched rather than silently mapped to the original image.

An independent unchanged Beat Link codec/AlbumArt control receives byte-identical
synthetic JPEG data (1246 bytes) and decodes80x80 pixels; unknown ID handling also
passes. The JPEG was generated locally by the reconstruction control, not copied
from a real USB. Targeted backend plain/ASan+UBSan/TSan tests cover original bytes,
missing ID, unsupported high-resolution request, chunk yields and file mutation
between reads with partial-buffer cleanup. No new full matrix, AArch64 execution,
native rendering or actual-artwork acceptance is claimed. See the
[artwork response record](acceptance/usb-hub-support-sharing-db-artwork.json).

### High-resolution artwork and combined transport control

The standalone backend now registers optional `_m` artwork companions derived
from indexed table13 paths and serves third-argument1 artwork requests. The
path convention matches pinned Beat Link8.0.0 `Util.highResolutionPath`.
Missing companions return empty responses; arbitrary filesystem images remain
unavailable. This supersedes the high-resolution pending statements above.
Unchanged Beat Link codecs and JPEG decoding verify exact synthetic80x80 and
240x240 images. File-catalog and metadata tests pass plain, ASan/UBSan and TSan.

The research server's explicit `--combined` mode now serves TCP metadata and
UDP file RPC from one staged source and generation, using a single owner loop.
A local control passed40 UDP attribute queries during the independent artwork
client run. Cancellation returned NFS STALE for the old handle and an empty
export list; shutdown passed. UDP checks use our own protocol control, not an
independent client. This is not streaming, native unmount, discovery, runtime
integration, AArch64 or actual-CDJ acceptance. The socket harness was tested
with a normal host build, not under sanitizers. See the
[combined artwork record](acceptance/usb-hub-support-sharing-combined-artwork.json).
No real AZ operations or publication occurred.

### Combined library browsing and full file downloads

The same standalone C service now passes an isolated-host control with unchanged
Beat Link browsing and Crate Digger full-file downloads launched concurrently.
The browser verifies2546 tracks,185 ordered nodes and8359 playlist memberships;
the file clients verify exact SHA256 hashes for the4419584-byte database and
10881597-byte MP3. Both transports share one staged catalog/generation. Packet
interleaving and latency were not measured. An unrelated file stays hidden, and
changing the owned temporary PDB invalidates the exported handle with STALE.
The fixed-port research mode refuses the initial network namespace and permits
only loopback interfaces. All fixtures are copied into an owned temporary tree;
the source USB is untouched. See the
[combined library record](acceptance/usb-hub-support-sharing-combined-library.json).

This replaces the earlier attributes-only limit for the host combined-service
control. Public discovery, multiple-source runtime lifecycle, native rendering,
emulator integration, concurrent local decks and actual external-player eject
still require validation. File download success is not playback acceptance.

### Reusable per-source transport lifecycle

`public-source-service.h` now owns the metadata view, eight bounded TCP sessions
and the UDP export adapter for one already staged library. Its caller supplies
nonblocking bound sockets, a provider identity and a mandatory peer policy used
for both transports. Each worker round admits at most one TCP client, dispatches
one datagram and advances each active session once. Withdrawal cancels the
source and closes its TCP sessions immediately while retaining the catalog for
STALE UDP responses. Accepted sockets are owned; listener/UDP sockets are borrowed.
The library must outlive the stopped service. Advertisement and unmount ordering
remain caller responsibilities, not implemented by this component.

The research server now uses this component. Independent full browsing and
PDB/MP3 transfer controls passed again after extraction. A focused plain,
ASan/UBSan and TSan test covers capacity, peer rejection, withdrawal, neighboring
source isolation, repeat destruction and listener ownership. That focused test
uses empty synthetic catalogs and stalled greetings; it does not prove two-source
streaming. The new test is included in the host matrix, but the full matrix has
not been rerun. See the [source service record](acceptance/usb-hub-support-sharing-source-service.json).
No mod worker, physical AZ operation or publication is included.

### Asynchronous source worker

`public-source-worker.h` now stages and serves one source on an owned pthread.
The controller supplies its generation, fresh incarnation nonce and bound
endpoints. Atomic requests distinguish withdrawal (close TCP, retain STALE UDP)
from full stop (release the view, library and root). A terminal-state join gate
must complete before the caller releases sockets, peer-policy context or mount.
Staging failure and early stop clean up on the same owner thread. No pointer to
the mutable library crosses into the controller.

The research server now uses this worker. The independent combined library
browse/PDB/MP3 control passed again. Focused plain, ASan/UBSan and TSan tests
passed eight start/stop incarnations, running withdrawal, join polling and
missing-PDB failure cleanup. Logs remain under ignored
`local/diagnostics/sharing-artwork-v1/worker-{library,sanitizers}.{stdout,stderr}`.
This does not yet attach workers to `community_refresh_presence` or the native
safe-eject flow. In particular, `safe_eject_source_poll` and the backing-mount
unmount phase still need the public advertisement/worker stop gates. No physical
AZ changes or publication occurred.

### Target worker lifecycle and native menu negotiation

The pinned AArch64 source-worker lifecycle test passed on the guarded localhost
emulator (kernel6.6.138-dirty), including staging, withdrawal, stop/join,
reconnection and missing-PDB cleanup. The owned temporary executable was removed.
See the [worker record](acceptance/usb-hub-support-sharing-source-worker.json).
This does not change supported firmware/kernel compatibility or establish mod
integration. No real AZ was contacted.

Review against the previously exercised Python provider found a native-client
gap in the C session: it accepted only single-argument setup. It now also accepts
the observed `[requester,20]` extended-menu negotiation and remembers that choice
per connection. Other second arguments are rejected. Extended item replies add
the same four trailing fields as the prototype (zero, two, empty UTF16 string,
track BPM); ordinary clients retain twelve fields. Focused session/menu tests
pass plain, ASan/UBSan and TSan, covering setup20, unknown21 rejection and both
row forms. These C extended rows have not yet been rendered by the native AZ
client; the earlier native prototype evidence is not acceptance of this change.
Logs: `local/diagnostics/sharing-artwork-v1/extended-sanitizers.{stdout,stderr}`.

### Source-owned database-port lookup

`public-db-port.h` implements the19-byte `RemoteDBServer` TCP query and returns
the source's two-byte network-order metadata port. It handles eight bounded
clients with fragmented reads, an absolute five-second deadline and the same
mandatory peer policy as the source service. Invalid requests and unavailable
sources close without publishing a port. The listener is caller-owned.

The source worker now optionally owns this responder alongside metadata and
file RPC. It becomes active only after staging succeeds and stops advertising
the metadata port when the catalog is withdrawn or invalidated. The isolated
research harness binds12523 only in its guarded fresh network namespace.
The combined library control verifies lookup, independent browsing, exact
PDB/MP3 transfers and port withdrawal following a temporary-fixture mutation.
Port responder and worker tests pass plain, ASan/UBSan and TSan; the full matrix
and native rendering remain pending. Evidence is in ignored
`local/diagnostics/sharing-artwork-v1/port-{library,sanitizers}.{stdout,stderr}`.
This is service-port lookup, not UDP device discovery or a native sharing
candidate. Those advertisement and mod-lifecycle integrations remain open.

### C public-provider packet encoding

`public-provider-packets.h` now builds the computer-library initial/claim,
stable identity and keepalive packets, activation responses, hello reply and
collection-slot media response used by the earlier isolated Python prototype.
Labels use strict UTF8 conversion to bounded UTF16BE; unsupported media slots,
short/malformed requests and oversized labels are rejected. Track counts saturate
at the protocol's16-bit field. This module only encodes packets; it does not bind
or send network traffic, choose addresses, resolve identity conflicts or schedule
activation. The prototype's fixed activation fields remain experimental.

An offline control executes the unchanged prototype's `status_service` function
with in-memory socket stand-ins and its `Activation.packet` helper. C activation,
hello and media replies match their complete bytes; identity/keepalive fields
match the declared fixture. This verifies translation, not independent protocol
correctness or native acceptance. Boundary/Unicode tests pass plain, ASan/UBSan
and TSan. Logs are in ignored
`local/diagnostics/sharing-artwork-v1/provider-{oracle,sanitizers}.{stdout,stderr}`.
Periodic transmission, per-peer activation and connection to the worker are
still pending. No real network packets, AZ operations or publication occurred.

### Provider scheduling and UDP worker adapter

`public-provider-state.h` adds the bounded claim/identity/keepalive schedule and
independent activation state for up to eight admitted peers per source. First
activation emits0x16 with one delayed repeat; subsequent requests emit0x47.
Peer state expires after60 seconds without activation traffic. Backpressure
does not advance a claim or allocate a new activation slot. Withdrawal clears
pending repeats and suppresses further announcements and replies.

`public-provider-udp.h` handles caller-bound nonblocking50000/50002 sockets,
validates peer policy/source port/datagram size, and suppresses the provider on
observing its number with a different MAC in supported identity/keepalive frames.
The source worker now optionally enables this adapter after catalog/service
readiness. A detected conflict stops the worker with EADDRINUSE. Endpoint creation,
address allocation and normal mod lifecycle integration remain caller work.

Focused scheduler/UDP/worker tests pass plain, ASan/UBSan and TSan. UDP tests use
two loopback addresses in a Docker network-none namespace. They verify initial
announcement, activation, withdrawal silence and identity-conflict suppression;
the full advertised worker and native client have not yet been tested together.
No automatic identity conflict recovery or complete native eject sequence is
claimed. See the [provider runtime record](acceptance/usb-hub-support-sharing-provider-runtime.json).
No physical AZ operation or publication occurred.

### First advertised C worker against the native emulator client

The standalone research server now has a guarded `--isolated-provider` mode:
one declared192.0.2.2 endpoint on `azpub1` inside an isolated namespace, bounded
lifetime, copied private PDB/audio/analysis fixtures and no physical NIC. This
mode enables discovery/status, port lookup, metadata and file RPC in one worker.
It is not linked into the mod. Existing loopback modes remain separate.

The first native attempt exposed an incorrect source-port admission assumption:
the AZ sends status queries from ephemeral UDP ports. The adapter now admits
these queries through the existing address/interface policy and replies to the
well-known50002 receiver. Focused plain/ASan/UBSan/TSan regression covers an
ephemeral query sender. A second attempt records the native hello request and C
hello reply, but no visible public source or browse acceptance. Review of the
previous successful prototype shows native portmapper requests to50111; the
advertised C harness now binds50111 instead of the independent client's111.
That change still requires the next native run. Failed/incomplete attempts are
retained under ignored `local/diagnostics/sharing-c-provider-v{1,2,3}`; they are
not passing source-sharing acceptance. The first failed bridge launch used an
occupied local NAT test port; its namespace was removed before retrying on a
different port. No physical AZ operation or publication occurred.

### First native discovery and track-browser acceptance of the C worker

The isolated native-client control now displays the C worker's public source
with2546 songs, then renders its track browser with titles, BPM and keys.
The source label is `Hub USB1 Research`; local/native USB rows remain visible.
This is one host C provider with copied private fixtures consumed by the
localhost native emulator, not the mod exporting a live stick.

RPC header capture located the earlier registration stop: the client received
an EXPORT list without a network group and issued no mount. The export adapter
now supports an optional bounded IPv4/netmask group. The advertised research
endpoint uses the previous prototype's `192.0.2.2/255.255.0.0` string; packet
admission remains the endpoint policy. The native run then issues mount,
READDIR and LOOKUP requests and displays the source. Default no-group replies
are preserved. Valid group encoding, invalid address/noncontiguous-mask rejection
and worker regression pass plain, ASan/UBSan and TSan.

The playlist view is blank and remains a failure requiring request tracing.
Track loading/playback, multi-source/native eject/concurrency and actual-CDJ
acceptance remain open. See the [native C browse record](acceptance/usb-hub-support-sharing-c-native-browse.json).
Screenshots and all raw packet/fixture evidence remain local and ignored.
No physical AZ operation, mod installation or publication occurred.

### Native C playlist rendering confirmed

Numeric request tracing and a settled screenshot confirm native root-folder
and nested-folder/playlist rendering from the C worker. The earlier blank
playlist screenshot was taken before the UI completed its asynchronous update;
it did not establish a protocol failure. No menu behavior change was needed.
Native requests include1105 `[16843777,0,0,1]` and1105
`[16909313,0,1,1]`, followed by eight-argument render requests for20 and21 rows.
The reviewed local screenshot shows both the root and nested folder panes.
This supersedes the playlist-failure statement in the preceding browse record.
See the [native playlist record](acceptance/usb-hub-support-sharing-c-native-playlists.json).

The research server has a compile-time numeric request observation hook and a
bounded5..600-second local test lifetime option. The source component's default
hook is a no-op; no private request strings are logged. The last scope's harness
wait was mistakenly left at170 seconds while its bridge duration was220. The
scope exited with that timeout and executed owned network cleanup. This does
not negate the reviewed rendering evidence, but is not a fully passing scope.
The next local scope script corrects the wait. Audio selection/loading was not
verified before the bounded server ended. No physical AZ operation or release.

### Native load and playback-position progression from the C worker

The native emulator now loads the selected real MP3 through the isolated host
C worker. Track1884's0x2102 load-info request and NFS READ requests are recorded;
the deck displays its title, full-track overview and detailed multicolor waveform.
After PLAY, reviewed screenshots show remaining time advancing from265324ms to
241369ms with waveform scrolling. This establishes native load and playback
position progression for one copied fixture, not captured audio or full-track
playback acceptance. The unchanged source MP3 is10881597 bytes with the previously
verified hash. No metadata or file-transport behavior changes were needed.
See the [native load record](acceptance/usb-hub-support-sharing-c-native-load.json).

This supersedes the pending-load statements for the single host-C/native-emulator
control. Live AZ-side export, multiple sources, all formats/analysis indicators,
concurrent local/remote use, disconnect/eject and physical model-specific trials
remain open. In particular, the screenshots do not accept beatgrid/bar indicators
or every analysis response. Source data, captures and screenshots remain ignored.
No physical AZ operation, mod installation or publication occurred.

### Live mounted-source combined AArch64 export

The standalone source worker now runs on the guarded localhost emulator and
exports the mounted hub0 directly over its isolated macvlan endpoint. Unchanged
Beat Link DeviceFinder discovers provider41 at192.0.2.3, then the unchanged
Client/menu codec verifies2546 tracks,185 ordered nodes and8359 memberships.
Two concurrently launched unchanged Crate Digger clients download the live
4419584-byte PDB and10881597-byte MP3 with their exact previously recorded hashes.
No source files are copied or written by this test. This closes the combined
live-source gap for one standalone AArch64 worker, not mod integration.
See the [live combined record](acceptance/usb-hub-support-sharing-live-combined.json).

The worker acknowledges withdrawal and exits0 after Q; this is a root/service
shutdown control, not native USB unmount or an external loaded-deck eject test.
This guest research endpoint serves port111 for FileFetcher. A50111 alias for
native clients remains necessary before production; the earlier host/native
control used50111 separately. Simultaneous clients were launched, but packet
interleaving and local/native deck concurrency were not measured here.

The first scope hit an occupied local NAT test port and cleaned up; the second
passed discovery/downloads but exhausted its65-second exhaustive-menu deadline.
The successful scope allows120 seconds for that complete traversal. No normal
interaction-latency acceptance is inferred. The reusable script now accepts an
explicit free NAT port and verifies remote child identity/termination during
failure cleanup instead of assuming SSH-client exit stopped the server. The
earlier timed-out server had exited before the follow-up process audit.
No physical AZ operation, firmware compatibility change or publication occurred.

### Dual RPC ports and externally observed source withdrawal

The per-source service now supports a second caller-owned UDP RPC socket. Both
aliases dispatch through the same staged catalog, generation and peer policy;
each worker round processes at most one datagram per alias. The advertised
research endpoints bind111 and50111 together, removing the previous guest/host
port split. Focused source-service/worker tests pass plain, ASan/UBSan and TSan,
including alias replies and borrowed descriptor ownership.

The live AArch64 control passes again with independent discovery, full menus
and exact PDB/MP3 downloads. An external lifecycle client retains a valid root
handle and completes the unchanged Beat Link Client handshake before withdrawal.
After E, both RPC aliases return STALE/empty exports, the TCP session and DB
port lookup close, and unchanged Beat Link DeviceFinder expires the source.
RPC lifecycle probes are repository-authored; discovery expiry and the metadata
handshake use independent library implementations. See the
[live withdrawal record](acceptance/usb-hub-support-sharing-live-withdrawal.json).

This retires the service, not the USB filesystem: no native unmount, external
loaded-deck playback/cache acceptance, neighboring-source or reconnect test is
included. No physical AZ operation or publication occurred.

### Source-incarnation controller

`public-source-manager.h` now coordinates up to eight source workers on one
controller thread. Presence/path/mount-identity changes create a new requested
incarnation. An identical observation leaves the current source alone. The old
worker must join before endpoint callbacks release or recreate its sockets;
each new incarnation receives a fresh generation and16 random nonce bytes.
Endpoint failures remain retired until a new observed incarnation, preventing
an unbounded retry loop. The drain predicate includes endpoint release and is
an additional lifecycle gate, not unmount proof.

Two real workers with empty synthetic libraries pass plain, ASan/UBSan and TSan
tests for replacement, nonce/generation freshness, neighbor isolation, retirement
and failed-open retry. The pinned AArch64 test also passes in the guarded local
emulator; its owned executable is removed. See the
[source manager record](acceptance/usb-hub-support-sharing-source-manager.json).
The endpoint allocator and native presence/safe-eject callbacks are not yet
wired to this controller. No multi-stick browsing, loaded external-deck eject,
physical AZ operation or publication is claimed.

### Managed network endpoints and two-phase retirement

`public-network-endpoint.h` creates six fresh sockets for one declared source
inside an already allocated namespace. A short helper thread performs setns,
interface IPv4/mask/broadcast/MAC checks and binding; the controller's network
namespace is unchanged. It binds RPC111/50111, discovery/status and the metadata
listeners to the owned interface. Partial failures close all new descriptors.
Interface creation/address allocation remain separate caller responsibilities.

The source manager now offers explicit withdrawal while retaining STALE RPC
responses; a later absent request stops/joins and releases endpoints. This
separates public retirement from the final drain needed before unmount. The
advertised research harness now uses manager callbacks with the socket factory
and fresh random incarnation nonces. Independent discovery/full-menu/exact-file
and external withdrawal controls pass again against the mounted emulator source.

A guarded AArch64 endpoint test runs from the emulator's principal namespace
against the distinct owned test namespace. It verifies namespace preservation,
six live descriptors, duplicate-bind partial cleanup, invalid MAC/broadcast
rejection and fresh reopening. Source-manager two-phase retirement tests pass
plain, ASan/UBSan and TSan. Loopback original/high-resolution artwork also passes.
See the [managed endpoint record](acceptance/usb-hub-support-sharing-managed-endpoint.json).
An initial endpoint run correctly rejected the test fixture's missing broadcast
configuration; the fixture now supplies it explicitly.

Native mod discovery/eject callbacks, automatic network allocation, multiple
live sources, aggregate memory/descriptor/scheduling budgets and native loaded
external-deck eject still require integration and validation. No real AZ changes,
release or compatibility expansion occurred.

### Measured protocol feasibility

- Independent Beat Link native controls retrieve7 root entries,20 playlist
  entries and2547 tracks. Crate Digger transfers the native4.4MB PDB and10.9MB
  audio with exact USB hashes. Native STOP with an NFS handle unmounts and later
  reads return STALE70; external loaded-track status was not simulated.
- The host prototype serves a snapshot of hub0's own PDB:2546 tracks,185 folder/
  playlist entries. The native client navigates folders, playlists and metadata.
- Eight public providers41..48 use distinct socket-bound virtual Ethernet/IP
  identities, independent menu maps and file handle namespaces. All eight have
  native track-browsing evidence. Filtered libraries101..808 distinguish routing;
  they are copies of one PDB, not eight live USBs. See
  [eight-provider browsing](acceptance/usb-hub-support-sharing-eight-provider-browse.json).
- An independent unchanged Crate Digger client retrieves complete PDB/audio
  from the fixture NFS service with exact source hashes. A native emulator deck
  loads one selected MP3 and advances playback position. No audible/decoded or
  full-track playback is claimed. Actual waveform/beatgrid/cue data is absent.

The successful multi-provider controls use real read-only file backends for
all providers and staggered startup. Earlier shared-MAC, empty-export and
simultaneous-start controls were unstable; they do not prove source-count
limits or isolate a single cause. Native computer icons above4 are blank,
while source labels and menus remain usable. Captures stay local/ignored.
Six portable suites/22 groups pass, including session isolation, bounded
shutdown, malformed stream rejection and eight-source handle replacement.

### Integration gates still open

The prepared target kernel config is4.4.194 with MODULES enabled and MACVLAN/
IPVLAN disabled. This is local prepared-config evidence, not a fresh physical
AZ inspection. Emulator6.6 virtual-interface support must not be extrapolated:
a compatible module or alternative transport still needs research/build.

A research-only standalone module now builds from the unmodified vendor GPL
MACVLAN driver with the pinned target config/compiler. Its stripped size is
29,592 bytes and vermagic matches4.4.194 SMP/preempt/aarch64. All79 undefined
symbols have static GPL export declarations, but Module.symvers is absent
and runtime resolution/load are untested. A pinned builder and source-export
auditor are provided as source only; the binary remains ignored. See the
[module build](acceptance/usb-hub-support-sharing-macvlan-module-build.json) and
[reproduction notes](HUB-LINK-TRANSPORT-BUILD.md). No installation occurred.

A new C root/file-lifetime component is implemented but not wired into the
public service or mod runtime. It confines relative paths, refuses symlinks
and non-regular files, caps reads/leases, rejects new operations on withdrawal
and drains accepted operations without waiting for a remote deck. Generation
replacement and neighbor isolation pass. All32 portable cases pass in each
plain/ASan+UBSan/TSan matrix; hook registration passes separately. The component
also executes successfully as pinned-toolchain AArch64 code in the emulator
with synthetic temporary files. See the [file-root record](acceptance/usb-hub-support-sharing-public-file-root.json).
File authorization and NFS handle/instance identity remain adapter obligations.
No public runtime integration or actual USB unmount is inferred.

The C NFSv2 READ body now uses source-owned file references and validates
device/inode to reject a replaced file. It encodes status/attributes/data with
zero padding and releases operation leases before network send. Its AArch64
test reads4096 PDB and8192 audio bytes from live emulator hub0; a host test
decoder verifies body fields and exact payload hashes. This is an SSH-emitted
body control, not C RPC/network interoperability or full-file playback. All33
cases pass in each portable matrix. See the [C READ record](acceptance/usb-hub-support-sharing-public-nfs-read.json).
RPC, owned opaque handles, authorized lookup/directory service and mod lifecycle
remain unconnected; published0.3.2 and the physical AZ are unchanged.

The bounded C ONC RPCv2 decoder now dispatches NFSv2 READ calls to that reader.
Truncated packets, invalid credentials and unsupported write procedures are rejected
before handle resolution. AUTH_SYS is syntax-checked only; its self-reported user
identity is not trusted as an OS credential. All35 cases pass in each portable
matrix, hook registration passes separately, and the pinned AArch64 RPC READ test
passes in the localhost emulator. See the [RPC READ record](acceptance/usb-hub-support-sharing-public-rpc-read.json).
This remains an unconnected component: no public socket, instance-scoped handle
catalog, directory/mount service or live metadata adapter is inferred.

A bounded source-owned C file catalog now issues32-byte opaque handles with a
caller-supplied fresh instance nonce and generation. Duplicate paths keep their
identity, references hold no descriptors, and withdrawal rejects all new resolves.
Targeted GCC/Clang plain, ASan+UBSan and TSan tests pass, including concurrent
withdrawal and neighbor isolation. See the [catalog record](acceptance/usb-hub-support-sharing-public-file-catalog.json).
The original256-entry layout is superseded by a compact dynamically allocated
catalog: up to65536 references and8MiB path bytes per source, with explicit ENOSPC
and no eviction. Hashed path registration and direct bounds-checked handle lookup
preserve identities through growth. A synthetic1025-path/read control, catalog/UDP
plain/ASan+UBSan/TSan tests and the independent Python transfer regression pass.
See the [growth record](acceptance/usb-hub-support-sharing-public-file-catalog-growth.json).
Full-library/global eight-source memory, exact capacity boundaries and allocation
failure controls remain open. This is not a full37-case or updated AArch64 run.

The C READ dispatcher and source catalog now run through a nonblocking UDP
adapter with mandatory peer policy and truncated-datagram rejection. Targeted
plain/ASan+UBSan/TSan controls and pinned AArch64 emulator loopback tests pass.
An independent handwritten Python wire client reconstructs30001 synthetic bytes,
checks EOF/write refusal and observes STALE after withdrawal. See the
[UDP record](acceptance/usb-hub-support-sharing-public-rpc-udp.json).
The research server is localhost-only with bounded lifetime and fresh nonce;
it is not a public provider or runtime daemon. Native mount/lookup/directory
procedures, real-library capacity validation and live metadata remain open. No unchanged
Crate Digger, actual-CDJ or full37-case acceptance is inferred from these tests.

C directory references now support the attached root and authorized nested folders
through confined operation leases. NFS GETATTR/LOOKUP bodies validate inode identity
and reveal only pre-registered nodes, hiding an existing unregistered file. Symlink,
type and traversal controls pass, as do replacement/withdrawal STALE checks. The
complete38-case plain/ASan+UBSan/TSan matrices and separate hook registration pass;
the pinned AArch64 synthetic tree test passes in the emulator. See the
[tree record](acceptance/usb-hub-support-sharing-public-nfs-tree.json).
GETATTR/LOOKUP now dispatch over RPC with strict native-style UTF16LE name
decoding, including surrogate pairs. The independent Python client traverses
accented/emoji folders from a root handle, queries the file and reconstructs30001
bytes over UDP. The updated38/38/38 matrix, separate hook registration and pinned
AArch64 tree/RPC test pass. See the [tree RPC record](acceptance/usb-hub-support-sharing-public-rpc-tree.json).
READDIR, mount/portmapper, unchanged Crate Digger and trusted live catalog
population remain open; this localhost harness is not the mod runtime.

The C service now adds paginated UTF16LE READDIR, synthetic read-only STATFS,
portmapper GETPORT and mountv1 MNT/EXPORT/UMNT. Cookies retain catalog identity;
reply bounds, invalid cookies, cannot-fit entries and strict UTF8 encoding are
checked. Peer UMNT never withdraws the USB; source withdrawal removes the export.
The independent Python client completes portmapper/mount/list/lookup/full READ
and checks withdrawal. All39 cases pass in each portable matrix; separate hook
registration and pinned AArch64 mount/export tests pass. See the
[mount/directory record](acceptance/usb-hub-support-sharing-public-rpc-mount.json).
This is still a localhost synthetic harness; unchanged Crate Digger, public
addressing and trusted live library/metadata integration remain unvalidated.

Unmodified Crate Digger0.2.1 now mounts and fetches files from the C service in
a fresh isolated loopback namespace. Complete synthetic Unicode30001-byte,
4419584-byte PDB and10881597-byte MP3 fixtures match exact source hashes. The
PDB/audio are ignored local copies of the emulator hub0 files, not a live export.
The guarded harness refuses port111 operation in the initial namespace; no physical
or emulator interface participates. See the [C Crate Digger record](acceptance/usb-hub-support-sharing-c-crate-digger.json).
This closes a C file-protocol interoperability gate, not metadata, playback or
runtime integration. Production component39/39/39 evidence remains the prior
mount record; only research harness/client code changed for this control.

A new bounded classic-PDB research reader follows only the active track page
chain and present-row flags. Its2546 IDs, titles, audio paths and analysis paths
match the existing Crate Digger snapshot exactly; four synthetic groups cover
structural/string bounds, cycles, duplicate identities and deleted rows. See the
[PDB path reader record](acceptance/usb-hub-support-sharing-pdb-path-reader.json).
This Python oracle is not an on-device parser or path authorization mechanism.
A bounded C implementation and live metadata/catalog population remain needed;
no native private requester or source server was changed.

The classic track reader is now implemented in bounded incremental C. One step
processes one page; the source adapter uses at-most8192-byte read leases and checks
generation/device/inode/size/mtime/ctime. Synthetic cancellation, withdrawal and
replacement tests pass without retained descriptors. All40 cases pass in each
portable matrix, with separate hook registration. A pinned AArch64 execution reads
the live emulator hub0 PDB: all2546 IDs/titles/audio/analysis paths match host C and
the Crate Digger oracle. See the [C reader record](acceptance/usb-hub-support-sharing-c-pdb-reader.json).
Staging/atomic publication, path authorization and complete metadata/playlists
remain unconnected. This standalone scan is not a new mod/provider or playback test.

A C staged file builder now populates independent catalogs from PDB audio/
analysis paths and known DAT companions, adding only required parents/PDB. Unsafe
paths fail the build; missing optional media is counted. The ready catalog is
withheld until full traversal and final source fingerprint validation; cancellation
withdraws only that public root. All41 cases pass per portable matrix, plus hook
registration. AArch64 staging on live emulator hub0 yields2546 tracks/10329 nodes,
408643 path bytes and16384 entry capacity;2529 audio paths are absent (17 present).
See the [file staging record](acceptance/usb-hub-support-sharing-library-file-staging.json).
The PDB-derived catalog now serves through a C RPC wrapper that blocks partial
BUILDING state and revalidates the PDB before valid file operations. Mutation
retires the catalog; malformed/write calls do not trigger PDB reads. Two unchanged
Crate Digger clients download complete PDB/audio fixtures with exact hashes and
measured overlapping READ intervals. Hidden unrelated-file and post-ready STALE
controls pass. Updated41/41/41, hook registration and AArch64 synthetic controls
pass. See the [library export record](acceptance/usb-hub-support-sharing-library-file-export.json).
This network test uses copied host fixtures (one audio), not live emulator USBs.
Complete metadata, public transport/lifecycle and concurrent local/eject remain open.

The standalone research harness now has a guarded emulator network mode for
192.0.2.3:111 on an owned azpub0 interface in a separate namespace. Exact6.6.138-dirty
and namespace/interface checks precede root/socket access; host invocation refuses.
Host/pinned AArch64 builds and the existing independent loopback flow pass. Native
emulator111/2049 are occupied, motivating namespace separation. No guest network
resource/listener has been created yet. See the [network preparation record](acceptance/usb-hub-support-sharing-live-network-preparation.json).
Next execute an owned, bounded macvlan/namespace live-root control with cleanup.

The standalone AArch64 C file service now serves the live emulator hub0 over an
owned guest MACVLAN/namespace192.0.2.3 and isolated host TAP192.0.2.2. Two unchanged
Crate Digger clients transfer complete PDB/audio with exact hashes and overlapping
READ intervals. Staging sees2546 tracks/10329 nodes; only17 audio files exist in
the image. Explicit service withdrawal drains leases; no USB unmount/deck-loaded
state was tested. Native NFS endpoints remain separate. See the
[live C service record](acceptance/usb-hub-support-sharing-live-c-file-service.json).
The initial interface attempt lacked a loaded6.6 MACVLAN driver. A local6.6 test
module loads and enables the control; all resources/module/QMP objects are removed
and independently checked. This does not validate the target4.4 module. The
explicit-artifact reconstruction script is test-live-library-network.py.
Public metadata/discovery, runtime lifecycle and concurrent native playback/eject
remain open; no standalone test is a new mod candidate or actual-CDJ acceptance.

C track extraction now includes comment and scalar/reference metadata: file size,
artwork/key/label/genre/album/artist IDs, bitrate/BPM/duration/rating. All16 fields
for2546 tracks match the Crate Digger v3 snapshot, and pinned AArch64 reads of the
live emulator PDB match host output. Updated41/41/41 and hook checks pass, with
nonzero synthetic scalar/comment assertions. See the
[track metadata record](acceptance/usb-hub-support-sharing-pdb-track-metadata.json).
This is extraction only: compact metadata indexing, referenced names/playlists,
TCP response encoding and native file-type/analysis coverage remain open.

A compact per-source C track index now retains all16 fields using scalar rows
and string offsets. Lookup is withheld until seal, keyed by exact generation and
track ID; duplicate IDs refuse, retirement blocks new queries without filesystem
leases, and neighbor indexes remain independent. All42 cases pass per portable
matrix plus hook registration. Host and live AArch64 index output matches all2546
Crate Digger rows. Measured row/text buffers reserve770048 bytes for this fixture
(excluding allocator/bookkeeping), with60-byte rows. See the
[compact index record](acceptance/usb-hub-support-sharing-compact-track-index.json).
It is not wired to staged source lifecycle or metadata TCP yet. Referenced names/
playlists and global eight-source memory/performance limits remain unvalidated.

The compact track index is now populated in the same staged PDB traversal as
the file catalog. Both become publicly accessible only after index seal and final
source validation. Cancel/failure or detected post-ready PDB change retires both;
missing audio keeps its metadata entry without a nonexistent file handle. Updated
42/42/42 and hook checks pass. AArch64 synthetic lifecycle and live2546-track/
10329-node staging pass, and independent concurrent PDB/audio transfers retain
exact hashes. See the [joint lifecycle record](acceptance/usb-hub-support-sharing-library-metadata-lifecycle.json).
Referenced names/playlists and metadata TCP remain unimplemented; this lifecycle
integration is standalone component work, not the mod runtime or a release.

Next implement a live per-stick backend and compatible public transport with
address ownership/conflict handling. Preserve local/private5/6 sessions, native
USB slots, independent SQL/tagged caches and0.3.2 Link continuity. Validate
concurrent local/remote browsing/audio, reconnect/replacement, and native-style
withdrawal/unmount with external loaded-track status. Then build/audit the exact
candidate and prepare separately authorized physical/model-specific trials.
No actual-CDJ, eight-source hardware or long-session acceptance is established.

## Latest publication: USB Hub Support0.3.2

[Release0.3.2](https://github.com/NachoCasta/nacho-az-mods/releases/tag/usb-hub-support-0.3.2)
is published and selected by the root catalog. Source/tag commit
`b03fcf14e7c1c2d302bffb3c18c9a4215790d410`; catalog commit
`6bb9348`. Both source CI jobs pass. A committed-archive rebuild reproduces the
accepted ELF exactly. Anonymous official SDK download checks and independent
catalog/package/member/icon/pinned-source checks pass. The physically accepted
package bytes, including candidate manifest wording, are unchanged. Earlier
release assets and tags are preserved. See the
[publication record](acceptance/usb-hub-support-0.3.2-publication.json).

[Index PR4](https://github.com/Kyle-Hosman/az-mods-index/pull/4) remains open and
draft with unchanged head7985a4e. Its body now covers0.3.2 continuity,59 hooks,
exact hashes, manual physical acceptance, emulator coverage and remaining limits.
Read-back matches;12 fresh index tests and the two-repository fetch check pass.
No reviews, comments or attached checks were present. See the
[PR update](acceptance/usb-hub-support-0.3.2-index-pr-update.json).
Earlier unpublished/pending-publication statements below are chronological
preparation history. Actual-CDJ, phase, eight-source hardware and long-session
acceptance remain open.

## Continuity candidate installed on the physical AZ

The user explicitly requested installation, exact pre-approval and reload.
Exact0.3.2 v5 was copied to the mods USB with0.3.1 rollback preserved, staged
through the normal loader, approved under the narrow SSH exception and activated
through one framework apply. Framework serial10 completes; PID3776252 loads
59 hooks with empty decks. USB, staged and reviewed ELF hashes match. See the
[delivery record](acceptance/usb-hub-support-link-continuity-az-delivery.json).
The user reports "works perfectly" after the requested loaded hub-track On/Off
continuity trial (continued playback, no warning triangle and fresh loading).
This is manual user-reported acceptance, not captured audio or an instrumented
physical test. No exact source count/cycle count or actual-CDJ/phase/long-session
acceptance is inferred.
No release, tag or catalog change was performed. Earlier statements that the
physical AZ was untouched describe the preceding emulator-only goal.

## Link continuity candidate ready for physical trial

Exact unpublished0.3.2 v5 is built and locally validated. It preserves owned
private members7/8 through the native mode reset and restores the separate
internal library gate immediately after the native setter. Native external
member removal and deck topology still execute; this is retention rather than
faster reannouncement or hidden warnings.

The [readiness audit](acceptance/usb-hub-support-link-continuity-readiness-audit.json)
maps every local goal gate to evidence. Plain/ASan+UBSan/TSan each pass31 portable
checks; SDK UI passes10/8/8. The94 source snapshot files and79 referenced local
evidence hashes match. Package SHA256:
`d461a7791ea0977eda3daf776933567197f19461b6abc73e4de370646b9caa81`;
ELF SHA256:
`a3d6c7ba438494008edf66fdda43d4be5ae702ed3afa4cdcb57791b9964d1add`.
The exact firmware tag, stable mod ID and kernel asset remain unchanged.

Empty, paused, playing native+hub, repeated On/Off and both private-member checks
pass in the emulator. Reviewed captures preserve labels and advancing positions
without mode-induced source warnings; fresh post-transition loads succeed.
Five hub sources and seven separate library DB descriptors are observed, with
four loaded local decks Off and native decks3/4 disabled On. Isolated software
peer master/SYNC70BPM, loss and reconnect72BPM pass. Individual loaded eject and
five-live-source whole-hub eject pass;2165 complete outgoing status messages pass
known-field privacy checks in the latter scope. See the
[plan](LINK-MODE-CONTINUITY-PLAN.md) and linked version-specific records.

Final emulator PID967182, normal framework apply serial28, has Link Off, EEEE,
five hub sources and seven library DB descriptors. Isolated network helpers
have exited and their namespace/QMP guard/channel are removed. One same-process
post-eject/replug capture had only four rows despite seven mounts/DBs; after
explicit unload and normal framework apply all five return. Keep this separate
replug observation visible. The existing cumulative core input-filter timing
guard remains unchanged; a fresh session passes eject but does not prove
long-session timing acceptance.

The [physical trial](LINK-MODE-CONTINUITY-AZ-TRIAL.md) is prepared. Emulator
lifecycle and sampled positions do not prove uninterrupted physical audio or
phase sync. Actual-CDJ, eight-source hardware and long-session acceptance remain
open. The physical AZ and published0.3.1 are untouched. This completes local
candidate readiness; physical testing needs separate action-specific permission,
and publication requires a later user decision.

## Official index PR updated for0.3.1

[Index PR4](https://github.com/Kyle-Hosman/az-mods-index/pull/4) remains open and
draft with unchanged head7985a4e and a diff adding only this repository URL.
Its description now covers0.3.1 native-body badges, the eject selector,57 hooks,
exact release hashes and physical UI approval, while keeping earlier network
and playback evidence version-specific. Known Link-mode interruption and remaining
validation limits are explicit. Read-back matches the prepared text. No comments,
reviews, inline feedback or attached checks were present at inspection. Fresh12
index tests and the two-repository fetch check pass. See the
[update record](acceptance/usb-hub-support-0.3.1-index-pr-update.json).

## Latest publication: USB Hub Support0.3.1

[USB Hub Support0.3.1](https://github.com/NachoCasta/nacho-az-mods/releases/tag/usb-hub-support-0.3.1)
is published and selected by the root catalog. Source/tag commit:
`8cbfa66af51c12e7baddc24cdce8630c0987d571`; catalog commit:
`0407a940ef4e0f94f58a49779feb284b747eb94f`. Both source CI jobs passed.
Anonymous catalog, package/ELF, icon and source retrieval checks passed, including
the official SDK repository download check. Published package and executable
are unchanged from the candidate installed and visually accepted on the AZ.
See the [publication record](acceptance/usb-hub-support-0.3.1-publication.json).

The user confirmed the physical UI looks good. This is appearance acceptance;
no new audio/eject/network or eight-device hardware coverage is inferred.
The known Link-mode interruption and other documented limits remain unchanged.
Earlier preparation and pending-review statements below are chronological history.

## USB Hub Support0.3.1 publication authorized

The user approved the native-body UI on the physical AZ and requested publication.
Publish the exact accepted v14 package, retaining its tested candidate wording:
package SHA256 `a449ef3122c4da039c5bd0d3342b3a686e1bdb7862cf71831e2dd6f9db684dc8`,
ELF SHA256 `60be7a089fe6cc71dcb404cfd80b96e2218da5f7a2ec63d36b8b70741a8bf462`.
Physical approval covers appearance; no new audio/eject/network acceptance or
eight-device hardware coverage is claimed. Release source CI and anonymous
catalog/package retrieval remain publication gates. See the
[0.3.1 release notes](releases/usb-hub-support-0.3.1.md).

## Native UI candidate installed, approved and activated on the AZ

The user approved the emulator screenshots and requested real-AZ installation,
exact-candidate pre-approval and reload. Exact native-body v14 is verified on the
mods USB and in framework-managed staging; the previous UIv3 package is retained
on USB. Pre-approval updated only this candidate's entry under the framework lock,
with the old approval list backed up on USB and unrelated entries preserved.
Initial delivery left PID2099107 running with `LLEE`. After the user unloaded
the tracks, fresh empty-deck telemetry and exact approval were verified before
one framework apply. New PID2362076 loads0.3.1 with57 hooks; framework state is
`done` (serial9), and the staged/mapped ELF matches the reviewed hash. The final
activation readback is `EEEE`. This verifies activation, not physical appearance,
playback or eject acceptance. See the
[native AZ delivery record](acceptance/hub-source-ui-native-az-delivery.json).

## Native badge revision: emulator screenshots ready for review

The user approved the emulator Browse and Waveform appearance from the native
reuse diagnostic. They authorized keeping the original USB1/2 bodies and using
the closest native font only for numbers3..8, preserving the original USB icon,
USB lettering and spacing. Implement this in private runtime composites; do not
export native artwork. Representative USB3/8 Source/Browse/Waveform screenshots
are saved locally and were approved by the user on2026-10-20. The approved eject
selector stays unchanged. The later physical delivery is recorded above;
publication remains outside the authorized scope.
The Source HUB bar retains the native LINK bar's22px overlay, including its
expected overlap with the icon. The user clarified that native overlay behavior
is desired; do not shrink the bar to20px or move/redraw the original icon.

Integrated native-body candidate0.3.1 v14 passes29 plain,29 ASan/UBSan and29
TSan portable components plus SDK UI10/8/8 checks. Exact emulator activation
prepares all24 native composites with zero outside-edit pixel differences.
The screenshot-only v15 maps two already-owned display labels to3/8; this is
not eight-device acceptance. Exact v14 is restored, diagnostic mapping removed,
decks empty, and native/hub Source rows remain after empty Link-mode recovery.
See the [native candidate record](acceptance/hub-source-ui-native-candidate.json)
and [implementation/evidence notes](HUB-BADGE-NATIVE-REUSE.md). Earlier custom
badge candidate statements below are historical and do not describe this build.

## Physical UI feedback: eject selector approved; source badges need revision

The user approved the appearance of the0.3.1 eject selector. Their Source and
Waveform photos show the custom hub icon/lettering differs from native USB in
font weight, size and spacing; the source badges are not visually accepted.
Revise the badge adapter to reuse the native icon, typography and layout at
runtime, changing only the hub identification and display number. Keep the
approved eject selector unchanged. Iterate in the emulator and save screenshots
for user review before another physical candidate delivery. No new real-AZ
operation is implied by this feedback. Earlier candidate-readiness statements
below describe the first iteration and do not override this visual rejection.

## UI candidate installed, approved and activated on the real AZ

The user requested installation and approval of exact UI candidate0.3.1.
All seven package members are verified on the mods USB, with0.3.0 retained there
for rollback. The normal loader staged the reviewed ELF and its hash was checked
against USB before exact-candidate SSH pre-approval. The previous approval list
is backed up on USB; unrelated entries were preserved under the framework lock.
Delivery initially left PID1356103 running0.3.0 with deck1 loaded (`LEEE`).
The user then requested reload; fresh empty-deck telemetry (`EEEE`) and approval
were verified before the single normal framework apply. New PID2099107 loads
0.3.1, its mapped ELF matches the reviewed hash, and framework status is `done`
(serial8). This verifies activation, not physical UI, eject or audio acceptance.
See the
[delivery record](acceptance/hub-source-ui-az-delivery.json).

## Hub source UI candidate: ready for physical UI validation

An unpublished local 0.3.1 v3 candidate adds hub-only Source/Browse/Waveform
badges, stable display numbers and the restyled mod-owned eject selector.
Published 0.3.0 remains unchanged. Package SHA256:
`183e2d8bdccd623ac757b2b298b195a499957d17672faf58f4237b8f1f059e7f`;
ELF SHA256: `ba9a31ffcba3492229a641a1feab78157797dad465fcb40cae599e1f9c4d2e8b`.
Frozen-v3 host checks pass 25 plain, 25 ASan/UBSan and 25 TSan components,
plus SDK UI 10/8/8 checks and the separate registration/negative controls.
The source-settings test uses the existing repository fixture because the build
export omits that research data. See the [candidate ledger](acceptance/hub-source-ui-candidate.json)
and [final AZ trial](HUB-SOURCE-UI-AZ-TRIAL.md).

Exact-v3 emulator checks cover five hub libraries, native USB controls in all
three views, Link Off/On, same-deck hub/native replacement, short STOP cancellation,
individual loaded-source eject, stable remaining numbers, removal of the selected
disk without retargeting, and whole-hub eject. Final mapped ELF matches the hash
above. The native mode-change warning remains outside this UI goal.

The initial v3 process hit the core's frame-filter time budget and the selector
refused ownership. Normal framework reload restored operation on the same bytes;
subsequent selector/eject checks passed. The same symptom exists in older evidence;
its cause is not attributed to this change. External ownership controls pass in
all three host-adapted widget families, but the synthetic media fixture did not
produce an external Source row. Actual external-CDJ UI, physical AZ input/audio
and long-session acceptance remain unvalidated. Earlier v2/Linaro observations
remain predecessor evidence. No real AZ operation or publication is included.

## Official index PR updated for0.3.0

[Index PR4](https://github.com/Kyle-Hosman/az-mods-index/pull/4) remains open and
draft, with unchanged head7985a4e and a diff adding only this repository URL.
Its description now covers0.3.0 hashes/source,54 hooks, independent private7/8
libraries, bounded physical Wi-Fi software-peer validation and the unresolved
loaded-mode interruption/warning. Read-back matches the prepared body. No comments,
reviews, inline feedback or attached GitHub checks were present at inspection.
Fresh12 local index tests and the two-repository fetch check pass; the fixture
matches the exact PR head after line-ending normalization. See
[update record](acceptance/usb-hub-support-0.3.0-index-pr-update.json).

## Latest publication: USB Hub Support 0.3.0

[USB Hub Support 0.3.0](https://github.com/NachoCasta/nacho-az-mods/releases/tag/usb-hub-support-0.3.0)
is published and the root catalog selects it. Source/tag commit:
`f6e69c2fefabf6ddbfbf1ac081591af7c5ef25b1`; catalog commit:
`3bcbad0710e31c99a2ae6bba764477074cc9816d`. Release-source CI passed both jobs,
including 22 plain/22 ASan+UBSan/22 TSan portable components and SDK UI10/8/8 checks.
Anonymous catalog/package/icon/source retrieval passed with accepted package and
ELF hashes unchanged. See the [publication record](acceptance/usb-hub-support-0.3.0-publication.json)
and [release notes](releases/usb-hub-support-0.3.0.md).

Known issue: mode changes briefly disconnect/reannounce hub sources and can leave
loaded-track warning triangles. Physical network tests used a software peer over
Wi-Fi; actual-CDJ interoperability and audible phase alignment remain unproven.
Earlier publication/preparation statements below are chronological history.

## Release preparation: USB Hub Support0.3.0

The user authorized publication of the exact tested package. Pinned SDK packaging
reproduces package SHA256
`38d70598ba8103d970f1b87a651c1e9268affbf4784e221bfb50ae87b0802eee`
and ELF SHA256
`5e8ecd3ee0426bec83d5fc0f73d7f7327aa982c89206a67c97d6fc5634be26bc`.
The immutable package retains experimental manifest wording; updated catalog and
release notes describe hardware results and the unresolved loaded-mode disconnect
and warning triangle. Publication/anonymous download results follow CI below.
The chronological preparation statements below preserve their original scope.

## Current authorization clarification

Fresh CAMI load with the external Wi-Fi peer active now has user confirmation.
After selecting native1 MASTER, the repeated player3 handoff succeeds at128BPM;
the user loads/plays another CAMI track on deck2 and reports success. The60-second
window records799 statuses and133 beat callbacks, with both native decks reporting
synced128BPM. Same native1356103 and CAMI remain afterward. Track selection/audio
are user-reported; protocol corroborates status/tempo, not content identity or
audible phase alignment. First attempt refused handoff without an observed native
master and is retained as a setup negative. See
[fresh hub-load evidence](acceptance/usb-hub-support-0.3.0-wifi-fresh-load-hardware.json).

Same-player Wi-Fi software-peer reconnect now passes bounded protocol checks:
after the prior peer exits, the new player3 observes native1 as master, negotiates
ownership and announces128BPM; native1 reports SYNC at128BPM. The60-second window
receives603 status callbacks but0 beat callbacks. CAMI/native1356103 persist; new
deck2 load and uninterrupted audio await user confirmation and are not implied by
loaded/SYNC status. See [reconnect evidence](acceptance/usb-hub-support-0.3.0-wifi-reconnect-hardware.json).

The real Wi-Fi master/SYNC trial now passes bounded controls: software player3
requests master from native1, receives confirmation in52.952ms, then announces
126BPM. The user enables SYNC and reports success/continued CAMI playback; native1
reports synced126BPM. The60-second window receives714 status and128 beat callbacks,
then the peer stops normally. CAMI and native1356103 remain during/after the trial.
This is protocol plus user-reported audio, not captured phase/latency or actual-CDJ
acceptance. Peer-loss/reconnect and the mode-transition warning remain open.
See [Wi-Fi master/SYNC evidence](acceptance/usb-hub-support-0.3.0-wifi-master-sync-hardware.json).

The user refined the physical mode-change result: switching Link in either
direction briefly disconnects/reannounces CAMI and leaves its already-loaded track
with the source-unavailable triangle; the native USB source/track remains connected
in the same comparison. Do not describe the earlier basic confirmations as seamless
loaded-source continuity. This matches the emulator warning limitation but remains
an unresolved regression/UX issue, not an unavoidable accepted behavior. The user
asked to record it and continue testing; Link is now On with CAMI playing.

Physical Wi-Fi peer discovery/status now passes a bounded control after the user
closed rekordbox:20seconds passive observation receives30 discovery-port packets
and67 beat-port packets; explicit physicalLAN Beat Link player3 status trial
receives200 status updates from native players1/2, with player1 master. No master
request or SYNC change was sent. Native1356103 and CAMI remain after cleanup.
Beat callbacks were0; no audio/playback/sync acceptance follows. IPv4 Java stack
and host harness guard correction were needed; supported mod bytes are unchanged.
See [physical Wi-Fi status evidence](acceptance/usb-hub-support-0.3.0-wifi-peer-status-hardware.json).

The user authorized exploratory physical peer testing through the existing Wi-Fi
network based on prior successful use. Read-only preflight finds the AZ on Wi-Fi
only and LAN carrier0. The new receive-only LAN observer could not bind: Windows
rekordbox PID47564 owns UDP50000/50001/50002. No peer packets were sent, no firewall
or interface changed, and rekordbox was not terminated. Await user closure before
retrying; this refusal does not establish a Wi-Fi or mod failure.

The user now confirms manual hub browsing/loading/playback with native PRO DJ LINK
Off, then confirms continued local hub operation after enabling Link and checking
retained playback/source availability/new load. Record:
[0.3.0 local-mode hardware acceptance](acceptance/usb-hub-support-0.3.0-link-modes-hardware.json).
These are user reports, not instrumented audio or external-player acceptance.
Physical external-player communication and long-session coverage remain pending.

The user explicitly authorized a narrow manual approval-list exception for the
exact tested mod. It is recorded in CLAUDE.md, AGENTS.md and the testing policy.
USB and staged ELF hashes were verified, the prior approval list was backed up
on USB, and only the usb-hub-support entry was replaced under the framework lock
with an atomic rename. Unrelated entries were verified unchanged. With fresh EEEE,
normal framework apply restarted1312330 ->1356103. The real AZ now reports
`mod=usb-hub-support|1|0.3.0|` and successful initialization with54 hooks, exact v5
ELF on kernel4.4.194. This establishes activation only, not browsing/audio/Link
coexistence hardware acceptance. Local records: `az-link-v5-preapprove-result.txt`
and `az-link-v5-after-authorized-approval.txt` under `local/diagnostics/`.

The user subsequently unloaded all decks (fresh EEEE) and authorized reload.
Supported `apply` IPC completed native restart1876957 ->1312330. Framework staging
matches the exact v5 ELF. The core now reports0.3.0, state6,
`New mod found: allow?`; the candidate is not active yet. Await the user's normal
Allow action, followed by supported reload if requested. No approval file edits.
Local snapshot: `local/diagnostics/az-link-v5-awaiting-allow.txt`.

After the user's installation authorization, exact supported v5 was copied to
the real mods USB at `/media/usb/sda1/MODS/community-mods/usb-hub-support`.
All seven members passed SHA256 verification; ELF remains `5e8ecd3ee0426bec83d5fc0f73d7f7327aa982c89206a67c97d6fc5634be26bc`.
The previous0.2.1 folder is preserved on that USB under
`MODS/backups/usb-hub-support-0.2.1-before-link-v5`. No approval/internal files
were manually edited and no apply/reload was requested. Native PID1876957 still
reports loaded0.2.1 and deck state LELE; activation awaits unloaded decks and
supported framework approval/reload. Copying is not candidate hardware acceptance.
Local receipt: `local/diagnostics/az-link-v5-usb-copy.json`.

The user explicitly removed mandatory remove-USB/full-restart recovery verification.
Use the normal AZ-MODS workflow for the emulator-validated candidate: USB delivery,
supported installation/approval/activation, bounded checks and standard restarts.
Manual internal controller edits remain forbidden. Recovery is unverified, but is
no longer an activation gate. This supersedes recovery-gate statements in the
chronological checkpoints below. See [current policy](AZ-TESTING-POLICY.md).

Read-only physical preflight identified running0.2.1, exact firmware tag and
kernel4.4.194, the expected cached/USB loader hashes and loaded decks1/3. No package
copy, activation, restart or recovery test was performed during that inspection.

## Final local preparation checkpoint — 2026-10-19

The supported v5 candidate and bounded emulator evidence are ready for final user
validation. Published **0.2.1** remains unchanged; no release, tag or catalog update
was performed. Physical recovery and activation remain deferred. This checkpoint
supersedes pending-task statements in the chronological notes below; their original
artifact-specific failures and limitations remain evidence, not current passes.

The [final preparation audit](acceptance/usb-hub-support-link-final-preparation-audit-v5.json)
verifies the package/ELF, all 65 frozen production files in both working source and
build export, and 324 local raw receipts across 17 v5 records. The
[requirement ledger](acceptance/usb-hub-support-link-goal-evidence.json) describes
the bounded matrix and the [final trial](PRO-DJ-LINK-CANDIDATE-TRIAL.md) separates
completed emulator work from physical prerequisites.

The final partial-eject control now passes: loaded source3 eject completes before
Off/On, four remaining rows survive both settled modes without resurrecting the
ejected source, and physical virtual-USB replug restores five rows and seven healthy
partitions. Same native PID860, On/Off/On epochs1/2/3; final decks EEEE. This is not
a simultaneous eject/mode race, forced partial-failure test or audio acceptance.
See [partial-eject evidence](acceptance/usb-hub-support-link-partial-eject-modes-v5.json).

## Current hardware workflow authorization

The user permits emulator-first USB-only candidate delivery, normal mods-framework
activation/checks and standard restarts for USB Hub Support / PRO DJ LINK work,
subject to recovery by removing the mods USB and fully restarting. Manual internal
AZ file/settings/approval/preload edits are forbidden. Framework-managed staging
is persistent internal storage, not RAM; a one-restart/no-mod recovery result has
not yet been established for the deployed loader. Until recovery is verified,
continue local/emulator preparation. See [testing policy](AZ-TESTING-POLICY.md).
Earlier action-specific authorization statements below preserve historical scope.

Read-only copies now establish the emulator's actual firmware-to-cached-loader
chain. The effective cached and USB loaders are byte-identical and do not retry a
refused initializer when the selected populated cache is unchanged. This supports
the startup-order concern but does not prove a cold-boot outcome or the real AZ's
loader behavior. See the [emulator loader audit](acceptance/usb-hub-support-link-recovery-emulator-loader-audit.json).


## Active coexistence work: experimental candidate, runtime validation pending

The published release remains **0.2.1**, with external PRO DJ LINK blocked and
its existing warning unchanged. Earlier coexistence trials below use opt-in
research overlays. A local **0.3.0** candidate now uses the supported framework
build and existing exact kernel paths; it is not published or hardware-ready.

The latest supported local candidate is under
`local/integration/link-coexistence-v5/`. Package SHA256:
`38d70598ba8103d970f1b87a651c1e9268affbf4784e221bfb50ae87b0802eee`;
ELF SHA256:
`5e8ecd3ee0426bec83d5fc0f73d7f7327aa982c89206a67c97d6fc5634be26bc`.
It retains private media-state ownership and removes a historical USB1-only
mode-replay restriction. The expanded portable mode test covers all eight
implemented slot assignments, absent/reserved exclusions, stale epochs, eject
deferral and 20000 transition/write races. All 22 plain/22 ASan+UBSan components
and SDK UI 10 plain/8 ASan checks pass; hook claims remain the same 54. Runtime
activation and both-direction recovery without duplicate rows now pass with
empty decks in PID859956. Loaded On/Off/On now restores all five rows,
all five sources load successfully, and four local decks advance in Off; On
retains two loaded decks. The final matrix remains incomplete. See
the [all-slot replay build](acceptance/usb-hub-support-0.3.0-all-slot-replay-candidate-build.json).

Exact v5 now also passes [loaded mode transitions](acceptance/usb-hub-support-link-loaded-modes-v5.json)
and [cold On startup](acceptance/usb-hub-support-link-cold-on-v5.json). The cold
fixture maps USB STOP2 to the virtual hub through one offline service environment
override; package bytes and real-hardware default roots are unchanged. See the
[image provenance](acceptance/usb-hub-support-link-emulator-eject-root-image.json).
In first native PID997, individual loaded-source eject and long-press removal of
the remaining four hub sources pass while both tracks remain loaded and their UI
clocks advance. Both native USB mounts survive. The isolated player3 obtains
master and deck2 follows126 BPM; 570 full1152-byte statuses pass the classified
private-source checks. A later On-to-Off change does not resurrect ejected rows.
See [eject/peer evidence](acceptance/usb-hub-support-link-loaded-eject-peer-v5.json).
These are bounded emulator UI/protocol results, not audio or real-CDJ acceptance.
Native yellow source-unavailable warnings remain visible after mode changes and
eject. [Cold Off](acceptance/usb-hub-support-link-cold-off-v5.json) now passes
in first native PID851 without restart, with five private rows and both native
mounts. [Exact-v5 USB-absent recovery](acceptance/usb-hub-support-link-normal-recovery-v5.json)
also passes on the emulator: cached init refuses with-40 in first/settled PID759,
no private workers/aliases start, and native USB1 browses/loads content1944 with
advancing UI playback. The framework remains mapped on that first recovery boot;
its normal loader writes a stock environment for the next boot without restarting
the application. Deterministic stale queued-stage controls, final adversarial transport coverage
and physical4.4 recovery still require final evidence.

A fresh exact-v5 lifecycle boot now also passes documented held-SYNC Instant
Doubles from native USB1 and private hub media, negotiated peer master transfer,
native peer removal and same-player reconnect, and a new hub load while following
128 BPM. A private-to-native content1944 replacement exposes the native standard
and auxiliary identities in the same first native packet, then remains visible
in189 packets while the other private deck stays masked. Earlier predecessor
visibility timing remains historical. See [ID/peer/native controls](acceptance/usb-hub-support-link-id-peer-native-v5.json).

Physical virtual-USB removal/reconnection on the same port/key changes devnum3
to8 and permits fresh normal load and held-SYNC retry. Another349 full statuses
pass classified private identity and selected-marker checks after reconnection.
See [replug/retry controls](acceptance/usb-hub-support-link-replug-retry-v5.json).
The old track had reached its end before unplug and was restarted from cache
while absent; this is not uninterrupted playback across removal. Manual stale
LOAD/Instant Doubles attempts did not establish queued provenance or execution,
so deterministic queued-stage cancellation remains an open final validation gate.

An offline [controlled request-delay variant](acceptance/usb-hub-support-link-request-delay-v5-build.json)
is now built for the remaining queued-stage test. The reconstruction verifies
all65 frozen production inputs and changes only `link-request-origin.h`: a
USB-resident one-shot marker delays a captured private common-stage request for8s
on emulator kernel6.6 before the unchanged stale check. Its package changes only
the ELF; hook claims/parser registration remain identical. The
[controlled runtime](acceptance/usb-hub-support-link-request-delay-v5-runtime.json)
now passes normal type1 pending-source removal and documented held-SYNC type5
replacement at this common-stage boundary. The normal request captures
generation23907/devnum8; targetdeck2 remains unloaded after removal and a later
fresh load succeeds. The ID request captures generation24331/devnum9; same-port
remove/add completes in0.118s during the pause, old target position remains
unchanged, and fresh normal+ID retry succeeds after devnum10. All seven partitions
remain healthy. These are derived diagnostic results; they do not establish all
native async task/destructor/final-admission interleavings on the supported ELF.
Tracks were unloaded through native UI, then the supported-v5 package was restored
through USB copying and normal framework Allow/Reload. Exact supported ELF5e8ecd
is active in native PID69363,54 hooks,EEEE, with five private rows and both native
USB rows restored. See [restoration evidence](acceptance/usb-hub-support-link-restored-after-delay-v5.json). The real AZ and published release remain untouched.

Exact supported v5 now passes [native USB mode controls](acceptance/usb-hub-support-link-native-usb-modes-v5.json):
USB1/USB2 browse and load with Link Off, both tracks continue through native On,
and explicit native track unload settles EEEE in each mode. Native screenshots
show no source warning triangles in this control, unlike carried private tracks.
All five private sources and both native backings remain available.

[Bounded adversarial controls](acceptance/usb-hub-support-link-adversarial-native-v5.json)
reject six external member7/8 UDP claims and repeated reserved-requester5/6 TCP
sessions, while requester3 receives a native response. Former private test codes
0xe0/e7 receive native errors rather than hub data. Private7/3 and8/7 still load
following these trials;265 complete private statuses pass the classified checks.
Actual fd-number reuse remains host-test scope, not a claim about these TCP runs.

The previous primary-interface mirror transport was UDP-capable but failed TCP:
slirp injected resets when it saw the mirrored guest's SYN-ACK. A queue-all capture
identifies the reset's NAT MAC. The new tracked
`link-primary-tap-transport.py` separates NAT management from the TAP-owned peer
address; six packet/namespace tests and a60-second canonical run pass, including
SSH during and after cleanup. See [transport evidence](acceptance/usb-hub-support-link-primary-tcp-transport-v5.json)
and the [peer guide](../src/azhub/research/link-peer/README.md).
No guest/core/mod changes were required for this transport correction.

The [native metadata baseline](acceptance/usb-hub-support-link-native-visibility-baseline-v5.json)
now reproduces the initial50 zero standard-source packets per deck with the core
active and USB Hub Support refused at init-40. Native auxiliary locations remain
valid in all50 prefix packets in both runs, then standard native tuples remain
visible. Candidate cadence spans~10s for the prefix versus baseline~6.7s; packet
count equivalence does not prove equal latency or the native internal cause.
The baseline's explicit TCPquery occurs~18.7s after first native identity, ruling
out that query as its trigger. No immediate metadata-availability guarantee is
made. The earlier no-core attempt did not load tracks and is retained solely as
a non-comparable preparation. Physical4.4 recovery remains deferred to user presence.

The supported candidate was restored again through a fresh positive cold boot
following the baseline: first/settled native PID860,54 hooks,NRestarts0,Link On,
EEEE, five private/two native rows and seven healthy mounts. See
[final cold restoration](acceptance/usb-hub-support-link-final-cold-restoration-v5.json).
No production source or package bytes changed during these controls. The final
requirement audit remains open; the real AZ has not been accessed.

The preserved media-state predecessor is under
`local/integration/link-coexistence-v4/`. Package SHA256:
`2edbd629de26a76521391b3c9a3f807c29491e393673099e803f407018f6d780`;
ELF SHA256:
`efff2ac033d7362846b11a9384ac75700a2a01591861585b0fc26e48a81a4547`.
It incorporates private media-state ownership after the controlled positive
experiment below. All22 plain and22 ASan/UBSan portable components, SDK UI10
plain/8 ASan checks, unchanged supported hook registrations and strict seven-
member package verification pass. Its bounded functional run is recorded below;
recovery and the full functional matrix are incomplete. See the
[media-state candidate build](acceptance/usb-hub-support-0.3.0-media-state-candidate-build.json).

The preserved parser-corrected predecessor is under
`local/integration/link-coexistence-v3/`. Package SHA256:
`aeb199c064c5ad043e4b8933b7e631119ab5f187e58921b5fcdb3b46c6471031`;
ELF SHA256:
`123703cd5028a3840ef61d13eb640ef7d1ab7c05a2e4741daac622e2a10ae19e`.
It fixes the parser registration omission described below. The pinned SDK build,
22 plain and 22 ASan/UBSan portable tests, SDK UI checks and the new supported-
preprocessor registration audit pass. TSan was not rerun for this correction.
Coordinator review confirms all seven package members and all 65 production/
exported source hashes. Bounded integration and normal-loader recovery have now
been repeated on these bytes; the full functional matrix remains pending. See the
[corrected candidate build record](acceptance/usb-hub-support-0.3.0-parser-candidate-build.json).

The worktree now has private media-state corrections in `link-status.h`,
`linkfake.c`, `test-link-status.c` and subsequent all-slot replay changes in
`link-mode-recovery.h`/`test-link-mode-recovery.c`. The preserved v3 export does
not contain these changes. V4 and v5 builds are separately frozen. The v3 results below continue to
identify the previous artifact, not the changed worktree or experimental build.

The corrected artifact now passes bounded normal-framework integration on the
corrected emulator image: PID 10961 loads 54 hooks, including `0xef05c8`, and
private source A browses its 2547-track library, loads a present track and shows
advancing playback. All five hub backings were connected at power-on; the source
screen shows native USB1 plus five private rows without the extra native USB2
entry. Only A was loaded in this step. The isolated primary-network adapter
passes ARP/ICMP and preserves SSH; Link peer acceptance remains separate. See
[v3 integration evidence](acceptance/usb-hub-support-link-normal-activation-v3.json)
and its [independent audit](acceptance/usb-hub-support-link-normal-activation-v3-independent-review.json).
The audit verifies all ten raw evidence hashes and the exact three image changes,
plus the source/browser/playback/unload screenshots. The earlier same-boot network
smoke test is explicitly separate from the final activation's Link behavior.
Exact-hash cold boots now pass and have an independent review of all 13 frozen
records: positive first PID 729 initializes 54 hooks with all five hub sources
present at power-on; negative first PID 465 refuses with -40 before effects and
remains stable without a restart. A later ordinary native USB loads and shows
advancing playback while the candidate stays inactive. See
[v3 recovery](acceptance/usb-hub-support-link-normal-recovery-v3.json) and
[independent recovery audit](acceptance/usb-hub-support-link-normal-recovery-v3-independent-review.json).
This is the 6.6 emulator path only. Physical 4.4 recovery and the complete
mode/peer/eject/privacy/native-replacement matrix remain pending.

The subsequent exact-ELF functional run (native PID 703) exposes an open source
visibility issue. With the native music/mods USB enumerated as USB2, all five private
servers acknowledge announcements, but the source screen lacks player 7/slot 7
and player 8/slot 7. The three visible private sources load tracks across four
decks with advancing UI clocks in Link Off. A later loaded Off-to-On transition
leaves the expected two local decks, but cannot establish source recovery:
the runtime investigator found an OHCI unrecoverable error at guest uptime
726.75 seconds and all five hub devices disconnected around 727.28 seconds,
before the Off packet window at 922.81 seconds and the subsequent transition.
The absence of private rows therefore does not establish a mode-recovery bug.
The source log confirms all five become absent and their aliases are released;
settled mounts contain only the native USB. Repeat the source-transition matrix
with stable backing devices before changing its recovery logic. Frozen local evidence is under
`local/diagnostics/link-normal-loader/matrix-*`. The Off packet audit covers 408
complete outgoing statuses and passes known private-field and track-marker
checks after unexpected controller loss, rather than proving fresh attached-source
privacy; native replacement and other lifecycle windows remain separate controls.
See [Off status audit](acceptance/usb-hub-support-link-matrix-off-known-status-privacy.json).
The [independent controller-loss review](acceptance/usb-hub-support-link-matrix-controller-loss-review.json)
records the frozen evidence hash and guest-uptime ordering.
The [partial functional matrix](acceptance/usb-hub-support-link-functional-matrix-v3-partial.json)
preserves 71 local evidence files with matching hashes and sizes verified by the
coordinator. It includes the negative source-list result, native restoration and
clean shutdown. A bounded peer handoff/126 BPM follow observation is recorded
[separately](acceptance/usb-hub-support-link-matrix-peer-handoff-review.json);
it occurred after controller loss and does not complete live-hub coexistence.

A normal offline emulator image reconstruction adds the matching xHCI PCI module
and its bounded early boot dependency. The new initrd SHA256 is
`6bf35ea5db8fca4b9ec8d95342d253a33e7fdbc230e2582f3f37a7fab7c725b8`.
The [independent image review](acceptance/usb-hub-support-link-emulator-xhci-image-independent-review.json)
confirms exactly four added entries and no changes to the remaining image,
including firmware, normal loader and USB renotifier. This is an emulator fixture
change only. A [bounded xHCI startup control](acceptance/usb-hub-support-link-xhci-five-no-native-control.json)
now confirms successful module initialization, six mounted disks and five private
rows through 303 seconds in first PID 974, followed by a clean shutdown. Its nine
raw evidence hashes and sizes match. Native music enumerated on `3-1` and had no
native source row, so it does not resolve native USB2 coexistence. A second EHCI
controller is being used to test native `2-1` while xHCI serves the nested hub.
Workload stability and recovery on the new fixture remain pending. The candidate
package and ELF remain unchanged.

The subsequent both-native cold boot now reproduces the slot7 visibility defect
without controller failure: exact candidate PID 762 initializes 54 hooks, all
seven physical USB partitions are mounted, and all five private announcements
are acknowledged. The source screen shows native USB1 and USB2 plus only the
three private slot3/slot8 rows; private player7/slot7 and player8/slot7 are absent.
The fifth nested disk `sdg1` is mounted by the mod's existing fallback helper.
Frozen local inputs are `both-native-init-state.txt` and
`both-native-source-startup.png` under `local/diagnostics/link-normal-loader/`.
This meets the prerequisite for the reviewed diagnostic overlay; it does not
identify the failing predicate or justify changing synthetic status bytes yet.
See the [independent stable-baseline review](acceptance/usb-hub-support-link-slot7-stable-baseline-independent-review.json).
The separate diagnostic ELF `bfe4a141a60bfe60cfdcecf517fdab83d14ca2913ead413e8675e50ad7c9b69a`
now reproduces the missing rows after supported framework activation with 55
hooks. Both missing private MediaIDs reach native mount callback entry, and
their capability calls return true. This narrows the next investigation to the
downstream root/list logic; entry logging does not prove successful completion
or insertion. See [diagnostic runtime evidence](acceptance/usb-hub-support-link-slot7-diagnostic-runtime.json).
These diagnostic bytes are not the supported candidate or final acceptance.

The subsequent 56-hook diagnostic confirms that both missing private slot7 keys
are inserted into the native root tree and source model with mounted state set.
Counts fall before later source additions, and the settled screen lacks those
two rows while all seven backing mounts remain healthy. This changes the next
investigation from initial admission to subsequent removal; the observation does
not identify the removal caller. See the
[closure runtime record](acceptance/usb-hub-support-link-slot7-closure-diagnostic-runtime.json).

A separate 58-hook removal/status diagnostic is now built and packaged locally,
with ASan/UBSan forwarding and consumed-payload/key checks, supported registration
checks and exact seven-member package verification. It observes the native
removal body, exact-key erase and actual outgoing private status fields without
changing packets or suppressing removals. Its package
SHA256 is `27b2903f2b2129b54b4d273ae2a738015a912a1a906b3b7014c59154039305d5`.
The production candidate and previous diagnostics remain unchanged. See the
[removal diagnostic preparation](acceptance/usb-hub-support-link-slot7-removal-diagnostic-build.json).
Normal emulator apply/Allow/Reload now activates those exact bytes in PID776827
with58 hooks. Both missing full keys are removed by native event2, and exact-key
erase reports one removed node for each, called from0x1c1dd90. Seven backing
mounts remain healthy and all decks remain empty. Startup private status records
alternate packet0x45e between0 and4, but their bounded window ends before private
mounts; this is a hypothesis lead, not contemporaneous causal proof. The removal
body's caller0x237d800 is queued dispatch and does not identify the original
consumer. See the [removal runtime record](acceptance/usb-hub-support-link-slot7-removal-diagnostic-runtime.json).

An isolated experiment now generates each private member's USB/SD/USB2/USB3 state
from its own present-source mask. With the same58 observers and unchanged seven
backings, PID792310 retains both native rows and all five private rows, reaches
root count8, and records no native remove/erase calls in its activation segment.
Both formerly missing player7/slot7 and player8/slot7 libraries browse2547 tracks,
load "Algo más" and show advancing playback clocks. Native unload returns EEEE;
all seven mounts remain healthy. This supports the correction but does not prove
full five-source loading, mode transitions, eject or peer coexistence on final
supported bytes. See the [experimental build](acceptance/usb-hub-support-link-slot7-owned-media-experiment-build.json)
and [runtime evidence](acceptance/usb-hub-support-link-slot7-owned-media-experiment-runtime.json).
The same behavior change is now in production source without diagnostic hooks;
supported runtime/regression validation is pending.

The exact v4 supported ELF now passes normal-framework activation in PID820744
with 54 hooks. In Link Off it retains both native USB rows and all five private
rows; all five private sources browse/load, and paired four-deck screenshots
show advancing playback. A live-backing privacy window contains 415 complete
1152-byte statuses with known private fields/selected markers clean. Native
Off-to-On confirmation leaves LLEE and preserves cached playback on 1/2, but only
the two private USB1 rows return. All seven mounts stay healthy. The v4 source
explicitly skips non-slot3 replay based on an older three-fixture experiment;
the resulting missing 7/7, 8/7, 7/8 rows are a recovery failure. Independently of
that failure, isolated peer3 requests native master1 and becomes master, and
deck2 follows 126 BPM for 60 samples; 370 complete On statuses have clean known
private fields. No audio-phase or real-CDJ acceptance is claimed. See the
[v4 partial matrix](acceptance/usb-hub-support-link-functional-matrix-v4-partial.json).
The v5 correction replays every present unreserved source after completion;
both directions must be tested for retained sources and duplicate rows.

Exact v5 normal apply/Allow/Reload initializes 54 hooks in PID859956 under
persisted Link On. Native confirmed On-to-Off-to-On transitions produce epochs
1/2/3, with all five private sources and both native USB rows restored each time.
After the return to On, selecting the first native row and seven encoder steps
reaches final MODMENU; the bottom list view contains the five private rows without
duplicates. All decks stay EEEE and all seven mounts remain healthy. An initial
Utility selection mistake changed Load Lock rather than Link; the native UI
restored Unlock and the actual Link modal/epoch3 are separately preserved. This
record proves empty-deck recovery, not loaded-transition or peer/eject acceptance.
See [v5 activation and empty transitions](acceptance/usb-hub-support-link-normal-activation-v5.json).

The preserved first candidate package SHA256 is
`f6c43ca830bfb2a7935b08720bb211d9955aa95729c3519199f792df615433a7`;
its ELF SHA256 is
`b6462687443b8ae55f9306a3b65988e4d5989bfffad1f2b144bce46571fdf03b`.
**This first packaged candidate is not ready for use:** its normal-framework
functional trial exposes a missing database-parser hook. The only parser claim
is inside the non-community preprocessor branch, so the supported build installs
zero claims at `0xef05c8`; private sources appear but browsing waits without
admission tickets. A corrected source/build check and separately preserved new
artifact are being prepared. Earlier initialization and recovery results below
remain evidence for these original bytes, not for a replacement build.
The missing-hook reproduction is preserved in the
[supported-parser negative record](acceptance/usb-hub-support-link-supported-parser-negative.json).
The source correction now has an independently reviewed, single parser claim in
the actual supported preprocessor output. A new registration check also exercises
missing/duplicate negative controls; rebuilt-artifact runtime acceptance is pending.
SDK build/package validation, all 22 portable components in plain/ASan/TSan
(66 invocations), and the separate SDK UI harness pass. TSan required a scoped
local container seccomp allowance after its first attempt failed before executing
a test. Independent coordinator review confirms all seven package members and
64 exported source hashes match the build record. Normal-loader activation and
bounded emulator cold-boot recovery now pass as detailed below; final functional
regressions remain pending. Research-binary results do not substitute for testing
these exact bytes. See the
[candidate build record](acceptance/usb-hub-support-0.3.0-coexistence-candidate-build.json).
The same 64 source files and final manifest have now been promoted into
`src/azhub/linkfake/` and independently hash-checked against that export. No
generated binary was copied into tracked source, and no commit or push was made.

Normal-framework setup activation now passes on the exact packaged ELF: after
managed removal of legacy `hub-libraries`, supported enable/apply and UI Allow/
Reload, the core reports `loaded 0.3.0 on ep147-9e9454e357389ece (53 hooks)`.
The native process is PID 22891 with HubMount/HubTransport workers and no
`AZHUB_*` environment controls. This is setup activation, not an independent
cold-boot result, functional regression or recovery pass. See the
[activation evidence](acceptance/usb-hub-support-link-normal-activation.json).

Independent positive/negative cold boots from the same populated, enabled cache
now pass on this exact emulator fixture. With the mods USB present, first native
PID 659 initializes 53 hooks and remains stable without a rescue restart. With
no mods USB attached or embedded, first PID 458 refuses initialization with -40
before patching, starts no candidate workers, and remains stable through the
settled capture with zero restarts. A separate later hotplug of an ordinary USB
without mods supports native browsing/loading and advancing playback while the
candidate stays refused. All 12 frozen evidence hashes and the native playback
screenshots were independently reviewed. See the
[recovery record](acceptance/usb-hub-support-link-normal-recovery.json) and
[independent audit](acceptance/usb-hub-support-link-normal-recovery-independent-review.json).
This establishes the 6.6 emulator/application path only: the physical 4.4 kernel
patch and applicable real-loader recovery still require witnessed validation.
Five-hub functional, packet-privacy and native same-ID replacement controls on
the packaged artifact remain pending.

The first five-hub fixture also exposed extra native USB2 announcements. Frozen
scripts and journal evidence attribute those writes to emulator patch37, which
overwrites the real loader's renotifier with a script that maps nested hub paths
to native USB2. Both the pristine firmware script and pinned core exclude those
paths. A workspace-local correction to the normal emulator image build preserves
the real-loader script and omits its conflicting emulator poller; four branch
fixtures pass. The corrected image SHA256 is
`b3c0b20a7a502615d0f45ca7e961099662451812770d0da63c928820e21f7c09`;
the native application, firmware loader and normal loader drop-in are unchanged.
Hub-at-start behavior must still
be retested. See the [renotifier provenance](acceptance/usb-hub-support-link-emulator-renotify-provenance.json).

Emulator evidence covers local hub browsing/loading, five source rows, native
Link mode transitions (four local decks Off; two local decks On), simulated
external-player discovery, master transfer, tempo following and native master
recovery. These results span different diagnostic binaries; they are not a
single integrated candidate acceptance. They do not establish audio-phase sync
or real-CDJ interoperability.

Current implementation priorities are request-origin/cancellation across queued
loads and cached-source privacy during failed or native replacement. Request
origin now survives native asynchronous common-load and allowance tasks in bounded
emulator trials. Reconnecting during either queued closure cancels the stale browser
request without the misleading recording alert; a fresh replacement-source load
succeeds in the same process. A separate final-admission guard now checks the
original physical origin against the complete native media/content key and rejects
missing provenance. Normal loading, a late reconnect rejection and fresh loading
from the replacement pass on one diagnostic binary. A later diagnostic binary
also passes native-API Instant Doubles, queued reconnect cancellation through the
native owner thread, and fresh replacement loading/copying. This is not UI-gesture
or audio-phase acceptance; superseded cleanup and exhausted-registry cases remain
host-modeled. A combined supported integration remains incomplete. See
[Instant Doubles evidence](acceptance/usb-hub-support-link-id-queue-cancel.json),
[browser cancellation evidence](acceptance/usb-hub-support-link-queue-cancel-browser.json)
and [final-admission evidence](acceptance/usb-hub-support-link-queue-final-origin.json).

Full external status capture has exposed another concrete privacy gap: following
abrupt physical disconnect/reconnect, retained hub tracks can report the native
player1/USB3 tuple and content ID. The existing safe-detach ticket classifier does
not cover this path. The fix must preserve committed loaded-source provenance
across physical removal, independently of pending requests and safe-eject tickets.
A repeated content ID at extended packet offset `0x194` is now traced to an
auxiliary TrackLocation row. The native serializer emits ten such rows at
`0x18c + 0x40*i` and another at `0x428`, each containing its own media identity;
their ownership cannot be inferred solely from the sending deck. A narrow mask
for explicitly private media references now passes host tests, together with the
committed-origin classifier for abrupt removal. The combined diagnostic capture
confirms clean auxiliary rows but still exposes standard source tuples before
the first USB STOP interaction. The committed-origin classifier incorrectly
required the unrelated safe-eject controller to have been captured first. Its
correction passes the focused sanitizer regression, which fails when the old
condition is restored. Diagnostic binary `0a0f041d768f8840e7b276384ac0af22280ccaaab5a2512a2a2750cee1adc3fc`
now passes known-field and selected-marker checks across 418 fresh and 421 cached
post-reconnect full statuses, before any USB STOP interaction. Both decks keep
advancing. This closes that bounded diagnostic reproduction, not all extended
packet semantics or final supported-artifact acceptance. Native same-ID replacement
reporting remains a required control. See the
[corrected runtime evidence](acceptance/usb-hub-support-link-known-status-privacy-v4.json),
[owner-zero audit](acceptance/usb-hub-support-link-committed-status-owner-zero.json) and
[location-schema evidence](acceptance/usb-hub-support-link-status-location-schema.json). Clearing standard
media fields alone does not establish complete packet privacy. The preliminary
integrated builds below predate this corrected runtime result. See the
[negative physical-disconnect capture](acceptance/usb-hub-support-link-physical-cache-status-negative.json).

After those fixes, integrate the tested research changes into supported source,
build/package with the exact firmware guard and rerun the mode, network, hub and
eject regressions on that one artifact. Physical activation remains deferred until
the remove-USB/full-restart recovery requirement is verified. The
[implementation plan](PRO-DJ-LINK-IMPLEMENTATION-PLAN.md) records exact experiments
and unresolved limits. The observations below retain their historical order and
must be interpreted with their individual binary hashes and test scope.

An isolated preliminary integration now exists under ignored
`local/integration/link-coexistence/`. Its 26 changed-source hashes match the
snapshot, and its patch applies to the current source tree. The SDK-built ELF
SHA256 is `661a9af8e66668789d1c0d478749e65211be7fbc5bd522837a6b782888192cf4`
(133416 bytes). It retains the exact firmware descriptor and supported kernel
initialization, composes the parser ownership logic into one hook, and adds an
early matching-USB-assets initialization guard. The guard precedes mount recovery,
kernel loading, hooks and workers; the community build disables the standalone
constructor. This is source/build review, not boot recovery evidence. Queue
cancellation is excluded from this snapshot, and no integrated emulator or
hardware acceptance is claimed. Both USB-present and USB-absent normal-loader
cold boots remain necessary because early USB mount timing can also refuse a
legitimately installed package.

The newer isolated `local/integration/link-coexistence-v2/` now includes the
request-origin and Instant Doubles cancellation implementation plus both privacy
fixes, without diagnostic markers, artificial delays or the diagnostic native API
caller. Its 64 recorded source hashes match, the patch applies, and the interim
SDK ELF SHA256 is
`5b9e0da1cfee91e074db2b3bf0fa4f223c52eef650afb43965ed4fe8b1d7ba78`
(143392 bytes; GLIBC 2.17 imports). The standard portable runner passes all 22
components in plain GCC and ASan/UBSan (44 invocations). This snapshot predates
the correction for an uncaptured safe-eject controller. Instant Doubles uses
supported inline hooks and the native cancelled-task
cleanup path; the attempted executable-segment vslot claims were rejected and are
not used. This earlier staged ELF did not run in the emulator. It has been
superseded by the packaged candidate at the top of this document; it must not be
used as that candidate's runtime evidence. Final artifact regressions and
normal-loader cold boots remain pending. Hardware activation remains deferred
until recovery is verified.

## Coexistence experiment chronology

A queued-load reconnect fault now exposes another release blocker: pausing the
common load closure for eight seconds, unplugging/replugging its source and
resuming allowed the old request into the allowance stage. Deck2 still showed
NOW LOADING approximately56 seconds after submission, without final admission
or completion in the captured log. Normal loading passed on the same diagnostic
binary. This is emulator-only evidence; cancellation and request-origin handling
remain unresolved. See [queued reconnect evidence](acceptance/usb-hub-support-link-queued-replug.json).

An identified coexistence blocker: a five-source whole-hub eject after loaded
On/Off/On was refused because the exact loaded-track proof for deck1 could not
be established. All five backing mounts remained present. Playback/mode tests
do not establish safe eject after mode changes. An emulator-only diagnostic
variant now logs native unload state/mode/media/epoch to investigate this;
no fix or hardware candidate is claimed. See
[refusal evidence](acceptance/usb-hub-support-link-mode-eject-refusal.json).

An opt-in load-origin retention experiment now passes a bounded follow-up:
one loaded hub track survives Off/On/Off and whole-hub eject; all five backing
mounts and private aliases disappear, native completion is observed, and cached
playback telemetry advances. The probe preserves mode2 load provenance and
checks the captured physical USB identity before detach. Disconnect/replug
negative coverage, queued-load races and production integration remain open.
See [retention evidence](acceptance/usb-hub-support-link-mode-load-retention.json).

The retention experiment also rejected an old cached load after its USB was
disconnected and reconnected as the same sdX/source slot with a new physical
generation. All five mounts stayed present. Reloading deck2 from the new
connection then allowed whole-hub eject with advancing cached playback. The
extracted origin-check helper passes ASan/UBSan tests; its rebuilt binary is
not yet installed. Queued-load correlation and seamless handling of old cached
tracks across reconnection remain open. See
[origin/reconnect evidence](acceptance/usb-hub-support-link-load-origin.json).

Latest coexistence research: separate private requester identities, pinned TCP
ownership and a partial UDP ingress guard now run in isolated QEMU. Three hub
fixtures enumerate; a loaded hub deck follows a simulated 126 BPM master, and
six external private-identity UDP markers are rejected. This is experimental,
not a hardware candidate: protocol coverage, receive-option/peek semantics,
lifecycle races and complete mode/regression tests remain open. See the
[implementation plan](PRO-DJ-LINK-IMPLEMENTATION-PLAN.md) and
[latest UDP evidence](acceptance/usb-hub-support-link-udp-integrated.json).
The following observations retain the earlier experiment chronology.

Latest peer testing corrected a simulator startup race: the new handoff role
waits for the existing master before requesting ownership. Two sessions of
player 3 now show wire-confirmed master transfer, with the loaded hub deck
following 129 and 127 BPM across disconnect/reconnect. Playback telemetry advanced
through both sessions and the gap. Automatic native-master fallback timing
remains unproven; this is protocol/emulator evidence only. See
[master reconnect evidence](acceptance/usb-hub-support-link-master-reconnect.json).

A loaded comparison without USB Hub Support now reproduces the short post-loss
status window: deck statuses stop about 9.3 seconds after the last peer packet,
while a later screenshot explicitly shows the local MASTER label and continuing
playback. Missing wire assertions alone therefore do not establish failed master
reclamation. This comparison requires the AZ-MODS core's emulator library key
provider; it is not completely stock firmware. Exact fallback timing remains
unmeasured. See [core-assisted baseline](acceptance/usb-hub-support-link-core-fallback.json).

The matching candidate subsequently reclaimed local MASTER without UI input:
the label was absent in the screenshot at nine seconds after the harness stop
event and present at eleven seconds, with advancing playback. The run used
three hub sources, member8/USB1 playback and 145 synced/nonmaster observations
at 129 BPM before disconnection. These are bounded screenshot observations,
not an exact timeout or audio-phase measurement. See
[candidate fallback evidence](acceptance/usb-hub-support-link-ui-fallback.json).

The same candidate now retains five hub source rows through loaded On/Off/On
changes. With Link Off, four loaded decks advanced; with Link On, decks3/4
unloaded while decks1/2 continued, and deck1 followed a simulated external
master at127 BPM. The fifth source (member7/slot8) browsed and loaded, although
its row lacks a native USB icon. All decks used content495, so this does not
establish different-track cache isolation. The loaded-track warning triangle
remains unresolved. See [five-source evidence](acceptance/usb-hub-support-link-five-source-modes.json).

Expanded socket research found native recv/MSG_PEEK usage outside the earlier
recvfrom-only inventory. An opt-in runtime audit then observed all three Link
UDP ports using recvfrom with flags0 and 4096-byte capacity during mode changes,
external traffic and hub playback; no read/recv use on those ports was observed
in these runs. The loaded external-peer session recorded 165 valid deck 1 status
samples. This supports the exercised receive path, not all indirect paths or
generic peek/option semantics. See [runtime audit](acceptance/usb-hub-support-link-udp-read-audit.json)
and [expanded static inventory](acceptance/usb-hub-support-link-socket-read-static.json).

The earlier socket research adds a reproducible static inventory: five direct native
recvfrom sites use flags0, no recvmsg PLT import is present, and reviewed direct
option8 setters use SO_RCVBUF rather than IP_PKTINFO. Indirect/library paths
remain unaudited, so this does not establish general option/peek compatibility.
See [socket inventory](acceptance/usb-hub-support-link-socket-static.json).

Latest eject/mode research: whole-hub loaded eject overlapped a native Off
request and completed with advancing cached playback telemetry. A subsequent
correction distinguishes finalized eject reservations from active withdrawal:
one ejected USB no longer blocks mode recovery of the others. The corrected
candidate passed partial eject plus Off/On/Off, retaining exactly two live
sources while the third stayed unmounted; member8 loading/playback afterward
also passed. See [partial-eject evidence](acceptance/usb-hub-support-link-mode-partial-eject.json)
and [whole-hub overlap evidence](acceptance/usb-hub-support-link-mode-eject.json).
These are distinct binaries and limited emulator checks, not hardware acceptance.

The preceding research candidate also defers an old completion epoch while a
newer transition is active. Its adapter test passes ASan/UBSan and TSan and
fails against the previous header as expected. An emulator On/Off/On sequence
retained three sources, followed by successful hub loading and 308 synced,
nonmaster deck 1 status samples at 126 BPM from the isolated peer. Native
transitions did not overlap in this UI test; pending-request handling remains
unvalidated in firmware. See [epoch/peer evidence](acceptance/usb-hub-support-link-mode-epoch.json).

The preceding mode adapter serializes its internal-cache check/write with native
transition entry and protects source replay with the source mutex. Its actual
adapter passed 20,000 modeled concurrent cycles under ASan/UBSan and TSan.
The exact new binary also passed an unloaded Off/On/Off emulator cycle with
three source rows. Loaded/external-peer and rapid-change coverage remain open
for this binary; see [serialization evidence](acceptance/usb-hub-support-link-mode-serialization.json).

The preceding research-only mode-recovery overlay completes Off/On/Off cycles
with exactly three source rows, first unloaded and then with one loaded,
advancing deck. It separates native network change detection from the internal
library-cache override and republishes withdrawn USB1 rows. A loaded-track
warning icon remains; concurrency, eject overlap, additional-source coverage
and external-peer regressions are still open. This is emulator telemetry, not
audio or hardware acceptance. See [mode-recovery evidence](acceptance/usb-hub-support-link-mode-recovery.json).

The preceding mode regression also confirmed four loaded, advancing local decks
from hub sources after starting with Link Off. However, switching live from
On to Off lost two source rows until application restart. This remains a
functional blocker, not an accepted restart requirement. See
[four-deck/transition evidence](acceptance/usb-hub-support-link-four-decks.json).

Active development is PRO DJ LINK coexistence. The isolated QEMU/WSL Ethernet
peer now establishes native discovery and status exchange with Beat Link 8.0.0:
Link off announces local identities 1..4; Link on announces 1/2, with the peer
assigned 3. Peer beat transmission is observed. No mod coexistence or audio-sync
acceptance is claimed. Beat Link truncates the AZ's 1152-byte statuses to 512
bytes and reports an unknown packet type; this remains a test-harness limit.
See [implementation plan](PRO-DJ-LINK-IMPLEMENTATION-PLAN.md) and
[native baseline](acceptance/usb-hub-support-link-native-baseline.json).

An emulator-only member 7/8 overlay has now enumerated three hub fixtures,
browsed sources 7 and 8, loaded two decks and ejected the whole hub while
retaining advancing cached playback telemetry. Physical Link remains filtered.
This is limited alternative-identity feasibility, not external coexistence,
audio or hardware acceptance. See the [feasibility record](acceptance/usb-hub-support-link-member-feasibility.json).
A separate private-loopback admission experiment is in progress.

The latest isolated-emulator external-traffic experiment exchanged peer status
while hub sources remained available, including member-8 browsing/loading and
playback telemetry. It is not ready for hardware: outgoing statuses expose the
private source identity, loaded master/sync adoption has not passed, and ingress,
handshake ownership and lifetime/race gates remain incomplete. See
[initial coexistence evidence](acceptance/usb-hub-support-link-initial-coexistence.json).

A subsequent idle-role correction and sync-before-play sequence demonstrated
following the simulated external master at 123 BPM, with native sync=true and
master=false. This is protocol/UI tempo evidence, not audio phase or loaded
master-handoff acceptance. See [tempo-follow evidence](acceptance/usb-hub-support-link-tempo-follow.json).

## Published release: USB Hub Support 0.2.1

Release: https://github.com/NachoCasta/nacho-az-mods/releases/tag/usb-hub-support-0.2.1
Source/tag commit: `0c21db0816a6f21bf6eff3aef9ace06b6d7055b7`.
[Source CI](https://github.com/NachoCasta/nacho-az-mods/actions/runs/39175867655)
passed behavior-tests and sdk-build; all 43 accepted build-source blobs match the
commit. The user authorized publishing the accepted progress. The package is
byte-identical to the selected emulator/hardware candidate: SHA256
`6a33b8dd496bb2130289cf7e49299b3cf24a29e4895afcf4c460184d37bd265f`
(355839 bytes); executable SHA256
`9feec49c06f10dd01e191ed5094f68eb1e7e7b35cd9ed76d757a8477a37a945b`.
The root catalog now advertises 0.2.1. Anonymous catalog/package download,
size/hash/content validation, all seven package members, the icon and pinned
public source manifest passed. See [publication evidence](acceptance/usb-hub-support-0.2.1-publication.json).
No rebuild or further hardware action occurred during publication. Previous
release assets and tags are retained.

Individual loaded-track eject from NACHO PLUGS passed on the real AZ. The user
reported complete playback of one track after eject. SSH confirmed absent backing
and library-alias mounts while the physical device remained connected, retained
exact-media cache proof, unchanged PID 1876957 and mounted CAMI/native USB1.
See [hardware acceptance](acceptance/usb-hub-support-0.2.1-az-loaded-eject.json).
The exact candidate also passed individual and two-playing-deck whole-hub eject,
seek/loop, replacement-media, reserved LOAD and failed-load regressions in QEMU.
Portable sanitizer/model, two-compiler and package checks passed. The source
snapshot and evidence are in [the ledger](acceptance/usb-hub-support-0.2.1-loaded-summary.json).

Long-session native memory and input timing remain unvalidated. Twelve local
cycles kept owned tickets/proofs/FDs/threads bounded, but native RSS grew 244580 KiB
from cycle 2 to 12. Same-connection load/unload controls stayed stable; a partial
unload-before-eject control reproduced growth without a loaded native-detach
request. Two QEMU attempts encountered core input-filter budget enforcement;
failed attempts are recorded separately from acceptance. The user chose publication
of the accepted functional progress with these limits disclosed. Broader loaded
hardware, all-format/cache-capacity and eight-source acceptance are not claimed.
Further real-AZ tests/restarts require their own authorization. Public 0.2.0
assets/tag remain available and unchanged.

## Previous release: USB Hub Support 0.2.0

Release: https://github.com/NachoCasta/nacho-az-mods/releases/tag/usb-hub-support-0.2.0
Source/tag commit: `1c75601c527fbdd821b5872a334f345aecb60396`.
Source CI: https://github.com/NachoCasta/nacho-az-mods/actions/runs/39158328964
Both behavior-tests and sdk-build passed. All 38 accepted source blobs match the
release commit. Package SHA256:
`1c24d4ea3e73dbfc7650e4ac9c47ee310a10ee883c1efbea41455458e6f41f09`
(350722 bytes); ELF SHA256:
`09b8ffa89b73ff9c5c402d81955bd6c567c2b520edb5c3f38fd00adcf865bd5b`.
Anonymous catalog/package download, size/hash/content validation and pinned public
source access passed. See [publication record](acceptance/usb-hub-support-0.2.0-publication.json).
The published package is byte-identical to the locally accepted and hardware-tested
candidate. Two-stick hardware observations are recorded below. Loaded-track
source eject remains unsupported; its refusal-banner visibility issue is disclosed.
The user explicitly chose publication of this progress and deferred loaded-track
support. Earlier draft/no-publication statements below preserve historical state.

Next development: research native cached playback and safe eject with loaded
tracks locally first; real-AZ changes/tests still require action-specific approval.


## 0.2.0 publication decision

The user explicitly requested committing and publishing the current progress after
successful two-stick hardware trials. The exact accepted package is retained.
Loaded-track ejection remains unsupported; the missing refusal banner is disclosed
in the release notes and deferred with that follow-up. Broader hardware coverage
is not claimed. Earlier no-authorization/pending-hardware statements below are
historical; see the current hardware record and release notes.


## Latest real-AZ activation

The user explicitly authorized installing, approving and activating the selected
09b8ffa8 stability executable. All seven USB package members were verified; the
previous diagnostic and approval file were backed up. Stock apply restarted the
application from PID 94558 to 1320281. A fresh snapshot showed EEEE and exact
firmware tag; the settings executable hash and loaded-mod log match the candidate.
User confirmed short-press selector open/BACK and individual safe eject after
unloading the track. Read-only SSH confirmed the hub filesystem and library
alias were unmounted, native USB1 stayed mounted, and the journal recorded state 2.
A loaded-track attempt kept the source available but no rejection message was
observed. User also confirmed whole-hub hold with one stick; a second SSH observation
confirmed hub backing/aliases absent, native USB1 mounted and journal state 2.
Two-stick hardware trial then passed: user confirmed browsing/loading CAMI
after individually ejecting NACHO PLUGS. After reconnecting both, whole-hub hold
removed both sources. SSH confirmed both hub backings/aliases absent, both
journal entries state 2 and native USB1 still mounted (same PID 1320281).
Loaded-track refusal-message visibility and broader acceptance remain open. See
[functional observations](acceptance/usb-hub-support-0.2.0-az-stability-functional.json) and
[activation record](acceptance/usb-hub-support-0.2.0-az-stability-activation.json).
The historical development handoff below predates this activation.

## Latest development status

Final local evidence: [stability summary](acceptance/usb-hub-support-0.2.0-stability-summary.json).
A/B emulator hotplug bypassed CLI snapshots; original fixture image byte identity
is unknown. The helper is corrected and independently tested with a disposable image.
No real AZ operation was performed during this autonomous continuation.

Active 2026-10-18 stability work follows `SAFE-EJECT-PUBLICATION-AUDIT.md`.
The last authorized real-AZ deployment was a separate diagnostic ELF:
`ae005285d2d131f1e2dfa7c9fa50723cd8aa4540e428758676cd7e8992a372de`.
No functional eject test was performed with it. The user is unavailable and
asked for autonomous local progress, deferring requests for participation or
additional hardware actions. Do not deploy further local changes implicitly.

Local stability preparation is complete for a separately authorized AZ trial. Event-driven panel readiness replaces
an invalid idle heartbeat deadline; completion adapters now use sixteen fixed
slots. Alias mounts use direct syscalls, workers/helper execution/logging use SDK
services, streams request close-on-exec, resources precede claims, and partial
initialization leaves adapters inactive with prepared workers stopped. Private
tagged caches are prepared before publication, without a native-cache fallback
on allocation failure. Production surface text is clipped to its area.

Latest selected local ELF:
`09b8ffa89b73ff9c5c402d81955bd6c567c2b520edb5c3f38fd00adcf865bd5b`.
Validated package: 350722 bytes, SHA256
`1c24d4ea3e73dbfc7650e4ac9c47ee310a10ee883c1efbea41455458e6f41f09`.
The independent SDK build/package also passes without compiler warnings; it is
not assigned this executable's runtime acceptance. See
`acceptance/usb-hub-support-0.2.0-stability-timeout-build.json`.

This revision latches a helper timeout or unknown helper state, inhibits new
fallback launches and refuses eject while completion is uncertain. An empty
`/run/azhub-mount-fault` marker preserves inhibition across application reloads;
the UI asks users to leave the hub connected and restart the AZ. All twelve
portable suites passed plain, ASan/UBSan and TSan again, including argument,
timeout, unknown-state and marker-restoration cases. Production UI passed SDK
10/8/8 checks. Native helper-timeout validation passed, including actual stop, refusal and
persistence across application reload. Selected-binary topology, partition,
reserved-load, failed-load and exFAT checks also passed; normal UI/main/STOP, journal failure and final resource observation also passed.
These tests do not instrument the proprietary firmware adapters or prove the
entire mod leak-free.

The preceding 04d5d9a1 executable passed matching SOURCE/BROWSE capture, eleven
main eject checks, native USB1 STOP, reserved LOAD/failed-load recovery, exFAT,
topology/SDK fallback and two-partition suites. Its helper argument regression
was fixed and verified by actual sdg mounting. The selected timeout revision also passed corresponding normal-path and
final resource checks; see the final summary. Historical
057d45, ab12dcc6 and other candidate records validate only their stated hashes.

A separate rejected-final-claim fixture passed worker shutdown, absent hub
aliases and responsive native USB UI. An older 43d4e45e process completed twelve
reconnect/eject cycles with unchanged endpoint FDs/threads and lower endpoint
RSS. Its following selector run correctly refused ownership after SDK filter
budget disablement; do not count that failed run as acceptance. Production
budget enforcement is unchanged; actual hardware timing is a later gate.

The first hardware short-STOP trial refused to open the selector. Temporary
logging subsequently confirmed STOP capture while panel-filter counters stayed
zero. Vendor GPL driver research shows that panel reads are change-triggered,
so an idle read deadline was invalid. The revised path still requires a future
authorized hardware trial. The separately authorized diagnostic ELF above
remains the last deployed real-AZ artifact; subsequent local fixes were not
installed there. Temporary SDK logging flags were removed after the earlier authorized capture.
The old diagnostic may still use its legacy private log; this is not a claim that
all diagnostic logging is disabled.
See `acceptance/usb-hub-support-0.2.0-az-selector-failure.json` and
`acceptance/usb-hub-support-0.2.0-az-input-diagnostic.json`.

Read-only preflight confirmed core 1.2.3, kernel 4.4.194 and the exact supported
firmware tag. The user identifies display version 2.01 as 2.00 with the AZ-MODS
patch; this does not broaden compatibility. Original 0.1.7 files and approval
backups were preserved during the authorized candidate installations.

Public release 0.1.8 remains unchanged. Hardware acceptance remains the user's
0.1.7 report with two hub sticks and native USB1. Source version 0.2.0 is an
unpublished development candidate, not hardware accepted or cleared for release.
The sections below retain historical research/release chronology.

## Verified stable build

Hub USB Libraries0.1.4: firmware2.00, EP147 tag `ep147-9e9454e357389ece`, kernel4.4.194,
Community Mods core1.2.3. Binary SHA256:
`6f03e2b5c9a36853f538019d7e4c1dc4123566c7be7cfab47da50e739bae7f98`.
The exact dev ZIP SHA256 is
`c7c7f141b5f406e388bb75bf421ed8ea22a8d872bb185216c00e79819bd39f7c`.

- Emulator acceptance is recorded in `src/azhub/research/community-014-acceptance.json`.
- SSH copy/verification is recorded in `src/azhub/research/az-014-usb-install.json`.
- After installation the user reported that it worked perfectly. This is hardware user acceptance,
  not a new instrumented hardware run; we did not independently inspect the process after their reload.
- Earlier JSON fields saying "awaiting reload" describe the time of the copy, not the latest user report.

## Architecture and fixes

Each source owns MusicLibraryServer, PersistentStorageServer, SQL factory cache and tagged-track
cache. Requests use private codes e0..e7. Two internal member identities3/4 have four media slots
apiece (3,7,8,2); total8 hub sources. These identities stay inside the AZ with loopback transport;
physical Pro DJ Link packets are filtered. The persisted Pro DJ Link mode can remain off.

The kernel module reversibly raises the hub-depth check for exactly the known firmware kernel.
Automatic discovery finds mounted hub libraries; a worker mounts devices beyond stock sda..sdf
rules and releases missing mounts after source handles and aliases clear.

0.1.3 fixed synthetic member labels on real subnets.0.1.4 fixed volume labels/Unicode and waveform
preferences: each source publishes its own validated DEVSETTING.DAT; alternating local-deck
status templates no longer overwrite color/position. Legitimate preference changes still repaint.

## Evidence and limits

Five sources were tested simultaneously in the emulator, with284-track playlists and2547 total
DB tracks. Only17 audio files exist in the images: use confirmed-present Algo mÃ¡s for playback.
0.1.4 tests include separate blue/center vs RGB/bottom preferences,200 alternating status templates
in20s,30 native-state samples during stress, restoration/live repaint and fifth-source load.
Audio output was not captured. A played03:22->01:39 then paused; fifth source loaded deck3.

Eight simultaneous sources, prolonged hardware sessions, removal of a playing source,
same-path replacement without observed absence, and different-content fixture isolation remain
unverified. Slot8 has a blank native icon/caption. Capacity/track-count announcement fields include
stubs. Automatic network identity selection needs an UP IPv4 subnet of /29 or shorter.

Read-only USB sysfs on the real AZ also confirmed the two connected Type-A buses use EHCI480Mb/s;
the presence of another xHCI5000Mb/s controller does not prove the Type-A ports are SuperSpeed.

## Public release 0.1.5

The user authorized public distribution after review. Personal Git attribution and historical
local paths/network references are retained by agreement. Current device/workstation configuration
is in ignored `local/`; screenshots and external firmware/analysis inputs remain excluded.

Release: https://github.com/NachoCasta/nacho-az-mods/releases/tag/hub-libraries-0.1.5
Catalog: `azmods-repo.json`; app repository identifier: `NachoCasta/nacho-az-mods`.
Anonymous catalog, package size/SHA256, icon and public source access were verified after
publication; the official SDK `repo check --download` also passed.
Source tag commit: `36a32fbf0cbd3a1c96a4197bee3d48a92cc50bc4`.
No Mod Index submission or new AZ deployment/test is part of publication.

The release contains the accepted Linaro executable, SHA256
`6f03e2b5c9a36853f538019d7e4c1dc4123566c7be7cfab47da50e739bae7f98`, and the unchanged accepted kernel asset.
The only source portability change is the SDK include path; runtime implementation is unchanged.
The SDK header is text-identical to v1.2.3. CI host behavior tests and SDK-container build/package
validation passed before publication; the container build uses a different compiler and is not
the shipped executable. The release package was validated separately with pinned SDK v1.2.3.

Package: 308564 bytes; SHA256 `c93e6b35b5da0a0d0dffe45e3d9834287010ff41836b590095dea8393a2ba131`.
The official packer regenerated the standard archive from current source assets, including
correct license-file line endings. This packaging change does not alter the executable or kernel.
0.1.5 has not been installed or additionally tested on the real AZ.

The validator warns about /home/root/settings: the mod READS the optional legacy azhub.conf
override there; it does not write that configuration file. This existing reference remains
unchanged to preserve the tested implementation. SDK-container CI uses another compiler; its
output hash/compatibility must not be confused with the bit-identical Linaro acceptance above.

## Previous release: USB Hub Support 0.1.6

The user explicitly authorized renaming the mod and ID after the first publication.
Current name: **USB Hub Support**; ID: `usb-hub-support`; release tag: `usb-hub-support-0.1.6`.
The 0.1.5 records above remain historical, not the current catalog entry.
Legacy `hub-libraries` must be uninstalled before enabling the new ID; no remote device
migration or AZ test was performed. Runtime changes are limited to ID strings used for
core hook/thread ownership, assets and logging. Current source remains in `src/azhub/linkfake`.
The rebuilt executable is no longer byte-identical to 0.1.4 because the ID changed.
Focused tests, direct ELF descriptor/legacy-string checks and SDK package validation passed.
No new emulator playback session or hardware acceptance is claimed for the renamed build.
See [release notes](releases/usb-hub-support-0.1.6.md) for hashes and limitations.

The renamed release is published at https://github.com/NachoCasta/nacho-az-mods/releases/tag/usb-hub-support-0.1.6.
Both CI jobs passed on source commit `6c6e82106de18843b4e242ba49706c61695fe329`. The public catalog lists only
`usb-hub-support`; anonymous catalog, package size/SHA256, icon and source checks passed,
as did the official SDK `repo check --download`. Legacy release assets were not replaced.

## Previous release: USB Hub Support 0.1.7

User-authorized metadata-only patch: the manifest now matches the simplified catalog
summary and description, without internal implementation details or the emulator test count.
Only version, summary and description changed in the manifest. All other package members,
including the executable and kernel asset, were verified byte-identical to 0.1.6.
SDK v1.2.3 package validation passed. At packaging time, no additional runtime test had been performed; subsequent user hardware acceptance is recorded below.
See [release notes](releases/usb-hub-support-0.1.7.md) for hashes and compatibility.

Release published: https://github.com/NachoCasta/nacho-az-mods/releases/tag/usb-hub-support-0.1.7.
Source commit: `20ce50addf25a2f92924ff6d4720e613973d5a22`; both CI jobs passed. Anonymous catalog version,
package size/SHA256, icon and source access passed after publication. No AZ changes were made.

## Subsequent user hardware acceptance: 0.1.7

The user confirmed installing 0.1.7 from our repository through the AZ-MODS app and
successfully browsing playlists, loading tracks and playing music on their XDJ-AZ.
Two USB sticks were connected through the hub and a third through native USB1,
for three simultaneous sticks. This confirms two hub sources alongside a native source;
it does not establish three hub sources or eight simultaneous hardware sources.
This is user-reported manual acceptance, not an agent-instrumented test. No new AZ
connection, deployment or mutation was performed by the agent. Firmware was previously
identified as 2.00 / `ep147-9e9454e357389ece`; it was not rechecked for this report.
Theme and test duration were not reported. No video or screenshot was requested.
Structured record: [hardware acceptance](acceptance/usb-hub-support-0.1.7-hardware.json).

## Original core workspace cleanup

The original Kyle-Hosman core checkout no longer contains `src/azhub`. A local cleanup
commit removes the 30 tracked research files after migration. Its resulting Git tree
matches `origin/main` exactly; no cleanup commit was pushed to Kyle-Hosman's repository.
All 573 files in the original hub workspace were verified by SHA256 after preservation:
331 screenshot duplicates remain in this checkout's ignored research shots directory;
the other original files and tooling are in ignored `local/core-workspace-cleanup/src/azhub`.
The ignored `local/core-workspace-cleanup/inventory.json` records every original file,
size, hash and retained location. This includes excluded raw research and generated tools;
those files remain local and must not be uploaded. The preserved SDK tool is under
`local/core-workspace-cleanup/src/azhub/publishing/sdk-reference/tools/azmod-pack`.
Active mod development and public documentation continue in this repository.

## Current release: USB Hub Support 0.1.8

Metadata-only patch adds a prominent external PRO DJ LINK incompatibility warning to the
manifest and catalog. While enabled, the mod filters physical Link packets; turning the
user's Link setting on does not disable it. Disable the mod and restart the AZ before
using external players over Link. Hub sources are not shared with external players.
The user reports hub use also worked with the Link setting on; external-player coexistence
was not tested or implemented. Automatic disabling on the user setting is not implemented.
All non-manifest package members are byte-identical to 0.1.7; only version and description
changed. SDK package validation passed. No new AZ operation or runtime test was performed.
See [0.1.8 release notes](releases/usb-hub-support-0.1.8.md).

0.1.8 is published. Both CI jobs passed on source commit `b8f4aba3807cb000da72e70d38de278be55e9c5b`.
The official SDK anonymous catalog/download check passed for 0.1.8. The index PR #4
was updated to disclose the external Link incompatibility and metadata-only release.

## Follow-up inspection: 2026-10-17

Official index PR #4 remains open. GitHub returned no comments, reviews or associated
status checks; this does not establish passing index CI. No PR mutation was performed.
Automatic handling still requires independent user-setting detection and lifecycle
research; see [PRO DJ LINK research](PRO-DJ-LINK.md). No runtime code, package, catalog
or hardware state changed during this follow-up.

The user subsequently requested parallel research into rear USB-C hosting, safe
ejection and PRO DJ LINK compatibility. Findings and proposed validation are in
[development options](DEVELOPMENT-OPTIONS.md). Rear-port host capability remains
unestablished; safe ejection and external-player coexistence are not implemented.
This research did not access the real AZ or change runtime/package artifacts.

## Emulator USB STOP interface prototype

The user authorized an emulator prototype for short-press hub selection and
long-press whole-hub requests. This interaction phase is now implemented and
validated locally; see [prototype notes](USB-STOP-UI-PROTOTYPE.md) and
[acceptance evidence](acceptance/usb-stop-ui-prototype-emulator.json).
Touch/knob selection, cancellation, single-action holds, intact native USB1 STOP
behavior and continued native UI response passed. Hub requests do not unmount
anything. The feature is excluded from normal builds and additionally gated to
the emulator. The regular build remains byte-identical to the published executable.
This historical UI phase does not establish safe ejection or hardware acceptance. No real AZ was
accessed during this implementation or validation.

## Emulator safe-eject implementation

The explicit `safe-eject-emulator` target now performs real background ejection:
connection-generation reservation, native load and library admission control,
independent server completion callbacks, alias closure, filesystem flush and
ordinary unmount. Short USB STOP selects one stick; a hold selects the whole hub.
Loaded/paused sources refuse before withdrawal. Failed busy unmounts retain
inhibition and allow retry. Completed states survive EP147 restarts and physical
reconnection restores availability.

See [implementation and limits](SAFE-EJECT-EMULATOR.md) and the exact tested binary
in [emulator acceptance](acceptance/safe-eject-emulator.json). Acceptance includes
one hub stick ejecting while another keeps playing, whole-hub behavior and actual
open-file failure/retry. This supersedes the earlier research/UI-only statements
for this explicitly gated emulator target. The normal executable remains identical
to the published version; no release, catalog change or real-AZ test is included.
Hardware mapping, FUSE support, failure-state metadata and additional race/fault
validation remain production gates. Two-stick emulator acceptance does not imply
eight-source or hardware acceptance.

## Safe-eject release preparation (in progress)

The user requested an eject flow ready for publication and a real-AZ trial.
SOURCE focus and stale failed-load metadata corrections have intermediate
emulator evidence. Stock exFAT-FUSE closure now has plain-volume and Rekordbox
library/bind-alias evidence, including busy failure/retry. Optional diagnostics
are helping validate late native LOAD admission and completion correlation.
The explicit `az-candidate` target enables the candidate for the already supported
AZ kernel as well as the emulator. Default builds now include the same candidate
flow; the public 0.1.8 catalog remains unchanged. A local 0.2.0 staging/package workflow passes SDK validation,
but its bytes must be regenerated after final code acceptance. No candidate is
yet declared ready for hardware or publication. See
[candidate preparation](SAFE-EJECT-RELEASE-CANDIDATE.md) for remaining tests,
package boundaries, and action-specific hardware permission gates.

# Community SDK port status

Current status: see [AZ-TEST.md](AZ-TEST.md) and the latest section of
[NOTES-MULTI-USB.md](../NOTES-MULTI-USB.md). Version 0.1.3 works on hardware;
0.1.4 fixes source names and waveform preferences and awaits hardware validation.
The sections below retain earlier development history.

## Historical initial port

`community.c` and `azmod.json` are development artifacts. Do not install on the
AZ yet. The SDK variant loads through the emulator core and all 14 hook claims succeed.
A and B have displayed full284-track playlists with metadata/waveforms through
SDK. B is intermittent: the latest success used two optional diagnostic hooks,
while the earlier14-hook run had an empty full list. Cold-start reliability,
SDK C/D and playback remain incomplete.
No hardware installation package is produced by build.sh.

Build in WSL with `bash build.sh`; override AZHUB_TOOLCHAIN_PREFIX if needed.
The public ELF symbol is az_mod_v1_entry; libc adapters have hidden visibility.
Hook installation uses the core claim_hook service, tied to the exact firmware
tag. There is no C constructor in the SDK build. Nine EP147 libc PLT hooks plus
five native hooks implement traffic adapters, slot capabilities, paired identity,
media lookup and dispenser registration. Startup factory/holder hooks are skipped.
An individual refused hook is counted; remaining native callbacks retain valid
original entry pointers, and no injector starts after a refusal.

Current development config is /home/root/settings/azhub.conf, using the same
player/ip/slot/path/label format as linkfake.conf. If that override is absent, the new startup discovery branch writes
/tmp/azhub-discovered.conf for mounted hub sticks with rekordbox DBs. The
standalone helper found all four emulator sticks and excluded direct USB1;
SDK discovery now runs in the emulator with no manual config and reports all
four sticks; discovered source A full playlist284 is verified. Removal handling
remains outstanding. The emulator's core azmod.so must remain
loaded for its existing emulation services; remove only linkfake.so from preload
before testing the SDK variant to avoid duplicate ownership and threads.

Next: investigate browsing/playback through the SDK hook path. The emulator
Allow UI and hook registration have passed, with linkfake.so removed from preload. Then implement discovery, local traffic isolation,
lifecycle and 4+ stick acceptance before generating an AZ test package.

LINKFAKE_TRACE_CLIENT enables two optional SDK diagnostic hooks (16 total).
Leave it off for production acceptance; tracing may change timing.

SDK enables LINKFAKE_LOCAL_ONLY: signature-scoped physical Link datagram filtering.
A five-second eth0 observation saw zero Link packets (9 other frames); loopback
positive control saw60. This is limited runtime evidence, not exhaustive coverage.

Fifth-stick runtime milestone: SDK auto-discovery found five physical emulator
hub images. The fifth uses player3 slot8, with its own open DB. All-tracks2547
and playlist284 with metadata/waveform previews are verified; the first playlist
entry was blank and worked after re-entry. This remains a reliability issue.
Fifth SDK load/playback progression is now verified with Algo mas and both
waveforms (03:22.647 to03:18.159). No audio capture or hotplug/removal acceptance.

Hardware startup dependency: the AZ 4.4.194 kernel rejects external hubs before
userspace discovery. The existing azhub.ko must be integrated as a validated
asset, and the mod must tolerate no sticks at initialization and discover them
after replug. Current startup-only discovery does not satisfy this requirement.

Delayed/append discovery now works in the SDK emulator test. It stays loaded
with no visible sticks, then registers a first batch; later scans append new
sources without changing existing assignments. Fixture tests verified0->5 and
2->5 with the same EP147 PID and separate DBs. Actual physical unplug/reinsert
and mount-path reuse are not handled yet. Alias replacement now checks source
identity to prevent stale binds from a previous process pointing at another stick.

Kernel asset integration now builds: SDK init skips known emulator6.6.138-dirty,
requires exact4.4.194 on AZ, reuses an existing azhub module or loads assets/azhub.ko
through finit_module. Other kernels/missing assets/kernel refusal fail before
hook claims. Six policy tests pass with mocked syscalls; no actual new kernel
load was performed, and the new build is not deployed yet. Packaging must include
the existing verified azhub.ko. Internal ProDJ mode gating remains unresolved for
standalone four-deck use; the emulator currently has ProDJLinkMode enabled.

Internal activation now tested with ProDjLinkMode OFF: the mod maintains the
native library/member cache separately, five hub sources appear, four-deck view
is available, and Algo mas from the fifth source loads on deck3 with waveforms.
The user setting remains0; short Ethernet capture still sees no Link packets.
This also exercised the kernel-asset loader's emulator skip branch. Hardware
kernel loading, removal/reinsert and final package acceptance remain unverified.

Controlled removal/reconnect milestone (PID460175): hiding sdc1 only from
/tmp/azhub-removal-mounts closed its native exportLibrary.db fd, withdrew its
SOURCE row, and released /media/player03/usb2. The other four DBs stayed open.
Restoring the fixture reopened sdc1 and its alias with the SAME EP147 PID;
screenshots removal-four-sources.png and removal-restored-five.png. This tests
same-path presence transitions only, not physical unplug, changed device names,
loaded-track removal, or post-reconnect playlist/playback. New SDK build allowed
through normal emulator UI. Hardware untouched. Fixture restored to all five.
Sparse-audio reminder remains: full DB, 17 audio files; Algo mas is verified
present on all five images. DB-only entries must not count as playback failures.


Actual emulator USB lifecycle (PID460175): discovery fixture path now symlinks
/proc/mounts, so scans use real kernel mounts. QMP device_del koi-usb2 removed
sdc1; its DB fd and player03/usb2 alias disappeared, other4 DBs stayed open.
device_add usb-storage drive=koi-hub2 bus=xhci.0 port=1.2 id=koi-usb2 restored
sdc1, its DB and alias in the same EP147 process. Reinserted library displays
artist/album and full Reggaeton284 with metadata/waveform previews:
physical-reinsert-library.png, physical-reinsert-playlist-selected/full.png.
Changed device paths and replacement without observed absence remain unresolved.
Development ZIP now generated by linkfake/package.py in linkfake/dist/, containing
SDK .so, manifest, assets/azhub.ko, test instructions and LICENSE. Not hardware
validated or a completion claim; see AZ-TEST.md for current limitations.

Package validation: archive passes the actual list_pkg function from deck/lib/az-community
under the emulator guest shell (root manifest/ELF, paths, duplicate/case/size checks).
ZIP integrity/ELF64 AArch64 and exact module vermagic checked by package.py.
SDK binary SHA2569945dd9836c730ab2779c424eb97c98b888020f8445da43c1fc346c52a8d326f
matches the running staged mod. Eight topology and six mocked kernel-policy cases
pass. Package-list success is not an end-to-end installer or hardware test.
Post-reinsert all-tracks initially blank0000; Playlist->Track retry renders2547
(physical-reinsert-alltracks[-later/-retry].png), so empty-view bug affects both
playlist and all-tracks requests, independent of actual missing sparse audio.

Post-physical-reinsert playback verified on PID460175: select known-present
Algo mas from sdc1 all-tracks, load deck1, both waveforms render. PLAY advances
remaining03:22->03:11 with waveform scrolling (physical-reinsert-algo-selection,
-loaded, -playing.png). Paused again after test. Audio output not captured.
All5 hub sticks currently attached; discovery mount-table path symlinks real
/proc/mounts. Development package exists, but empty-browser bug and source
replacement/path-reuse robustness remain active work; no real AZ deployment.


Source reuse build (SDK SHA25660617e8041ebd51c3eb42db9132a9fb3e2de36082ea730736e1057ff73cf3733):
config parser now parses into a local candidate, scans even at8 allocated slots,
and reuses absent pairs after unmount_pending clears. Keeps native server objects,
player/slot, transport and private request code. Gate FuseFilsine requests for
absent sources. Socket setup failures now close their fd. Active assignments stay
unchanged. Built and allowed through normal emulator UI, PID469735.
research/scripts/test-source-reuse.sh passed five successive changed-path fixtures
for sdc1: /media/usb/koi-reuse-1..5. Each cycle withdrew source, waited8sec,
reintroduced under new path, verified reuse/mount logs, samePID and5 DB handles.
Ten total lifetime paths (initial5+5new) with only initial10 registered servers
during the cycles. Fixture uses real device metadata and directory symlinks, not
five distinct-content replacement sticks. It restores real /proc/mounts on exit.
That immediate restoration allocated a sixth pair (player4 slot8) while prior
removal was pending, then settled at5 visible sources/DBs and12 retained servers.
Thus startup/post-removal path reuse passes; concurrent arrivals may allocate
until the8-pair limit. Same-path replacement with no observed absence remains
unhandled. No hardware changes. Development ZIP regenerated with this binary.

Post-reuse restored sdc1/player4 slot8 browser verified: Reggaeton full284
with metadata/waveforms (reuse-postrestore-playlist-full.png). Side preview
was initially empty (reuse-postrestore-playlist-selected.png); full view worked
on the next deliberate tap. Empty-view reliability remains unresolved.
Current development archive SHA256c182a27a12d6253a571cb051a0f4c145a80f8b00458bb867772dc1eb9af31c66.

SDK diagnostics expanded behind LINKFAKE_TRACE_CANCEL plus TRACE_CLIENT: adds
1142460 list cancel, e61ff0 cancel scheduling, 1fd2d08 request-agent destructor
(19 hooks total; default still14). First19hook PID476476 B alltracks2547/full
Reggaeton284 passed, so not a reproduction of empty UI. Optional diagnostics
remain source-only; dist ZIP is still prior tested reuse build.
Ghidra confirms 1fd2d08 cancels pending request if adaptor+18, reqID+30 nonzero
and status+34==0. Virtual deleting destructor1fd3920 duplicates this code and
is not separately hooked. 1fd34e8 request reply accepts only matching64-bit
reqID+28 and32-bit txn+30; wait state+38 must3(current) or4(lower); otherwise
warns wrong reply. 1fd01b0 marshals TrackList results to UI ->1fc7958 ->
1fcf800 listener dispatch. No suppression/retention patch made.
Code review: send wrapper used patched stack buffer after its lexical scope;
now calls real_send within that scope (as write already did). Inspected old
AArch64 disassembly: it did keep stack allocated through BLR, so this source
lifetime fix is not evidence for the observed empty-view cause.
SDK forced packet hex logging, coupled to routing fd tracking. Removed forced
TCP_LOG; fake TCP connections now tracked regardless of logging, dump always
records setup state but emits hex only when TCP_LOG explicitly set. Preparing
quiet19hook runtime to reproduce without packet-dump timing/volume overhead.

Quiet19hook PID480573 reproduced blank Playlist root (quiet-trace-B-selected.png).
Request owner destruction at31157.346 cancels Playlist ticket vt2b7d510 txn17,
then header/footer arrive31157.355/.412 and all observers reject(result1).
More decisive single category tap: from populated Track2547, tap Playlist once;
quiet-trace-playlist-single-tap.png stays blank. Ticket371 header accepted at
31224.817, owner destruction31224.818 cancels it31224.819, footer rejected31224.837.
Caller chain is200f2d4(base update-event destructor) <-1ffe3bc <-201fd7c
(ListUpdateHandler replacing current operation) <-2021420 <-2057cf8 <-2058740.
Ghidra:2057c20 DeviceSettingUpdateEventFacade calls20213f0, constructs event
via20193a0/201e478: type11, vt3165bf0. vt+48 ->201e4e0 is WaveformRenewEvent,
confirmed source-path string. It updates existing waveform images/style and
does not fetch the missing list. This repaint replaces/destroys pending list
operation, cancelling its request; results are discarded afterward.
Base current event owns request-agent shared_ptr at+28/+30 and BrowserData+58.
MusicLibraryRequestAgent vt3161140: request media+28, ticket+30, status+34.
Proposed narrowly scoped guard at201f5d8: only incoming vt3165bf0, a known-present
hub media, that request-agent type with pending ticket/status0, and empty main/
lower row vectors at BrowserData+248/+250. Leave caller owning incoming event
so it deletes normally; preserve current operation. Optional BROWSER_GUARD=1
is staged for emulator testing, not yet accepted/default or added to dist.

Browser cancellation investigation: current emulator PID489740,20 hooks
(14normal+browser guard+5diagnostics), TCP packet hex logging OFF. Env still has
TRACE_CLIENT=1,TRACE_CANCEL=1,BROWSER_GUARD=1; real mount discovery symlink intact.
Default community build has guard OFF until accepted; no new dist ZIP generated.
New browser-guard.h and12-case test-browser-guard.c pass optimized + ASan/UBSan
checks, covering pending empty hub case vs local/other peer/wrong agent/completed/
nonempty/unrelated media. Initial source identity comparison missed client high
flags; now compares low48 bits (client ticket e.g.01012200000202 vs registry
identity012200000202). This was caught by the positive test before final runtime.
Runtime guard field probe confirms actual agent/browser offsets. Corrected guard
skips empty-pane renewal at31777.776 and31780.210 player3slot7. Playlist root now
populated (guard-fixed-B-playlist-root.png). However selecting Reggaeton leaves
side preview blank (guard-fixed-B-selected.png): main tree nonempty makes guard
fall through. Earlier31777.978 pending request with empty=0 was cancelled too.
Thus guard is PARTIAL, not a complete reliability fix or accepted package change.
Need preserve pending main/lower requests even when other pane has rows, ideally
coalesce/defer WaveformRenewEvent rather than discard legitimate waveform color
updates. Do not blanket suppress cancellations or apply stale events to a new
source. A deferred event would require native UI completion/ownership handling:
caller owns incoming event via pointer argument; normal handler consumes it and
sets pointer0. Existing guard leaves it unchanged so caller deletes normally.
All source/build edits uncommitted; current dist remains c182a27a... prior reuse
build, without quiet-log change/diagnostics/guard. Real hardware untouched.

Deferral implementation replaces empty-row suppression (not yet runtime accepted):
3 optional guard hooks201f5d8 apply-event,1fc9ae0 generic UI reply-dispatch,200f0d0
base update-event destructor. Normal+guard17 hooks; with diagnostic5 ->22.
A pending hub MusicLibraryRequestAgent retains the newest matching WaveformRenew
event in a16-entry native-UI-thread TLS queue; sets caller pointer0 to take
ownership. Repeated renewals delete prior retained event through native vt+8.
After genericUI reply returns, completed request flushes renewal through native
apply-event. Navigation drops queued event before replacing current operation.
Base event destructor drops records before releasing current agent/context.
Queue entries carry handler/current/agent identity, preventing stale replay after
context change. Queue-full fallback lets caller delete repaint and keeps request.
Native201f5d8 really uses onlyx0/x1 (x2 overwritten immediately; decompiler
spuriousparam3/4 come from compiler CAS variables), so replay passes zeros for
extra registers without keeping caller stack pointers.
Optimized15-case ownership/lifecycle suite and ASan+UBSan pass: retain/coalesce/
complete/navigation/destruction/local/other-media/completed/wrong-agent/absent/
original-declines-consumption/stale-current/stale-agent/full-queue. AArch64 build
clean. New SDK staged; awaiting normal Allow UI. Dist still prior c182a27a.

Deferral runtime positive PID495601,22 hooks: B playlist root, selected
Reggaeton preview, full284, Track2547 all populated (defer-B-playlist-root,
-selected, -full, -alltracks.png). At32219.947/.151 refresh deferred and
32220.154 applied afterreply; same at32222.178 ->32222.251. This proves actual
retain/replay branch, not just no cancellation occurring. No crash during sweep.
Next normal build has guard enabled by SDK itself, version0.1.1; packetlogging
and TRACE_CLIENT/TRACE_CANCEL off. Staged, awaiting normal Allow; expected17hooks.
Do not claim full five-source acceptance or update ZIP until trace-off checks.

Normal0.1.1 acceptance PID499084: SDK17 hooks,TRACE_CLIENT/TRACE_CANCEL and
TCP_LOG off, ProDjLinkMode persisted0. All5 separate DB FDs open. A/B/C/D/E
Reggaeton preview and full284 passed on first entry (normal-defer-[A-E]-selected/
-full.png). E alltracks2547 (normal-defer-E-alltracks.png). Actual deferral
branches replay for A,D,E in trace-off build. Fifth Algo mas selected, loaded
on deck3 then1 (normal-defer-E-algo-selection, -deck3-loaded, -two-decks-loaded);
4DECK view shows bothwaveforms, PLAY1 advances03:22->03:05 (-playing.png), then
paused. Short passive capture eth0:14frames10UDP0Link; loopback620frames68Link
positive control. Limited capture, not exhaustive proof of all network routes.
0.1.1 development package built; ZIP integrity/ELF64 AArch64/modulevermagic
checks and actual deck list_pkg validator pass. ZIP SHA256
b0e5f2f685c71231be8fd55287cbc5699b10ab9051e2cae9e06478d085767c73.
SDK binary SHA256b6f7e8123f9fa605619261288384beb78d113071ead9ea45fe7968ef60f8192f.
Real AZ untouched. Existing0.1.0 ZIP retained as historical rollback artifact.

FINAL CHECK FAILED; GOAL REMAINS ACTIVE. 0.1.1 five-source browsing and playback
sweep passed, but must not recommend package for AZ yet. QMP unplug koi-usb2
(sdc1) removed own DBfd and03/usb2 alias; stale04/usb3 bind from a previous
6-pair process remained. Linux reinsert assignedsdg1; vendor98-local.rules
only matches sd[a-f]/sd[a-f][1-8], so no device-mount@sdg1 job was ever started.
Current device-mount.sh itself accepts sd[a-h] and handles nestedbus2-1.2
without localUSB1/2 announcement. Need standalone mod fallback for hub devices
that vendor udev skips; do not patch real AZ rootfs or rely on manual mounting.
Reserved /media/usb/sdg1 for emulator renotifier, unmounted stale04/usb3 bind,
manually ran existing /bin/bash /home/root/scripts/device-mount.sh add sdg1.
That restored primary mount. It was a diagnostic intervention, not acceptance.

Unexpected SIGSEGV at10:12:58 UTC (before manual mount): HubLib4 tid499725 on
PID499084. Native12553b0 ldrx25,[x21] withx21=0, fault0. Backtrace12fb950,
1303c44,136016c,13698a4,core+6c600,e6a558. Core crash report saved in
research/hublib4-011-crash.log (append-only includes three OLD MsgDispenser
crashes; relevant last section beginsline557 timestamp1792231978). Native
12552e0 calls virtualprovider+28 target1829990, returns null handle, derefs
without checking. 1829990 searches its media/DB vector+0xa0..a8 for requested
LocalMediaID ushort (entry+98), returnsNULL when absent. This differs from
e5e7d8 media registry lookup that always returns an object. Do not conflate them.
12552e0 param1+8 media key, +10 provider; crashx20(this)=ffff7c931020, x23
requesteditem=0x820, x25 virtualtarget1829990. Further caller12fae60 huge list
entry builder ->1303bb0 menu request ->135ff08 ListRequestHandler. Need inspect
actual requestedLocalMediaID and provider ownership/registration; possible
shared backend cache mismatch is a hypothesis, not proven. Do not paper over
by blindly suppressing the native null dereference. UI renewal guard is not
on crash stack (crash is HubLib4, not nativeUI thread). Two decks1/3 had loaded
fifth Algo mas; PLAY1 thenpaused. Timing followed unplugging a different stick,
so removal vs delayed playback/list refresh causality is unresolved.

Systemd auto-restarted EP as507534 (coreloaded0.1.1,17hooks, not quarantined).
When sdg1 was manually mounted, native5 DBfds opened; new process had first4
sources then appendedsdg1. Effective current assignments differ from sorted
config: sdb1=3slot3,sdd1=3slot7,sde1=4slot3,sdf1=4slot7,sdg1=3slot8. Do not
assume old Brow590 is sdg1 now. QMP drivekoi-hub2 remains port1.2 idkoi-usb2.
All5 physically attached. Currentprocess no loaded tracks after auto-restart.
Discovery path stillsymlink/proc/mounts; TRACE_CLIENT/CANCEL/TCP_LOG off, browser
deferral on bydefaultSDK. Archive README now marks investigation/notAZ-ready.

DB cache cause confirmed with diagnostic18hook PID514395: HubLib4 andHubLib0
both query TaggedTrackCacheService providerffff67ee8200, ID0102, vectorcount1
while all5 independently opened SQL DBs exist. RTTIvt2c6d0f8 (typeinfo2c6d048
name at2c6d018) -> tagged_trk_cache::TaggedTrackCacheService.
Withdrew onlysdg1 via discoveryfixture (no physical unplug, no tracks loaded).
At33792.562 ownsource3slot7 absent andunmountqueued;33794.691 aliasreleased.
At33802.840 HubLib0(notremoved) requests item2114, key0102 wanted0102, SAME
provider count0 found0, thennativecrash. This causally reproduces sharedcache
unmount wiping other sources' entry; dual-deck playback is NOT required.
EP autorestarted517548 andcore quarantinedmod (state5: Suspended itcrasheddeck).
Do not keep restarting/bypasscore; normalResumeUI afterfix is available.
Restored discoveryfixture symlink/proc/mounts. No rootfs/hardwarechanges.
LiveREAD of514395 /proc/PID/mem beforewithdraw confirmedvt2c6d0f8 and1entry
(provider+a0/a8), usingBusyBoxdd bs1 skipdecimal64 count16. Aftercrash oldaddress
innewPIDreadzeros is NOT valid sameprocess evidence; count0probe beforefault IS.
Servicevirtualmethods vt: index2=1829d28(register/open),3=1828968(remove),
4=1829610(update),5=1829990(lookup, x8sret),6=18282d0(query),7=18285b0(x8sret),
8=1827ff0,9=1827d20. Ctor1828db8; serviceallocated0xc0, RWLock+8, vector
shared_ptr entries+a0/a8/b0, backendkey+b8. Nativefactory7d6598 usesit.
Need per-stick TaggedTrackCacheService scoped alongside own music/info servers;
currently only per-stick SQLfactorycache is private, taggedcache remainsglobal.
Do not assume remote-slot ID mismatch: diagnosticquerykey/wantedboth0102,
entry0102found beforewithdraw. 186af10 is globalLocalID normalizer, not a simple
getter:186ab90 selects optionalnormalizer bylow5bit sourcekind, otherwise
returnsinputushort. SDKsource db-cache-probe.h observes normalnon-sret12552e0
caller safely, never directlyhooksx8lookup1829990. Dist remainsmarkednotAZready.

Private tagged-cache draft: structfake nowowns process-lifetime tag_cache.
Native TaggedTrackCacheService ctor1828db8 initializesvt2c6d0f8, RWLock+8 via
2489f50, emptyvector+a0/a8/b0, limit100+b8, THEN globallyregistersservice.
Private instances reproduce ONLYprefix, avoidingglobalregistration. Eight
native virtualmethods are ABI-transparentreceiver-adapted perHubLib/HubInfo
thread or pairedhub_info_libraryTLS; stockcallerskeeporiginalservice. Redirects
open/close/update/lookup/otherquerymethodsalike, not onlynullquery suppression.
Entryassembly preservesx1..x8 andstackargs, changesonlyx0receiver, tailbranches
tooriginaltrampoline. Hiddenx8aggregate returns are thereforeuntouched.
NativeAArch64staticABItest test-tag-cache-abi.c compiledvendorGCC andran in
emulator: all8adapters passscalar/x8aggregate/10arguments(includingstack).
Featureopt-in PRIVATE_TAG_CACHE=1; DBprobereports effectiveprivateprovider.
Cleanbuild; stagedforruntime. Currentcorequarantinefromreproducedcrash requires
normalResumeUI thennewhashAllow; nevereditapproval/quarantinefilesbyhand.

Private-tag runtime NOT accepted yet. NormalTryAgainUI cleared per-mod
quarantine; rapidmanualtestrestarts thenhitglobalstartupquarantine, recovered
throughCommunityResumeUI. UIReloadnow requiresconfirmation(fullx730y590) and
writes /run/az-community.req apply, but oldemulatorwatcherdoesnotconsumeit.
AfterUIReloadrequested, manualsystemctlrestart honorscoreintent exemption.
LatestPID529941 initfailed -4: coreclaim_hook rejects first3 taggedmethods
1829d28/1828968/1829610 due16-byte source-kindcheck+conditionalbranch+RET.
Remaining5 methods accepted. Newsourcepatch uses body entry+0x10 BEFORE
prologue (1829d38,1828978,1829620), so receiver/args unaffected exceptunused
scratchw2 orw3. x8 adapters unchanged. Alsofixed SDKinit to notparse/publish
sources orlaunchinjector when anyprobehookclaimfails (previouscodecould
return-4 only AFTERstartingworkerwithpartialhooks). Mustrestartfailedprocess
withcorrectedbuildbeforetesting; do notbrowsepartial-initstate.
Fixedcontextownershipcheck in h_mlookup andtagreceiver: onlynonnull paired
TLS maymatchsrv_music. Previously TLS0==unconstructedserver0 couldincorrectly
selectsource0 forstockthreads duringstartup. ExactHubLib/Info namesstillmatch.

Private tagged-cache removal/restore PASS on PID533159 (26hooks: normal17 +
8tagcache adapters +1DB diagnostic). NormalResume/Allow/Reload UI used; no
approval/quarantine state files edited. Core accepts correctedbodytargets
1829d38,1828978,1829620. Newcacheinstancescreatedon ORIGINAL registermethod0
caller1376f44, actualHubLib0..4 threads, not lazily fromdiagnosticquery.
Source0 ffffa2aa9480;1 ffffa0e80480;2 ffff9e262480;3 ffff9d88b480;4 ffff9bb88480;
originalglobal ffff874e8200 preserved. Eachprivatecache has1ID0102 entry.
Source1(sdg1,player3slot7) hiddenfromdiscoveryfixture at34909.784; itsDBfd
closedandaliasreleased. Live/proc/PID/mem cachevectorcounts afterwithdraw:
{0:1,1:0,2:1,3:1,4:1}. SAMEPID. SourcesAandE bothbrowseall2547trackswhileB
absent (private-tag-A/E-while-B-absent.png); DBprobesshow eachusingitsOWNprovider
withfound1 ID0102. Stayedalive >1minute; thisexactsequence previouslycrashed
HubLib0 withsharedcount0. Restoring real/proc/mounts symlinkreannouncedB,
reopenedall5DBfds, livecachecountsall1 withsamePID. EvidenceJSON:
research/private-tag-removal-state.json. No nullquerysuppression applied.

Privatecachefeature remainsopt-in LINKFAKE_PRIVATE_TAG_CACHE=1 inemulatorenv;
SDKdefault notyetenabled. TRACE_DB_CACHE=1 on; TRACE_CLIENT/CANCEL/TCP_LOG off.
CurrentstagedSDKdiffersfromdist0.1.1 investigationZIP; DO NOTrepackandrecommend
untilnormaldefault25hook build +mountfallback +physicalhotplug/fivebrowse/play
acceptance. All5devicespresent, sdg1 remainsmanuallymounted, fixtureprocmounts
restored. No realAZchanges.

Draft assets/mount-hub.sh addedbutNOT integrated/packaged/runtime-tested (sh-n
passes). Validatesdevicebasename/block/sysfsnestedUSB, supportsstockvfat/exfat/
hfsplusoptions, explicitwritemodehardwareRO, neverannounceslocalUSBslots and
neverrecursivelydeletesmountdirectories. Need ownmountworker, offinjectorthread,
forvendorrule-skippeddevicesg+ (includingmulti-letter names); wait/avoidracing
stocka-fmounts. Mustalsotrackfallbackmounts andunmountmissingdevices AFTER
nativeDB/aliasrelease, sincevendorremoveudevrulesalsoskipg+. Do notjustmount
withoutcleanup or relyonmanuallycallingvendorhelper. Emulatorrenotifier must
skipnewhubmounts dynamically (currentlyreservedlist includessdb..sdg1), so
automountworker testcannotannounceg/h aslocalUSB2. AssetsremainonthemodUSB;
currentemulatorstagingwasmanualandmayneedreal MODS/community-mods/hub-libraries
folderonsda1 forcoreassets_dir resolution, withmatchingmanualowner.


## Mount fallback integration (2026-10-17, acceptance in progress)

Added mount-worker.h (core HubMount thread), packaged assets/mount-hub.sh, and
SDK default private tagged caches. The worker only mounts nested devices named
beyond stock sda..sdf; normal filesystem policy remains vfat/exfat/hfsplus. It
executes the helper with an explicit environment without LD_PRELOAD. Missing
backing mounts are released only after EP147 FDs and all aliases clear, using
normal umount. Assets are looked up through core assets_dir on the mod USB.

First runtime identified concurrent getmntent static-buffer corruption. All
new mount scans and discovery now use getmntent_r. Corrected build PID544361
loads25 hooks, initializes five pairs. Device policy18, topology8, browser15,
mocked kernel policy6 tests pass. Physical unplug/reinsert remains to verify.


Mount worker acceptance follow-up: PID544361 remained unchanged through actual
QMP removal of koi-usb2 (sdg1). Its DB closed, native alias released at35625.731,
and backing mount released at35628.669. Reinsert became sdc1, stock-mounted,
and regained its DB and populated2547-track browser with waveforms. All five DB
FDs remained open afterward. A separate temporary FAT image on hub port1.6
became sdg1: HubMount automatically mounted it (journal10:57:04), then released
it at35729.630 after physical unplug. This temporary image contained no library;
it tests filesystem fallback rather than a sixth source. QMP node and image
removed afterward. Screenshot mount-worker-B-tracks-after-hotplug.png. Corrected
concurrent scanner maintained five sources; final package/readiness sweep pending.


## 0.1.2 emulator acceptance / manual AZ handoff

Exact tested binary a772ff4349f43747b7f60536b826fa09884f5ec22a4c99a92bb990dde5fd6404 packaged as0.1.2.
Five first-entry full playlists284 pass on PID544361. Fifth E displays2547 tracks,
Algo mas loads decks3/1, PLAY1 advances03:20->03:07 then paused. Link mode0
verified from persisted SETTINGS.DAT. Ethernet10s Link0, loopback10s Link221.
Final archive SHA256 724ffe5f1c55d94e3d182dbe4f0c3b8c0441e94ab02bfa38c7b18cc598ad470f.
Deck list_pkg passes after final README refresh. Full acceptance record is
../research/community-012-acceptance.json. Real AZ untouched; hardware test pending.


### 0.1.3 subnet fix acceptance / awaiting permission for AZ

{
  "version": "0.1.3",
  "emulator_pid": 35295,
  "native_interface": "192.168.1.115/24",
  "ssh_secondary_address": "10.0.2.15/24",
  "automatic_internal_labels": [
    "192.168.1.254",
    "192.168.1.253"
  ],
  "baseline_012_same_native_network": "repeated unacknowledged announcements",
  "binary_sha256": "89eddbd7fef4fa89b43ddcef6f6bbe487a893641b840cf589eacb100e6a9e54b",
  "archive_sha256": "b25a880169abb5bc1d870ea93019c8098333e8caa6057be0dfb6a5cade27f41c",
  "hook_count": 25,
  "starts_without_hub": true,
  "late_attach_same_pid_sources": 5,
  "separate_open_databases": 5,
  "playlist_rows_each": 284,
  "fifth_all_tracks": 2547,
  "fifth_present_audio": "Algo mas",
  "loaded_decks": [
    3,
    1
  ],
  "deck1_remaining_seconds": [
    200,
    170
  ],
  "paused_after_test": true,
  "persisted_link_mode": 0,
  "ethernet_link_signatures_10sec": 0,
  "loopback_link_signatures_10sec": 224,
  "network_policy_cases": 14,
  "package_deck_list_pkg": "pass",
  "az_tests_since_user_required_approval": 0,
  "az_013_installed": false,
  "limitations": [
    "configured IPv4 subnet /29 or shorter required by current auto selector",
    "reload after subnet changes",
    "unconfigured networking and multiple active subnets unverified",
    "clone sparse audio fixtures",
    "audio output not captured",
    "first C/D waveform previews delayed, populated on revisit",
    "playing-source removal unverified"
  ]
}

Current code uses network-identity.h only, no subnet-gate native hook.
Original0.1.2 fails on native192.168.1.115, new build passes0->5late sources,
all284-tracklists, fifth2547tracks and decks3/1 Algo masPLAY03:20->02:50.
Final package passes deck list_pkg and byte/hash matching against running ELF.
Do not run even read-only AZ tests until user approves. No AZ updates made.

# Public hub sharing candidate readiness

This checklist preserves the full active goal. It is not a release approval or
an actual-CDJ compatibility claim. CURRENT-STATE.md and individual acceptance
records supply the current evidence; historical fixture results do not prove
the final integrated package.

## Latest physical scope

[Candidate0.4.4](acceptance/usb-hub-support-sharing-044-physical-ethernet.json) passes
one-source physical Ethernet software-client discovery, complete library browsing,
exact PDB/MP3 and native USB file controls. Target MACVLAN loading is observed.
Current-helper loaded-eject and97 portable tests per sanitizer mode pass. Full
normal/trace mode repeats pass after one unexplained earlier failure; physical
GUI mode observation remains pending. See the current [physical report](HUB-LINK-PHYSICAL-ETHERNET.md).
This supersedes historical no-physical-trial statements below, without converting
earlier emulator capacity/topology evidence into hardware acceptance.

## Historical software-candidate gate matrix

| Candidate requirement | Current evidence | Scope |
|---|---|---|
| Eight independent libraries, menus and exact files | [Capacity](acceptance/usb-hub-support-sharing-capacity-eight-final.json) | Eight concurrent software clients,19 exact file fetches and all-source renewals |
| Native loaded withdrawal reference | [Native reference](acceptance/usb-hub-support-sharing-native-paired-reference.json) | Matching source1/USB slot3/track1884/file, native store admission, STOP and stale retained handle |
| Individual loaded public eject/reconnect | [Individual](acceptance/usb-hub-support-sharing-individual-full-tuple.json) | Matching source41/COLLECTION slot4/track1884/file; continuing wire status, neighbor preserved, reconnect and stale old handles |
| Whole-hub loaded withdrawal | [Combined](acceptance/usb-hub-support-sharing-whole-hub-full-tuple.json) | Eight complete clients, native USB2 playback, matching public tuple and native admission,56 foreign-root checks, all withdrawals and native reads afterward |
| Native USB preservation/concurrency | Capacity and combined results above | Fresh native USB1/2 local loads and advancing engine observations; no captured audio |
| Local hub continuity/deck topology | [Topology](acceptance/usb-hub-support-sharing-topology-final.json) | Two On/four Off/returned two On, actual hub backing, primary engine observations during clients and secondary rejection |
| Resource lifetime and regressions | Topology and [source checks](acceptance/usb-hub-support-sharing-final-source-checks.json) | Retention,96 portable tests per mode, SDK UI10/8/8 and hooks; IPC faults refuse eject |
| Target package and source | [Package](acceptance/usb-hub-support-sharing-package-draft.json), [build procedure](HUB-LINK-CANDIDATE-BUILD.md) | Validated local0.4.0 package; byte-identical target rebuild; final source snapshot verifies232 unchanged production inputs |
| Firmware/core boundary | Manifest, runtime and kernel guards | Exact ep147-9e9454e357389ece, kernel4.4.194, core>=1.2.3; no broader firmware claim |
| Network recovery/collision | [Integrated network recovery](acceptance/usb-hub-support-sharing-final-integrated-network-recovery.json) and current component tests | Current integrated runtime/helper recovery passes; all-slot collision retirement/reselection/exhaustion covered by current portable suite; historical live collision results remain scoped |
| Target MACVLAN transport | Reproducible GPL module/static export audit | Actual4.4.194 ABI/symbol resolution/load remains a physical gate |
| Model-specific interoperability | [Physical plan](HUB-LINK-PHYSICAL-TRIALS.md) | Actual CDJs, audio, sustained timing and model-specific source capacity remain unconfirmed |
| Environment restoration | Local cleanup receipt and normal framework status | Extra disks removed, original seven mounts, normal0.3.2 active with suspended=0; no physical AZ touched |
| Publication | Separate user decision | No release/tag/catalog update |

## Authoritative candidate status (latest integrated evidence)

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

## Explicit failure boundary

An authenticated graceful retirement acknowledgement permits source closure and
unmount. A lost IPC channel, missing helper process or SDK stop request does not
establish retirement of outstanding generations. The controller latches FAULT
when such generations remain, logs the reason and refuses their drain gate;
automatic retry/unmount recovery is not implemented for that case. A documented
fault therefore remains a failed eject, not a successful safe-removal result.
Do not unplug on a preparing message or infer that a mod reload proves prior
network-resource cleanup. Physical trials must record any such failure and
review recovery separately. Normal loaded-source eject must pass without
external manual unload; fault handling does not replace that acceptance gate.

If evidence establishes a protocol or firmware limit requiring a reduced
outcome, keep the evidence and obtain agreement before changing scope. Do not
mark the goal complete merely because the existing tests pass or because a
smaller subset is easier to validate.

## Historical capacity preparation (superseded by final capacity result)

The managed runner accepts `--capacity-source-count 5` or `8` for an integrated
runtime control. It requires every selected source to become ready, uses one
independent DeviceFinder with concurrent complete library clients, and fetches
the reference PDB and MP3 from each selected endpoint with exact size/hash checks.
When short-grant refresh is enabled, every selected source must show an accepted
renewal. These controls are prepared, not live capacity acceptance. The existing
two-source results must not be represented as eight-source validation.

Three additional writable virtual-disk overlays are prepared locally for hub
ports6..8 but are not attached. Attach them only after the active topology run
and its cleanup finish, verify actual device/port/backing ownership, and retain
safe removal and restoration of the original emulator configuration. Never use
these images or emulator results as evidence of physical AZ/CDJ acceptance.

`sharing-capacity-media.py inspect|attach|detach` is the explicit localhost-only
media control. Syntax and read-only inspection pass. Attachment refuses an
active managed test, requires empty decks and unused ports, and journals only
the three owned devices/backends. Detachment verifies backend ownership and
refuses any mounted disk/partition identity, including bind aliases; use normal
safe eject first. No attachment/removal acceptance is established by inspection.

## Historical native-file preparation (superseded by final integrated results)

The isolated independent Crate Digger client now has explicit native USB1/USB2
fixture options, fixed to the emulator native address192.0.2.15 and exports `/C/`
and `/B/`. Existing namespace/interface checks remain mandatory. The managed
runner's `--native-file-control` invokes the supported post-start notification
and adds exact complete native USB1 PDB/MP3 downloads plus the exact candidate
mod-file download from native USB2, alongside integrated hub clients. Java
compilation and Python syntax pass; live execution is pending. This does not
yet prove fresh native UI browsing/loading or either physical USB port.

# Research map

Read CURRENT-STATE.md first. Then:

| Resource | Purpose |
|---|---|
| SAFE-EJECT-AZ-TRIAL.md | Deferred action-specific hardware trial plan; no current authorization |
| SAFE-EJECT-PUBLICATION-AUDIT.md | Active guideline checklist, sanitizer scope and remaining stability gates |
| SAFE-EJECT-LIFETIMES.md | Exact-firmware one-shot completion contract and bounded adapter ownership |
| SAFE-EJECT-RELEASE-CANDIDATE.md | Active publication/AZ-trial preparation, corrected regressions, candidate build and remaining gates |
| SAFE-EJECT-EMULATOR.md | Actual emulator-only safe-eject coordinator, reproduction, acceptance and production limits |
| PRO-DJ-LINK.md | Current external Link warning, automatic-handling research gates and latest index PR inspection |
| DEVELOPMENT-OPTIONS.md | Rear USB-C feasibility, safe-ejection design and separate Link research goals |
| USB-STOP-UI-PROTOTYPE.md | Emulator-only short/long USB STOP selector, build gates, acceptance and remaining limits |
| SAFE-EJECT-RESEARCH.md | Native STOP routing, completion listeners and proposed safe-ejection coordinator |
| USB-C-RESEARCH.md | Rear USB-C role, controller and VBUS read-only findings |
| PRO-DJ-LINK-COEXISTENCE.md | Native Link mode propagation and coexistence architecture options |
| acceptance/usb-hub-support-0.1.7-hardware.json | User-confirmed repository install, browsing/loading/playback with two hub sticks and one native USB1 stick |
| ../src/azhub/NOTES-MULTI-USB.md | Chronological working notes/handoffs; earlier status can be superseded |
| ../src/azhub/research/GHIDRA-FINDINGS.md | Exact addresses, layouts, routing, ABI and tested corrections |
| ../src/azhub/research/community-014-acceptance.json | Final emulator binary/hash and acceptance |
| ../src/azhub/research/az-014-usb-install.json | SSH installation evidence for hardware USB |
| ../src/azhub/research/waveform-014-*-settings.log | Per-source native color/position samples |
| ../src/azhub/research/ghscripts/Probe.java | Headless decompile/xref/string/vtable research helper |
| ../src/azhub/research/scripts/vmem.py | Read the local firmware ELF by virtual address (not a live hardware writer) |
| ../src/azhub/research/scripts/device-settings-sample.c | Read-only native settings diagnostic |
| ../src/azhub/research/scripts/stress-status-template.c | MUTATING emulator-only transport stress helper |
| ../src/azhub/research/scripts/test-source-reuse.sh | MUTATING lifecycle fixture; read before execution |
| ../src/azhub/research/scripts/build-emulator-xhci.py | Emulator-only controller-module rebuild |
| ../src/azhub/linkfake/test-*.c | Focused behavior/lifecycle tests |

All scripts are preserved as research; several early shell scripts reference expired temp
paths. Check LOCAL-RESOURCES.md for current resource locations instead of rerunning blindly.
Raw decompiled function device-setting-serialize.c was deliberately not copied; its facts and
packing offsets are documented in GHIDRA-FINDINGS.md. Firmware and full analysis DBs stay local.

## Local captures (not in Git)

`src/azhub/research/shots/` contains the retained local screenshots. Useful families:

- `waveform-014-source-names-ready.png`: real volume labels in native source UI.
- `waveform-014-independent-A-B.png`: independent blue and RGB source styles.
- `waveform-014-restored-A-B.png`: legitimate live restoration to RGB.
- `waveform-014-fifth-loaded.png`: fifth-source RGB waveform and overview.
- `acceptance-*-full.png`: five populated playlist views before later fixes.
- `subnet-013-*.png`: native192.168.1.x regression and recovery.
- `mount-worker-*`, `physical-reinsert-*`: removal/reinsert acceptance.

Captured filenames/hashes from the migration are indexed in migration-inventory.json; image
bytes remain local and ignored. The user explicitly does not require screenshot backups.

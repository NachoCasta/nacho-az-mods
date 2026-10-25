# Working rules

- All repository-facing prose must be in English: documentation, code comments, commit messages,
  branch/PR/issue descriptions, changelogs, release notes, workflow labels, and agent instruction files.
  Preserve literal identifiers, native strings, filenames, and observed test data when required for accuracy.
- Read docs/RELEASING.md before preparing or publishing a version. CLAUDE.md links to the same rules.

- Read docs/CURRENT-STATE.md and docs/RESEARCH-INDEX.md first. Historical notes contain superseded experiments.
- Follow docs/AZ-TESTING-POLICY.md. The user authorizes emulator-first USB candidate
  installation, normal mods-framework activation/testing and standard restarts for
  USB Hub Support / PRO DJ LINK work through the normal AZ-MODS workflow.
  The user removed the separate remove-USB/full-restart recovery prerequisite.
  Do not repeatedly ask within this authorized scope or claim unperformed recovery tests.
- Do not manually edit internal AZ firmware, OS, services, core, settings/approval
  files or process memory. Device file writes are limited to the mods USB; use the
  framework's supported UI/API/IPC for managed activation and settings. Its persistent
  staging is not RAM and must not be represented as USB-only persistence.
- Explicit testing exception: the user permits SSH pre-approval of the exact USB
  Hub Support candidate by updating only its entry in the framework approval list.
  Follow the hash, USB-backup, locking and atomic-update requirements in CLAUDE.md.
  All other manual internal controller edits remain forbidden.
- Local code/build/emulator work may proceed. Never deploy implicitly from a build or release script.
- Latest release0.4.8 adds recoverable input health and touch eject fallback after SDK quarantine, with61 emulator open/cancel cycles over30 minutes and owner-reported general physical success. Physical quarantine/prolonged-session acceptance remains unclaimed. Earlier0.4.7 automatically recovers hubs present at power-on. One physical cold boot has instrumented recovery and owner-reported success; a second repetition remains pending. Earlier0.4.6 adds HFS+ hub library and analysis support. Original reduced-image emulator controls cover playlists/loading, both waveform views, mixed FAT/exFAT, native FAT playback, Link transitions and loaded eject/reconnect. The owner accepted ordinary USB operation on the physical AZ; the original HFS+ USB was unavailable and its physical trial is explicitly deferred. Core1.2.5 was used for that physical regression; minimum core remains1.2.3. Earlier0.4.5 source presentation/waveforms and0.2.1 loaded eject acceptance remain documented. No actual-CDJ, audible phase-sync, eight-source or long-session hardware acceptance is claimed. Before0.3.2, loaded Link mode changes briefly disconnected hub sources;0.3.2 fixed that transition regression. Large libraries can exceed the existing public sharing catalog capacity. See CURRENT-STATE.md and the release acceptance records for exact scopes.
- Keep per-source servers, SQL caches and tagged caches independent. Never borrow a local USB library slot.
- The user authorized renaming the mod to USB Hub Support, ID usb-hub-support, in 0.1.6.
  Keep that ID stable. Uninstall legacy hub-libraries before enabling the new ID; both claim the same hooks.
- Snapshots/screenshots stay local and gitignored. Do not create screenshot backups or upload them.
  Exception: the user explicitly requested two release screenshots for0.4.5. The selected
  emulator Source/eject previews may be packaged and uploaded as release assets; raw research
  captures remain local and ignored. Do not commit the image files to Git.
- Never commit firmware, keys, raw firmware decompilation, full disassemblies, USB images or music.
  Document facts (addresses/layouts/control flow) and keep reconstruction scripts instead.
- Do not commit generated binaries except the deliberate self-built GPL kernel release asset.
- Do not publish this repository, releases or a public catalog without the user's publishing decision.
- No agents/delegation unless explicitly requested by the user.

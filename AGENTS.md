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
- Latest release0.3.1 adds native-body HUB labels and the revised eject selector; the user approved its physical UI appearance. Earlier0.3.0 passed user-reported local hub operation in both Link modes and software-peer Wi-Fi master/SYNC/reconnect/fresh-load controls. Actual-CDJ and phase-sync acceptance remain open. Loaded mode changes briefly disconnect hub sources and leave warning triangles. Earlier0.2.1 acceptance covered individual loaded-source eject and full-track playback for one track, with SSH mount/cache-proof checks. Earlier0.2.0 covered two hub sticks plus native USB1 after unloading. Distinguish user-reported audio from instrumented acceptance; no eight-source or long-session acceptance is claimed.
- Keep per-source servers, SQL caches and tagged caches independent. Never borrow a local USB library slot.
- The user authorized renaming the mod to USB Hub Support, ID usb-hub-support, in 0.1.6.
  Keep that ID stable. Uninstall legacy hub-libraries before enabling the new ID; both claim the same hooks.
- Snapshots/screenshots stay local and gitignored. Do not create screenshot backups or upload them.
- Never commit firmware, keys, raw firmware decompilation, full disassemblies, USB images or music.
  Document facts (addresses/layouts/control flow) and keep reconstruction scripts instead.
- Do not commit generated binaries except the deliberate self-built GPL kernel release asset.
- Do not publish this repository, releases or a public catalog without the user's publishing decision.
- No agents/delegation unless explicitly requested by the user.

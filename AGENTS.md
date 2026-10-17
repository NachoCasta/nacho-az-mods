# Working rules

- All repository-facing prose must be in English: documentation, code comments, commit messages,
  branch/PR/issue descriptions, changelogs, release notes, workflow labels, and agent instruction files.
  Preserve literal identifiers, native strings, filenames, and observed test data when required for accuracy.
- Read docs/RELEASING.md before preparing or publishing a version. CLAUDE.md links to the same rules.

- Read docs/CURRENT-STATE.md and docs/RESEARCH-INDEX.md first. Historical notes contain superseded experiments.
- The user's real AZ must not be tested, reloaded, restarted, patched or changed without asking first.
  Existing authorization is action-specific; permission to copy a package does not authorize a hardware test.
- Local code/build/emulator work may proceed. Never deploy implicitly from a build or release script.
- Current stable user-confirmed hardware build is0.1.4. Distinguish user reports from instrumented acceptance.
- Keep per-source servers, SQL caches and tagged caches independent. Never borrow a local USB library slot.
- The user authorized renaming the mod to USB Hub Support, ID usb-hub-support, in 0.1.6.
  Keep that ID stable. Uninstall legacy hub-libraries before enabling the new ID; both claim the same hooks.
- Snapshots/screenshots stay local and gitignored. Do not create screenshot backups or upload them.
- Never commit firmware, keys, raw firmware decompilation, full disassemblies, USB images or music.
  Document facts (addresses/layouts/control flow) and keep reconstruction scripts instead.
- Do not commit generated binaries except the deliberate self-built GPL kernel release asset.
- Do not publish this repository, releases or a public catalog without the user's publishing decision.
- No agents/delegation unless explicitly requested by the user.

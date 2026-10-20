# Claude project instructions

Follow [AGENTS.md](AGENTS.md). These instructions supplement that file; they do not replace it.

## Language

Write all repository content, code comments, commits, PRs, issues, and release notes in English.
Keep literal identifiers and observed test data unchanged. Conversation with the user can follow
the user's language; repository artifacts must remain in English.

## Read first

1. [Current state](docs/CURRENT-STATE.md): stable build, prepared version, evidence, and known limits.
2. [Research index](docs/RESEARCH-INDEX.md): existing findings and tools; avoid repeating completed research.
3. [Release guide](docs/RELEASING.md) before version, package, catalog, tag, or release work.

## Releasing

The canonical repository-specific procedure is [docs/RELEASING.md](docs/RELEASING.md).
It links to the official SDK publishing guide and its pinned v1.2.3 source. Follow that procedure
for version bumps, exact-artifact testing, packaging, checksums, release assets, and catalog updates.
The current GitHub workflow only checks builds and packages; pushing a tag does not publish a release.

Keep `usb-hub-support` stable. The user explicitly authorized the 0.1.6 rename from
`hub-libraries`; legacy copies must be uninstalled before enabling the renamed mod.
The repository is public and 0.1.5 publication and the 0.1.6 rename were authorized.
Future releases and Mod Index submissions still need the user's publishing decision.
Follow [AZ testing policy](docs/AZ-TESTING-POLICY.md): the user authorizes the bounded
emulator-first USB/framework workflow and standard restarts without a separate
remove-USB recovery prerequisite. Do not manually edit internal controller files
or bypass approvals. Never deploy as
an implicit side effect of a build or release script.

## Hardware candidate testing

For authorized candidate tests, obtain approval for the exact candidate executable
through the supported AZ-MODS installation/Allow flow before requesting the reload
that activates it. Check approval readiness before reload; do not deliberately
reload an unapproved candidate and leave the user at "New mod found: allow?".
If the installed framework requires a discovery reload before it can offer Allow,
explain that limitation and distinguish discovery from the activation reload.
Explicit user-authorized exception: for the exact USB Hub Support candidate under
test, pre-approval over SSH may update only its entry in
`/home/root/settings/azmod-community/approved` before activation reload. Verify the
reviewed executable SHA256 against both USB and staged bytes, preserve a rollback
copy on USB, hold the framework approval lock, preserve every other mod's entry,
and replace the list atomically. This exception does not permit other internal
settings/file edits, process-memory writes, or approval of unrelated mods. Use the
normal AZ-MODS apply/reload mechanism afterward, with empty decks.

## Research assets

Keep notes and useful scripts in Git. Screenshots stay local and gitignored, without backups.
Do not upload firmware, keys, raw decompiler output, full disassemblies, USB images, or music.

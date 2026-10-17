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
Ask before any real-AZ test, reload, restart, installation, or mutation. Local build/emulator work
can proceed. Never deploy as an implicit side effect of a build or release script.

## Research assets

Keep notes and useful scripts in Git. Screenshots stay local and gitignored, without backups.
Do not upload firmware, keys, raw decompiler output, full disassemblies, USB images, or music.

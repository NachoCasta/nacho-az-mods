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

Keep mod IDs stable. The existing `hub-libraries` ID is already installed on the user's deck;
renaming it needs a migration plan. New mods use their own IDs and independent version tags.

The repository is private. Do not make it public, publish a release/catalog, or submit a Mod Index
PR without the user's explicit publishing decision. Prepare the files and validation evidence first.
Ask before any real-AZ test, reload, restart, installation, or mutation. Local build/emulator work
can proceed. Never deploy as an implicit side effect of a build or release script.

## Research assets

Keep notes and useful scripts in Git. Screenshots stay local and gitignored, without backups.
Do not upload firmware, keys, raw decompiler output, full disassemblies, USB images, or music.

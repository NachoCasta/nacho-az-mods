# Publishing Nacho AZ Mods

Primary guide: https://kyle-hosman.github.io/az-mods-sdk/06-publishing/#azmod-pack-reference
Pinned SDK: v1.2.3 (62a156abf3a4789e9e3cdd610fe106227a3a8967).

## Public distribution

The user authorized public source, research notes, release assets and the catalog after
review. Personal commit attribution and historical local paths/network addresses are retained
with the user's agreement. Current machine configuration is ignored in `local/`.
Screenshots, firmware, music, full analysis databases and keys are excluded.

Add `NachoCasta/nacho-az-mods` in the desktop app or deck repository list. Both fetch
`https://raw.githubusercontent.com/NachoCasta/nacho-az-mods/HEAD/azmods-repo.json`
without authentication. Do not put GitHub tokens in a catalog, URL or USB.
The optional Mod Index needs a separate PR; no listing has been submitted.

## Standard release

Each mod owns azmod.json, source/Makefile, icon.png256x256, LICENSE and optional assets/.
The catalog `azmods-repo.json` is at repository root and can hold many mods. Use independent tags
such as `hub-libraries-0.1.5`; a future mod uses its own ID/version/tag. Do not use a catch-all
v* release trigger for multiple mods. Package filename is .azmod, not the development .zip.
A package is still a ZIP internally, but the official tool validates and constructs it.

Original0.1.4 manifest was accepted for local development but standard publication validator
refused missing summary, license and icon. The0.1.5 preparation adds these fields without
claiming additional hardware validation. Keep the old dev archive immutable.

Use the step-by-step [release guide](RELEASING.md) for source/version preparation, build and
exact-artifact testing, package creation, release tags/assets, catalog updates, and anonymous
end-to-end verification. [CLAUDE.md](../CLAUDE.md) points future Claude sessions to that procedure.
The existing CI workflow validates builds/packages; it does not publish releases when a tag is pushed.

Add catalog entries only after real downloadable release URLs exist. A validator passing a URL's
syntax does not prove it is anonymously accessible. The `source` field must refer to the actual
public source location before distribution. Package size limit16MB. Future releases need authorization; preparing a package alone does not authorize publication.
The first release was explicitly authorized; Mod Index submission is a separate decision.

## Orca

A Git repository is the versioned project; an Orca project registers its path and manages
worktrees/terminals around it. This is a separate repository/project from Kyle-Hosman's core.
The old project keeps its original origin and uncommitted work. This project holds our collection.
One collection repository supports many mods; there is no need for one Orca project per mod.
Separate worktrees can be used later for different mod tasks. Do not nest this repo in the core
or replace the core repository's origin. Register the independent checkout with `orca repo add`.

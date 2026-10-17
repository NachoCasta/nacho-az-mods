# Publishing Nacho AZ Mods

Primary guide: https://kyle-hosman.github.io/az-mods-sdk/06-publishing/#azmod-pack-reference
Pinned SDK: v1.2.3 (62a156abf3a4789e9e3cdd610fe106227a3a8967).

## Private development vs distribution

The repository is private because that is what the user requested. The deck/app fetches
`https://raw.githubusercontent.com/OWNER/REPO/HEAD/azmods-repo.json` without GitHub authentication.
Consequently entering this private repository in AZ-MODS cannot provide installs/updates.
Do not put GitHub tokens in a catalog, URL or USB as a workaround.

Before publishing, choose either to make a reviewed source/catalog repository public, or export
only distributable sources and release metadata to another public repository. Research notes,
local screenshots and credentials need not accompany the distribution. The SDK's guidelines
require public source. The Mod Index is optional and is a separate later PR; no PR was submitted.

## Standard release

Each mod owns azmod.json, source/Makefile, icon.png256x256, LICENSE and optional assets/.
The catalog `azmods-repo.json` is at repository root and can hold many mods. Use independent tags
such as `hub-libraries-0.1.5`; a future mod uses its own ID/version/tag. Do not use a catch-all
v* release trigger for multiple mods. Package filename is .azmod, not the development .zip.
A package is still a ZIP internally, but the official tool validates and constructs it.

Original0.1.4 manifest was accepted for local development but standard publication validator
refused missing summary, license and icon. The0.1.5 preparation adds these fields without
claiming additional hardware validation. Keep the old dev archive immutable.

Once source hosting is actually public and review is complete:

```sh
python3 .azmod-sdk/tools/azmod-pack build src/azhub/linkfake
python3 .azmod-sdk/tools/azmod-pack package src/azhub/linkfake --github OWNER/REPO --tag hub-libraries-0.1.5
# Create release/tag only after approving the concrete files; attach .azmod and icon.
python3 .azmod-sdk/tools/azmod-pack repo add azmods-repo.json src/azhub/linkfake/dist/hub-libraries-0.1.5.entry.json
python3 .azmod-sdk/tools/azmod-pack repo check azmods-repo.json
# After publication, verify anonymous download like a deck:
python3 .azmod-sdk/tools/azmod-pack repo check OWNER/REPO --download
```

The catalog stays empty until real downloadable release URLs exist. A validator passing a URL's
syntax does not prove it is anonymously accessible. The `source` field must refer to the actual
public source location before distribution. Package size limit16MB. No public release or index
submission is authorized by preparing this private repository.

## Orca

A Git repository is the versioned project; an Orca project registers its path and manages
worktrees/terminals around it. This is a separate repository/project from Kyle-Hosman's core.
The old project keeps its original origin and uncommitted work. This project holds our collection.
One collection repository supports many mods; there is no need for one Orca project per mod.
Separate worktrees can be used later for different mod tasks. Do not nest this repo in the core
or replace the core repository's origin. Register the independent checkout with `orca repo add`.

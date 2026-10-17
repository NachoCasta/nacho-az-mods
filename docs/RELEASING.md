# Releasing a mod

This is the repository-specific release procedure. Read it before changing versions, making
packages, creating tags/releases, or updating the catalog. All release artifacts and Git messages
must be in English.

## Authoritative references

- [Official SDK publishing guide](https://kyle-hosman.github.io/az-mods-sdk/06-publishing/).
- [azmod-pack command reference](https://kyle-hosman.github.io/az-mods-sdk/06-publishing/#azmod-pack-reference).
- [Publishing guide at the pinned SDK version](https://github.com/Kyle-Hosman/az-mods-sdk/blob/v1.2.3/docs/06-publishing.md).
- [Mod guidelines at the pinned SDK version](https://github.com/Kyle-Hosman/az-mods-sdk/blob/v1.2.3/docs/GUIDELINES.md).

The current SDK is v1.2.3, commit `62a156abf3a4789e9e3cdd610fe106227a3a8967`.
Use the matching tool/template versions. Upgrading the SDK or compiler is an explicit change;
review the guide and validate the resulting binary again.

## 1. Establish the release target

Read [CURRENT-STATE.md](CURRENT-STATE.md), the mod manifest, and relevant acceptance records.
USB Hub Support currently lives in `src/azhub/linkfake/`. Its stable user-confirmed hardware
build is 0.1.4; 0.1.6 is the explicitly authorized name/ID migration.

This repository and its distribution catalog are public, authorized by the user after review.
The app/deck fetches the root `azmods-repo.json` and package URLs without GitHub authentication.
The first release is `hub-libraries-0.1.5`; subsequent publication still requires authorization.
See [PUBLISHING.md](PUBLISHING.md) for the distribution model.

Local builds, validation, and candidate packaging can proceed while private. Do not change repository
visibility, publish a release, or submit a Mod Index PR as an automatic consequence of this guide.
Public release commands below assume the user approved publication and that the working checkout,
its `origin`, the manifest's `source`, and `RELEASE_REPO` all refer to the intended distribution repo.
If development stays private, perform publication from the reviewed public export instead.

## 2. Prepare the version and source

- Keep `usb-hub-support` stable. The user authorized the 0.1.6 rename from `hub-libraries`.
  Uninstall the legacy copy before enabling the new ID; both claim the same hooks.
  An ID migration is a new installation, not an automatic update.
- Bump `azmod.json` to a strictly greater `MAJOR.MINOR.PATCH` version, without suffixes or leading zeros.
  Do not reuse a published version for different package bytes, including metadata or asset changes.
- Review the name, author, summary, description, source URL, GPL license, and minimum core version.
  State actual functionality and remaining limits; do not claim eight-source hardware validation.
- For hooks, both the manifest and descriptor must contain the exact tested firmware tag. Do not
  broaden compatibility just to pass installation checks. Review the kernel asset if firmware changes.
- Include the original 256x256 PNG icon, LICENSE, and all required assets. USB Hub Support needs
  `azhub.ko`, its license, and `mount-hub.sh`. The kernel asset has separate GPL-2.0 source/build notes.
- Update the current-state notes and write English release notes describing behavior, validation,
  compatibility, and material limitations. Version screenshots only if deliberately approved;
  research screenshots remain local and ignored by default.

Use independent tags: `<mod-id>-<version>`, for example `usb-hub-support-0.1.6`.
Future mods use their own tags. Avoid a global `v*` trigger in a repository containing several mods.

## 3. Build, test, and validate the candidate

The examples below use a POSIX shell (Linux or WSL) from the repository root, with Docker available.
If `.azmod-sdk` does not exist, clone it once:

```sh
git clone --branch v1.2.3 --depth 1 https://github.com/Kyle-Hosman/az-mods-sdk .azmod-sdk
git -C .azmod-sdk rev-parse HEAD
# Expected: 62a156abf3a4789e9e3cdd610fe106227a3a8967
```

Select the mod and derive names from its manifest, rather than retyping its version in several places:

```sh
MOD_DIR=src/azhub/linkfake
MOD_ID=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["id"])' "$MOD_DIR/azmod.json")
VERSION=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["version"])' "$MOD_DIR/azmod.json")
TAG="$MOD_ID-$VERSION"
RELEASE_REPO=NachoCasta/nacho-az-mods
# Change RELEASE_REPO if the user chooses a different public distribution repository.

python3 .azmod-sdk/tools/azmod-pack build "$MOD_DIR"
python3 .azmod-sdk/tools/azmod-pack validate "$MOD_DIR"
```

Run focused behavior tests relevant to the change. `.github/workflows/check.yml` runs host tests,
builds with the pinned SDK, and validates a local package and catalog. It DOES NOT publish anything.
Check the conclusions of both CI jobs; a queued or still-running workflow is not a passing check.

Use the emulator to validate the exact binary/assets being shipped. Hardware tests or deployment
require asking the user first. Do not substitute an earlier build's acceptance just because its
version/source looks similar. The 0.1.5 Linaro preparation was byte-identical to the accepted 0.1.4
ELF, but an SDK-container rebuild uses another compiler and may produce different bytes.
Record binary hash, package hash, test environment, and actual results.

For sparse emulator fixtures, the database has 2547 tracks but only 17 audio files. Use confirmed
present audio such as `Algo mÃ¡s` when checking track loading and playback. Keep waveform preference,
source isolation, browser completion, and removal/reinsert checks separate where relevant.

Read validator warnings. The existing `/home/root/settings` warning refers to the optional legacy
configuration override; the mod reads that file. Do not silently dismiss a newly introduced write
or dependency warning as though it were the same known reference.

## 4. Make a distributable package and entry

A local `.azmod` can be made before publication approval, without a public entry:

```sh
python3 .azmod-sdk/tools/azmod-pack package "$MOD_DIR"
```

After final public URLs have been chosen, prepare the release package and catalog entry locally:

```sh
python3 .azmod-sdk/tools/azmod-pack package "$MOD_DIR" --github "$RELEASE_REPO" --tag "$TAG"
PACKAGE="$MOD_DIR/dist/$MOD_ID-$VERSION.azmod"
ICON="$MOD_DIR/dist/$MOD_ID-$VERSION-icon.png"
ENTRY="$MOD_DIR/dist/$MOD_ID-$VERSION.entry.json"
python3 .azmod-sdk/tools/azmod-pack validate "$PACKAGE"
sha256sum "$PACKAGE"
```

The `.azmod` is the standard package, not a renamed development ZIP. The official packer writes
its size/hash and final HTTPS URLs into the entry. Check the entry's ID/version against the manifest.
The package download/unpacked limit is 16 MB. Never include firmware, extracted stock assets, keys,
raw decompiler output, research databases, USB images, or music.

For hosting outside GitHub Releases, use the official `--package-url` and optional `--icon-url`
options instead of `--github`. Do not invent URLs or add an entry until those assets can be served.

Review the concrete package, manifest, source commit, notes, and validation evidence with the user
before the first public publication. Preparing a private repository did not authorize publication.

## 5. Tag and publish the approved release

The current workflow does not listen for release tags; these are MANUAL steps. Run them only after
publication is authorized, from the intended distribution checkout with reviewed/staged source.
Keep the candidate package bytes unchanged after acceptance.

```sh
git status --short
# Stage only reviewed source, manifest, assets, notes, and documentation; never local references.
git commit -m "Prepare $MOD_ID $VERSION"
git tag -a "$TAG" -m "$MOD_ID $VERSION"
git push origin main "$TAG"
```

The tag must identify the source used to build the candidate. If source changed after the build,
rebuild, validate, and recheck the candidate before tagging. Do not move an existing release tag.
If the source commit already exists, skip the redundant commit step rather than creating an empty one.

Create a release-notes file before publication and set its actual path, for example:

```sh
RELEASE_NOTES="docs/releases/$MOD_ID-$VERSION.md"
# This file must already contain reviewed English notes.
gh release create "$TAG" "$PACKAGE" "$ICON"   --repo "$RELEASE_REPO" --verify-tag --title "$MOD_ID $VERSION" --notes-file "$RELEASE_NOTES"
```

Do not use `--clobber` to replace assets for a published version. If bytes change, issue a new
version/tag and regenerate its entry. A failed upload can be repaired only by uploading the exact
accepted bytes to the matching release; recheck the downloads before announcing it.

## 6. Update the root catalog

After the release assets exist, add the generated entry to the root catalog and validate it:

```sh
python3 .azmod-sdk/tools/azmod-pack repo add azmods-repo.json "$ENTRY"
python3 .azmod-sdk/tools/azmod-pack repo check azmods-repo.json
git add azmods-repo.json
git commit -m "Update catalog for $MOD_ID $VERSION"
git push origin main
```

Our root catalog already has its name, maintainer, and homepage. To create a new catalog in a
separate public export, `repo add` also needs `--name "Nacho AZ Mods" --maintainer NachoCasta`
and `--homepage https://github.com/OWNER/REPO` on the first call.

A local catalog check validates structure; it does not prove that a deck can download the package.
Make the anonymous end-to-end check after publication:

```sh
python3 .azmod-sdk/tools/azmod-pack repo check "$RELEASE_REPO" --download
```

That fetches the same catalog address as the deck, downloads each package, checks its size/hash,
and validates its contents. Do not announce a successful release until this passes. Verify the
public source link separately. Adding the repository through the AZ-MODS desktop app can follow;
installation/testing on the real AZ still requires the user's authorization.

The Mod Index is optional. A separate, explicitly authorized PR can add the public repository to
`Kyle-Hosman/az-mods-index`; direct repository installs do not require that listing.

## 7. Record what shipped

Update [CURRENT-STATE.md](CURRENT-STATE.md) and the mod's acceptance records with the release tag,
source commit, package and ELF hashes, SDK/compiler, test results, and public release URL. Distinguish
user-reported hardware success from instrumented testing. Commit these records in English.
Keep known limits and rollback instructions accurate; disabling the mod and reloading is a user action,
not something this documentation automatically performs.

## Future automation and additional mods

Before enabling automatic publication, start from the pinned SDK release workflow template and
review it for this repository. Keep workflows at root `.github/workflows/`, one per mod; set its
`MOD_DIR`, pin `SDK_REF`, and restrict tags to `<that-mod-id>-*`. The publisher needs `contents: write`.
Decide explicitly whether it updates the root catalog (`UPDATE_INDEX`) or leaves that to a reviewed
manual commit. Never run both catalog-update methods for the same release. Pull workflow-created
catalog commits before the next push. Review asset replacement behavior instead of copying a
`--clobber` publishing step unchanged. Do not enable that workflow merely by editing this guide.

For a new mod, use the pinned SDK's `new` command in its own directory, choose a unique author-prefixed
ID, and give it independent versions. Move its generated workflow to the repository root, merge
ignore/attributes rules there, and add its own entry to the shared catalog only after release.

## Catalog copy edits

The catalog's summary and description are storefront text and may be edited without
repackaging an immutable release. Keep package URLs, ID/version, size and SHA256 unchanged.
Use plain language about connecting USB sticks, browsing playlists and tracks; keep server,
loopback, cache and firmware-patching implementation details in technical documentation.
Version 0.1.7 synchronized the package manifest with the simplified catalog text. For future
releases, copy the current catalog summary/description into the manifest before packaging. Always validate the
catalog and verify its package download after an editorial change.

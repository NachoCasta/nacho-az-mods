# PRO DJ LINK handling research

## Current behavior

USB Hub Support 0.1.8 warns users to disable the mod and restart the AZ before
using external PRO DJ LINK players. Automatic disabling and external-player
coexistence are not implemented. Hub sources are not advertised to external players.

The user reported hub operation with the Link setting enabled, but did not test
external CDJs. That report does not establish external-player compatibility.

`community_enable_internal_link()` in `src/azhub/linkfake/community.c` writes the
exact-firmware internal cache at `0x3f86a08`. Discovery polling invokes it repeatedly.
This cache is separate from the persisted `ProDjLinkMode` setting and cannot serve
as a detector for the user's choice. Historical emulator evidence for that separation
is in the "Internal activation with user Link mode OFF and deck3 load verified"
section of `src/azhub/research/GHIDRA-FINDINGS.md`.

The current mod descriptor supplies initialization without a shutdown callback.
Initialization prepares the kernel module, claims hooks and starts source and mount
machinery. Skipping discovery after a setting change would not establish that these
effects were undone. Removing physical packet filtering alone would also leave the
internal player identities and routing unresolved.

## Investigation before implementation

1. Identify and validate the exact-firmware accessor for the user's selected mode,
   independently of the internal cache. Establish when it becomes valid during
   startup and when live setting changes become visible. The documented serialized
   settings record alone does not establish a safe runtime accessor.
2. Evaluate startup refusal separately from handling a live setting change. A
   startup guard must check before kernel preparation, hook claims and worker
   creation. Verify how core 1.2.3 reports initialization refusal and whether it
   retains or retries any mod state.
3. Determine the core's supported lifecycle for stopping workers, withdrawing source
   handles, releasing hooks and undoing kernel changes. Validate the state of an
   active track before considering automatic shutdown. Do not assume that changing
   a flag safely unloads the mod.
4. Choose user-visible behavior for a live change only after lifecycle validation.
   If restart remains necessary, communicate that requirement accurately; do not
   report external Link as available while the mod's filtering remains active.

## Required local validation

Use the emulator and exact supported firmware tag `ep147-9e9454e357389ece`.
Check startup with the user setting off and on, transitions in both directions,
and changes during browsing, loading and playback. Establish whether failed or
unavailable setting reads permit partial initialization. Verify native USB behavior,
independent per-source caches and four-deck operation where applicable.

These are proposed checks, not completed acceptance. Any real-AZ SSH access, test,
installation or restart requires action-specific user permission. External-CDJ
coexistence and network sharing of hub sources remain separate research features.

## Official index status

On 2026-10-17, GitHub reported [index PR #4](https://github.com/Kyle-Hosman/az-mods-index/pull/4)
as open, with no comments, reviews or status checks associated with the PR.
An empty check list is not evidence of passing CI. No PR changes or messages were
submitted during this inspection.

Index PR #4 was checked again on 2026-10-18 through GitHub CLI: OPEN, no reviews, comments or status checks reported; updated at 2026-10-18T10:17:01Z. No message or external modification was made.

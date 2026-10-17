# Local external resources

Machine-specific addresses and paths belong in `local/config.json`, which is ignored
by Git. Copy `../local-config.example.json` there for a new checkout and fill in
local values. Never store passwords, tokens or private key contents in the example.
The SSH identity field is a path to a key stored outside this repository.

`local/MACHINE-RESOURCES.md` retains the original workstation resource inventory
locally. It is ignored and is not required to clone or build the repository.

The configuration is a local reference, not an automatic deployment mechanism.
Existing research scripts do not automatically read it: inspect their arguments
and paths before running them. Historical notes and test fixtures may retain old
private-network addresses; they are evidence, not the current device configuration.
Real AZ operations still require the user's authorization.

Required external resources include the matching EP147 ELF, a local Ghidra project,
the vendor GPL kernel/toolchain tree, and emulator USB images. Firmware, full
analysis databases, music and screenshots stay outside Git. Local captures remain
in `src/azhub/research/shots/`, ignored without a backup requirement.

To reproduce Ghidra research, supply your own matching firmware, create/analyze
EP147 in a local project, and use `research/ghscripts/Probe.java` via
`analyzeHeadless` with `-readOnly`. Probe output goes to `~/probe.out` and is
overwritten on each run: extract facts into notes, and do not commit decompiled
source output. See GHIDRA-FINDINGS for commands and addresses.

Kernel build instructions are in `src/azhub/README.md` and
`research/scripts/kenv.sh`. The included release asset `azhub.ko` was built from
our GPL-2.0-only `azhub.c` against the matching configured vendor GPL kernel tree.

Original core-workspace artifacts and the preserved SDK checkout are now under ignored
`local/core-workspace-cleanup/`; its inventory provides SHA256 and retained locations.
The old core checkout no longer contains `src/azhub`. Active source is in this repository.

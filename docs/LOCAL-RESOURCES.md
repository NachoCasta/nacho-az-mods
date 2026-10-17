# Local external resources

These resources are NOT uploaded. Paths are the original workstation/WSL locations, not
portable dependencies and not evidence that a fresh clone already includes them.

| Resource | Current location | Approximate size |
|---|---|---|
| Ghidra project | WSL Ubuntu /home/icast/ghproj, EP147 project |1.5GB|
| EP147 ELF | WSL /home/icast/fw/EP147 |60MB|
| Full disassembly | WSL /home/icast/ep147.dis |491MB|
| Vendor GPL kernel/toolchain tree | WSL /home/icast/xdjaz-gpl |1.7GB|
| Ghidra12.1.4 | WSL /home/icast/tools/ghidra_12.1.4_PUBLIC | external tool |
| Emulator | Windows C:/Users/icast/xdj-az-emu | USB images/audio remain there |
| Local captures | This checkout src/azhub/research/shots | ignored; no backup requested |

Do not copy private SSH keys or authentication files from .azmods/.ssh into this project.
The user must authorize real AZ operations. Deployment evidence retains historical host/path
facts, not credentials. All proprietary firmware and analysis inputs stay outside Git.

To reproduce Ghidra research, supply your own matching firmware, create/analyze EP147 in a
local project, and use research/ghscripts/Probe.java via analyzeHeadless with -readOnly.
Probe output goes to ~/probe.out and is overwritten on each run: extract facts into notes;
do not commit the decompiler's source output. See GHIDRA-FINDINGS for commands/addresses.

Kernel build instructions are src/azhub/README.md and research/scripts/kenv.sh. The included
release asset azhub.ko was built from our GPL-2.0-only azhub.c against the known vendor GPL
kernel. Rebuilding requires the matching configured GPL tree, not a newer generic kernel.

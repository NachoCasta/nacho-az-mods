#!/bin/bash
# gh.sh <cmd> [<cmd> ...]  -- run Probe.java against the analyzed EP147 (read-only).
# Output: ~/probe.out, copied to the scratchpad as gh-out.txt.
S=/mnt/c/Users/icast/AppData/Local/Temp/claude/C--Users-icast-orca-workspaces-xdj-az-mods-koi/12a52a08-a9f1-4102-96c8-4804f5965775/scratchpad
G=$(ls -d ~/tools/ghidra_*_PUBLIC)
export MAXMEM=12G
rm -f ~/probe.out
"$G/support/analyzeHeadless" ~/ghproj EP147 -process EP147 -noanalysis -readOnly \
	-scriptPath "$S/ghscripts" -postScript Probe.java "$@" 2>&1 | grep -E '^ERROR|Exception' | head -20
cp ~/probe.out "$S/gh-out.txt"
wc -l < ~/probe.out

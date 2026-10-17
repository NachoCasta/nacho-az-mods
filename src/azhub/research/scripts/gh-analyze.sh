#!/bin/bash
# Import and auto-analyze EP147 in a headless Ghidra project (~/ghproj/EP147).
set -e
S=/mnt/c/Users/icast/AppData/Local/Temp/claude/C--Users-icast-orca-workspaces-xdj-az-mods-koi/12a52a08-a9f1-4102-96c8-4804f5965775/scratchpad
G=$(ls -d ~/tools/ghidra_*_PUBLIC)
mkdir -p ~/ghproj ~/fw
cp "$S/fw/EP147" ~/fw/EP147
export MAXMEM=12G
"$G/support/analyzeHeadless" ~/ghproj EP147 -import ~/fw/EP147 -overwrite \
	-max-cpu "$(nproc)" -analysisTimeoutPerFile 14400 2>&1 | grep -vE '^INFO  (Reading|Analyzing|.*Auto Analysis)' | tail -40

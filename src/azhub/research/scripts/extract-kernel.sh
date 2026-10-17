#!/bin/bash
# Extract only the kernel tree (and any prebuilt toolchain) from the Pioneer GPL tarball.
set -e
F=/mnt/c/Users/icast/AppData/Local/Temp/claude/C--Users-icast-orca-workspaces-xdj-az-mods-koi/12a52a08-a9f1-4102-96c8-4804f5965775/scratchpad/gpl-src/XDJ-AZ.tar.gz
mkdir -p ~/xdjaz-gpl && cd ~/xdjaz-gpl
echo "== toolchains in tarball"
grep -E '/bin/aarch64-[a-z-]*gcc$' ~/xdjaz-list.txt | head
tar -xzf "$F" XDJ-AZ/kernel
du -sh XDJ-AZ/kernel
echo "== version"; head -5 XDJ-AZ/kernel/Makefile; cat XDJ-AZ/kernel/.scmversion 2>/dev/null
echo "== hub.c nested"; grep -n -B3 -A6 "nested too deep" XDJ-AZ/kernel/drivers/usb/core/hub.c
echo "== huberr / HUBERR"; grep -rn "HUBERR" XDJ-AZ/kernel/drivers/usb | head

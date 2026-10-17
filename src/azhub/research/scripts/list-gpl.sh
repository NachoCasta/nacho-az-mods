#!/bin/bash
# List the Pioneer GPL tarball and find the kernel tree and toolchain.
F=/mnt/c/Users/icast/AppData/Local/Temp/claude/C--Users-icast-orca-workspaces-xdj-az-mods-koi/12a52a08-a9f1-4102-96c8-4804f5965775/scratchpad/gpl-src/XDJ-AZ.tar.gz
tar -tzf "$F" > ~/xdjaz-list.txt
wc -l ~/xdjaz-list.txt
awk -F/ '{print $1"/"$2"/"$3}' ~/xdjaz-list.txt | sort -u | head -60
echo "== interesting"
grep -E 'drivers/usb/core/hub\.c$|bin/aarch64-linux-gnu-gcc$|\.tar(\.gz|\.xz|\.bz2)?$|\.zip$' ~/xdjaz-list.txt | head -40

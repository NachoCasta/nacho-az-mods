#!/bin/bash
# First survey of EP147: symbols, and anything naming media slots / USB sources.
. "$(dirname "$0")/kenv.sh" true
E=/mnt/c/Users/icast/AppData/Local/Temp/claude/C--Users-icast-orca-workspaces-xdj-az-mods-koi/12a52a08-a9f1-4102-96c8-4804f5965775/scratchpad/fw/EP147
P=${CROSS_COMPILE}
${P}readelf -h "$E" | grep -E 'Type|Machine'
${P}readelf -S "$E" | grep -E '\.symtab|\.dynsym|\.debug'
echo "== symtab count"; ${P}nm "$E" 2>/dev/null | wc -l
echo "== dynsym count"; ${P}nm -D "$E" 2>/dev/null | wc -l
${P}nm -C "$E" 2>/dev/null > ~/ep147.syms || true
${P}nm -C -D "$E" 2>/dev/null >> ~/ep147.syms || true
echo "== udev / mailbox users"; grep -iE 'udev|mailbox|UsbDevice|usbctn' ~/ep147.syms | head -30
echo "== slot / media / source types"; grep -iE '(Media|Slot|Source|Storage|Device)(Type|Id|Kind|Manager|Monitor|Watcher)' ~/ep147.syms | awk '{$1="";$2="";print}' | sed 's/(.*//' | sort -u | head -60

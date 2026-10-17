#!/bin/bash
# RTTI class names and plain strings in EP147 that relate to media slots.
E=/mnt/c/Users/icast/AppData/Local/Temp/claude/C--Users-icast-orca-workspaces-xdj-az-mods-koi/12a52a08-a9f1-4102-96c8-4804f5965775/scratchpad/fw/EP147
strings -n 6 "$E" > ~/ep147.str
# Itanium typeinfo names look like N<len>ns<len>ClassE or <len>Class
grep -E '^N?[0-9]+[A-Za-z_]' ~/ep147.str | c++filt -t 2>/dev/null > ~/ep147.rtti
wc -l < ~/ep147.rtti
echo "== media/usb classes"
grep -iE 'usb|media|storage|mount|slot|device(manager|monitor|watch)|source' ~/ep147.rtti | grep -viE 'juce::|std::|boost' | sort -u | head -80
echo "== strings"
grep -iE 'usb ?[1-4]|USB[0-9]|udev|media_?slot|mediaSlot|SLOT_|SourceType|kUsb' ~/ep147.str | sort -u | head -60

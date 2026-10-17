#!/bin/bash
# Disassemble EP147 once (cached) and find code referencing given strings.
# usage: ep-dis.sh "<string1>" ["<string2>" ...]
. "$(dirname "$0")/kenv.sh" true
E=/mnt/c/Users/icast/AppData/Local/Temp/claude/C--Users-icast-orca-workspaces-xdj-az-mods-koi/12a52a08-a9f1-4102-96c8-4804f5965775/scratchpad/fw/EP147
D=~/ep147.dis
P=${CROSS_COMPILE}
if [ ! -s $D ]; then
	${P}objdump -d --no-show-raw-insn "$E" > $D
	${P}readelf -S -W "$E" > ~/ep147.sections
fi
python3 "$(dirname "$0")/xref.py" "$E" $D "$@"

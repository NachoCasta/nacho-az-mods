#!/bin/bash
# build-mod.sh <windows-side module dir>: copy it to the Linux fs, build it against the
# deck's kernel, copy the .ko back next to the sources.
set -e
SRC=$1
NAME=$(basename "$SRC")
. "$(dirname "$0")/kenv.sh" true
W=$HOME/azmod-build/$NAME
rm -rf "$W"; mkdir -p "$W"; cp "$SRC"/*.c "$SRC"/Makefile "$W"/
kmake M="$W" KBUILD_MODPOST_WARN=1 modules 2>&1 | grep -vE '^  (CC|LD|MODPOST|Building)' || true
modinfo "$W/$NAME.ko" | grep -E 'vermagic|description'
cp "$W/$NAME.ko" "$SRC/"

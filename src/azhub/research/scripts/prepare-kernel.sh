#!/bin/bash
# Prepare the Pioneer 4.4.194 tree for out-of-tree modules with the deck's own config
# and the Linaro 6.3.1 toolchain Pioneer ships.
set -e
S=/mnt/c/Users/icast/AppData/Local/Temp/claude/C--Users-icast-orca-workspaces-xdj-az-mods-koi/12a52a08-a9f1-4102-96c8-4804f5965775/scratchpad
cd ~/xdjaz-gpl
TC=XDJ-AZ/prebuilts/gcc/linux-x86/aarch64/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu
[ -x "$TC/bin/aarch64-linux-gnu-gcc" ] || tar -xzf "$S/gpl-src/XDJ-AZ.tar.gz" "$TC"
"$TC/bin/aarch64-linux-gnu-gcc" --version | head -1
cd XDJ-AZ/kernel
cp "$S/deck.config" .config
export ARCH=arm64 CROSS_COMPILE=$HOME/xdjaz-gpl/$TC/bin/aarch64-linux-gnu-
make CC="${CROSS_COMPILE}gcc" HOSTCFLAGS="-std=gnu11 -fcommon -w" olddefconfig >/dev/null
make CC="${CROSS_COMPILE}gcc" HOSTCFLAGS="-std=gnu11 -fcommon -w" -j"$(nproc)" modules_prepare 2>&1 | tail -15
echo "== kernelrelease: $(make -s kernelrelease)"
grep -o '"4[^"]*"' include/generated/utsrelease.h

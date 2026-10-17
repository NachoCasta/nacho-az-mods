#!/bin/bash
# Source-able build environment for the deck's 4.4.194 kernel, then run "$@" in the tree.
export KDIR=$HOME/xdjaz-gpl/XDJ-AZ/kernel
export ARCH=arm64
export CROSS_COMPILE=$HOME/xdjaz-gpl/XDJ-AZ/prebuilts/gcc/linux-x86/aarch64/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
kmake() { make -C "$KDIR" CC="${CROSS_COMPILE}gcc" HOSTCFLAGS="-std=gnu11 -fcommon -w" "$@"; }
cd "$KDIR"
eval "$@"

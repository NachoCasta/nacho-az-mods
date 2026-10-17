#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "$0")"
prefix="${AZHUB_TOOLCHAIN_PREFIX:-$HOME/xdjaz-gpl/XDJ-AZ/prebuilts/gcc/linux-x86/aarch64/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-}"
"${prefix}gcc" -O2 -Wall -shared -fPIC -o linkfake.so linkfake.c -ldl -lpthread
# The shared research source contains diagnostics excluded from SDK hook setup.
"${prefix}gcc" -O2 -Wall -Wno-unused-function -shared -fPIC -fvisibility=hidden \
    -I../../azmod -o hub-libraries.so community.c -ldl -lpthread
"${prefix}nm" -D --defined-only hub-libraries.so

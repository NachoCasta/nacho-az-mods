#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Portable policy and actual-worker controls; no emulator or physical device access.
set -eu
cd "$(dirname "$0")"
TEST_CC=${TEST_CC:-gcc}
TEST_CXX=${TEST_CXX:-g++}
TEST_OUT=${TEST_OUT:-../../local/native-name-tests}
AZ_SDK_INCLUDE=${AZ_SDK_INCLUDE:-../../local/sdk/include}
SAN=${SAN:-"plain asan tsan"}
mkdir -p "$TEST_OUT"
for pass in $SAN; do
    case "$pass" in
        plain) flags="-O2" ;;
        asan) flags="-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer" ;;
        tsan) flags="-O1 -g -fsanitize=thread -fno-omit-frame-pointer" ;;
        *) echo "Unknown sanitizer mode: $pass" >&2;exit 2 ;;
    esac
    for name in name-policy volume-label; do
        "$TEST_CC" -Wall -Wextra -Werror $flags "test-$name.c" -o "$TEST_OUT/test-$name-$pass"
        if [ "$pass" = tsan ] && [ "${TEST_NO_ASLR:-0}" = 1 ]; then
            setarch "$(uname -m)" -R "$TEST_OUT/test-$name-$pass"
        else
            "$TEST_OUT/test-$name-$pass"
        fi
    done
    for name in worker source-refresh information-task; do
        "$TEST_CXX" -std=c++11 -Wall -Wextra -Wno-unused-function $flags -pthread \
            -I"$AZ_SDK_INCLUDE" "tests/test-$name.cpp" -o "$TEST_OUT/test-$name-$pass"
        if [ "$pass" = tsan ] && [ "${TEST_NO_ASLR:-0}" = 1 ]; then
            setarch "$(uname -m)" -R "$TEST_OUT/test-$name-$pass"
        else
            "$TEST_OUT/test-$name-$pass"
        fi
    done
    echo "Native USB Names $pass: policy, filesystem lookup and actual-worker gates pass"
done

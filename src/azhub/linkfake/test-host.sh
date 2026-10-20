#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Portable production-component tests; native firmware integration is separate.
set -eu
cd "$(dirname "$0")"
compiler=${TEST_CC:-gcc}
passes=${SAN:-plain asan tsan}
sdk_include=${AZ_SDK_INCLUDE:-../../azmod}
test -f "$sdk_include/azmod-mod.h" || { echo 'Set AZ_SDK_INCLUDE to the pinned SDK include directory.' >&2; exit 2; }
output=$(mktemp -d /tmp/azhub-host-tests.XXXXXX)
trap 'rm -rf "$output"' EXIT HUP INT TERM
TEST_CC="$compiler" AZ_SDK_INCLUDE="$sdk_include" python3 ./test-hook-registration.py
for pass in $passes; do
    case "$pass" in
        plain) flags='-O2'; symbol='' ;;
        asan) flags='-g -O1 -fsanitize=address,undefined -fno-sanitize-recover=undefined'; symbol='__asan_init' ;;
        tsan) flags='-g -O1 -fsanitize=thread'; symbol='__tsan_init' ;;
        *) echo "Unknown sanitizer pass: $pass" >&2; exit 2 ;;
    esac
    for name in source-name browser-guard network-identity link-status usb-presence link-socket-registry link-mode-recovery link-load-origin link-udp-policy link-udp-receive link-request-tasks link-cached-status eject-gesture eject-source eject-input-health eject-listener eject-detach alias-mount discovery kernel-asset mount-worker source-settings; do
        echo "TEST $pass $name"
        # flags is an intentional list of compiler options, not shell code.
        # shellcheck disable=SC2086
        "$compiler" $flags -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
            -I"$sdk_include" "test-$name.c" -pthread -o "$output/$name"
        if [ -n "$symbol" ]; then
            nm "$output/$name" | grep -q "$symbol" || { echo 'Missing sanitizer instrumentation' >&2; exit 1; }
        fi
        if [ "$name" = source-settings ]; then
            ASAN_OPTIONS=detect_leaks=1 "$output/$name" ../research/devsetting-fixture.dat
        else
            ASAN_OPTIONS=detect_leaks=1 "$output/$name"
        fi
    done
done
echo 'PASS: portable production components. SDK whole-mod harness and exact-binary emulator integration are separate gates.'

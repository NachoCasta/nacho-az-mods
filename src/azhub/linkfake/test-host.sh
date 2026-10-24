#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Portable production-component tests; native firmware integration is separate.
set -eu
cd "$(dirname "$0")"
compiler=${TEST_CC:-gcc}
case "$compiler" in
    *clang) cxx=${TEST_CXX:-${compiler}++} ;;
    *gcc) cxx=${TEST_CXX:-${compiler%gcc}g++} ;;
    *) cxx=${TEST_CXX:-c++} ;;
esac
passes=${SAN:-plain asan tsan}
sdk_include=${AZ_SDK_INCLUDE:-../../azmod}
test -f "$sdk_include/azmod-mod.h" || { echo 'Set AZ_SDK_INCLUDE to the pinned SDK include directory.' >&2; exit 2; }
output=$(mktemp -d /tmp/azhub-host-tests.XXXXXX)
trap 'rm -rf "$output"' EXIT HUP INT TERM
TEST_CC="$compiler" AZ_SDK_INCLUDE="$sdk_include" python3 ./test-hook-registration.py
TEST_CC="$compiler" python3 ./test-port-reconciliation.py
TEST_CC="$compiler" python3 ./test-source-media.py
TEST_CC="$compiler" python3 ./test-source-announcement.py
TEST_CC="$compiler" python3 ./test-source-mount-metadata.py
for pass in $passes; do
    case "$pass" in
        plain) flags='-O2'; symbol='' ;;
        asan) flags='-g -O1 -fsanitize=address,undefined -fno-sanitize-recover=undefined'; symbol='__asan_init' ;;
        tsan) flags='-g -O1 -fsanitize=thread'; symbol='__tsan_init' ;;
        *) echo "Unknown sanitizer pass: $pass" >&2; exit 2 ;;
    esac
    for name in info-directory library-path port-topology source-name source-properties source-metadata source-metadata-native source-display source-order source-order-native source-badge-render source-badge-native-compose source-badge-native-text source-badge-native-image source-badge-native-glyph browser-guard network-identity link-status usb-presence link-socket-registry link-mode-recovery link-member-retention link-load-origin link-udp-policy link-udp-receive link-request-tasks link-cached-status eject-gesture eject-presentation eject-source eject-input-health eject-listener eject-detach public-address-conflict public-address-inventory public-address-remove public-network-namespace public-network-link public-network-acquire public-address-arp-namespace public-network-ready public-dhcp-event public-network-lease public-link-local public-dhcp-acquire public-dhcp-refresh public-dhcp-decline public-dhcp-decline-namespace public-broker-wire public-broker-probe public-broker-source public-broker-generation public-broker-client public-broker-inventory public-broker-emulator-policy public-source-mac public-player-pool public-player-discovery public-player-reconcile public-sharing-inventory public-sharing-runtime public-helper-logs public-macvlan-asset public-peer-network public-parent-rx-probe public-acd-jitter public-broker-root-policy public-broker-policy public-broker-dispatch public-broker-lease public-broker-address-retirement public-broker-allocator public-file-root public-nfs-read public-rpc public-rpc-read public-file-catalog public-rpc-udp public-nfs-tree public-rpc-export public-pdb-reader public-pdb-tables public-entity-index public-library-metadata public-db-wire public-db-session public-db-port public-provider-packets public-provider-state public-provider-udp public-db-menus public-source-service public-source-worker public-source-manager public-mod-emulator public-analysis-reader public-cues public-library-files public-track-index alias-mount discovery kernel-asset mount-worker source-settings; do
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
    echo "TEST $pass link-member-native-unwind"
    # Native C++ callbacks may throw through the C adapter. Exercise the actual
    # adapter, including its cleanup scope, with both normal and throwing peers.
    # shellcheck disable=SC2086
    "$compiler" $flags -fexceptions -Wall -Wextra -Wno-unused-function \
        -pthread -c test-link-member-unwind.c -o "$output/member-unwind.o"
    # shellcheck disable=SC2086
    "$cxx" $flags -fexceptions -Wall -Wextra -pthread \
        test-link-member-unwind.cpp "$output/member-unwind.o" -o "$output/member-unwind"
    if [ -n "$symbol" ]; then
        nm "$output/member-unwind" | grep -q "$symbol" || { echo 'Missing sanitizer instrumentation' >&2; exit 1; }
    fi
    ASAN_OPTIONS=detect_leaks=1 "$output/member-unwind"
done
echo 'PASS: portable production components. SDK whole-mod harness and exact-binary emulator integration are separate gates.'

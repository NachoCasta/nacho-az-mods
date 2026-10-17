#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Emulator-only path-churn fixture. Never run on a real AZ.
set -eu
[ "$(uname -r)" = 6.6.138-dirty ] || exit 2
pid=$(pidof EP147)
[ -n "$pid" ] || exit 2
fixture=/tmp/azhub-removal-mounts
[ "$(readlink "$fixture")" = /proc/mounts ] || exit 2
original=$(grep '^/dev/sdc1 /media/usb/sdc1 ' /proc/mounts)
[ -n "$original" ] || exit 2
grep -v '^/dev/sdc1 ' /proc/mounts > /tmp/koi-reuse-base
restore() {
    ln -sf /proc/mounts /tmp/koi-reuse-restore
    mv -f /tmp/koi-reuse-restore "$fixture"
}
trap restore EXIT INT TERM
for cycle in 1 2 3 4 5; do
    echo "cycle $cycle: withdraw"
    cp /tmp/koi-reuse-base /tmp/koi-reuse-next
    mv -f /tmp/koi-reuse-next "$fixture"
    sleep 8
    [ "$(pidof EP147)" = "$pid" ]
    path=/media/usb/koi-reuse-$cycle
    [ ! -e "$path" ] || exit 3
    ln -s /media/usb/sdc1 "$path"
    cp /tmp/koi-reuse-base /tmp/koi-reuse-next
    printf '%s\n' "$original" | sed "s| /media/usb/sdc1 | $path |" >> /tmp/koi-reuse-next
    mv -f /tmp/koi-reuse-next "$fixture"
    sleep 14
    [ "$(pidof EP147)" = "$pid" ]
    grep -q "hub source reused .*path=$path$" /tmp/linkfake.log
    grep -q "hub mount queued .*path=$path$" /tmp/linkfake.log
    count=$(ls -l /proc/$pid/fd 2>/dev/null | grep -c '/exportLibrary.db$' || true)
    [ "$count" = 5 ] || { echo "FAIL DB count=$count"; exit 4; }
    echo "cycle $cycle: passed; same PID $pid; 5 databases"
done
echo 'Path reuse passed: five replacements beyond eight lifetime paths'

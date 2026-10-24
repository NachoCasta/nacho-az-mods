#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Retry failed external hub probes after the guarded kernel patch is ready.
# Never unbind a driver or reset a controller, hub, or storage device.
set -eu
[ "$#" = 1 ] && [ "$1" = startup ] || exit 2
case "$(uname -r)" in
    4.4.194) [ -d /sys/module/azhub ] || exit 3; emulator=0 ;;
    6.6.138-dirty) emulator=1 ;;
    *) exit 3 ;;
esac
[ -w /sys/bus/usb/drivers/hub/bind ] || exit 3
for interface in /sys/bus/usb/devices/*:*; do
    name=${interface##*/}
    # Root hubs have port zero; configuration/interface suffixes are mandatory.
    printf '%s\n' "$name" | grep -Eq '^[1-9][0-9]*-[1-9][0-9]*(\.[1-9][0-9]*)*:[1-9][0-9]*\.[0-9]+$' || continue
    [ -d "$interface" ] || continue
    if [ "$emulator" = 0 ]; then
        topology=$(readlink -f "$interface") || continue
        case "$topology" in
            */fe380000.usb/usb1/*|*/fe3c0000.usb/usb2/*) ;;
            *) continue ;;
        esac
    fi
    class=$(cat "$interface/bInterfaceClass" 2>/dev/null) || continue
    [ "$class" = 09 ] || continue
    # A successful, foreign, or dangling driver link is never touched. The hub
    # driver's own match/probe checks remain authoritative if hotplug races us.
    [ ! -e "$interface/driver" ] && [ ! -L "$interface/driver" ] || continue
    printf 'hub startup retry %s\n' "$name"
    if printf '%s\n' "$name" > /sys/bus/usb/drivers/hub/bind; then
        printf 'hub startup probe accepted %s\n' "$name"
    else
        printf 'hub startup probe refused %s\n' "$name" >&2
    fi
done

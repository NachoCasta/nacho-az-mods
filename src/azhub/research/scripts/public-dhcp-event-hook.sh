#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Experimental udhcpc hook. Report only to an explicitly inherited event pipe.
# Never configure interfaces/routes/DNS or call the native default DHCP script.
# The launcher must pass a bounded event channel as fd3 and retain supervision.
[ "${AZHUB_DHCP_EVENT_FD-}" = 3 ] || exit 1
[ "${interface-}" = "${AZHUB_DHCP_INTERFACE-}" ] || exit 1
case "${interface-}" in azpub*) ;; *) exit 1 ;; esac
case "$interface" in *[!a-zA-Z0-9_-]*) exit 1 ;; esac
[ "${#interface}" -lt 16 ] || exit 1
event=${1-}
case "$event" in
    bound|renew)
        valid=1
        for value in "${ip-}" "${subnet-}" "${serverid-}"; do
            case "$value" in ''|*[!0-9.]*) valid=0 ;; esac
            [ "${#value}" -le 15 ] || valid=0
        done
        case "${lease-}" in ''|*[!0-9]*) valid=0 ;; esac
        [ "${#lease}" -le 10 ] || valid=0
        if [ "$valid" = 1 ]; then
            printf '1\t%s\t%s\t%s\t%s\t%s\t%s\n' \
                "$event" "$interface" "$ip" "$subnet" "$lease" "$serverid" >&3
            exit $?
        fi
        event=invalid
        ;;
    deconfig|nak|leasefail) ;;
    *) exit 1 ;;
esac
printf '1\t%s\t%s\t-\t-\t-\t-\n' "$event" "$interface" >&3

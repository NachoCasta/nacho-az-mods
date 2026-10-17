#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Mount a sysfs-verified hub block device without announcing any local USB slot.
set -eu
[ "$#" = 1 ] || exit 2
device=$1
printf '%s\n' "$device" | grep -Eq '^sd[a-z]+[0-9]*$' || exit 2
[ -b "/dev/$device" ] || exit 3
topology=$(readlink -f "/sys/class/block/$device")
printf '%s\n' "$topology" | tr / '\n' | grep -Eq '^[0-9]+-[0-9]+(\.[0-9]+)+$' || exit 3
mount_dir=/media/usb/$device
mountpoint -q "$mount_dir" && exit 0
fs=$(/sbin/blkid -o value -s TYPE "/dev/$device" 2>/dev/null) || exit 4
mode=rw
[ "$(cat "/sys/class/block/$device/ro" 2>/dev/null || echo 0)" = 1 ] && mode=ro
case "$fs" in
    vfat) type=vfat; options=$mode,noatime,shortname=mixed,dmask=000,fmask=000,codepage=437,iocharset=iso8859-1,usefree,utf8,flush ;;
    exfat)
        if [ -x /usr/sbin/mount.exfat-fuse ]; then
            type=exfat-fuse; options=$mode,noatime,flush,nonempty
        else
            type=exfat; options=$mode,noatime
        fi ;;
    hfsplus) type=hfsplus; options=$mode,noatime,force ;;
    *) exit 4 ;;
esac
# A leftover directory must be empty. Never recursively remove its contents.
if [ -d "$mount_dir" ]; then rmdir "$mount_dir" || exit 5; fi
mkdir -p "$mount_dir"
if ! mount -t "$type" -o "$options" "/dev/$device" "$mount_dir"; then
    rmdir "$mount_dir" 2>/dev/null || true
    exit 6
fi
printf 'hub mounted %s on %s (%s)\n' "$device" "$mount_dir" "$fs"

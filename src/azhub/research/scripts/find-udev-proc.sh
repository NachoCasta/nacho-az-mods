#!/bin/bash
# Find the vendor driver that creates /proc/udev_usb1/2 in the GPL kernel tree.
cd ~/xdjaz-gpl/XDJ-AZ/kernel
grep -rln 'udev_usb' --include=*.c --include=*.h --include=Makefile --include=Kconfig . | head

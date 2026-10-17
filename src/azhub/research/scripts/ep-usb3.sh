#!/bin/bash
# Everything in EP147 that names a USB3/USB4 slot or a model switch.
grep -E 'Usb3|USB3|usb3|Usb4|USB4|usb4' ~/ep147.str | grep -v 'usb_gadget' | sort -u | head -80
echo "== slot enums / names"
grep -E '^(kSlot|SLOT|Slot)|MediaSlot|SlotType|slot_?type|SlotId' ~/ep147.str | sort -u | head -40
echo "== models"
grep -E 'OPUS|Opus|XDJ-AZ|XDJAZ|XDJ_AZ|ModelType|modelType|isOpus|isXdjAz|CDJ-3000' ~/ep147.str | sort -u | head -40
echo "== source images"
grep -E '^img_(Source|Shortcut)_' ~/ep147.str | sort -u

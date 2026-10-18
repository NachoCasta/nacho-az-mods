/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HUB_EJECT_UI_MODEL_H
#define HUB_EJECT_UI_MODEL_H
#include <limits.h>
struct eject_item {
    char label[128],disk[64],location[64],topology[PATH_MAX]; int port;
#ifdef LF_SAFE_EJECT
    char usb[PATH_MAX]; unsigned long long generation; unsigned devnum;
    int state;
#endif
};
struct eject_inventory { struct eject_item items[EJECT_MAX]; int count, hubs[2]; unsigned overflow; };
#endif

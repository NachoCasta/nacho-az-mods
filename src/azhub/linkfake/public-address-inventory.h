/* SPDX-License-Identifier: GPL-3.0-only */
/* Native-controller namespace snapshot, separate from reflected-MAC filtering.
 * This is an initial candidate guard, not an address-change notification feed. */
#ifndef HUB_PUBLIC_ADDRESS_INVENTORY_H
#define HUB_PUBLIC_ADDRESS_INVENTORY_H
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
struct hub_public_address_inventory { struct in_addr addresses[32];unsigned count; };
static int hub_public_address_inventory_contains(const struct hub_public_address_inventory *inventory,struct in_addr address) {
    for (unsigned i=0;i<inventory->count;++i) if (inventory->addresses[i].s_addr==address.s_addr) return 1;
    return 0;
}
static int hub_public_address_inventory_add(struct hub_public_address_inventory *inventory,struct in_addr address) {
    if (!inventory || inventory->count>32) { errno=EINVAL;return -1; }
    if (!address.s_addr || hub_public_address_inventory_contains(inventory,address)) return 0;
    if (inventory->count==32) { errno=E2BIG;return -1; }
    inventory->addresses[inventory->count++]=address;return 0;
}
static int hub_public_address_inventory_native(struct hub_public_address_inventory *inventory) {
    if (!inventory) { errno=EINVAL;return -1; }
    memset(inventory,0,sizeof *inventory);struct ifaddrs *interfaces;
    if (getifaddrs(&interfaces)<0) return -1;
    int result=0,error=0;
    for (struct ifaddrs *it=interfaces;it;it=it->ifa_next) if (it->ifa_addr && it->ifa_addr->sa_family==AF_INET) {
        struct in_addr address=((struct sockaddr_in *)it->ifa_addr)->sin_addr;
        if (hub_public_address_inventory_add(inventory,address)<0) { result=-1;error=errno;break; }
    }
    freeifaddrs(interfaces);if (result<0) errno=error;return result;
}
#endif

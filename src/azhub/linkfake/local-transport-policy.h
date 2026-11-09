/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HUB_LOCAL_TRANSPORT_POLICY_H
#define HUB_LOCAL_TRANSPORT_POLICY_H
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <string.h>
#ifndef HUB_LOCAL_GETIFADDRS
#define HUB_LOCAL_GETIFADDRS(list) getifaddrs(list)
#define HUB_LOCAL_FREEIFADDRS(list) freeifaddrs(list)
#endif
/* Native observed IPv4 can belong to Wi-Fi. Only an addressed, live Ethernet
 * interface can make the external Link transport available. Read-only and used
 * only by native lifecycle callbacks, never the UDP/frame receive hot paths.
 * A failed observation is unknown, rather than evidence of an external link. */
static int hub_local_ethernet_state(void) {
    struct ifaddrs *list=NULL;
    if (HUB_LOCAL_GETIFADDRS(&list)) return -1;
    int ready=0;
    for (const struct ifaddrs *entry=list;entry;entry=entry->ifa_next) {
        if (!entry->ifa_name || strcmp(entry->ifa_name,"eth0") || !entry->ifa_addr ||
            entry->ifa_addr->sa_family!=AF_INET ||
            (entry->ifa_flags&(IFF_UP|IFF_RUNNING))!=(IFF_UP|IFF_RUNNING)) continue;
        if (((const struct sockaddr_in *)entry->ifa_addr)->sin_addr.s_addr) {ready=1;break;}
    }
    HUB_LOCAL_FREEIFADDRS(list);return ready;
}
#endif

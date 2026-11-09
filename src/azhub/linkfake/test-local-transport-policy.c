/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
static struct ifaddrs entries[4];static int failed,frees;
static int observe(struct ifaddrs **out) {if(failed)return -1;*out=entries;return 0;}
static void release(struct ifaddrs *list) {assert(list==entries);frees++;}
#define HUB_LOCAL_GETIFADDRS(list) observe(list)
#define HUB_LOCAL_FREEIFADDRS(list) release(list)
#include "local-transport-policy.h"
int main(void) {
    struct sockaddr_in address={.sin_family=AF_INET,.sin_addr={.s_addr=1}};
    struct sockaddr_in zero={.sin_family=AF_INET};
    struct sockaddr_in6 ipv6={.sin6_family=AF_INET6};
    entries[0]=(struct ifaddrs){.ifa_name="mlan0",.ifa_addr=(void *)&address,.ifa_flags=IFF_UP|IFF_RUNNING,.ifa_next=entries+1};
    entries[1]=(struct ifaddrs){.ifa_name="mgmt0",.ifa_addr=(void *)&address,.ifa_flags=IFF_UP|IFF_RUNNING,.ifa_next=entries+2};
    entries[2]=(struct ifaddrs){.ifa_name="eth0",.ifa_addr=(void *)&zero,.ifa_flags=IFF_UP|IFF_RUNNING,.ifa_next=entries+3};
    entries[3]=(struct ifaddrs){.ifa_name="eth0",.ifa_addr=(void *)&ipv6,.ifa_flags=IFF_UP|IFF_RUNNING};
    assert(!hub_local_ethernet_state()); /* Wi-Fi/management IPv4 are not wired Link. */
    entries[2].ifa_addr=(void *)&address;entries[2].ifa_flags=IFF_UP;
    assert(!hub_local_ethernet_state()); /* No carrier. */
    entries[2].ifa_flags=IFF_RUNNING;assert(!hub_local_ethernet_state()); /* Admin down. */
    entries[2].ifa_flags=IFF_UP|IFF_RUNNING;assert(hub_local_ethernet_state()==1);
    entries[2].ifa_addr=NULL;assert(!hub_local_ethernet_state());
    entries[2].ifa_name=NULL;assert(!hub_local_ethernet_state());
    failed=1;int before=frees;assert(hub_local_ethernet_state()==-1 && frees==before);
    puts("Read-only Ethernet policy: Wi-Fi/management-only IPv4, missing address/carrier, admin down, IPv6, null entries, usable wired link and observation failure passed");
    return 0;
}

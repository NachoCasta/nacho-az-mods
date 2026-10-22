/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
static struct ifaddrs *fixture;
static int test_getifaddrs(struct ifaddrs **out) {
    if (fixture) { *out=fixture;return 0; }
    return getifaddrs(out);
}
static void test_freeifaddrs(struct ifaddrs *list) { if (list!=fixture) freeifaddrs(list); }
#define getifaddrs test_getifaddrs
#define freeifaddrs test_freeifaddrs
#include "public-peer-network.h"
#undef getifaddrs
#undef freeifaddrs
static int allowed(struct hub_public_peer_network *network,const char *ip) {
    struct sockaddr_in peer={.sin_family=AF_INET,.sin_addr={.s_addr=inet_addr(ip)}};
    return hub_public_peer_network_allowed(network,&peer);
}
int main(void) {
    struct hub_public_peer_network network={0};
    assert(!hub_public_peer_network_add(&network,inet_addr("192.0.2.15"),inet_addr("255.255.255.0")));
    assert(allowed(&network,"192.0.2.2") && !allowed(&network,"192.0.2.15"));
    assert(!hub_public_peer_network_link_local(&network));
    for (unsigned i=0;i<6;++i) assert(!allowed(&network,(const char *[]){"192.0.2.0","192.0.2.255","198.51.100.2","127.0.0.1","224.0.0.1","0.0.0.0"}[i]));
    assert(!hub_public_peer_network_add(&network,inet_addr("169.254.2.3"),inet_addr("255.255.0.0")));
    assert(allowed(&network,"169.254.4.5") && !allowed(&network,"169.254.2.3"));
    assert(hub_public_peer_network_link_local(&network));
    struct hub_public_peer_network reversed={.count=2};reversed.entries[0]=network.entries[1];reversed.entries[1]=network.entries[0];
    assert(hub_public_peer_network_same(&network,&reversed));
    reversed.entries[0].mask=inet_addr("255.255.255.0");assert(!hub_public_peer_network_same(&network,&reversed));
    assert(hub_public_peer_network_add(&network,inet_addr("192.0.2.1"),inet_addr("255.0.255.0"))<0);
    assert(hub_public_peer_network_snapshot(&network,"azhub-nonexistent")<0 && !network.count);
    assert(!allowed(&network,"192.0.2.2"));
    struct sockaddr_in alias={.sin_family=AF_INET,.sin_addr={.s_addr=inet_addr("169.254.7.213")}};
    struct sockaddr_in foreign={.sin_family=AF_INET,.sin_addr={.s_addr=inet_addr("198.51.100.3")}};
    struct sockaddr_in mask={.sin_family=AF_INET,.sin_addr={.s_addr=inet_addr("255.255.0.0")}};
    struct ifaddrs other={.ifa_name="eth01:avahi",.ifa_addr=(void *)&foreign,.ifa_netmask=(void *)&mask};
    struct ifaddrs labelled={.ifa_next=&other,.ifa_name="eth0:avahi",.ifa_addr=(void *)&alias,.ifa_netmask=(void *)&mask};
    fixture=&labelled;
    assert(!hub_public_peer_network_snapshot(&network,"eth0") && network.count==1);
    assert(hub_public_peer_network_link_local(&network));
    assert(allowed(&network,"169.254.7.214") && !allowed(&network,"169.254.7.213") && !allowed(&network,"198.51.100.2"));
    assert(!hub_public_peer_network_interface("eth0:","eth0"));
    assert(!hub_public_peer_network_interface("eth01","eth0"));
    fixture=NULL;
    puts("PASS: native subnet peers, native-address/broadcast/multicast exclusion, link-local prefix and missing-interface refusal");
}

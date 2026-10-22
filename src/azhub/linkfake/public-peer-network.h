/* SPDX-License-Identifier: GPL-3.0-only */
/* Immutable native-interface IPv4 admission snapshot. Refresh/restart policy
 * on native network changes belongs to the controller, not service workers. */
#ifndef HUB_PUBLIC_PEER_NETWORK_H
#define HUB_PUBLIC_PEER_NETWORK_H
#include <ifaddrs.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
struct hub_public_peer_network { struct { uint32_t address,mask; } entries[32];unsigned count; };
static int hub_public_peer_network_link_local(const struct hub_public_peer_network *network) {
    if (!network || network->count>32) return 0;
    for (unsigned i=0;i<network->count;++i) {
        uint32_t address=ntohl(network->entries[i].address);
        if (network->entries[i].mask==htonl(UINT32_C(0xffff0000)) &&
                address>=UINT32_C(0xa9fe0100) && address<=UINT32_C(0xa9fefeff)) return 1;
    }
    return 0;
}
static int hub_public_peer_network_same(const struct hub_public_peer_network *a,const struct hub_public_peer_network *b) {
    if (!a || !b || a->count>32 || b->count!=a->count) return 0;
    for (unsigned i=0;i<a->count;++i) {
        int found=0;for (unsigned j=0;j<b->count;++j)
            if (a->entries[i].address==b->entries[j].address && a->entries[i].mask==b->entries[j].mask) found=1;
        if (!found) return 0;
    }
    return 1;
}
static int hub_public_peer_network_add(struct hub_public_peer_network *network,uint32_t address,uint32_t mask) {
    uint32_t host=ntohl(address),inverse=~ntohl(mask);
    if (!network || network->count>=32 || !host || (host>>24)==127 || host>=0xe0000000 ||
            !mask || !inverse || (inverse&(inverse+1)) || inverse<3) { errno=EINVAL;return -1; }
    for (unsigned i=0;i<network->count;++i) if (network->entries[i].address==address && network->entries[i].mask==mask) return 0;
    network->entries[network->count].address=address;network->entries[network->count++].mask=mask;return 0;
}
/* Linux getifaddrs may report an IPv4 address label such as eth0:avahi.
 * Include labels belonging to this interface, never similarly prefixed devices. */
static int hub_public_peer_network_interface(const char *label,const char *name) {
    if (!label || !name || !*name) return 0;
    size_t length=strlen(name);
    return !strcmp(label,name) || (!strncmp(label,name,length) && label[length]==':' && label[length+1]);
}
static int hub_public_peer_network_snapshot(struct hub_public_peer_network *network,const char *name) {
    if (!network || !name || !*name) { errno=EINVAL;return -1; }
    memset(network,0,sizeof *network);struct ifaddrs *interfaces;if (getifaddrs(&interfaces)<0) return -1;
    int error=0;
    for (struct ifaddrs *it=interfaces;it;it=it->ifa_next) if (hub_public_peer_network_interface(it->ifa_name,name) &&
            it->ifa_addr && it->ifa_netmask && it->ifa_addr->sa_family==AF_INET && it->ifa_netmask->sa_family==AF_INET) {
        if (hub_public_peer_network_add(network,((struct sockaddr_in *)it->ifa_addr)->sin_addr.s_addr,
                ((struct sockaddr_in *)it->ifa_netmask)->sin_addr.s_addr)<0) { error=errno;break; }
    }
    freeifaddrs(interfaces);
    if (error || !network->count) { memset(network,0,sizeof *network);errno=error ? error:ENETUNREACH;return -1; }
    return 0;
}
static int hub_public_peer_network_allowed(void *opaque,const struct sockaddr_in *peer) {
    const struct hub_public_peer_network *network=opaque;
    if (!network || network->count>32 || !peer || peer->sin_family!=AF_INET) return 0;
    uint32_t address=peer->sin_addr.s_addr,host=ntohl(address);
    if (!host || (host>>24)==127 || host>=0xe0000000) return 0;
    for (unsigned i=0;i<network->count;++i) if (address==network->entries[i].address) return 0;
    for (unsigned i=0;i<network->count;++i) {
        uint32_t mask=network->entries[i].mask,subnet=network->entries[i].address&mask;
        if ((address&mask)==subnet && address!=subnet && address!=(subnet|~mask)) return 1;
    }
    return 0;
}
#endif

/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef AZHUB_NETWORK_IDENTITY_H
#define AZHUB_NETWORK_IDENTITY_H
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ifaddrs.h>
#include <net/if.h>
/* Values are network-byte-order. These are internal labels only: no address
 * is assigned to an interface and all transport remains loopback. */
static int hub_choose_peers(uint32_t address,uint32_t netmask,uint32_t peers[2]) {
    uint32_t own=ntohl(address),mask=ntohl(netmask),hosts=~mask;
    if (!mask || (hosts&(hosts+1)) || hosts<7 || !own || (own>>24)==127 || (own>>28)>=14) return 0;
    uint32_t base=own&mask,last=base|hosts;unsigned count=0;
    for (uint32_t candidate=last-1;candidate>base && count<2;candidate--)
        if(candidate!=own) peers[count++]=htonl(candidate);
    return count==2;
}
static int hub_network_peers(uint32_t preferred,uint32_t peers[2]) {
    struct ifaddrs *list;
    if (getifaddrs(&list)) return 0;
    char route_if[IF_NAMESIZE]={0};
    FILE *routes=fopen("/proc/net/route","re");
    if (routes) {
        char line[512],name[64];unsigned destination,flags;
        while(fgets(line,sizeof line,routes)) {
            if(sscanf(line,"%63s %x %*x %x",name,&destination,&flags)==3 && !destination && (flags&1)) {
                snprintf(route_if,sizeof route_if,"%.*s",IF_NAMESIZE-1,name);break;
            }
        }
        fclose(routes);
    }
    int found=0,best=-1;
    for (struct ifaddrs *i=list;i;i=i->ifa_next) {
        if(!i->ifa_addr || !i->ifa_netmask || i->ifa_addr->sa_family!=AF_INET ||
           !(i->ifa_flags&IFF_UP) || (i->ifa_flags&IFF_LOOPBACK)) continue;
        uint32_t address=((struct sockaddr_in *)i->ifa_addr)->sin_addr.s_addr;
        uint32_t mask=((struct sockaddr_in *)i->ifa_netmask)->sin_addr.s_addr;
        uint32_t candidate[2];if(!hub_choose_peers(address,mask,candidate))continue;
        int score=(preferred && address==preferred) ? 2 : !strcmp(i->ifa_name,route_if) ? 1 : 0;
        if(score<=best)continue;
        best=score;memcpy(peers,candidate,sizeof candidate);found=1;
    }
    freeifaddrs(list);return found;
}
#endif

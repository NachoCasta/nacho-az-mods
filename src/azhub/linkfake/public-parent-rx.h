/* SPDX-License-Identifier: GPL-3.0-only */
/* Descriptor-owned parent receive-mode reference for secondary MAC delivery.
 * Protocol zero receives no frames. Close/process exit removes only this
 * membership; never overwrite interface flags or another user's reference. */
#ifndef HUB_PUBLIC_PARENT_RX_H
#define HUB_PUBLIC_PARENT_RX_H
#include <linux/if_packet.h>
#include <net/if.h>
static int hub_public_parent_rx_membership(const char *name) {
    unsigned index=if_nametoindex(name);
    if(!index) {errno=ENODEV;return -1;}
    int fd=socket(AF_PACKET,SOCK_RAW|SOCK_CLOEXEC,0);
    if(fd<0) return -1;
    struct packet_mreq request={.mr_ifindex=(int)index,.mr_type=PACKET_MR_PROMISC};
    if(setsockopt(fd,SOL_PACKET,PACKET_ADD_MEMBERSHIP,&request,sizeof request)<0) {
        int error=errno;close(fd);errno=error;return -1;
    }
    return fd;
}
#endif

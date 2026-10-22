/* SPDX-License-Identifier: GPL-3.0-only */
/* Nonblocking Ethernet ARP adapter for an already owned interface/namespace.
 * Caller owns network lifecycle and must inventory all local interface MACs.
 * Does not configure addresses, create interfaces or change namespaces. */
#ifndef HUB_PUBLIC_ADDRESS_ARP_H
#define HUB_PUBLIC_ADDRESS_ARP_H
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include "public-address-conflict.h"
struct hub_public_arp { int fd,index; unsigned char mac[6]; };
static void hub_public_arp_close(struct hub_public_arp *arp) {
    if (arp && arp->fd>=0) { close(arp->fd); arp->fd=-1; }
}
static int hub_public_arp_open(struct hub_public_arp *arp,const char *interface,
        const unsigned char mac[6]) {
    if (!arp || !interface || !mac || strnlen(interface,IFNAMSIZ)==IFNAMSIZ ||
            strncmp(interface,"azpub",5) || (mac[0]&1)) { errno=EINVAL; return -1; }
    *arp=(struct hub_public_arp){.fd=-1};
    int fd=socket(AF_PACKET,SOCK_DGRAM|SOCK_NONBLOCK|SOCK_CLOEXEC,htons(ETH_P_ARP));
    if (fd<0) return -1;
    struct ifreq req={0};strcpy(req.ifr_name,interface);
    if (ioctl(fd,SIOCGIFFLAGS,&req)<0) goto failed;
    if (!(req.ifr_flags&IFF_UP) || req.ifr_flags&IFF_LOOPBACK) { errno=EINVAL; goto failed; }
    if (ioctl(fd,SIOCGIFHWADDR,&req)<0) goto failed;
    if (req.ifr_hwaddr.sa_family!=ARPHRD_ETHER || memcmp(req.ifr_hwaddr.sa_data,mac,6)) {
        errno=EINVAL; goto failed;
    }
    if (ioctl(fd,SIOCGIFINDEX,&req)<0) goto failed;
    struct sockaddr_ll address={.sll_family=AF_PACKET,.sll_protocol=htons(ETH_P_ARP),.sll_ifindex=req.ifr_ifindex};
    if (bind(fd,(struct sockaddr *)&address,sizeof address)<0) goto failed;
    arp->fd=fd;arp->index=req.ifr_ifindex;memcpy(arp->mac,mac,6);return 0;
failed: {
    int error=errno;close(fd);errno=error;return -1;
    }
}
/* Success confirms the full ARP was queued; caller then completes its action
 * with a fresh monotonic timestamp. EAGAIN leaves the action pending. */
static int hub_public_arp_send(const struct hub_public_arp *arp,const struct hub_public_acd *acd) {
    unsigned char packet[28];
    if (!arp || arp->fd<0 || !acd || memcmp(arp->mac,acd->mac,6) ||
            hub_public_acd_packet(acd,packet)<0) { errno=EINVAL; return -1; }
    struct sockaddr_ll destination={.sll_family=AF_PACKET,.sll_protocol=htons(ETH_P_ARP),
        .sll_ifindex=arp->index,.sll_halen=6};memset(destination.sll_addr,255,6);
    ssize_t sent=syscall(SYS_sendto,arp->fd,packet,sizeof packet,MSG_DONTWAIT,
        &destination,sizeof destination);
    if (sent==(ssize_t)sizeof packet) return 0;
    if (sent>=0) errno=EIO;
    return -1;
}
/* At most32 packets per controller iteration; truncated/foreign-interface,
 * non-Ethernet, outgoing and inconsistent Ethernet/ARP senders are ignored.
 * Returns1 on conflict,0 on no conflict,-1 on socket error. */
static int hub_public_arp_receive(const struct hub_public_arp *arp,struct hub_public_acd *acd,
        const unsigned char (*local_mac)[6],size_t count) {
    if (!arp || arp->fd<0 || !acd || (count && !local_mac)) { errno=EINVAL; return -1; }
    for (unsigned i=0;i<32;++i) {
        unsigned char packet[256];struct sockaddr_ll source={0};socklen_t size=sizeof source;
        ssize_t length=recvfrom(arp->fd,packet,sizeof packet,MSG_DONTWAIT|MSG_TRUNC,
            (struct sockaddr *)&source,&size);
        if (length<0) {
            if (errno==EAGAIN || errno==EWOULDBLOCK) return 0;
            if (errno==EINTR) continue;
            return -1;
        }
        if (length<28 || length>(ssize_t)sizeof packet || size!=sizeof source ||
                source.sll_family!=AF_PACKET || source.sll_ifindex!=arp->index ||
                source.sll_protocol!=htons(ETH_P_ARP) || source.sll_hatype!=ARPHRD_ETHER ||
                source.sll_halen!=6 || source.sll_pkttype==PACKET_OUTGOING ||
                source.sll_pkttype==PACKET_OTHERHOST || memcmp(source.sll_addr,packet+8,6)) continue;
        if (hub_public_acd_observe(acd,packet,(size_t)length,local_mac,count)) return 1;
    }
    return 0;
}
#endif

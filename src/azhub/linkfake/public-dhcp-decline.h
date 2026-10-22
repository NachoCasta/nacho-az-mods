/* SPDX-License-Identifier: GPL-3.0-only */
/* DHCPDECLINE for this allocator's MAC-based client identity. Never transmits
 * with the conflicting address as IPv4 source. A matching observed ACD conflict
 * and owned ARP transport are required by the sender. No automatic retry. */
#ifndef HUB_PUBLIC_DHCP_DECLINE_H
#define HUB_PUBLIC_DHCP_DECLINE_H
#include "public-address-arp.h"
#include "public-dhcp-event.h"
#define HUB_PUBLIC_DECLINE_SIZE 328
static uint32_t hub_public_decline_sum(uint32_t sum,const unsigned char *p,size_t size) {
    while (size>=2) { sum+=(unsigned)p[0]*256+p[1];p+=2;size-=2; }
    if (size) sum+=(unsigned)*p*256;
    return sum;
}
static uint16_t hub_public_decline_checksum(uint32_t sum) {
    while (sum>>16) sum=(sum&65535)+(sum>>16);
    return (uint16_t)~sum;
}
static void hub_public_decline_u16(unsigned char *p,unsigned value) {
    p[0]=(unsigned char)(value>>8);p[1]=(unsigned char)value;
}
static int hub_public_dhcp_decline_packet(unsigned char *out,size_t capacity,
        const struct hub_public_dhcp_event *grant,const unsigned char mac[6],const unsigned char xid[4]) {
    if (!out || capacity<HUB_PUBLIC_DECLINE_SIZE || !grant || !mac || !xid ||
            grant->kind>HUB_DHCP_RENEW || !hub_public_dhcp_unicast(grant->address) ||
            !hub_public_dhcp_unicast(grant->server) || (mac[0]&1) ||
            !memcmp(mac,"\0\0\0\0\0\0",6) || !memcmp(xid,"\0\0\0\0",4)) return -1;
    memset(out,0,HUB_PUBLIC_DECLINE_SIZE);
    out[0]=0x45;hub_public_decline_u16(out+2,HUB_PUBLIC_DECLINE_SIZE);out[8]=64;out[9]=17;
    memset(out+16,255,4);
    unsigned char *udp=out+20,*bootp=udp+8;
    hub_public_decline_u16(udp,68);hub_public_decline_u16(udp+2,67);hub_public_decline_u16(udp+4,308);
    bootp[0]=1;bootp[1]=1;bootp[2]=6;memcpy(bootp+4,xid,4);memcpy(bootp+28,mac,6);
    memcpy(bootp+236,"\x63\x82\x53\x63",4);
    unsigned char *option=bootp+240;
    *option++=53;*option++=1;*option++=4;
    *option++=50;*option++=4;memcpy(option,&grant->address.s_addr,4);option+=4;
    *option++=54;*option++=4;memcpy(option,&grant->server.s_addr,4);option+=4;
    *option++=61;*option++=7;*option++=1;memcpy(option,mac,6);option+=6;
    static const char reason[]="Address conflict";
    *option++=56;*option++=sizeof reason-1;memcpy(option,reason,sizeof reason-1);option+=sizeof reason-1;
    *option=255;
    uint16_t checksum=hub_public_decline_checksum(hub_public_decline_sum(0,out,20));
    hub_public_decline_u16(out+10,checksum);
    uint32_t sum=hub_public_decline_sum(0,out+12,8)+17+308;
    checksum=hub_public_decline_checksum(hub_public_decline_sum(sum,udp,308));
    hub_public_decline_u16(udp+6,checksum ? checksum:65535);
    return HUB_PUBLIC_DECLINE_SIZE;
}
/* The caller supplies a fresh nonzero transaction ID and an actual grant
 * owned by this client. A full queued send is not server acknowledgement. */
static int hub_public_dhcp_decline_send(const struct hub_public_arp *transport,
        const struct hub_public_acd *acd,const struct hub_public_dhcp_event *grant,const unsigned char xid[4]) {
    unsigned char packet[HUB_PUBLIC_DECLINE_SIZE];
    if (!transport || transport->fd<0 || transport->index<=0 || !acd || !grant || acd->state!=HUB_ACD_CONFLICT ||
            memcmp(acd->mac,transport->mac,6) || memcmp(acd->address,&grant->address.s_addr,4) ||
            hub_public_dhcp_decline_packet(packet,sizeof packet,grant,transport->mac,xid)<0) { errno=EINVAL;return -1; }
    int fd=socket(AF_PACKET,SOCK_DGRAM|SOCK_NONBLOCK|SOCK_CLOEXEC,htons(ETH_P_IP));
    if (fd<0) return -1;
    struct ifreq check={.ifr_ifindex=transport->index};
    if (ioctl(fd,SIOCGIFNAME,&check)<0 || strncmp(check.ifr_name,"azpub",5) ||
            ioctl(fd,SIOCGIFHWADDR,&check)<0 || check.ifr_hwaddr.sa_family!=ARPHRD_ETHER ||
            memcmp(check.ifr_hwaddr.sa_data,transport->mac,6)) {
        close(fd);errno=EINVAL;return -1;
    }
    struct sockaddr_ll local={.sll_family=AF_PACKET,.sll_protocol=htons(ETH_P_IP),.sll_ifindex=transport->index};
    struct sockaddr_ll destination=local;destination.sll_halen=6;memset(destination.sll_addr,255,6);
    if (bind(fd,(struct sockaddr *)&local,sizeof local)<0) { int error=errno;close(fd);errno=error;return -1; }
    ssize_t sent=syscall(SYS_sendto,fd,packet,sizeof packet,MSG_DONTWAIT,&destination,sizeof destination);
    int error=errno;close(fd);
    if (sent==(ssize_t)sizeof packet) return 0;
    errno=sent<0 ? error:EIO;return -1;
}
#endif

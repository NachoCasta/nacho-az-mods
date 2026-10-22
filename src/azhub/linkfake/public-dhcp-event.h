/* SPDX-License-Identifier: GPL-3.0-only */
/* Bounded udhcpc event proposals; never apply untrusted DHCP values directly.
 * Parser requires one complete record and an exact caller-owned interface.
 * A proposal is not address ownership: lease lifecycle, ARP acquisition and
 * network/service retirement remain the supervising controller's duties. */
#ifndef HUB_PUBLIC_DHCP_EVENT_H
#define HUB_PUBLIC_DHCP_EVENT_H
#include <arpa/inet.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
enum hub_public_dhcp_kind { HUB_DHCP_BOUND, HUB_DHCP_RENEW, HUB_DHCP_DECONFIG,
    HUB_DHCP_NAK, HUB_DHCP_LEASEFAIL, HUB_DHCP_INVALID };
struct hub_public_dhcp_event {
    enum hub_public_dhcp_kind kind;
    struct in_addr address,mask,broadcast,server;
    uint32_t lease_seconds;
};
static int hub_public_dhcp_unicast(struct in_addr address) {
    uint32_t value=ntohl(address.s_addr);
    return value>=0x01000000u && (value>>24)!=127 && value<0xe0000000u;
}
static int hub_public_dhcp_event_parse(const void *input,size_t size,const char *interface,
        struct hub_public_dhcp_event *out) {
    if (!input || !interface || !out || size<10 || size>=192 ||
            strnlen(interface,16)>=16 || strncmp(interface,"azpub",5)) return -1;
    const char *bytes=input;
    if (bytes[size-1]!='\n' || memchr(bytes,0,size) || memchr(bytes,'\n',size-1)) return -1;
    char text[192];memcpy(text,input,size);text[size-1]=0;
    char *fields[7]={text};unsigned count=1;
    for (size_t i=0;i<size-1;++i) {
        if (text[i]=='\t') {
            if (count==7) return -1;
            text[i]=0;fields[count++]=text+i+1;
        }
    }
    if (count!=7 || strcmp(fields[0],"1") || strcmp(fields[2],interface)) return -1;
    static const char *const names[]={"bound","renew","deconfig","nak","leasefail","invalid"};
    unsigned kind=0;while (kind<6 && strcmp(fields[1],names[kind])) ++kind;
    if (kind==6) return -1;
    struct hub_public_dhcp_event result={.kind=(enum hub_public_dhcp_kind)kind};
    if (kind>=HUB_DHCP_DECONFIG) {
        for (unsigned i=3;i<7;++i) if (strcmp(fields[i],"-")) return -1;
        *out=result;return 0;
    }
    if (inet_pton(AF_INET,fields[3],&result.address)!=1 ||
            inet_pton(AF_INET,fields[4],&result.mask)!=1 ||
            inet_pton(AF_INET,fields[6],&result.server)!=1 ||
            !hub_public_dhcp_unicast(result.address) || !hub_public_dhcp_unicast(result.server)) return -1;
    uint32_t mask=ntohl(result.mask.s_addr),inverse=~mask,value=ntohl(result.address.s_addr);
    if (!mask || inverse<3 || (inverse&(inverse+1)) || !(value&inverse) ||
            (value&inverse)==inverse) return -1;
    result.broadcast.s_addr=htonl(value|inverse);
    const char *digits=fields[5];uint32_t lease=0;
    if (!*digits) return -1;
    for (;*digits;++digits) {
        if (*digits<'0' || *digits>'9' || lease>(UINT32_MAX-(unsigned)(*digits-'0'))/10) return -1;
        lease=lease*10+(unsigned)(*digits-'0');
    }
    if (!lease) return -1;
    /* BusyBox1.27.2 caps its ACK timer at2147483 seconds. Never extend beyond
     * that timer or a shorter server-granted lifetime, even if the client
     * rounds very short leases up internally. */
    result.lease_seconds=lease>2147483 ? 2147483 : lease;
    *out=result;return 0;
}
#endif

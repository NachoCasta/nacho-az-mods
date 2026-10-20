/* SPDX-License-Identifier: GPL-3.0-only */
/* Emulator-only IP_PKTINFO diagnostic, port 50009 (not native Link ports).
 * Sends one local marker, receives that and one isolated external marker.
 * Emits metadata only. No firmware hooks or device configuration changes.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include "link-udp-policy.h"
int main(void) {
    struct utsname kernel;
    if (uname(&kernel) || strcmp(kernel.release,"6.6.138-dirty")) return 10;
    int fd=socket(AF_INET,SOCK_DGRAM|SOCK_CLOEXEC,0),on=1;
    struct timeval timeout={.tv_sec=20};
    if (fd<0 || setsockopt(fd,IPPROTO_IP,IP_PKTINFO,&on,sizeof on) ||
        setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof timeout)) return 11;
    struct sockaddr_in destination={0};destination.sin_family=AF_INET;destination.sin_port=htons(50009);
    if (bind(fd,(void *)&destination,sizeof destination)) return 12;
    uint8_t marker[54]={0};memcpy(marker,"Qspt1WmJOL",10);
    marker[10]=6;marker[0x24]=7;marker[0x2c]=127;marker[0x2d]=77;marker[0x2f]=7;
    destination.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    if (sendto(fd,marker,sizeof marker,0,(void *)&destination,sizeof destination)!=sizeof marker) return 13;
    printf("{\"event\":\"ready\",\"port\":50009,\"loopback_ifindex\":%u}\n",if_nametoindex("lo"));fflush(stdout);
    int local=0,external=0;
    for (int count=0;count<2;count++) {
        uint8_t data[54];struct sockaddr_in source={0};
        union { struct cmsghdr align; unsigned char bytes[CMSG_SPACE(sizeof(struct in_pktinfo))]; } control;
        struct iovec iov={data,sizeof data};
        struct msghdr message={.msg_name=&source,.msg_namelen=sizeof source,.msg_iov=&iov,.msg_iovlen=1,
            .msg_control=control.bytes,.msg_controllen=sizeof control.bytes};
        ssize_t size=recvmsg(fd,&message,0);if(size!=54 || message.msg_flags&(MSG_TRUNC|MSG_CTRUNC)) return 14;
        unsigned ingress=0;
        for(struct cmsghdr *c=CMSG_FIRSTHDR(&message);c;c=CMSG_NXTHDR(&message,c))
            if(c->cmsg_level==IPPROTO_IP && c->cmsg_type==IP_PKTINFO && c->cmsg_len>=CMSG_LEN(sizeof(struct in_pktinfo))) {
                struct in_pktinfo info;memcpy(&info,CMSG_DATA(c),sizeof info);ingress=info.ipi_ifindex;
            }
        if(!ingress) return 15;
        int is_local=ingress==if_nametoindex("lo") && source.sin_addr.s_addr==htonl(INADDR_LOOPBACK);
        /* Only this diagnostic owns the exact local source endpoint. */
        unsigned trusted=is_local && source.sin_port==htons(50009) ? 7:0;
        int rejected=probe_udp_reject(data,(size_t)size,50000,trusted);
        if(is_local) {local++;if(rejected) return 16;}
        else {external++;if(!rejected) return 17;}
        char address[INET_ADDRSTRLEN];inet_ntop(AF_INET,&source.sin_addr,address,sizeof address);
        printf("{\"event\":\"received\",\"source\":\"%s\",\"ingress_ifindex\":%u,\"trusted_member\":%u,\"private_claim_rejected\":%s}\n",
            address,ingress,trusted,rejected ? "true":"false");fflush(stdout);
    }
    close(fd);return local==1 && external==1 ? 0:18;
}

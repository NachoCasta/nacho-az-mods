/* SPDX-License-Identifier: GPL-3.0-only */
/* Research-only passive IPv4/UDP Link signature counter. No packet data logged. */
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
int main(int argc,char **argv) {
    if (argc!=3) return 2;
    int fd=socket(AF_PACKET,SOCK_RAW,htons(ETH_P_ALL));
    if (fd<0) { perror("socket");return 1; }
    struct sockaddr_ll address={.sll_family=AF_PACKET,.sll_protocol=htons(ETH_P_ALL),.sll_ifindex=if_nametoindex(argv[1])};
    if (!address.sll_ifindex || bind(fd,(void *)&address,sizeof address)) { perror("bind");return 1; }
    unsigned frames=0,udp=0,link=0;
    struct timespec start,now; clock_gettime(CLOCK_MONOTONIC,&start);
    int seconds=atoi(argv[2]); if(seconds<1 || seconds>30) return 2;
    for (;;) {
        clock_gettime(CLOCK_MONOTONIC,&now);
        if (now.tv_sec-start.tv_sec>=seconds) break;
        struct pollfd ready={.fd=fd,.events=POLLIN};
        if(poll(&ready,1,100)<=0) continue;
        unsigned char packet[2048]; ssize_t size=recv(fd,packet,sizeof packet,0);
        if(size<14)continue;
        frames++;
        if(packet[12]!=8 || packet[13]!=0 || size<34 || packet[23]!=17)continue;
        size_t payload=14+(packet[14]&15)*4+8;
        if(payload>=(size_t)size)continue;
        udp++;
        if((size_t)size>=payload+10 && !memcmp(packet+payload,"Qspt1WmJOL",10))link++;
    }
    close(fd);printf("interface=%s frames=%u udp=%u link_signature=%u\n",argv[1],frames,udp,link);
    return link ? 3 : 0;
}

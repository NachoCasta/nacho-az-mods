/* SPDX-License-Identifier: GPL-3.0-only */
/* Emulator-only loopback packet timing. Prints metadata, never packet bodies. */
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

static double now(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
    return t.tv_sec+t.tv_nsec/1e9;
}
int main(void) {
    struct utsname u;
    if (uname(&u) || strcmp(u.release,"6.6.138-dirty") || access("/dev/subucom_ctrl",F_OK)) return 2;
    int fd=socket(AF_PACKET,SOCK_DGRAM,htons(ETH_P_IP));
    if (fd<0) { perror("socket"); return 1; }
    struct sockaddr_ll a={.sll_family=AF_PACKET,.sll_protocol=htons(ETH_P_IP),.sll_ifindex=if_nametoindex("lo")};
    if (bind(fd,(void *)&a,sizeof a)) { perror("bind"); return 1; }
    struct timeval tv={.tv_sec=1}; setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof tv);
    setvbuf(stdout,NULL,_IOLBF,0);
    double end=now()+80;
    printf("probe-start %.3f\n",now());
    while (now()<end) {
        unsigned char p[4096]; ssize_t n=recv(fd,p,sizeof p,0);
        if (n<0) { if (errno==EAGAIN || errno==EINTR) continue; perror("recv"); return 1; }
        if (n<28 || (p[0]>>4)!=4 || p[9]!=17) continue;
        unsigned h=(p[0]&15)*4;
        if (n<h+8+44) continue;
        unsigned source=(p[h]<<8)|p[h+1],dest=(p[h+2]<<8)|p[h+3];
        unsigned char *d=p+h+8;
        if ((dest!=50000 && dest!=50002) || memcmp(d,"Qspt1WmJOL",10)) continue;
        unsigned player=dest==50000 ? d[0x24]:d[0x21];
        if (player!=3 && player!=4) continue;
        printf("%.3f player=%u dest=%u source=%u type=%#x slot=%u\n",now(),player,dest,source,d[10],d[0x2b]);
    }
    close(fd); puts("probe-finished"); return 0;
}

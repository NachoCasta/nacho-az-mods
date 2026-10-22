/* SPDX-License-Identifier: GPL-3.0-only */
/* Single-threaded emulator control ONLY. Fork is not a mod integration API.
 * Uses production DHCP/lease/ACD/ARP components; native ip applies/removes only
 * the validated lease on the isolated fixture interface. No music endpoints. */
#ifndef HUB_PUBLIC_ADDRESS_FLOW_PROBE_H
#define HUB_PUBLIC_ADDRESS_FLOW_PROBE_H
#include <ifaddrs.h>
#include <poll.h>
#include "../../linkfake/public-address-arp.h"
#include "../../linkfake/public-dhcp-decline.h"
static int address_probe_decline(const struct hub_public_arp *transport,const struct hub_public_acd *arp,
        const struct hub_public_dhcp_event *grant) {
    unsigned char xid[4];size_t used=0;int fd=open("/dev/urandom",O_RDONLY|O_CLOEXEC);
    if (fd<0) return -1;
    while (used<sizeof xid) {
        ssize_t got=read(fd,xid+used,sizeof xid-used);
        if (got<0 && errno==EINTR) continue;
        if (got<=0) { close(fd);return -1; } used+=(size_t)got;
    }
    close(fd);
    for (unsigned attempt=0;attempt<10;++attempt) {
        if (!hub_public_dhcp_decline_send(transport,arp,grant,xid)) return 0;
        if (errno!=EAGAIN && errno!=EWOULDBLOCK && errno!=EINTR) return -1;
        struct timespec pause={0,5000000};nanosleep(&pause,NULL);
    }
    return -1;
}
#include "public-address-library-probe.h"
static int address_probe_ipv4(const struct hub_public_dhcp_event *event,int present) {
    struct ifaddrs *addresses; if (getifaddrs(&addresses)) return -1;
    unsigned count=0,matching=0;
    for (struct ifaddrs *a=addresses;a;a=a->ifa_next) {
        if (strcmp(a->ifa_name,"azpub0") || !a->ifa_addr || a->ifa_addr->sa_family!=AF_INET) continue;
        ++count;
        if (a->ifa_netmask && ((struct sockaddr_in *)a->ifa_addr)->sin_addr.s_addr==event->address.s_addr &&
                ((struct sockaddr_in *)a->ifa_netmask)->sin_addr.s_addr==event->mask.s_addr) ++matching;
    }
    freeifaddrs(addresses);return present ? (count==1 && matching==1 ? 0:-1) : (count ? -1:0);
}
static int address_probe_ip(const struct hub_public_dhcp_event *event,int add) {
    char address[INET_ADDRSTRLEN],broadcast[INET_ADDRSTRLEN],cidr[32];
    if (!inet_ntop(AF_INET,&event->address,address,sizeof address) ||
            !inet_ntop(AF_INET,&event->broadcast,broadcast,sizeof broadcast)) return -1;
    uint32_t mask=ntohl(event->mask.s_addr);unsigned prefix=0;
    while (mask&0x80000000u) { ++prefix;mask<<=1; }
    if (mask || !prefix) return -1;
    snprintf(cidr,sizeof cidr,"%s/%u",address,prefix);
    char *new_argv[]={"/sbin/ip","addr","add",cidr,"brd",broadcast,"dev","azpub0",NULL};
    char *del_argv[]={"/sbin/ip","addr","del",cidr,"dev","azpub0",NULL};
    char *env[]={"PATH=/usr/sbin:/usr/bin:/sbin:/bin",NULL};pid_t pid;
    if (posix_spawn(&pid,"/sbin/ip",NULL,NULL,add ? new_argv:del_argv,env)) return -1;
    uint64_t deadline=now_ms()+3000;int status;
    for (;;) {
        pid_t got=waitpid(pid,&status,WNOHANG);
        if (got==pid) return WIFEXITED(status) && !WEXITSTATUS(status) ? 0:-1;
        if (got<0 && errno!=EINTR) return -1;
        if (now_ms()>=deadline) {
            kill(pid,SIGKILL);waitpid(pid,&status,0);return -1;
        }
        struct timespec pause={0,1000000};nanosleep(&pause,NULL);
    }
}
static int address_probe_roundtrip(const struct hub_public_dhcp_event *event) {
    int fd=socket(AF_INET,SOCK_DGRAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0);if (fd<0) return -1;
    struct sockaddr_in local={.sin_family=AF_INET,.sin_addr=event->address};
    struct sockaddr_in peer={.sin_family=AF_INET,.sin_port=htons(49000),.sin_addr=event->server};
    int okay=bind(fd,(struct sockaddr *)&local,sizeof local)==0 &&
        connect(fd,(struct sockaddr *)&peer,sizeof peer)==0;
    const char text[]="AZHUB-ADDRESS-READY";
    if (okay) okay=send(fd,text,sizeof text-1,0)==sizeof text-1;
    struct pollfd wait={.fd=fd,.events=POLLIN};
    if (okay) okay=poll(&wait,1,3000)==1 && (wait.revents&POLLIN);
    char reply[64];ssize_t got=okay ? recv(fd,reply,sizeof reply,0):-1;
    close(fd);return got==16 && !memcmp(reply,"AZHUB-ADDRESS-OK",16) ? 0:-1;
}
static int address_probe_child(int namespace_fd,struct hub_public_network_lease lease,int library) {
    if (setns(namespace_fd,CLONE_NEWNET)) return 1;
    if (address_probe_ipv4(&lease.current,0)) return 2;
    struct hub_public_arp transport={.fd=-1};int configured=0,result=3;
    if (hub_public_arp_open(&transport,"azpub0",mac)) return result;
    struct hub_public_acd arp;
    if (hub_public_acd_init(&arp,(const unsigned char *)&lease.current.address.s_addr,mac,now_ms(),500)) goto done;
    uint64_t limit=now_ms()+15000;
    while (now_ms()<limit) {
        uint64_t now=now_ms();
        if (hub_public_lease_poll(&lease,now)<0 || (lease.state!=HUB_LEASE_CANDIDATE && lease.state!=HUB_LEASE_READY)) goto done;
        int conflict=hub_public_arp_receive(&transport,&arp,NULL,0);
        if (conflict<0) goto done;
        if (conflict>0) {
            result=configured ? 11:10;
            fprintf(stderr,"ADDRESS_FLOW: foreign conflict observed before_configuration=%d\n",!configured);
            if (address_probe_decline(&transport,&arp,&lease.current)) result=12;
            else fprintf(stderr,"ADDRESS_FLOW: DHCPDECLINE queued without using conflicting source IP\n");
            goto done;
        }
        enum hub_public_acd_action action=hub_public_acd_poll(&arp,now);
        if (action==HUB_ACD_CLAIM) {
            if (address_probe_ip(&lease.current,1)) goto done;
            configured=1;
            if (address_probe_ipv4(&lease.current,1) ||
                    hub_public_lease_configured(&lease,lease.generation,now_ms(),&arp)) goto done;
            fprintf(stderr,"ADDRESS_FLOW: configured after three probes and quiet period\n");
            if (hub_public_acd_complete(&arp,action,now_ms(),500)) goto done;
        } else if (action==HUB_ACD_PROBE || action==HUB_ACD_ANNOUNCE) {
            if (hub_public_arp_send(&transport,&arp)) {
                if (errno!=EAGAIN && errno!=EWOULDBLOCK && errno!=EINTR) goto done;
            } else {
                if (hub_public_acd_complete(&arp,action,now_ms(),500)) goto done;
                fprintf(stderr,"ADDRESS_FLOW: sent %s\n",action==HUB_ACD_PROBE ? "probe":"announcement");
            }
        }
        if (hub_public_acd_ready(&arp) && lease.state==HUB_LEASE_CANDIDATE) {
            if (hub_public_lease_activate(&lease,lease.generation,now_ms(),&arp)) goto done;
        }
        if (arp.state==HUB_ACD_ACTIVE) {
            if (!hub_public_lease_ready(&lease,now_ms()) || address_probe_roundtrip(&lease.current)) goto done;
            result=library ? address_library_probe(namespace_fd,&lease,&transport,&arp):0;break;
        }
        struct timespec pause={0,5000000};nanosleep(&pause,NULL);
    }
done:
    /* There are no library endpoints in this probe. Its sole UDP request socket
     * is already closed; acknowledge that drain before removing the address. */
    hub_public_lease_retire(&lease);
    if (hub_public_lease_drained(&lease,lease.generation,now_ms())) result=4;
    hub_public_arp_close(&transport);
    if (configured && address_probe_ip(&lease.current,0)) result=5;
    if (address_probe_ipv4(&lease.current,0)) result=5;
    if (hub_public_lease_removed(&lease,lease.generation,now_ms())) result=6;
    if (!result) fprintf(stderr,"ADDRESS_FLOW: ready, UDP roundtrip, drain and exact address removal passed\n");
    return result;
}
static int hub_public_address_flow_probe(int namespace_fd,struct hub_public_network_lease lease,int expect_conflict,int library) {
    /* Test executable is single-threaded, with all DHCP children already reaped. */
    pid_t child=fork();if (child<0) return -1;
    if (!child) _exit(address_probe_child(namespace_fd,lease,library));
    int status;if (waitpid(child,&status,0)!=child) return -1;
    return WIFEXITED(status) && WEXITSTATUS(status)==(expect_conflict ? 10:0) ? 0:-1;
}
#endif

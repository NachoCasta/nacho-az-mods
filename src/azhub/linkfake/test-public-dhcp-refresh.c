/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include "public-dhcp-refresh.h"
static uint64_t clock_ms(void) {
    struct timespec ts;assert(!clock_gettime(CLOCK_MONOTONIC,&ts));return (uint64_t)ts.tv_sec*1000+ts.tv_nsec/1000000;
}
static struct hub_public_network_lease ready(uint64_t now) {
    struct hub_public_dhcp_event offer={.kind=HUB_DHCP_BOUND,.lease_seconds=60,
        .address={inet_addr("192.0.2.3")},.mask={inet_addr("255.255.255.0")},
        .broadcast={inet_addr("192.0.2.255")},.server={inet_addr("192.0.2.2")}};
    struct hub_public_network_lease lease={0};assert(!hub_public_lease_offer(&lease,&offer,now,now));
    lease.state=HUB_LEASE_READY;lease.configured=1;return lease;
}
int main(void) {
    const unsigned char mac[6]={2,0,0,0,0x12,3};
    struct hub_public_network_lease lease=ready(1000);struct hub_public_dhcp_refresh r={0};
    assert(!hub_public_dhcp_refresh_arm(&r,&lease,2000) && r.next==31000);
    assert(!hub_public_dhcp_refresh_poll(&r,&lease,-1,"azpub0",mac,"/not/launched","/not/launched",30000));
    assert(!r.running && lease.expires==61000);
    assert(!hub_public_dhcp_refresh_poll(&r,&lease,-1,"azpub0",mac,"/not/launched","/not/launched",31000));
    assert(r.next==36000 && r.last_error==EINVAL && lease.state==HUB_LEASE_READY);
    /* Accepted-complete child boundary is synthetic here; real acquisition
     * completion/reaping is covered by the acquisition component controls. */
    r.running=1;r.acquire=(struct hub_public_dhcp_acquire){.fd=-1,.complete=1,.reaped=1,.eof=1,.started=32000,.proposal=lease.current};
    assert(hub_public_dhcp_refresh_poll(&r,&lease,-1,"azpub0",mac,"/not/launched","/not/launched",33000)==1);
    assert(lease.generation==1 && lease.state==HUB_LEASE_READY && lease.configured && lease.expires==92000 && r.next==62000);
    r.running=1;r.acquire.started=34000;r.acquire.proposal.address.s_addr=inet_addr("192.0.2.4");
    assert(hub_public_dhcp_refresh_poll(&r,&lease,-1,"azpub0",mac,"/not/launched","/not/launched",35000)<0 && errno==ENETRESET);
    assert(lease.state==HUB_LEASE_DRAINING && lease.current.address.s_addr==inet_addr("192.0.2.3") && lease.configured);
    assert(!hub_public_dhcp_refresh_stop(&r,35000));
    lease=ready(1000);memset(&r,0,sizeof r);assert(!hub_public_dhcp_refresh_arm(&r,&lease,2000));
    r.running=1;r.acquire=(struct hub_public_dhcp_acquire){.fd=-1,.complete=1,.reaped=1,.eof=1,.started=60000,.proposal=lease.current};
    assert(hub_public_dhcp_refresh_poll(&r,&lease,-1,"azpub0",mac,"/not/launched","/not/launched",61000)<0);
    assert(lease.expires==61000 && lease.state==HUB_LEASE_DRAINING && !hub_public_dhcp_refresh_stop(&r,61000));
    lease=ready(1000);memset(&r,0,sizeof r);assert(!hub_public_dhcp_refresh_arm(&r,&lease,2000));
    assert(hub_public_dhcp_refresh_poll(&r,&lease,-1,"azpub0",mac,"/not/launched","/not/launched",1999)<0 && errno==ERANGE);
    /* A completed failed refresh must preserve the old grant only until its
     * original expiry. Retry scheduling must not extend ownership or affect
     * an independent source, including when the backoff reaches expiry. */
    lease=ready(1000);memset(&r,0,sizeof r);assert(!hub_public_dhcp_refresh_arm(&r,&lease,2000));
    struct hub_public_network_lease neighbor=ready(2000),saved_neighbor=neighbor;
    r.running=1;r.acquire=(struct hub_public_dhcp_acquire){.fd=-1,.complete=1,.reaped=1,.eof=1,.error=ETIMEDOUT};
    assert(!hub_public_dhcp_refresh_poll(&r,&lease,-1,"azpub0",mac,"/not/launched","/not/launched",58000));
    assert(!r.running && r.last_error==ETIMEDOUT && r.next==61000);
    assert(lease.expires==61000 && lease.generation==1 && hub_public_lease_ready(&lease,60999));
    assert(!hub_public_dhcp_refresh_poll(&r,&lease,-1,"azpub0",mac,"/not/launched","/not/launched",60999));
    assert(hub_public_dhcp_refresh_poll(&r,&lease,-1,"azpub0",mac,"/not/launched","/not/launched",61000)<0 && errno==EADDRNOTAVAIL);
    assert(!r.running && lease.state==HUB_LEASE_DRAINING && lease.expires==61000);
    assert(!memcmp(&neighbor,&saved_neighbor,sizeof neighbor) && hub_public_lease_ready(&neighbor,61000));
    assert(!hub_public_dhcp_refresh_stop(&r,61000));
    /* Periodic DHCP attempts during explicit IPv4LL ownership cannot extend a
     * synthetic lease or apply an ACK over the live link-local address. */
    memset(&lease,0,sizeof lease);memset(&r,0,sizeof r);
    assert(!hub_public_lease_link_local(&lease,(struct in_addr){inet_addr("169.254.2.3")},1000));
    lease.state=HUB_LEASE_READY;lease.configured=1;
    assert(!hub_public_dhcp_refresh_arm(&r,&lease,2000) && r.next==62000 && !lease.seen_grant);
    r.running=1;r.acquire=(struct hub_public_dhcp_acquire){.fd=-1,.complete=1,.reaped=1,.eof=1,.error=ENODATA};
    assert(!hub_public_dhcp_refresh_poll(&r,&lease,-1,"azpub0",mac,"/not/launched","/not/launched",62000));
    assert(r.next==122000 && lease.origin==HUB_ADDRESS_LINK_LOCAL && lease.state==HUB_LEASE_READY);
    struct hub_public_network_lease offered=ready(1000);
    r.running=1;r.acquire=(struct hub_public_dhcp_acquire){.fd=-1,.complete=1,.reaped=1,.eof=1,.started=122000,.proposal=offered.current};
    assert(hub_public_dhcp_refresh_poll(&r,&lease,-1,"azpub0",mac,"/not/launched","/not/launched",122001)<0 && errno==ENETRESET);
    assert(lease.state==HUB_LEASE_DRAINING && lease.current.address.s_addr==inet_addr("169.254.2.3") && !lease.seen_grant);
    /* Real supervised child cancellation, without namespace/network mutation. */
    char directory[]="/tmp/azhub-refresh-XXXXXX";assert(mkdtemp(directory));char helper[256];
    snprintf(helper,sizeof helper,"%s/helper",directory);int fd=open(helper,O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,0700);assert(fd>=0);
    const char body[]="#!/bin/sh\nexec /bin/sleep 30\n";assert(write(fd,body,sizeof body-1)==sizeof body-1 && !close(fd));
    uint64_t now=clock_ms();lease=ready(now);memset(&r,0,sizeof r);assert(!hub_public_dhcp_refresh_arm(&r,&lease,now));r.next=now;
    int ns=open("/proc/self/ns/net",O_RDONLY|O_CLOEXEC);assert(ns>=0);
    assert(!hub_public_dhcp_refresh_poll(&r,&lease,ns,"azpub0",mac,helper,helper,now) && r.running && r.acquire.pid>0);
    pid_t child=r.acquire.pid;int result;uint64_t deadline=clock_ms()+5000;
    while ((result=hub_public_dhcp_refresh_stop(&r,clock_ms()))==1) { assert(clock_ms()<deadline);struct timespec pause={0,1000000};nanosleep(&pause,NULL); }
    assert(!result && !r.running && r.acquire.complete && r.acquire.reaped && r.acquire.eof && r.acquire.fd==-1);
    assert(waitpid(child,NULL,WNOHANG)<0 && errno==ECHILD);
    close(ns);assert(!unlink(helper) && !rmdir(directory));
    puts("PASS: half-life scheduling, bounded retry, unchanged-generation refresh, changed/late ACK retirement, backward clock and real child cancellation/reaping");
}

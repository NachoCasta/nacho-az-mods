/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include "public-address-arp.h"
static int conflict(struct hub_public_arp *arp,struct hub_public_acd *acd) {
    for (unsigned i=0;i<100;++i) {
        int result=hub_public_arp_receive(arp,acd,NULL,0);assert(result>=0);
        if (result) return result;
        struct timespec pause={0,1000000};nanosleep(&pause,NULL);
    }
    return 0;
}
int main(int argc,char **argv) {
    /* Explicit isolated fixture with two veth endpoints; no real-LAN test. */
    assert(argc==2 && !strcmp(argv[1],"--isolated-veth"));
    const unsigned char a_mac[6]={2,0,0,0,0x33,1},b_mac[6]={2,0,0,0,0x33,2};
    const unsigned char ip[4]={192,0,2,33};
    struct hub_public_arp a={.fd=-1},b={.fd=-1},failed={.fd=-1};
    assert(hub_public_arp_open(&failed,"lo",a_mac)<0);
    assert(hub_public_arp_open(&failed,"azpubt0",b_mac)<0 && failed.fd==-1);
    assert(!hub_public_arp_open(&a,"azpubt0",a_mac));
    assert(!hub_public_arp_open(&b,"azpubt1",b_mac));
    struct hub_public_acd aa,bb;
    assert(!hub_public_acd_init(&aa,ip,a_mac,0,0));
    assert(!hub_public_acd_init(&bb,ip,b_mac,0,0));
    assert(hub_public_acd_poll(&aa,0)==HUB_ACD_PROBE);
    assert(!hub_public_arp_send(&a,&aa));
    assert(aa.probes==0); /* adapter must not commit caller state */
    assert(conflict(&b,&bb) && bb.state==HUB_ACD_CONFLICT);
    assert(!hub_public_acd_complete(&aa,HUB_ACD_PROBE,0,0));
    assert(!hub_public_acd_init(&bb,ip,b_mac,0,0));
    /* Foreign announcement conflicts with our in-progress candidate. */
    bb.state=HUB_ACD_ANNOUNCING;bb.pending=HUB_ACD_ANNOUNCE;
    assert(!hub_public_arp_send(&b,&bb));assert(conflict(&a,&aa));
    assert(!hub_public_acd_ready(&aa));
    /* Conflict monitoring must continue after acquisition, not merely probe. */
    assert(!hub_public_acd_init(&aa,ip,a_mac,0,0));aa.state=HUB_ACD_ACTIVE;
    assert(hub_public_acd_ready(&aa));
    assert(!hub_public_arp_send(&b,&bb));assert(conflict(&a,&aa));
    assert(!hub_public_acd_ready(&aa));
    hub_public_arp_close(&a);hub_public_arp_close(&b);hub_public_arp_close(&failed);
    assert(a.fd==-1 && b.fd==-1);
    puts("PASS: isolated live Ethernet ARP probes/claims, MAC validation, nonblocking receipt and conflict retirement");
    return 0;
}

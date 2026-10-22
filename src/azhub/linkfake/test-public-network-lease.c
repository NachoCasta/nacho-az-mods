/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "public-network-lease.h"
static struct hub_public_dhcp_event offer(unsigned host,unsigned seconds) {
    char line[160];int n=snprintf(line,sizeof line,
        "1\tbound\tazpub0\t192.0.2.%u\t255.255.255.0\t%u\t192.0.2.2\n",host,seconds);
    struct hub_public_dhcp_event event;
    assert(!hub_public_dhcp_event_parse(line,(size_t)n,"azpub0",&event));return event;
}
static void activate(struct hub_public_network_lease *lease,uint64_t now) {
    struct hub_public_acd proof={.state=HUB_ACD_CLAIMING,.pending=HUB_ACD_CLAIM};
    memcpy(proof.address,&lease->current.address.s_addr,4);
    assert(!hub_public_lease_ready(lease,now));
    assert(hub_public_lease_activate(lease,lease->generation,now,&proof)<0);
    assert(!hub_public_lease_configured(lease,lease->generation,now,&proof));
    proof.state=HUB_ACD_ANNOUNCING;proof.pending=HUB_ACD_ANNOUNCE;
    assert(hub_public_lease_activate(lease,lease->generation,now,&proof)<0);
    proof.announcements=1;proof.address[3]^=1;
    assert(hub_public_lease_activate(lease,lease->generation,now,&proof)<0);
    proof.address[3]^=1;
    assert(!hub_public_lease_activate(lease,lease->generation,now,&proof));
    assert(hub_public_lease_ready(lease,now));
}
int main(void) {
    struct hub_public_network_lease a={0},b={0};
    struct hub_public_dhcp_event first=offer(3,60),second=offer(4,60);
    assert(!hub_public_lease_offer(&a,&first,100,200));
    assert(a.generation==1 && a.state==HUB_LEASE_CANDIDATE && a.expires==60100);
    activate(&a,300);
    assert(!hub_public_lease_offer(&b,&second,100,300));activate(&b,300);
    uint64_t old=a.generation;
    /* Renewal retains endpoints/generation; source time, not receipt, sets TTL. */
    first.kind=HUB_DHCP_RENEW;
    assert(!hub_public_lease_offer(&a,&first,30000,30500));
    assert(a.generation==old && a.expires==90000 && hub_public_lease_ready(&a,30500));
    assert(hub_public_lease_offer(&a,&first,30000,31000)<0); /* replay */
    assert(hub_public_lease_offer(&a,&first,92000,91000)<0); /* future grant */
    assert(a.state==HUB_LEASE_DRAINING); /* old TTL elapsed before bad event */
    assert(a.configured && !hub_public_lease_ready(&a,91000));
    assert(hub_public_lease_removed(&a,old,91000)<0); /* must drain first */
    assert(!hub_public_lease_drained(&a,old,91000));
    assert(a.configured && a.state==HUB_LEASE_REMOVING);
    assert(!hub_public_lease_removed(&a,old,91000));assert(a.state==HUB_LEASE_IDLE && !a.configured);
    /* New address cannot bypass draining/removal, even when queued repeatedly. */
    a=(struct hub_public_network_lease){0};first=offer(3,60);
    assert(!hub_public_lease_offer(&a,&first,0,0));activate(&a,0);old=a.generation;
    assert(!hub_public_lease_offer(&a,&second,1000,1000));
    assert(a.state==HUB_LEASE_DRAINING && a.configured && a.current.address.s_addr==first.address.s_addr);
    assert(!hub_public_lease_ready(&a,1000));
    assert(hub_public_lease_configured(&a,old,1000,NULL)<0);
    assert(!hub_public_lease_drained(&a,old,1000));
    assert(!hub_public_lease_offer(&a,&first,2000,2000));
    assert(a.state==HUB_LEASE_REMOVING && a.queued);
    assert(!hub_public_lease_removed(&a,old,2000));
    assert(a.generation==old+1 && a.state==HUB_LEASE_CANDIDATE && !a.configured);
    assert(hub_public_lease_activate(&a,old,2000,NULL)<0);
    activate(&a,2000);
    /* A different source remains unaffected by another source's retirement. */
    assert(hub_public_lease_ready(&b,2000) && b.generation==1);
    struct hub_public_dhcp_event lost={.kind=HUB_DHCP_DECONFIG};
    assert(!hub_public_lease_offer(&a,&lost,0,3000));
    assert(!hub_public_lease_ready(&a,3000) && hub_public_lease_ready(&b,3000));
    /* Expired queued candidate is discarded; it must never be republished. */
    second=offer(4,1);assert(!hub_public_lease_offer(&a,&second,4000,4000));
    old=a.generation;
    assert(!hub_public_lease_drained(&a,old,5000));
    assert(!hub_public_lease_removed(&a,old,5000));assert(a.state==HUB_LEASE_IDLE && !a.queued);
    /* Delayed grants cannot acquire a fresh full TTL on delivery. */
    assert(hub_public_lease_offer(&a,&second,6000,7000)<0);
    assert(a.state==HUB_LEASE_IDLE);
    assert(!hub_public_lease_offer(&a,&first,8000,8000));
    assert(hub_public_lease_ready(&a,7999)==0 && a.failed);
    assert(a.state==HUB_LEASE_DRAINING);
    assert(!hub_public_lease_drained(&a,a.generation,8000));
    assert(!hub_public_lease_removed(&a,a.generation,8000));
    assert(hub_public_lease_offer(&a,&first,9000,9000)<0);
    a=(struct hub_public_network_lease){.generation=UINT64_MAX};
    assert(hub_public_lease_offer(&a,&first,100,100)<0 && a.failed && a.state==HUB_LEASE_IDLE);
    a=(struct hub_public_network_lease){0};
    assert(hub_public_lease_offer(&a,&first,UINT64_MAX-100,UINT64_MAX-100)<0);
    assert(a.state==HUB_LEASE_IDLE);
    assert(!hub_public_lease_ready(NULL,0));
    /* Full ACD state machine, not just the phase-control fixture above. */
    a=(struct hub_public_network_lease){0};first=offer(3,60);
    assert(!hub_public_lease_offer(&a,&first,0,0));
    struct hub_public_acd arp;const unsigned char mac[6]={2,0,0,0,0x35,1};
    assert(!hub_public_acd_init(&arp,(const unsigned char *)&first.address.s_addr,mac,0,0));
    for (unsigned i=0;i<3;++i) {
        assert(hub_public_acd_poll(&arp,i*1000)==HUB_ACD_PROBE);
        assert(!hub_public_acd_complete(&arp,HUB_ACD_PROBE,i*1000,0));
        assert(!hub_public_lease_ready(&a,i*1000));
    }
    assert(hub_public_acd_poll(&arp,4000)==HUB_ACD_CLAIM);
    assert(!hub_public_lease_configured(&a,a.generation,4000,&arp));
    assert(!hub_public_acd_complete(&arp,HUB_ACD_CLAIM,4000,0));
    assert(hub_public_acd_poll(&arp,4000)==HUB_ACD_ANNOUNCE);
    assert(hub_public_lease_activate(&a,a.generation,4000,&arp)<0);
    assert(!hub_public_acd_complete(&arp,HUB_ACD_ANNOUNCE,4000,0));
    assert(!hub_public_lease_activate(&a,a.generation,4000,&arp));
    assert(hub_public_lease_ready(&a,59999));
    assert(!hub_public_lease_ready(&a,60000) && a.state==HUB_LEASE_DRAINING);
    puts("PASS: lease renewal/generations, ARP/configuration publication gate, expiry and drain-before-removal/replacement, delayed records and neighbor isolation");
    return 0;
}

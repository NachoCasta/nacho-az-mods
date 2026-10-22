/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "public-address-conflict.h"
static const unsigned char ip[4]={169,254,22,33},mac[6]={2,1,2,3,4,5};
static void fresh(struct hub_public_acd *a) { assert(!hub_public_acd_init(a,ip,mac,100,500)); }
static void activate(struct hub_public_acd *a) {
    fresh(a);
    const uint64_t times[]={600,1600,3600};
    for (unsigned i=0;i<3;++i) {
        assert(hub_public_acd_poll(a,times[i])==HUB_ACD_PROBE);
        assert(!hub_public_acd_complete(a,HUB_ACD_PROBE,times[i],0));
    }
    assert(hub_public_acd_poll(a,5599)==HUB_ACD_NONE);
    assert(hub_public_acd_poll(a,5600)==HUB_ACD_CLAIM);
    assert(!hub_public_acd_ready(a));
    assert(!hub_public_acd_complete(a,HUB_ACD_CLAIM,5600,0));
    assert(hub_public_acd_poll(a,5600)==HUB_ACD_ANNOUNCE);
    assert(!hub_public_acd_ready(a));
    unsigned char p[28];assert(!hub_public_acd_packet(a,p));
    assert(!memcmp(p+14,ip,4) && !memcmp(p+24,ip,4));
    assert(!hub_public_acd_complete(a,HUB_ACD_ANNOUNCE,5600,0));
    assert(hub_public_acd_ready(a));
    assert(hub_public_acd_poll(a,7599)==HUB_ACD_NONE);
    assert(hub_public_acd_poll(a,7600)==HUB_ACD_ANNOUNCE);
    assert(!hub_public_acd_complete(a,HUB_ACD_ANNOUNCE,7600,0));
    assert(a->state==HUB_ACD_ACTIVE && hub_public_acd_poll(a,99999)==HUB_ACD_NONE);
}
int main(void) {
    struct hub_public_acd_retry retry={0},neighbor={0};
    for (unsigned i=0;i<10;++i) {
        assert(!hub_public_acd_retry_begin(&retry,i*2));
        assert(!hub_public_acd_retry_conflict(&retry,i*2+1));
    }
    assert(retry.conflicts==10);
    assert(hub_public_acd_retry_begin(&retry,60018)<0);
    assert(!hub_public_acd_retry_begin(&retry,60019));
    assert(hub_public_acd_retry_begin(&retry,60020)<0);
    assert(!hub_public_acd_retry_begin(&neighbor,60020));
    assert(!hub_public_acd_retry_conflict(&retry,60021));
    assert(hub_public_acd_retry_begin(&retry,120020)<0);
    assert(!hub_public_acd_retry_begin(&retry,120021));
    assert(hub_public_acd_retry_conflict(&retry,120020)<0);
    assert(hub_public_acd_retry_begin(&neighbor,UINT64_MAX)<0 && neighbor.exhausted);
    struct hub_public_acd_retry declined={0};
    assert(!hub_public_acd_retry_begin(&declined,0));
    assert(!hub_public_acd_retry_declined(&declined,100));
    assert(declined.conflicts==1 && hub_public_acd_retry_begin(&declined,10099)<0);
    assert(!hub_public_acd_retry_begin(&declined,10100));
    declined.conflicts=9;
    assert(!hub_public_acd_retry_declined(&declined,10200));
    assert(declined.conflicts==10 && hub_public_acd_retry_begin(&declined,70199)<0);
    assert(!hub_public_acd_retry_begin(&declined,70200));
    struct hub_public_acd a;
    fresh(&a);assert(!hub_public_acd_ready(&a));
    assert(hub_public_acd_poll(&a,599)==HUB_ACD_NONE);
    assert(hub_public_acd_poll(&a,600)==HUB_ACD_PROBE);
    unsigned char packet[28];assert(!hub_public_acd_packet(&a,packet));
    assert(!memcmp(packet+14,"\0\0\0\0",4) && !memcmp(packet+24,ip,4));
    assert(!memcmp(packet+8,mac,6));
    /* No state advance merely because send is delayed or fails. */
    assert(hub_public_acd_poll(&a,10000)==HUB_ACD_PROBE && a.probes==0);
    assert(hub_public_acd_complete(&a,HUB_ACD_ANNOUNCE,10000,0)<0);
    assert(hub_public_acd_complete(&a,HUB_ACD_PROBE,9999,0)<0);
    assert(!hub_public_acd_complete(&a,HUB_ACD_PROBE,10000,1000));
    assert(hub_public_acd_poll(&a,11999)==HUB_ACD_NONE);
    assert(hub_public_acd_poll(&a,12000)==HUB_ACD_PROBE);
    /* Exact ARP layout; all truncated/header-corrupt packets are ignored. */
    packet[8]=4;
    for (size_t n=0;n<28;++n) { fresh(&a);assert(!hub_public_acd_observe(&a,packet,n,NULL,0)); }
    for (unsigned i=0;i<8;++i) {
        unsigned char bad[28];memcpy(bad,packet,28);bad[i]^=0x40;
        fresh(&a);assert(!hub_public_acd_observe(&a,bad,28,NULL,0));
    }
    fresh(&a);assert(hub_public_acd_observe(&a,packet,28,NULL,0));
    assert(a.state==HUB_ACD_CONFLICT && !hub_public_acd_ready(&a));
    assert(hub_public_acd_poll(&a,99999)==HUB_ACD_NONE);
    /* Echo from any local interface is ignored, foreign parallel probe wins. */
    const unsigned char inventory[2][6]={{4,1,2,3,4,5},{2,9,8,7,6,5}};
    fresh(&a);assert(!hub_public_acd_observe(&a,packet,28,inventory,2));
    packet[8]=6;assert(hub_public_acd_observe(&a,packet,28,inventory,2));
    /* A foreign claimed address conflicts even if its target differs. */
    fresh(&a);memcpy(packet+14,ip,4);packet[24]=192;packet[7]=2;
    assert(hub_public_acd_observe(&a,packet,28,NULL,0));
    activate(&a);assert(hub_public_acd_observe(&a,packet,28,NULL,0));
    assert(!hub_public_acd_ready(&a));
    /* Established owners ignore ordinary queries/probes and keep serving. */
    activate(&a);memset(packet+14,0,4);memcpy(packet+24,ip,4);packet[7]=1;
    assert(!hub_public_acd_observe(&a,packet,28,NULL,0) && hub_public_acd_ready(&a));
    hub_public_acd_stop(&a);assert(!hub_public_acd_ready(&a));
    assert(hub_public_acd_poll(&a,99999)==HUB_ACD_NONE);
    assert(hub_public_acd_packet(&a,packet)<0);
    unsigned char invalid[4]={127,0,0,1};
    assert(hub_public_acd_init(&a,invalid,mac,0,0)<0);
    assert(hub_public_acd_init(&a,ip,mac,0,1001)<0);
    assert(hub_public_acd_init(NULL,ip,mac,0,0)<0);
    assert(hub_public_acd_init(&a,ip,mac,UINT64_MAX,0)<0);
    fresh(&a);assert(hub_public_acd_poll(&a,UINT64_MAX)==HUB_ACD_PROBE);
    assert(hub_public_acd_complete(&a,HUB_ACD_PROBE,UINT64_MAX,0)<0);
    assert(a.state==HUB_ACD_STOPPED && hub_public_acd_poll(&a,UINT64_MAX)==HUB_ACD_NONE);
    puts("PASS: ARP candidate timing/send completion, simultaneous/ongoing conflicts, local echoes, bounded parsing and retirement");
    return 0;
}

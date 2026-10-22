/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include "public-provider-state.h"
struct capture {
    unsigned count,kind,port;
    uint32_t peer;
    int outcome;
};
static int send_packet(void *opaque,uint32_t peer,unsigned port,const void *data,size_t length) {
    struct capture *capture=opaque; const unsigned char *bytes=data;
    assert(length>=37 && length<=296 && !memcmp(bytes,"Qspt1WmJOL",10));
    if (capture->outcome<=0) return capture->outcome;
    ++capture->count; capture->kind=bytes[10]; capture->peer=peer; capture->port=port; return 1;
}
int main(void) {
    struct hub_public_provider_identity identity;
    const unsigned char ip[]={192,0,2,2},mac[]={2,0,0,0,17,41};
    assert(!hub_public_provider_identity_init(&identity,41,ip,mac,"Hub USB1"));
    struct hub_public_provider_state state; struct capture capture={.outcome=1};
    hub_public_provider_start(&state,&identity,0,send_packet,&capture);
    const unsigned kinds[]={10,0,0,0,2,2,2,4,4,4};
    for (unsigned i=0;i<10;++i) {
        assert(!hub_public_provider_tick(&state,i*300));
        assert(capture.count==i+1 && capture.kind==kinds[i] && !capture.peer && capture.port==50000);
        assert(!hub_public_provider_tick(&state,i*300+1) && capture.count==i+1);
    }
    assert(!hub_public_provider_tick(&state,3000) && capture.count==12 && capture.kind==6);
    unsigned char request[48]={0}; memcpy(request,"Qspt1WmJOL",10); request[10]=0x46;
    assert(hub_public_provider_receive(&state,1,request,48,1,3100)==1 && capture.kind==0x16);
    assert(hub_public_provider_receive(&state,2,request,48,1,3200)==1 && capture.kind==0x16);
    assert(hub_public_provider_receive(&state,1,request,48,1,3300)==1 && capture.kind==0x47);
    assert(state.peers[0].repeat_pending && state.peers[1].repeat_pending);
    assert(!hub_public_provider_tick(&state,6100) && !state.peers[0].repeat_pending && state.peers[1].repeat_pending);
    assert(!hub_public_provider_tick(&state,6200) && capture.peer==2 && capture.kind==0x16);
    for (uint32_t peer=3;peer<=8;++peer) assert(hub_public_provider_receive(&state,peer,request,48,1,6300)==1);
    assert(!hub_public_provider_receive(&state,9,request,48,1,6300));
    assert(hub_public_provider_receive(&state,1,request,48,1,70000)==1 && capture.kind==0x16);
    capture.outcome=0;
    assert(!hub_public_provider_receive(&state,2,request,48,1,70100));
    assert(!state.peers[1].address);
    unsigned count=capture.count; hub_public_provider_withdraw(&state);
    assert(!hub_public_provider_tick(&state,80000));
    assert(!hub_public_provider_receive(&state,1,request,48,1,80000) && capture.count==count);
    hub_public_provider_start(&state,&identity,90000,send_packet,&capture);
    assert(!hub_public_provider_tick(&state,90000) && state.phase==0);
    capture.outcome=1; assert(!hub_public_provider_tick(&state,90300) && state.phase==1);
    capture.outcome=-1; assert(hub_public_provider_tick(&state,90600)<0 && state.phase==1);
    puts("PASS: bounded discovery timing, independent peer activation, delayed replies, capacity, expiry, backpressure and withdrawal");
    return 0;
}

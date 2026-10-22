/* SPDX-License-Identifier: GPL-3.0-only */
/* Single-owner discovery timing and per-peer activation. The transport must
 * admit peers before receive and bind sends to the source's own endpoint.
 * No endpoint, peer context or queued send may outlive this source generation. */
#ifndef HUB_PUBLIC_PROVIDER_STATE_H
#define HUB_PUBLIC_PROVIDER_STATE_H
#include "public-provider-packets.h"
#define HUB_PUBLIC_PROVIDER_PEERS 8
/* peer0 means the endpoint's configured broadcast, never INADDR_ANY.
 * Return1 sent,0 backpressure,-1 transport failure. Never retain the bytes. */
typedef int (*hub_public_provider_send)(void *,uint32_t,unsigned,const void *,size_t);
struct hub_public_provider_peer {
    uint32_t address;
    uint64_t last_seen,repeat_at;
    int activated,repeat_pending;
};
struct hub_public_provider_state {
    struct hub_public_provider_identity identity;
    struct hub_public_provider_peer peers[HUB_PUBLIC_PROVIDER_PEERS];
    unsigned phase;
    uint64_t next_identity,next_alive;
    int active;
    hub_public_provider_send send;
    void *context;
};
static void hub_public_provider_start(struct hub_public_provider_state *state,
        const struct hub_public_provider_identity *identity,uint64_t now,
        hub_public_provider_send send,void *context) {
    memset(state,0,sizeof *state); state->identity=*identity;
    state->active=1; state->next_identity=now; state->send=send; state->context=context;
}
static void hub_public_provider_withdraw(struct hub_public_provider_state *state) {
    state->active=0; memset(state->peers,0,sizeof state->peers);
}
static int hub_public_provider_tick(struct hub_public_provider_state *state,uint64_t now) {
    if (!state->active || !state->send) return 0;
    unsigned char packet[72]; size_t size; int result;
    if (now>=state->next_identity) {
        unsigned kind=state->phase==0 ? 10 : state->phase<=3 ? 0 : state->phase<=6 ? 2 : state->phase<=9 ? 4 : 2;
        unsigned sequence=state->phase<=3 ? state->phase : state->phase<=6 ? state->phase-3 : state->phase<=9 ? state->phase-6 : 6;
        size=hub_public_provider_discovery(&state->identity,kind,sequence,packet);
        result=state->send(state->context,0,50000,packet,size);
        if (result<0) return -1;
        state->next_identity=now+(state->phase<10 ? 300 : 1000);
        if (result && state->phase<10) {
            ++state->phase;
            if (state->phase==10) state->next_alive=now+300;
        }
    }
    if (state->phase==10 && now>=state->next_alive) {
        size=hub_public_provider_discovery(&state->identity,6,0,packet);
        if (state->send(state->context,0,50000,packet,size)<0) return -1;
        state->next_alive=now+3000;
    }
    for (unsigned i=0;i<HUB_PUBLIC_PROVIDER_PEERS;++i) {
        struct hub_public_provider_peer *peer=state->peers+i;
        if (!peer->address) continue;
        if (now-peer->last_seen>=60000) { memset(peer,0,sizeof *peer); continue; }
        if (peer->repeat_pending && now>=peer->repeat_at) {
            size=hub_public_provider_activation(state->identity.number,0x16,packet);
            result=state->send(state->context,peer->address,50002,packet,size);
            if (result<0) return -1;
            if (result) peer->repeat_pending=0;
            else peer->repeat_at=now+100;
        }
    }
    return 0;
}
/* Only admitted complete status datagrams reach this function. Unknown packets
 * cannot consume a peer slot or advance another peer's activation state. */
static int hub_public_provider_receive(struct hub_public_provider_state *state,uint32_t address,
        const unsigned char *request,size_t length,uint32_t tracks,uint64_t now) {
    if (!state->active || !state->send || !address || !request || length<36 ||
        memcmp(request,"Qspt1WmJOL",10)) return 0;
    unsigned char packet[296]; size_t size;
    if (request[10]!=0x46) {
        size=hub_public_provider_status(&state->identity,request,length,tracks,packet);
        return size ? state->send(state->context,address,50002,packet,size) : 0;
    }
    struct hub_public_provider_peer *peer=NULL,*available=NULL;
    for (unsigned i=0;i<HUB_PUBLIC_PROVIDER_PEERS;++i) {
        struct hub_public_provider_peer *item=state->peers+i;
        if (item->address && now-item->last_seen>=60000) memset(item,0,sizeof *item);
        if (item->address==address) peer=item;
        else if (!item->address && !available) available=item;
    }
    if (!peer) peer=available;
    if (!peer) return 0;
    size=hub_public_provider_activation(state->identity.number,peer->activated ? 0x47 : 0x16,packet);
    int result=state->send(state->context,address,50002,packet,size);
    if (result<=0) return result;
    if (!peer->activated) { peer->repeat_pending=1; peer->repeat_at=now+3000; }
    peer->address=address; peer->last_seen=now; peer->activated=1;
    return 1;
}
#endif

/* SPDX-License-Identifier: GPL-3.0-only */
/* Controller-side identity reconciliation. Authenticated client state is the
 * retirement authority; unavailable capacity is local to this source. */
#ifndef HUB_PUBLIC_PLAYER_RECONCILE_H
#define HUB_PUBLIC_PLAYER_RECONCILE_H
#include "public-player-pool.h"
#include "public-broker-client.h"
struct hub_public_player_reconcile { uint64_t collision_generation[8]; };
/*1 selected number,0 absent/reserved/waiting,-1 invalid ownership. The caller
 * must still reconcile withdrawal when present/reserved says to remove. */
static int hub_public_player_reconcile(struct hub_public_player_reconcile *state,
        struct hub_public_player_pool *pool,const struct hub_public_broker_client *client,
        unsigned slot,int present,int reserved,uint64_t now,unsigned *number) {
    if (!state || !pool || !client || !number || slot>=8 || client->lost) { errno=EINVAL;return -1; }
    const struct hub_public_broker_client_slot *live=client->slots+slot;
    if (!present || reserved) {
        if (live->phase==HUB_BROKER_EMPTY && pool->reserved[slot] &&
                hub_public_player_release(pool,slot,pool->reserved[slot])<0) return -1;
        return 0;
    }
    if (live->wanted && live->phase==HUB_BROKER_EMPTY && live->error==EEXIST &&
            state->collision_generation[slot]!=live->generation) {
        if (!live->generation || hub_public_player_observe(pool,live->desired.player,now)<0) return -1;
        if (pool->reserved[slot] && hub_public_player_release(pool,slot,live->desired.player)<0) return -1;
        state->collision_generation[slot]=live->generation;
    }
    int selected=hub_public_player_reserve(pool,slot,now);
    if (selected<0) return errno==ENOSPC || errno==EEXIST ? 0:-1;
    *number=(unsigned)selected;return 1;
}
#endif

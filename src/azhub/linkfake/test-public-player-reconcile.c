/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include "public-player-reconcile.h"
int main(void) {
    for (unsigned slot=0;slot<8;++slot) {
        struct hub_public_player_pool pool={0};struct hub_public_player_reconcile state={0};
        struct hub_public_broker_client client={0};unsigned number=0;
        struct hub_public_broker_client_slot *live=client.slots+slot;
        live->wanted=1;live->generation=1;live->desired.player=41+slot;live->phase=HUB_BROKER_DRAINING;live->error=EEXIST;
        assert(hub_public_player_reserve(&pool,slot,0)==41+(int)slot);
        assert(hub_public_player_reconcile(&state,&pool,&client,slot,1,0,1,&number)==1 && number==41+slot);
        assert(!state.collision_generation[slot]); /* DRAINING is not retirement. */
        live->phase=HUB_BROKER_EMPTY;
        assert(hub_public_player_reconcile(&state,&pool,&client,slot,1,0,2,&number)==1 && number!=41+slot);
        assert(state.collision_generation[slot]==1 && number>=41 && number<=48);
        unsigned replacement=number;
        assert(hub_public_player_reconcile(&state,&pool,&client,slot,1,0,3,&number)==1 && number==replacement);
        assert(pool.foreign[slot]==2); /* Duplicate EMPTY cannot extend the foreign hold. */
        assert(!hub_public_player_reconcile(&state,&pool,&client,slot,1,1,4,&number) && !pool.reserved[slot]);
    }
    struct hub_public_player_pool pool={0};struct hub_public_player_reconcile state={0};struct hub_public_broker_client client={0};unsigned number;
    for (unsigned slot=0;slot<8;++slot) {
        assert(hub_public_player_reserve(&pool,slot,0)==41+(int)slot);
        client.slots[slot].phase=HUB_BROKER_ACTIVE;client.slots[slot].generation=1;
        client.slots[slot].wanted=1;client.slots[slot].desired.player=41+slot;
    }
    client.slots[0].phase=HUB_BROKER_EMPTY;client.slots[0].error=EEXIST;
    assert(!hub_public_player_reconcile(&state,&pool,&client,0,1,0,10,&number));
    assert(!hub_public_player_reconcile(&state,&pool,&client,0,1,0,20,&number) && pool.foreign[0]==10);
    for (unsigned slot=1;slot<8;++slot) assert(pool.reserved[slot]==41+slot && client.slots[slot].phase==HUB_BROKER_ACTIVE);
    assert(!hub_public_player_reconcile(&state,&pool,&client,7,0,1,21,&number) && pool.reserved[7]==48);
    client.slots[7].phase=HUB_BROKER_EMPTY;
    assert(!hub_public_player_reconcile(&state,&pool,&client,7,0,1,22,&number) && !pool.reserved[7]);
    assert(hub_public_player_reconcile(&state,&pool,&client,0,1,0,23,&number)==1 && number==48);
    puts("PASS: all-slot retired-generation reselection, duplicate status idempotence, eject precedence, exhaustion isolation and delayed reservation release");
}

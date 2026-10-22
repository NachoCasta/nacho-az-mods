/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "public-broker-generation.h"
int main(void) {
    struct hub_public_broker_generations g={0};
    for (unsigned i=0;i<8;++i) assert(!hub_public_broker_generation_source(&g,i,1));
    assert(hub_public_broker_generation_source(&g,0,1)<0 && errno==ESTALE);
    assert(hub_public_broker_generation_source(&g,0,2)<0 && errno==EBUSY);
    assert(!hub_public_broker_generation_detach(&g,0,1));
    assert(hub_public_broker_generation_ready(&g,0,1)<0);
    assert(hub_public_broker_generation_drained(&g,0,2)<0);
    assert(!hub_public_broker_generation_drained(&g,0,1));
    assert(hub_public_broker_generation_source(&g,0,1)<0);
    assert(!hub_public_broker_generation_source(&g,0,2));
    assert(hub_public_broker_generation_detach(&g,0,1)<0);
    assert(!hub_public_broker_generation_ready(&g,0,2));
    for (unsigned i=1;i<8;++i) assert(g.slots[i].phase==HUB_BROKER_STARTING && g.slots[i].generation==1);
    hub_public_broker_generation_stop(&g);
    assert(hub_public_broker_generation_source(&g,0,3)<0 && errno==ESHUTDOWN);
    for (unsigned i=0;i<8;++i) {
        assert(g.slots[i].phase==HUB_BROKER_DRAINING);
        assert(!hub_public_broker_generation_drained(&g,i,i ? 1:2));
    }
    struct hub_public_broker_generations terminal={0};
    assert(!hub_public_broker_generation_source(&terminal,7,UINT64_MAX));
    assert(!hub_public_broker_generation_detach(&terminal,7,UINT64_MAX));
    assert(!hub_public_broker_generation_drained(&terminal,7,UINT64_MAX));
    assert(hub_public_broker_generation_source(&terminal,7,1)<0 && errno==ESTALE);
    assert(hub_public_broker_generation_source(&g,8,1)<0);
    puts("PASS: consumed generations, late ready/detach rejection, drain-before-replacement, eight-slot isolation and terminal stop");
}

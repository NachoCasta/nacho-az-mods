/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include "public-sharing-inventory.h"
static void status(int peer,const unsigned char token[16],unsigned slot,uint64_t generation,unsigned phase,unsigned error) {
    struct hub_public_broker_frame frame={.op=HUB_BROKER_STATUS,.slot=slot,.generation=generation,.length=1,.error=error};
    memcpy(frame.token,token,16);frame.payload[0]=phase;assert(!hub_public_broker_send(peer,&frame));
}
int main(void) {
    char root[]="/tmp/azhub-sharing-inventory-XXXXXX";assert(mkdtemp(root));
    int pair[2];assert(!socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_NONBLOCK|SOCK_CLOEXEC,0,pair));
    unsigned char token[16]={1},parent[6]={0x52,0x54,0,0x12,0x34,0x56};
    struct hub_public_sharing_inventory state;assert(!hub_public_sharing_inventory_init(&state,token,parent));
    struct hub_public_sharing_view views[8]={0};struct hub_public_broker_frame frame;
    /* Missing source0 never prevents another source from starting. */
    views[4]=(struct hub_public_sharing_view){root,1,0};
    assert(!hub_public_sharing_inventory_poll(&state,pair[0],views,0));
    assert(hub_public_broker_receive_token(pair[1],token,&frame)==1 && frame.slot==4 && frame.op==HUB_BROKER_ALLOCATE);
    hub_public_broker_dispose(&frame);status(pair[1],token,4,1,HUB_BROKER_ACTIVE,0);
    views[0]=(struct hub_public_sharing_view){"/nonexistent/azhub-root",1,0};
    assert(!hub_public_sharing_inventory_poll(&state,pair[0],views,1));
    assert(state.error[0]==ENOENT && state.client.slots[4].phase==HUB_BROKER_ACTIVE);
    assert(!state.players.reserved[0]); /* A never-published bad root cannot exhaust the pool. */
    views[4].reserved=1;
    assert(!hub_public_sharing_inventory_poll(&state,pair[0],views,2));
    assert(hub_public_broker_receive_token(pair[1],token,&frame)==1 && frame.op==HUB_BROKER_DETACH && frame.slot==4);
    assert(!hub_public_broker_client_drained(&state.client,4) && state.players.reserved[4]==45);
    status(pair[1],token,4,1,HUB_BROKER_EMPTY,0);
    assert(!hub_public_sharing_inventory_poll(&state,pair[0],views,3));
    assert(hub_public_broker_client_drained(&state.client,4) && !state.players.reserved[4]);
    assert(state.retired_generation[4]==1 && state.retired_error[4]==0);
    views[0].present=0;views[4].reserved=0;
    assert(!hub_public_sharing_inventory_poll(&state,pair[0],views,4));
    assert(hub_public_broker_receive_token(pair[1],token,&frame)==1 && frame.slot==4 && frame.generation==2);
    hub_public_broker_dispose(&frame);status(pair[1],token,4,2,HUB_BROKER_ACTIVE,0);
    assert(!hub_public_sharing_inventory_poll(&state,pair[0],views,5));
    /* A whole retirement batch can be consumed before the next allocation.
     * Preserve its authenticated evidence even when current phase is STARTING. */
    status(pair[1],token,4,2,HUB_BROKER_DRAINING,EADDRINUSE);
    status(pair[1],token,4,2,HUB_BROKER_EMPTY,EADDRINUSE);
    assert(!hub_public_sharing_inventory_poll(&state,pair[0],views,6));
    assert(state.retired_generation[4]==2 && state.retired_error[4]==EADDRINUSE);
    assert(state.client.slots[4].generation==3 && state.client.slots[4].phase==HUB_BROKER_STARTING);
    assert(hub_public_broker_receive_token(pair[1],token,&frame)==1 && frame.generation==3);
    hub_public_broker_dispose(&frame);
    status(pair[1],token,4,1,HUB_BROKER_EMPTY,ESTALE);
    assert(!hub_public_sharing_inventory_poll(&state,pair[0],views,7));
    assert(state.retired_generation[4]==2 && state.retired_error[4]==EADDRINUSE);
    close(pair[1]);assert(hub_public_sharing_inventory_poll(&state,pair[0],views,8)<0);
    assert(state.client.lost && !hub_public_broker_client_drained(&state.client,4));
    hub_public_broker_client_close(&state.client);close(pair[0]);
    assert(!socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_NONBLOCK|SOCK_CLOEXEC,0,pair));
    assert(!hub_public_sharing_inventory_init(&state,token,parent));
    assert(!hub_public_sharing_inventory_poll(&state,pair[0],views,0));
    assert(hub_public_broker_receive_token(pair[1],token,&frame)==1);hub_public_broker_dispose(&frame);
    status(pair[1],token,4,1,HUB_BROKER_ACTIVE,0);
    assert(!hub_public_sharing_inventory_poll(&state,pair[0],views,1));
    status(pair[1],token,4,1,HUB_BROKER_DRAINING,0);
    assert(!hub_public_sharing_inventory_poll(&state,pair[0],views,2) && state.quiescing);
    status(pair[1],token,4,1,HUB_BROKER_EMPTY,0);status(pair[1],token,0,0,4,0);
    assert(hub_public_sharing_inventory_poll(&state,pair[0],views,3)==1 && state.stopped);
    assert(hub_public_broker_receive_token(pair[1],token,&frame)==0); /* No republish while helper stops. */
    hub_public_broker_client_close(&state.client);close(pair[0]);close(pair[1]);
    assert(!socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_NONBLOCK|SOCK_CLOEXEC,0,pair));
    assert(!hub_public_sharing_inventory_init(&state,token,parent));
    assert(!hub_public_sharing_inventory_poll(&state,pair[0],views,0));
    assert(hub_public_broker_receive_token(pair[1],token,&frame)==1);hub_public_broker_dispose(&frame);
    status(pair[1],token,0,0,4,0);
    assert(hub_public_sharing_inventory_poll(&state,pair[0],views,1)<0 && errno==EPROTO && !state.stopped);
    hub_public_broker_client_close(&state.client);close(pair[0]);close(pair[1]);assert(!rmdir(root));
    puts("PASS: source0-independent startup, per-root failure isolation, native reservation withdrawal, confirmed retirement/replug and channel-loss gate retention");
}

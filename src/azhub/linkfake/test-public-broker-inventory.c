/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include "public-broker-inventory.h"
int main(void) {
    char parent[]="/tmp/azhub-inventory-XXXXXX";assert(mkdtemp(parent));
    unsigned char token[16]={1};struct hub_public_broker_client client;hub_public_broker_client_init(&client,token);
    struct hub_public_broker_inventory inventory={0};struct hub_public_broker_source sources[8];
    for (unsigned i=0;i<8;++i) {
        struct hub_public_broker_source *s=sources+i;
        *s=(struct hub_public_broker_source){.identity=1,.device=1,.inode=1,.player=41+i,.mac={2,0,0,0,1,(unsigned char)i}};
        snprintf(s->directory,sizeof s->directory,"%s/source%u",parent,i);assert(!mkdir(s->directory,0700));
        snprintf(s->interface,sizeof s->interface,"azpub%u",i);snprintf(s->label,sizeof s->label,"Hub USB %u",i+1);
        assert(hub_public_broker_inventory_observe(&inventory,&client,i,s,1,0)==1);
        int pinned=client.slots[i].root;
        assert(!hub_public_broker_inventory_observe(&inventory,&client,i,s,1,0) && client.slots[i].root==pinned);
    }
    struct hub_public_broker_client_slot *live=client.slots;
    live->generation=1;live->identity=1;live->phase=HUB_BROKER_ACTIVE;
    live->device=live->desired.device;live->inode=live->desired.inode;
    char previous[256];snprintf(previous,sizeof previous,"%s/previous",parent);
    assert(!rename(sources[0].directory,previous) && !mkdir(sources[0].directory,0700));
    int neighbor=client.slots[1].root;
    assert(hub_public_broker_inventory_observe(&inventory,&client,0,sources,1,0)==1);
    assert(live->desired.identity==2 && live->desired.inode!=(uint64_t)live->inode && live->identity==1);
    assert(!hub_public_broker_client_drained(&client,0) && client.slots[1].root==neighbor);
    assert(hub_public_broker_inventory_observe(&inventory,&client,0,sources,1,1)==1);
    assert(!live->wanted && live->root==-1 && !hub_public_broker_client_drained(&client,0));
    assert(hub_public_broker_inventory_observe(&inventory,&client,0,sources,1,0)==1 && live->desired.identity==3);
    assert(!rmdir(sources[0].directory));
    assert(hub_public_broker_inventory_observe(&inventory,&client,0,sources,1,0)<0 && !live->wanted);
    assert(!mkdir(sources[0].directory,0700));inventory.slots[0].incarnation=UINT64_MAX;
    assert(hub_public_broker_inventory_observe(&inventory,&client,0,sources,1,0)<0 && errno==EOVERFLOW);
    assert(client.slots[1].root==neighbor && fcntl(neighbor,F_GETFD)>=0);
    hub_public_broker_client_close(&client);
    for (unsigned i=0;i<8;++i) assert(!rmdir(sources[i].directory));
    assert(!rmdir(previous) && !rmdir(parent));
    puts("PASS: independent eight-root observation, no-gap replacement, pinned unchanged roots, eject priority, disappearance and overflow isolation");
}

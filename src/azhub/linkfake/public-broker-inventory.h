/* SPDX-License-Identifier: GPL-3.0-only */
/* Single-controller source observations. Capabilities remain in the client;
 * incarnation counters survive disappearance, replacement and eject reservation. */
#ifndef HUB_PUBLIC_BROKER_INVENTORY_H
#define HUB_PUBLIC_BROKER_INVENTORY_H
#include "public-broker-client.h"
struct hub_public_broker_inventory_slot { uint64_t incarnation;int seen; };
struct hub_public_broker_inventory { struct hub_public_broker_inventory_slot slots[8]; };
static int hub_public_broker_inventory_withdraw(struct hub_public_broker_inventory *inventory,
        struct hub_public_broker_client *client,unsigned slot) {
    inventory->slots[slot].seen=0;
    if (!client->slots[slot].wanted && client->slots[slot].root<0) return 0;
    return hub_public_broker_client_desire(client,slot,NULL,-1)<0 ? -1:1;
}
/* Template identities are supplied by the caller's authorized inventory policy.
 * Return1 changed,0 unchanged,-1 error after withdrawing this desired source.
 * Caller must still send/poll retirement on error; errors are not drain proof. */
static int hub_public_broker_inventory_observe(struct hub_public_broker_inventory *inventory,
        struct hub_public_broker_client *client,unsigned slot,const struct hub_public_broker_source *declared,
        int present,int reserved) {
    if (!inventory || !client || slot>=8) { errno=EINVAL;return -1; }
    if (!present || reserved) return hub_public_broker_inventory_withdraw(inventory,client,slot);
    struct hub_public_broker_inventory_slot *observation=inventory->slots+slot;
    struct hub_public_broker_client_slot *live=client->slots+slot;
    struct stat actual;
    int error=0;
    if (!declared || !hub_public_broker_source_valid(declared)) error=EINVAL;
    else if (stat(declared->directory,&actual)<0) error=errno;
    else if (!S_ISDIR(actual.st_mode)) error=ENOTDIR;
    if (error) goto failed;
    if (observation->seen && live->wanted && live->desired.device==(uint64_t)actual.st_dev &&
            live->desired.inode==(uint64_t)actual.st_ino && live->desired.player==declared->player &&
            !memcmp(live->desired.mac,declared->mac,6) && !strcmp(live->desired.directory,declared->directory) &&
            !strcmp(live->desired.interface,declared->interface) && !strcmp(live->desired.label,declared->label)) return 0;
    if (observation->incarnation==UINT64_MAX) { error=EOVERFLOW;goto failed; }
    struct hub_public_broker_source next=*declared;
    next.identity=observation->incarnation+1;next.device=actual.st_dev;next.inode=actual.st_ino;
    int root=hub_public_broker_source_open(&next);
    if (root<0) { error=errno;goto failed; }
    int result=hub_public_broker_client_desire(client,slot,&next,root);error=errno;close(root);
    if (result<0) goto failed;
    observation->incarnation=next.identity;observation->seen=1;return 1;
failed:
    hub_public_broker_inventory_withdraw(inventory,client,slot);errno=error;return -1;
}
#endif

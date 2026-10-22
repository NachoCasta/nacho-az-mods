/* SPDX-License-Identifier: GPL-3.0-only */
/* One exact netlink removal per lease incarnation. A failed/ambiguous operation
 * is terminal until explicit reset: never silently retry or claim removal. */
#ifndef HUB_PUBLIC_BROKER_ADDRESS_RETIREMENT_H
#define HUB_PUBLIC_BROKER_ADDRESS_RETIREMENT_H
#include "public-address-remove.h"
struct hub_public_broker_address_retirement {
    struct hub_public_address_remove operation;
    struct hub_public_network_spec declared;
    dev_t device;ino_t inode;
    int state,error;
};
static int hub_public_broker_address_retirement_reset(struct hub_public_broker_address_retirement *r) {
    if (!r || r->operation.started) { errno=EBUSY;return -1; }
    memset(r,0,sizeof *r);return 0;
}
/* Compatible with lease release callbacks via the runtime's per-slot context.
 * Caller must prove worker join/endpoint close before the first call. */
static int hub_public_broker_address_retire(struct hub_public_broker_address_retirement *r,
        const struct hub_public_network_spec *spec) {
    if (!r || !spec) { errno=EINVAL;return -1; }
    struct stat ns;if (fstat(spec->namespace_fd,&ns)<0) return -1;
    if (r->state && (r->device!=ns.st_dev || r->inode!=ns.st_ino ||
            strcmp(r->declared.interface,spec->interface) ||
            r->declared.address.s_addr!=spec->address.s_addr || r->declared.mask.s_addr!=spec->mask.s_addr ||
            r->declared.broadcast.s_addr!=spec->broadcast.s_addr || memcmp(r->declared.mac,spec->mac,6))) {
        errno=ESTALE;return -1;
    }
    if (!r->state) {
        r->declared=*spec;r->device=ns.st_dev;r->inode=ns.st_ino;
        if (hub_public_address_remove_start(&r->operation,spec)<0) {
            r->state=3;r->error=errno;return -1;
        }
        r->state=1;
    }
    if (r->state==2) return 0;
    if (r->state==3) { errno=r->error;return -1; }
    int result=hub_public_address_remove_poll(&r->operation);
    if (result==1) return 1;
    r->state=result<0 ? 3:2;r->error=result<0 ? errno:0;return result;
}
#endif

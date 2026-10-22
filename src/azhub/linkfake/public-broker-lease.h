/* SPDX-License-Identifier: GPL-3.0-only */
/* Source-owned lease/endpoint coupling. One controller owns all lease calls.
 * Acquisition, ARP, interface creation/configuration and source inventory remain
 * explicit caller duties. Never fabricate READY to enable a public endpoint. */
#ifndef HUB_PUBLIC_BROKER_LEASE_H
#define HUB_PUBLIC_BROKER_LEASE_H
#include "public-network-endpoint.h"
#include "public-network-lease.h"
#include "public-broker-dispatch.h"
struct hub_public_broker_lease_slot {
    struct hub_public_network_lease *lease;
    struct hub_public_network_spec spec;
    uint64_t generation;
    int prepared;
};
struct hub_public_broker_leases {
    struct hub_public_broker_lease_slot slots[8];
    uint64_t (*now)(void *);
    /* 0 address removed, 1 still pending, -1 fault. Receives the exact retained
     * namespace/interface/address, not a potentially newer lease replacement. */
    int (*remove)(void *,unsigned,const struct hub_public_network_spec *);
    void *context;
};
static int hub_public_broker_lease_prepare(struct hub_public_broker_leases *owner,unsigned slot,
        struct hub_public_network_lease *lease,const struct hub_public_network_spec *spec) {
    if (!owner || slot>=8 || !owner->now || !owner->remove || !lease || !spec || spec->namespace_fd<0) {
        errno=EINVAL;return -1;
    }
    struct hub_public_broker_lease_slot *s=owner->slots+slot;
    if (s->prepared) { errno=EBUSY;return -1; }
    if (!hub_public_lease_ready(lease,owner->now(owner->context)) ||
            spec->address.s_addr!=lease->current.address.s_addr ||
            spec->mask.s_addr!=lease->current.mask.s_addr ||
            spec->broadcast.s_addr!=lease->current.broadcast.s_addr) { errno=EADDRNOTAVAIL;return -1; }
    int fd=fcntl(spec->namespace_fd,F_DUPFD_CLOEXEC,0);if (fd<0) return -1;
    s->spec=*spec;s->spec.namespace_fd=fd;s->lease=lease;s->generation=lease->generation;s->prepared=1;
    return 0;
}
/* Call before dispatch poll. False requires source retirement; it is never a
 * permission to remove an address before worker/endpoint draining. */
static int hub_public_broker_lease_ready(struct hub_public_broker_leases *owner,unsigned slot) {
    if (!owner || slot>=8 || !owner->now) return 0;
    struct hub_public_broker_lease_slot *s=owner->slots+slot;
    return s->prepared && s->lease->generation==s->generation &&
        hub_public_lease_ready(s->lease,owner->now(owner->context));
}
static int hub_public_broker_lease_open(void *opaque,unsigned slot,struct hub_public_source_binding *binding) {
    struct hub_public_broker_leases *owner=opaque;
    if (!hub_public_broker_lease_ready(owner,slot)) { errno=EADDRNOTAVAIL;return -1; }
    return hub_public_network_open(&owner->slots[slot].spec,binding);
}
static void hub_public_broker_lease_close(void *opaque,unsigned slot,struct hub_public_source_binding *binding) {
    (void)opaque;(void)slot;hub_public_network_close(binding);
}
/* Only the dispatch release callback may call this, after actual worker join
 * and endpoint close. Returns 0 retired, 1 pending, -1 not safely retired. */
static int hub_public_broker_lease_release(struct hub_public_broker_leases *owner,unsigned slot) {
    if (!owner || slot>=8 || !owner->now || !owner->remove) { errno=EINVAL;return -1; }
    struct hub_public_broker_lease_slot *s=owner->slots+slot;
    if (!s->prepared) return 0;
    uint64_t now=owner->now(owner->context);
    if (s->lease->generation!=s->generation) { errno=ESTALE;return -1; }
    if (s->lease->state!=HUB_LEASE_REMOVING) {
        if (s->lease->state==HUB_LEASE_READY || s->lease->state==HUB_LEASE_CANDIDATE)
            hub_public_lease_retire(s->lease);
        if (hub_public_lease_drained(s->lease,s->generation,now)<0) { errno=EPROTO;return -1; }
    }
    int result=owner->remove(owner->context,slot,&s->spec);
    if (result) return result<0 ? -1:1;
    if (hub_public_lease_removed(s->lease,s->generation,owner->now(owner->context))<0) { errno=EPROTO;return -1; }
    close(s->spec.namespace_fd);s->spec.namespace_fd=-1;s->prepared=0;s->lease=NULL;return 0;
}
/* Use this controller poll instead of dispatch_poll when endpoint state is
 * lease-owned. Loss/expiry withdraws before waiting for worker/network drain.
 * The dispatch release callback must invoke lease_release after manager drain. */
static int hub_public_broker_lease_dispatch_poll(struct hub_public_broker_leases *owner,
        struct hub_public_broker_dispatch *dispatch,unsigned slot) {
    if (!owner || !dispatch || slot>=8) { errno=EINVAL;return -1; }
    unsigned phase=dispatch->generations.slots[slot].phase;
    if ((phase==HUB_BROKER_STARTING || phase==HUB_BROKER_ACTIVE) &&
            !hub_public_broker_lease_ready(owner,slot)) {
        dispatch->error[slot]=EADDRNOTAVAIL;
        if (hub_public_broker_dispatch_retire(dispatch,slot)<0) return -1;
    }
    return hub_public_broker_dispatch_poll(dispatch,slot);
}
#endif

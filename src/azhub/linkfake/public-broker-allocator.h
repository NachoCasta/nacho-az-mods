/* SPDX-License-Identifier: GPL-3.0-only */
/* Standalone helper callbacks for ALLOCATE. One controller owns all slots,
 * SIGCHLD and polling. No EP147 process or native USB library slot is used. */
#ifndef HUB_PUBLIC_BROKER_ALLOCATOR_H
#define HUB_PUBLIC_BROKER_ALLOCATOR_H
#include <limits.h>
#include "public-broker-dispatch.h"
#include "public-network-acquire.h"
#include "public-network-ready.h"
#include "public-broker-address-retirement.h"
#include "public-dhcp-decline-namespace.h"
#include "public-address-inventory.h"
#include "public-dhcp-refresh.h"
#include "public-link-local.h"
struct hub_public_broker_allocated_source {
    struct hub_public_network_acquire acquire;
    struct hub_public_network_lease lease;
    struct hub_public_network_ready ready;
    struct hub_public_network_spec spec;
    struct hub_public_broker_address_retirement removal;
    struct hub_public_decline_namespace decline;
    struct hub_public_network_link link_removal;
    struct hub_public_dhcp_refresh refresh;
    int link_removed;
    int phase,decline_attempted,decline_error,retry_recorded;
};
struct hub_public_broker_allocator {
    struct hub_public_broker_allocated_source sources[8];
    struct hub_public_source_manager *manager;
    char parent[IFNAMSIZ],helper[1024],hook[1024];unsigned char parent_mac[6];
    unsigned char local_mac[32][6];size_t local_count;
    uint64_t (*now)(void *);unsigned (*jitter)(void *);
    int (*authorize)(void *,unsigned,const struct hub_public_broker_source *,int);
    hub_public_rpc_peer_policy allowed;
    void *context,*peer_context;
    int last_decline_attempted[8],last_decline_error[8];
    int last_acquire_phase[8],last_acquire_error[8];
    unsigned refreshes[8];
    struct hub_public_acd_retry retry[8];
    int allow_link_local,entropy_fd;
};
/* An existing candidate/active source keeps its reservation. Simultaneous
 * completed grants use slot order, so the same address cannot publish twice.
 * This is not observed ARP conflict and must not trigger its retry policy. */
static int hub_public_broker_allocator_candidate(struct hub_public_broker_allocator *a,unsigned slot,
        struct in_addr address,const struct hub_public_address_inventory *native) {
    if (!a || slot>=8 || !native || native->count>32 || !address.s_addr) { errno=EINVAL;return -1; }
    if (hub_public_address_inventory_contains(native,address)) { errno=EADDRNOTAVAIL;return -1; }
    for (unsigned i=0;i<8;++i) if (i!=slot) {
        struct hub_public_broker_allocated_source *other=a->sources+i;
        int reserved=other->phase==2;
        int earlier=a->sources[slot].phase!=2 && i<slot && other->phase==1 && other->acquire.complete && !other->acquire.error && other->acquire.dhcp.bound;
        struct in_addr owned=reserved ? other->spec.address:other->acquire.dhcp.proposal.address;
        if ((reserved || earlier) && owned.s_addr==address.s_addr) { errno=EADDRNOTAVAIL;return -1; }
    }
    return 0;
}
/* Selection commits only a candidate; ARP/configuration still follows. */
static int hub_public_broker_allocator_link_local(struct hub_public_broker_allocator *a,unsigned slot,
        const struct hub_public_address_inventory *native,uint64_t now) {
    if (!a || slot>=8 || !native || !a->allow_link_local ||
            !hub_public_dhcp_acquire_no_offer(&a->sources[slot].acquire.dhcp)) { errno=EPROTO;return -1; }
    for (unsigned attempt=0;attempt<32;++attempt) {
        struct in_addr candidate;
        if (hub_public_link_local_sample(a->entropy_fd,&candidate)<0) return -1;
        if (hub_public_broker_allocator_candidate(a,slot,candidate,native)<0) continue;
        if (hub_public_lease_link_local(&a->sources[slot].lease,candidate,now)<0) { errno=EPROTO;return -1; }
        return 0;
    }
    errno=EADDRNOTAVAIL;return -1;
}
/* 0 retained backoff,1 acquisition started,-1 invalid/exhausted policy. The
 * policy belongs to the allocator slot, never to a disposable candidate. */
static int hub_public_broker_allocator_begin(struct hub_public_broker_allocator *a,unsigned slot,uint64_t now) {
    struct hub_public_acd_retry *retry=a->retry+slot;
    if (retry->exhausted || now<retry->last) { errno=ERANGE;return -1; }
    if (now<retry->not_before || (retry->conflicts>=10 && now<retry->next)) return 0;
    if (hub_public_acd_retry_begin(retry,now)<0) { errno=ERANGE;return -1; }
    struct hub_public_broker_allocated_source *s=a->sources+slot;
    struct hub_public_network_link_spec link={.namespace_fd=-1};
    strcpy(link.parent,a->parent);strcpy(link.name,s->spec.interface);
    memcpy(link.parent_mac,a->parent_mac,6);memcpy(link.mac,s->spec.mac,6);
    if (hub_public_network_acquire_start(&s->acquire,&link,a->helper,a->hook)<0) return -1;
    if (a->allow_link_local && hub_public_network_acquire_allow_no_offer(&s->acquire)<0) return -1;
    s->phase=1;return 1;
}
static int hub_public_broker_allocator_allocate(void *opaque,unsigned slot,const struct hub_public_broker_source *source,int root) {
    struct hub_public_broker_allocator *a=opaque;
    if (!a || !hub_public_broker_source_valid(source) || slot>=8 || !a->manager || !a->now || !a->jitter || !a->authorize || !a->allowed ||
            a->local_count>32 || !a->parent[0] || strnlen(a->parent,IFNAMSIZ)==IFNAMSIZ) { errno=EINVAL;return -1; }
    struct hub_public_broker_allocated_source *s=a->sources+slot;
    if (s->phase) { errno=EBUSY;return -1; }
    for (unsigned i=0;i<8;++i) if (i!=slot && a->sources[i].phase && a->sources[i].spec.player==source->player) {
        errno=EEXIST;return -1;
    }
    if (a->authorize(a->context,slot,source,root)<0) return -1;
    memset(s,0,sizeof *s);s->spec.namespace_fd=s->acquire.namespace_fd=-1;
    a->last_decline_attempted[slot]=a->last_decline_error[slot]=0;
    a->last_acquire_phase[slot]=a->last_acquire_error[slot]=0;
    a->refreshes[slot]=0;
    s->phase=3;strcpy(s->spec.interface,source->interface);memcpy(s->spec.mac,source->mac,6);
    s->spec.player=source->player;strcpy(s->spec.label,source->label);s->spec.allowed=a->allowed;s->spec.policy=a->peer_context;
    if (hub_public_broker_allocator_begin(a,slot,a->now(a->context))<0) { s->phase=0;return -1; }
    return 0;
}
static int hub_public_broker_allocator_ready(void *opaque,unsigned slot) {
    struct hub_public_broker_allocator *a=opaque;if (!a || !a->now || !a->jitter || slot>=8) { errno=EINVAL;return -1; }
    struct hub_public_broker_allocated_source *s=a->sources+slot;uint64_t now=a->now(a->context);
    if (s->phase==3) {
        int result=hub_public_broker_allocator_begin(a,slot,now);if (result<=0) return result;
        return 0;
    }
    if (s->phase==1) {
        int result=hub_public_network_acquire_poll(&s->acquire,now);
        /* Only readiness ACD conflict authorizes the parent's automatic
         * conflict retry. Acquisition EADDRINUSE is not ARP conflict proof. */
        if (result<0) { a->last_acquire_phase[slot]=s->acquire.phase;a->last_acquire_error[slot]=errno; }
        if (result<0 && errno==EADDRINUSE) errno=EEXIST;
        if (result<=0) return result;
        struct hub_public_address_inventory native;
        if (hub_public_address_inventory_native(&native)<0) return -1;
        if (result==2) {
            if (!a->allow_link_local || !hub_public_dhcp_acquire_no_offer(&s->acquire.dhcp)) { errno=EPROTO;return -1; }
            if (hub_public_broker_allocator_link_local(a,slot,&native,now)<0) return -1;
        } else {
            if (hub_public_broker_allocator_candidate(a,slot,s->acquire.dhcp.proposal.address,&native)<0) return -1;
            if (hub_public_lease_offer(&s->lease,&s->acquire.dhcp.proposal,s->acquire.dhcp.started,now)<0) { errno=EPROTO;return -1; }
        }
        s->spec.namespace_fd=s->acquire.namespace_fd;s->spec.address=s->lease.current.address;
        s->spec.mask=s->lease.current.mask;s->spec.broadcast=s->lease.current.broadcast;
        if (hub_public_network_ready_start(&s->ready,&s->lease,&s->spec,now)<0) return -1;
        s->phase=2;
    }
    if (s->phase!=2) { errno=EINVAL;return -1; }
    unsigned char inventory[41][6];size_t count=a->local_count;
    memcpy(inventory,a->local_mac,count*6);
    memcpy(inventory[count++],a->parent_mac,6);
    for (unsigned i=0;i<8;++i) if (a->sources[i].phase) memcpy(inventory[count++],a->sources[i].spec.mac,6);
    int result=hub_public_network_ready_poll(&s->ready,now,a->jitter(a->context),inventory,count);
    if (result!=1) return result;
    if (!s->refresh.armed && hub_public_dhcp_refresh_arm(&s->refresh,&s->lease,now)<0) return -1;
    result=hub_public_dhcp_refresh_poll(&s->refresh,&s->lease,s->spec.namespace_fd,s->spec.interface,s->spec.mac,a->helper,a->hook,now);
    if (result<0) return -1;
    if (result==1) {
        struct hub_public_address_inventory native;
        if (hub_public_address_inventory_native(&native)<0 ||
                hub_public_broker_allocator_candidate(a,slot,s->spec.address,&native)<0) {
            int error=errno;hub_public_lease_retire(&s->lease);errno=error;return -1;
        }
        if (a->refreshes[slot]<UINT_MAX) ++a->refreshes[slot];
    }
    return 1;
}
static int hub_public_broker_allocator_open(void *opaque,unsigned slot,struct hub_public_source_binding *binding) {
    struct hub_public_broker_allocator *a=opaque;
    int ready=hub_public_broker_allocator_ready(a,slot);
    if (ready!=1) { if (!ready) errno=EADDRNOTAVAIL;return -1; }
    return hub_public_network_open(&a->sources[slot].spec,binding);
}
static void hub_public_broker_allocator_close(void *opaque,unsigned slot,struct hub_public_source_binding *binding) {
    (void)opaque;(void)slot;hub_public_network_close(binding);
}
/* Dispatch calls only after worker join/endpoint close. Pending allocation is
 * canceled/reaped; successful configuration is explicitly retired before final
 * namespace release. No root FD or caller inventory pointer is retained here. */
static int hub_public_broker_allocator_release(void *opaque,unsigned slot) {
    struct hub_public_broker_allocator *a=opaque;
    if (!a || !a->manager || !a->now || slot>=8 || !hub_public_source_manager_drained(a->manager,slot)) { errno=EBUSY;return -1; }
    struct hub_public_broker_allocated_source *s=a->sources+slot;
    if (!s->phase) return 0;
    uint64_t now=a->now(a->context);
    if (s->acquire.phase && !s->acquire.complete) {
        hub_public_network_acquire_cancel(&s->acquire,ECANCELED,now);
        int result=hub_public_network_acquire_poll(&s->acquire,now);if (!result) return 1;
    }
    int refreshed=hub_public_dhcp_refresh_stop(&s->refresh,now);if (refreshed) return refreshed;
    if (s->ready.phase) {
        if (s->ready.arp.state==HUB_ACD_CONFLICT && s->lease.origin==HUB_ADDRESS_LINK_LOCAL && !s->retry_recorded) {
            s->retry_recorded=1;
            if (hub_public_acd_retry_conflict(a->retry+slot,now)<0) a->retry[slot].exhausted=1;
        }
        if (s->ready.arp.state==HUB_ACD_CONFLICT && s->lease.origin==HUB_ADDRESS_DHCP && !s->decline_attempted) {
            s->decline_attempted=1;
            if (hub_public_decline_namespace_start(&s->decline,s->spec.namespace_fd,&s->ready.transport,&s->ready.arp,&s->lease.current)<0)
                s->decline_error=errno;
        }
        if (s->decline.started) {
            int result=hub_public_decline_namespace_poll(&s->decline);if (result==1) return 1;
            if (result<0 && s->decline.started) return -1;
            if (result<0) s->decline_error=errno;
        }
        if (s->decline_attempted && !s->retry_recorded) {
            s->retry_recorded=1;
            if (hub_public_acd_retry_declined(a->retry+slot,a->now(a->context))<0) a->retry[slot].exhausted=1;
        }
        int result=hub_public_network_ready_quiesce(&s->ready);if (result) return result;
    }
    if (s->lease.configured || s->ready.applied) {
        if (s->lease.state==HUB_LEASE_CANDIDATE || s->lease.state==HUB_LEASE_READY) hub_public_lease_retire(&s->lease);
        if (s->lease.state==HUB_LEASE_DRAINING && hub_public_lease_drained(&s->lease,s->lease.generation,now)<0) { errno=EPROTO;return -1; }
        int result=hub_public_broker_address_retire(&s->removal,&s->spec);if (result) return result;
        if (hub_public_lease_removed(&s->lease,s->lease.generation,a->now(a->context))<0) { errno=EPROTO;return -1; }
        s->ready.applied=0;
    }
    /* Namespace destruction can be deferred by kernel socket references.
     * Explicit ACK of link deletion frees the parent MACVLAN identity before
     * advertising EMPTY or accepting a replacement with the same MAC. */
    if (s->acquire.namespace_fd>=0 && s->acquire.phase>=3 && !s->link_removed) {
        if (s->link_removal.done && !s->link_removal.started && s->link_removal.error) {
            errno=s->link_removal.error;return -1;
        }
        if (!s->link_removal.started && !s->link_removal.done) {
            struct hub_public_network_link_spec link=s->acquire.spec;
            link.namespace_fd=s->acquire.namespace_fd;
            if (hub_public_network_link_remove_start(&s->link_removal,&link)<0) return -1;
        }
        int result=hub_public_network_link_poll(&s->link_removal);
        if (result) return result<0 ? -1:1;
        s->link_removed=1;
    }
    if (s->acquire.namespace_fd>=0) close(s->acquire.namespace_fd);
    a->last_decline_attempted[slot]=s->decline_attempted;a->last_decline_error[slot]=s->decline_error;
    memset(s,0,sizeof *s);s->spec.namespace_fd=s->acquire.namespace_fd=-1;return 0;
}
#endif

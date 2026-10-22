/* SPDX-License-Identifier: GPL-3.0-only */
/* Single-controller DHCP candidate -> namespace ARP -> async configure -> READY.
 * Caller owns namespace/spec/lease throughout. Keep polling after READY for
 * conflicts/expiry. Loss retires lease but never releases borrowed endpoints.
 * Call quiesce after withdrawal/drain before removing the configured address. */
#ifndef HUB_PUBLIC_NETWORK_READY_H
#define HUB_PUBLIC_NETWORK_READY_H
#include "public-address-arp-namespace.h"
#include "public-address-remove.h"
#include "public-network-lease.h"
struct hub_public_network_ready {
    struct hub_public_arp_namespace factory;
    struct hub_public_arp transport;
    struct hub_public_acd arp;
    struct hub_public_address_remove configure;
    struct hub_public_network_spec spec;
    struct hub_public_network_lease *lease;
    uint64_t generation;
    int phase,error,applied;
};
static int hub_public_network_ready_fail(struct hub_public_network_ready *r,int error) {
    if (!r->error) r->error=error ? error:EIO;
    if (r->lease->state==HUB_LEASE_CANDIDATE || r->lease->state==HUB_LEASE_READY) hub_public_lease_retire(r->lease);
    if (r->arp.state!=HUB_ACD_CONFLICT) hub_public_acd_stop(&r->arp);
    errno=r->error;return -1;
}
static int hub_public_network_ready_start(struct hub_public_network_ready *r,
        struct hub_public_network_lease *lease,const struct hub_public_network_spec *spec,uint64_t now) {
    if (!r || !lease || !spec || r->phase || hub_public_lease_poll(lease,now)<0 || lease->failed ||
            lease->state!=HUB_LEASE_CANDIDATE || lease->configured ||
            lease->current.address.s_addr!=spec->address.s_addr || lease->current.mask.s_addr!=spec->mask.s_addr ||
            lease->current.broadcast.s_addr!=spec->broadcast.s_addr) { errno=EINVAL;return -1; }
    memset(r,0,sizeof *r);r->transport.fd=-1;r->spec=*spec;r->lease=lease;r->generation=lease->generation;
    if (hub_public_arp_namespace_start(&r->factory,spec->namespace_fd,spec->interface,spec->mac)<0) return -1;
    r->phase=1;return 0;
}
/* 0 not ready, 1 READY (continue polling), -1 retired/failure (quiesce required).
 * jitter is independently supplied0..1000ms each iteration; inventory must
 * include all other local MACs so reflected local packets cannot conflict. */
static int hub_public_network_ready_poll(struct hub_public_network_ready *r,uint64_t now,unsigned jitter,
        const unsigned char (*local_mac)[6],size_t count) {
    if (!r || !r->phase || jitter>1000 || (count && !local_mac)) { errno=EINVAL;return -1; }
    if (r->error) { errno=r->error;return -1; }
    if (hub_public_lease_poll(r->lease,now)<0 || r->lease->generation!=r->generation ||
            (r->lease->state!=HUB_LEASE_CANDIDATE && r->lease->state!=HUB_LEASE_READY))
        return hub_public_network_ready_fail(r,EADDRNOTAVAIL);
    if (r->phase==1) {
        int result=hub_public_arp_namespace_poll(&r->factory);
        if (result==1) return 0;
        if (result<0) return hub_public_network_ready_fail(r,errno);
        if (hub_public_arp_namespace_take(&r->factory,&r->transport)<0 ||
                hub_public_acd_init(&r->arp,(const unsigned char *)&r->spec.address.s_addr,r->spec.mac,now,jitter)<0)
            return hub_public_network_ready_fail(r,errno);
        r->phase=2;
    }
    int conflict=hub_public_arp_receive(&r->transport,&r->arp,local_mac,count);
    if (conflict) return hub_public_network_ready_fail(r,conflict>0 ? EADDRINUSE:errno);
    if (r->phase==3) {
        int result=hub_public_address_remove_poll(&r->configure);
        if (result==1) return 0;
        if (result<0) return hub_public_network_ready_fail(r,errno);
        r->applied=1;
        if (hub_public_lease_configured(r->lease,r->generation,now,&r->arp)<0 ||
                hub_public_acd_complete(&r->arp,HUB_ACD_CLAIM,now,jitter)<0)
            return hub_public_network_ready_fail(r,EPROTO);
        r->phase=4;
    }
    enum hub_public_acd_action action=hub_public_acd_poll(&r->arp,now);
    if (action==HUB_ACD_CLAIM) {
        if (hub_public_address_configure_start(&r->configure,&r->spec)<0) return hub_public_network_ready_fail(r,errno);
        r->phase=3;return 0;
    }
    if (action==HUB_ACD_PROBE || action==HUB_ACD_ANNOUNCE) {
        if (hub_public_arp_send(&r->transport,&r->arp)<0) {
            if (errno!=EAGAIN && errno!=EWOULDBLOCK && errno!=EINTR) return hub_public_network_ready_fail(r,errno);
        } else if (hub_public_acd_complete(&r->arp,action,now,jitter)<0) return hub_public_network_ready_fail(r,EPROTO);
    }
    if (r->lease->state==HUB_LEASE_CANDIDATE && hub_public_acd_ready(&r->arp))
        if (hub_public_lease_activate(r->lease,r->generation,now,&r->arp)<0) return hub_public_network_ready_fail(r,EPROTO);
    return hub_public_lease_ready(r->lease,now);
}
/* 1 pending, 0 factory/configure threads joined and ARP closed, -1 join fault.
 * Not address removal proof. An ambiguous configure failure retains namespace
 * ownership at the caller for exact reconciliation/retirement. */
static int hub_public_network_ready_quiesce(struct hub_public_network_ready *r) {
    if (!r || !r->phase) { errno=EINVAL;return -1; }
    if (r->lease->state==HUB_LEASE_CANDIDATE || r->lease->state==HUB_LEASE_READY) hub_public_lease_retire(r->lease);
    if (r->factory.started) {
        int result=hub_public_arp_namespace_poll(&r->factory);
        if (result==1) return 1;
        if (result<0 && r->factory.started) return -1;
    }
    if (r->configure.started) {
        int result=hub_public_address_remove_poll(&r->configure);
        if (result==1) return 1;
        if (result<0 && r->configure.started) return -1;
        if (!result) r->applied=1;
    }
    hub_public_arp_close(&r->factory.transport);hub_public_arp_close(&r->transport);hub_public_acd_stop(&r->arp);return 0;
}
#endif

/* SPDX-License-Identifier: GPL-3.0-only */
/* Supervised fresh acquisition with the same client MAC at half lease lifetime
 * or periodic DHCP attempts while independently owning a link-local address.
 * This is not RFC2131 unicast RENEWING. Same-configuration ACK extends ownership;
 * changed configuration requests retirement, never live address replacement.
 * Keep the borrowed namespace alive until stop returns0 (child reaped/EOF). */
#ifndef HUB_PUBLIC_DHCP_REFRESH_H
#define HUB_PUBLIC_DHCP_REFRESH_H
#include "public-dhcp-acquire.h"
#include "public-network-lease.h"
struct hub_public_dhcp_refresh {
    struct hub_public_dhcp_acquire acquire;
    uint64_t next,last;
    int armed,running,stopping,error,last_error;
};
static int hub_public_dhcp_refresh_arm(struct hub_public_dhcp_refresh *r,
        struct hub_public_network_lease *lease,uint64_t now) {
    if (!r || !lease || r->running || r->stopping || r->error || !hub_public_lease_ready(lease,now)) { errno=EINVAL;return -1; }
    if (lease->origin==HUB_ADDRESS_LINK_LOCAL) {
        if (now>UINT64_MAX-60000) { errno=ERANGE;return -1; }
        r->next=now+60000;
    } else {
        if (lease->origin!=HUB_ADDRESS_DHCP || !lease->seen_grant || lease->expires<=lease->latest_grant) { errno=EINVAL;return -1; }
        r->next=lease->latest_grant+(lease->expires-lease->latest_grant)/2;
    }
    r->last=now;r->armed=1;return 0;
}
static int hub_public_dhcp_refresh_fail(struct hub_public_dhcp_refresh *r,
        struct hub_public_network_lease *lease,int error,uint64_t now) {
    if (!r->error) r->error=error;
    hub_public_lease_retire(lease);
    if (r->running) hub_public_dhcp_acquire_cancel(&r->acquire,r->error,now);
    errno=r->error;return -1;
}
static void hub_public_dhcp_refresh_retry(struct hub_public_dhcp_refresh *r,
        const struct hub_public_network_lease *lease,uint64_t now,int error) {
    unsigned delay=lease->origin==HUB_ADDRESS_LINK_LOCAL ? 60000:5000;
    r->last_error=error;r->next=now>UINT64_MAX-delay ? UINT64_MAX:now+delay;
    if (lease->origin==HUB_ADDRESS_DHCP && r->next>lease->expires) r->next=lease->expires;
}
/*0 valid old lease/pending or backed-off refresh,1 accepted same-config ACK,
 * -1 retirement required. Old expiry is checked before reading a queued ACK. */
static int hub_public_dhcp_refresh_poll(struct hub_public_dhcp_refresh *r,struct hub_public_network_lease *lease,
        int namespace_fd,const char *interface,const unsigned char mac[6],const char *helper,const char *hook,uint64_t now) {
    if (!r || !lease || !r->armed || r->stopping) { errno=EINVAL;return -1; }
    if (r->error) { errno=r->error;return -1; }
    if (now<r->last) return hub_public_dhcp_refresh_fail(r,lease,ERANGE,r->last);
    r->last=now;
    if (!hub_public_lease_ready(lease,now)) return hub_public_dhcp_refresh_fail(r,lease,EADDRNOTAVAIL,now);
    if (!r->running) {
        if (now<r->next) return 0;
        if (hub_public_dhcp_acquire_begin(&r->acquire,helper,namespace_fd,interface,hook,mac,now)<0) {
            hub_public_dhcp_refresh_retry(r,lease,now,errno);return 0;
        }
        r->running=1;return 0;
    }
    int result=hub_public_dhcp_acquire_poll(&r->acquire,now);
    if (!result) return 0;
    if (!r->acquire.complete || !r->acquire.reaped || !r->acquire.eof)
        return hub_public_dhcp_refresh_fail(r,lease,EIO,now);
    r->running=0;
    if (result<0) { hub_public_dhcp_refresh_retry(r,lease,now,r->acquire.error ? r->acquire.error:EIO);return 0; }
    if (lease->origin==HUB_ADDRESS_LINK_LOCAL) return hub_public_dhcp_refresh_fail(r,lease,ENETRESET,now);
    if (!hub_public_lease_same(&lease->current,&r->acquire.proposal))
        return hub_public_dhcp_refresh_fail(r,lease,ENETRESET,now);
    if (hub_public_lease_offer(lease,&r->acquire.proposal,r->acquire.started,now)<0)
        return hub_public_dhcp_refresh_fail(r,lease,EPROTO,now);
    r->last_error=0;
    if (hub_public_dhcp_refresh_arm(r,lease,now)<0) return hub_public_dhcp_refresh_fail(r,lease,EPROTO,now);
    return 1;
}
/*1 pending,0 all child ownership released,-1 inconsistent ownership retained. */
static int hub_public_dhcp_refresh_stop(struct hub_public_dhcp_refresh *r,uint64_t now) {
    if (!r) { errno=EINVAL;return -1; }
    r->stopping=1;
    if (!r->running) return 0;
    hub_public_dhcp_acquire_cancel(&r->acquire,ECANCELED,now);
    if (!hub_public_dhcp_acquire_poll(&r->acquire,now)) return 1;
    if (!r->acquire.complete || !r->acquire.reaped || !r->acquire.eof) { errno=EIO;return -1; }
    r->running=0;return 0;
}
#endif

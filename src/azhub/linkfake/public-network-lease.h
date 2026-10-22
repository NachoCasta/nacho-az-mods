/* SPDX-License-Identifier: GPL-3.0-only */
/* Single-controller lease/address lifecycle; no sockets or network mutations.
 * Offers must come from the validated parser. grant_at is a conservative lower
 * bound on grant time on the SAME monotonic clock as now, not the receipt time
 * of a possibly queued IPC record. The current v1 hook cannot supply this
 * guarantee alone: timestamped/supervised acquisition remains integration work.
 * Caller order: candidate -> ARP -> configured -> ARP ready -> activate.
 * Retirement order: withdraw/drain/join sockets -> drained -> remove address
 * -> removed -> next candidate. None of these acknowledgements is implicit. */
#ifndef HUB_PUBLIC_NETWORK_LEASE_H
#define HUB_PUBLIC_NETWORK_LEASE_H
#include "public-dhcp-event.h"
#include "public-address-conflict.h"
enum hub_public_lease_state { HUB_LEASE_IDLE,HUB_LEASE_CANDIDATE,HUB_LEASE_READY,
    HUB_LEASE_DRAINING,HUB_LEASE_REMOVING };
enum hub_public_address_origin { HUB_ADDRESS_DHCP,HUB_ADDRESS_LINK_LOCAL };
struct hub_public_network_lease {
    struct hub_public_dhcp_event current,next;
    uint64_t generation,expires,next_expires,last,latest_grant;
    enum hub_public_lease_state state;
    enum hub_public_address_origin origin;
    int configured,queued,seen_grant,failed;
};
static int hub_public_lease_same(const struct hub_public_dhcp_event *a,const struct hub_public_dhcp_event *b) {
    return a->address.s_addr==b->address.s_addr && a->mask.s_addr==b->mask.s_addr &&
        a->broadcast.s_addr==b->broadcast.s_addr && a->server.s_addr==b->server.s_addr;
}
static void hub_public_lease_retire(struct hub_public_network_lease *lease) {
    lease->queued=0;
    if (lease->state!=HUB_LEASE_IDLE && lease->state!=HUB_LEASE_REMOVING)
        lease->state=HUB_LEASE_DRAINING;
}
static int hub_public_lease_poll(struct hub_public_network_lease *lease,uint64_t now) {
    if (!lease) return -1;
    if (now<lease->last) {
        lease->failed=1;hub_public_lease_retire(lease);return -1;
    }
    lease->last=now;
    if (lease->queued && now>=lease->next_expires) lease->queued=0;
    if (lease->origin==HUB_ADDRESS_DHCP && (lease->state==HUB_LEASE_CANDIDATE || lease->state==HUB_LEASE_READY) && now>=lease->expires)
        hub_public_lease_retire(lease);
    return 0;
}
static int hub_public_lease_begin(struct hub_public_network_lease *lease,
        const struct hub_public_dhcp_event *offer,uint64_t expires) {
    if (lease->generation==UINT64_MAX) { lease->failed=1;return -1; }
    ++lease->generation;lease->current=*offer;lease->expires=expires;
    lease->origin=HUB_ADDRESS_DHCP;
    lease->configured=0;lease->state=HUB_LEASE_CANDIDATE;return 0;
}
/* Explicit IPv4LL ownership, not a DHCP ACK. ARP/configuration/readiness and
 * native/cross-source address exclusion remain mandatory separate gates. */
static int hub_public_lease_link_local(struct hub_public_network_lease *lease,struct in_addr address,uint64_t now) {
    uint32_t host=ntohl(address.s_addr);
    if (!lease || host<UINT32_C(0xa9fe0100) || host>UINT32_C(0xa9fefeff) ||
            lease->state!=HUB_LEASE_IDLE || lease->configured || lease->queued ||
            hub_public_lease_poll(lease,now)<0 || lease->failed) return -1;
    struct hub_public_dhcp_event candidate={.kind=HUB_DHCP_BOUND,.address=address,
        .mask={.s_addr=htonl(UINT32_C(0xffff0000))},.broadcast={.s_addr=htonl(UINT32_C(0xa9feffff))}};
    if (hub_public_lease_begin(lease,&candidate,UINT64_MAX)<0) return -1;
    lease->origin=HUB_ADDRESS_LINK_LOCAL;lease->seen_grant=0;lease->latest_grant=0;return 0;
}
static int hub_public_lease_offer(struct hub_public_network_lease *lease,
        const struct hub_public_dhcp_event *offer,uint64_t grant_at,uint64_t now) {
    if (!lease || !offer || hub_public_lease_poll(lease,now)<0 || lease->failed) return -1;
    if (offer->kind>=HUB_DHCP_DECONFIG) { hub_public_lease_retire(lease);return 0; }
    if (offer->kind>HUB_DHCP_RENEW || !offer->lease_seconds || offer->lease_seconds>2147483 ||
            grant_at>now || (lease->seen_grant && grant_at<=lease->latest_grant)) return -1;
    uint64_t duration=(uint64_t)offer->lease_seconds*1000;
    if (grant_at>UINT64_MAX-duration || grant_at+duration<=now) return -1;
    uint64_t expires=grant_at+duration;
    lease->latest_grant=grant_at;lease->seen_grant=1;
    if ((lease->state==HUB_LEASE_CANDIDATE || lease->state==HUB_LEASE_READY) &&
            hub_public_lease_same(&lease->current,offer)) {
        lease->current=*offer;lease->expires=expires;return 0;
    }
    if (lease->state==HUB_LEASE_IDLE) return hub_public_lease_begin(lease,offer,expires);
    lease->next=*offer;lease->next_expires=expires;lease->queued=1;
    if (lease->state!=HUB_LEASE_REMOVING) lease->state=HUB_LEASE_DRAINING;
    return 0;
}
static int hub_public_lease_configured(struct hub_public_network_lease *lease,uint64_t generation,uint64_t now,
        const struct hub_public_acd *arp) {
    if (hub_public_lease_poll(lease,now)<0 || lease->failed || generation!=lease->generation ||
            lease->state!=HUB_LEASE_CANDIDATE || lease->configured || !arp ||
            arp->state!=HUB_ACD_CLAIMING || arp->pending!=HUB_ACD_CLAIM ||
            memcmp(arp->address,&lease->current.address.s_addr,4)) return -1;
    lease->configured=1;return 0;
}
static int hub_public_lease_activate(struct hub_public_network_lease *lease,uint64_t generation,
        uint64_t now,const struct hub_public_acd *arp) {
    if (hub_public_lease_poll(lease,now)<0 || lease->failed || generation!=lease->generation ||
            lease->state!=HUB_LEASE_CANDIDATE || !lease->configured || !hub_public_acd_ready(arp) ||
            memcmp(arp->address,&lease->current.address.s_addr,4)) return -1;
    lease->state=HUB_LEASE_READY;return 0;
}
static int hub_public_lease_drained(struct hub_public_network_lease *lease,uint64_t generation,uint64_t now) {
    if (hub_public_lease_poll(lease,now)<0 || generation!=lease->generation || lease->state!=HUB_LEASE_DRAINING) return -1;
    lease->state=HUB_LEASE_REMOVING;return 0;
}
static int hub_public_lease_removed(struct hub_public_network_lease *lease,uint64_t generation,uint64_t now) {
    if (hub_public_lease_poll(lease,now)<0 || generation!=lease->generation || lease->state!=HUB_LEASE_REMOVING) return -1;
    lease->configured=0;lease->state=HUB_LEASE_IDLE;
    if (!lease->queued || lease->failed) return 0;
    lease->queued=0;return hub_public_lease_begin(lease,&lease->next,lease->next_expires);
}
static int hub_public_lease_ready(struct hub_public_network_lease *lease,uint64_t now) {
    return hub_public_lease_poll(lease,now)==0 && !lease->failed && lease->state==HUB_LEASE_READY;
}
#endif

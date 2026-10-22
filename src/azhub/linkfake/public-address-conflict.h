/* SPDX-License-Identifier: GPL-3.0-only */
/* RFC5227 candidate probing. No sockets, interface mutation or sleeping.
 * The caller supplies monotonic milliseconds and independent uniform jitter,
 * configures only after CLAIM, and commits only successfully transmitted ARPs.
 * Any conflict retires this candidate; the allocator must withdraw services,
 * remove its address and enforce RFC5227 retry rate limits across candidates.
 * This component does not acquire a DHCP lease or choose an IPv4LL address. */
#ifndef HUB_PUBLIC_ADDRESS_CONFLICT_H
#define HUB_PUBLIC_ADDRESS_CONFLICT_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
enum hub_public_acd_action { HUB_ACD_NONE, HUB_ACD_PROBE, HUB_ACD_CLAIM, HUB_ACD_ANNOUNCE };
enum hub_public_acd_state { HUB_ACD_PROBING, HUB_ACD_CLAIMING, HUB_ACD_ANNOUNCING,
    HUB_ACD_ACTIVE, HUB_ACD_CONFLICT, HUB_ACD_STOPPED };
struct hub_public_acd {
    unsigned char address[4],mac[6];
    uint64_t deadline,last;
    unsigned probes,announcements;
    enum hub_public_acd_state state;
    enum hub_public_acd_action pending;
};
/* Retained per owned interface across ALL candidate addresses. Reset only on
 * explicit interface lifecycle reset, never merely because an address changes.
 * After ten conflicts, permit at most one new attempt per60 seconds. */
struct hub_public_acd_retry {
    uint64_t last,next,not_before;
    unsigned conflicts;
    int exhausted;
};
static int hub_public_acd_retry_begin(struct hub_public_acd_retry *retry,uint64_t now) {
    if (!retry || retry->exhausted || now<retry->last || now<retry->not_before ||
            (retry->conflicts>=10 && now<retry->next)) return -1;
    if (now>UINT64_MAX-60000) { retry->exhausted=1; return -1; }
    retry->last=now; retry->next=now+60000; return 0;
}
/* Invoke once per retired conflicting candidate, not once per received ARP. */
static int hub_public_acd_retry_conflict(struct hub_public_acd_retry *retry,uint64_t now) {
    if (!retry || retry->exhausted || now<retry->last) return -1;
    retry->last=now;
    if (retry->conflicts<10) ++retry->conflicts;
    if (retry->conflicts>=10) {
        if (now>UINT64_MAX-60000) { retry->exhausted=1; return -1; }
        retry->next=now+60000;
    }
    return 0;
}
/* DHCP requires a separate quiet interval after DECLINE. Call this instead of
 * retry_conflict for that candidate, so one conflict is counted only once.
 * Ten or more conflicts retain the stricter60-second interface backoff. */
static int hub_public_acd_retry_declined(struct hub_public_acd_retry *retry,uint64_t now) {
    if (hub_public_acd_retry_conflict(retry,now)<0) return -1;
    unsigned delay=retry->conflicts>=10 ? 60000:10000;
    if (now>UINT64_MAX-delay) { retry->exhausted=1;return -1; }
    retry->not_before=now+delay;return 0;
}
static uint64_t hub_public_acd_after(uint64_t now,unsigned delay) {
    return now>UINT64_MAX-delay ? UINT64_MAX : now+delay;
}
static int hub_public_acd_init(struct hub_public_acd *acd,const unsigned char ip[4],
        const unsigned char mac[6],uint64_t now,unsigned jitter_ms) {
    if (!acd || !ip || !mac || jitter_ms>1000 || now>UINT64_MAX-1000 || !ip[0] || ip[0]==127 || ip[0]>=224 ||
            (mac[0]&1) || !memcmp(mac,"\0\0\0\0\0\0",6)) return -1;
    memset(acd,0,sizeof *acd); memcpy(acd->address,ip,4); memcpy(acd->mac,mac,6);
    acd->deadline=hub_public_acd_after(now,jitter_ms); acd->last=now;
    acd->state=HUB_ACD_PROBING; return 0;
}
static enum hub_public_acd_action hub_public_acd_poll(struct hub_public_acd *acd,uint64_t now) {
    if (!acd || now<acd->last) return HUB_ACD_NONE;
    acd->last=now;
    if (acd->state==HUB_ACD_CONFLICT || acd->state==HUB_ACD_STOPPED ||
            acd->state==HUB_ACD_ACTIVE) return HUB_ACD_NONE;
    if (acd->pending) return acd->pending;
    if (now<acd->deadline) return HUB_ACD_NONE;
    if (acd->state==HUB_ACD_PROBING) {
        if (acd->probes<3) acd->pending=HUB_ACD_PROBE;
        else { acd->state=HUB_ACD_CLAIMING; acd->pending=HUB_ACD_CLAIM; }
    } else if (acd->state==HUB_ACD_ANNOUNCING) acd->pending=HUB_ACD_ANNOUNCE;
    return acd->pending;
}
/* CLAIM completion means interface configuration succeeded; no public service
 * is ready yet. A failed transmission/configuration leaves its action pending.
 * probe_jitter_ms is uniform0..1000, used only between the first three probes. */
static int hub_public_acd_complete(struct hub_public_acd *acd,enum hub_public_acd_action action,
        uint64_t now,unsigned probe_jitter_ms) {
    if (!acd || !action || action!=acd->pending || now<acd->last || probe_jitter_ms>1000) return -1;
    /* A saturated deadline cannot establish a real waiting interval. Retire
     * rather than permit several probes/announcements at UINT64_MAX. */
    if (now>UINT64_MAX-2000) { acd->state=HUB_ACD_STOPPED; acd->pending=HUB_ACD_NONE; return -1; }
    acd->last=now; acd->pending=HUB_ACD_NONE;
    if (action==HUB_ACD_PROBE) {
        ++acd->probes;
        acd->deadline=hub_public_acd_after(now,acd->probes==3 ? 2000 : 1000+probe_jitter_ms);
    } else if (action==HUB_ACD_CLAIM) {
        acd->state=HUB_ACD_ANNOUNCING; acd->deadline=now;
    } else if (action==HUB_ACD_ANNOUNCE) {
        ++acd->announcements;
        if (acd->announcements==2) acd->state=HUB_ACD_ACTIVE;
        else acd->deadline=hub_public_acd_after(now,2000);
    }
    return 0;
}
/* ARP payload only, starting at hrd; Ethernet/VLAN framing belongs to adapter.
 * local_mac is the caller's inventory of ALL host interface MACs, not merely
 * the selected virtual interface. It prevents echoed probes from other local
 * interfaces from being misclassified. A foreign claim always wins; we elect
 * RFC5227 section2.4(a), immediate retirement rather than optional defense. */
static int hub_public_acd_observe(struct hub_public_acd *acd,const void *packet,size_t size,
        const unsigned char (*local_mac)[6],size_t local_count) {
    if (!acd || !packet || size<28 || (local_count && !local_mac) ||
            acd->state==HUB_ACD_CONFLICT || acd->state==HUB_ACD_STOPPED) return 0;
    const unsigned char *p=packet;
    if (p[0] || p[1]!=1 || p[2]!=8 || p[3] || p[4]!=6 || p[5]!=4 ||
            p[6] || (p[7]!=1 && p[7]!=2)) return 0;
    int own=!memcmp(p+8,acd->mac,6);
    for (size_t i=0;!own && i<local_count;++i) own=!memcmp(p+8,local_mac[i],6);
    if (own) return 0;
    int claim=!memcmp(p+14,acd->address,4);
    int probing=acd->state==HUB_ACD_PROBING || acd->state==HUB_ACD_CLAIMING;
    int probe=probing && p[7]==1 && !memcmp(p+14,"\0\0\0\0",4) &&
        !memcmp(p+24,acd->address,4);
    if (!claim && !probe) return 0;
    acd->state=HUB_ACD_CONFLICT; acd->pending=HUB_ACD_NONE; return 1;
}
static int hub_public_acd_ready(const struct hub_public_acd *acd) {
    return acd && (acd->state==HUB_ACD_ACTIVE ||
        (acd->state==HUB_ACD_ANNOUNCING && acd->announcements));
}
static void hub_public_acd_stop(struct hub_public_acd *acd) {
    if (acd) { acd->state=HUB_ACD_STOPPED; acd->pending=HUB_ACD_NONE; }
}
static int hub_public_acd_packet(const struct hub_public_acd *acd,unsigned char out[28]) {
    if (!acd || !out || (acd->pending!=HUB_ACD_PROBE && acd->pending!=HUB_ACD_ANNOUNCE)) return -1;
    memset(out,0,28);out[1]=1;out[2]=8;out[4]=6;out[5]=4;out[7]=1;
    memcpy(out+8,acd->mac,6);memcpy(out+24,acd->address,4);
    if (acd->pending==HUB_ACD_ANNOUNCE) memcpy(out+14,acd->address,4);
    return 0;
}
#endif

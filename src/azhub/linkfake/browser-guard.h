/* SPDX-License-Identifier: GPL-3.0-only */
/* Native UI thread only. Retain the newest waveform-only update until the
 * current hub request finishes. The event destructor hook invalidates records
 * before their handler/agent can go away. */
static fn8 o_browser_update,o_browser_reply,o_browser_event_destroy;
#define HUB_RENEWALS 16
struct hub_renewal { uint64_t handler,current,agent,event; };
static __thread struct hub_renewal hub_renewals[HUB_RENEWALS];
#ifndef HUB_DESTROY_RENEWAL
static void hub_destroy_renewal(uint64_t event) {
    ((void (*)(uint64_t))(*(uint64_t **)event)[1])(event);
}
#define HUB_DESTROY_RENEWAL(event) hub_destroy_renewal(event)
#endif
static void hub_drop_renewal(int i) {
    uint64_t event=hub_renewals[i].event;
    memset(&hub_renewals[i],0,sizeof hub_renewals[i]);
    if (event) HUB_DESTROY_RENEWAL(event);
}
static int hub_pending_agent(uint64_t agent) {
    return agent && *(uint64_t *)agent==0x3161140 &&
           *(uint32_t *)(agent+0x30) && !*(uint8_t *)(agent+0x34);
}
static void hub_flush_renewals(void) {
    for (int i=0;i<HUB_RENEWALS;i++) {
        struct hub_renewal item=hub_renewals[i];
        if (!item.event) continue;
        /* A current event is still alive here: its base destructor removes
         * the record before releasing its request agent or handler context. */
        if (*(uint64_t *)(item.handler+0x10)!=item.current ||
            *(uint64_t *)(item.current+0x28)!=item.agent) {
            hub_drop_renewal(i);
            continue;
        }
        if (hub_pending_agent(item.agent)) continue;
        memset(&hub_renewals[i],0,sizeof hub_renewals[i]);
        lf_log("hub deferred waveform renewal applied handler=%#lx",item.handler);
        o_browser_update(item.handler,(uint64_t)&item.event,0,0,0,0,0,0);
        if (item.event) HUB_DESTROY_RENEWAL(item.event);
    }
}
static uint64_t h_browser_reply(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    uint64_t result=o_browser_reply(a,b,c,d,e,f,g,h);
    hub_flush_renewals();
    return result;
}
static uint64_t h_browser_event_destroy(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    for (int i=0;i<HUB_RENEWALS;i++)
        if (hub_renewals[i].event && hub_renewals[i].current==a) hub_drop_renewal(i);
    return o_browser_event_destroy(a,b,c,d,e,f,g,h);
}
static uint64_t h_browser_update(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    uint64_t incoming=b ? *(uint64_t *)b : 0;
    uint64_t current=*(uint64_t *)(a+0x10);
    uint64_t agent=current ? *(uint64_t *)(current+0x28) : 0;
    int defer=0;
    if (incoming && *(uint64_t *)incoming==0x3165bf0 && hub_pending_agent(agent)) {
        uint64_t media=*(uint64_t *)(agent+0x28);
        if (((*(uint64_t *)(incoming+0x98)^media)&0x0000ffffffffffffULL)==0)
            for (int i=0;i<nfakes;i++) {
                uint64_t identity=((uint64_t)(0x100|media_sdid(fakes[i].slot))<<32)|((fakes[i].player-1)<<8)|2;
                if (fakes[i].present && identity==(media&0x0000ffffffffffffULL)) defer=1;
            }
    }
    int free_slot=-1;
    for (int i=0;i<HUB_RENEWALS;i++) {
        /* Navigation discards the old repaint. Repeated repaint notifications
         * coalesce, preserving only the latest style/media snapshot. */
        if (hub_renewals[i].event && hub_renewals[i].handler==a) hub_drop_renewal(i);
        if (!hub_renewals[i].event && free_slot<0) free_slot=i;
    }
    if (defer) {
        if (free_slot<0) {
            lf_log("hub waveform renewal queue full; retaining pending request");
            return 0; /* Caller retains/deletes incoming; never cancel its list. */
        }
        hub_renewals[free_slot]=(struct hub_renewal){a,current,agent,incoming};
        *(uint64_t *)b=0; /* Transfer ownership from the caller into our queue. */
        lf_log("hub waveform renewal deferred media=%#lx",*(uint64_t *)(agent+0x28));
        return 0;
    }
    return o_browser_update(a,b,c,d,e,f,g,h);
}

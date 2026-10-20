/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HUB_NATIVE_DETACH_H
#define HUB_NATIVE_DETACH_H
/* Exact-firmware bridge. Pool/proof operations hold safe_lock; native calls,
 * waits and logging never hold it. Native task completion is forwarded before
 * publishing completion to the worker. */
static uintptr_t safe_native_interface(uintptr_t *api_out,uintptr_t *entry_out) {
    uintptr_t controller=safe_controller;
    uintptr_t api=controller ? *(uintptr_t *)(controller+0x5d0):0;
    if (!api || *(uintptr_t *)api!=0x29e96f0) return 0;
    uintptr_t entry=*(uintptr_t *)(*(uintptr_t *)api+0x10),owner=api-0xd8;
    if (entry!=0xa59e78 || *(uintptr_t *)owner!=0x29e9610) return 0;
    if (api_out) *api_out=api;
    if (entry_out) *entry_out=entry;
    return owner;
}
static void safe_cached_forget_locked(int deck) {
    if (deck<0 || deck>=4 || !safe_cached_decks[deck].valid) return;
    hub_detach_release(&safe_detach_pool,safe_cached_decks[deck].ticket);
    safe_cached_decks[deck].valid=0;
}
static int safe_cached_valid_locked(int deck,uint64_t media,int check_media) {
    if (deck<0 || deck>=4 || !safe_cached_decks[deck].valid) return 0;
    int index=safe_cached_decks[deck].ticket;
    if (index<0 || index>=HUB_DETACH_SLOTS) return 0;
    const struct hub_detach_ticket *ticket=&safe_detach_pool.slots[index];
    if (ticket->phase!=HUB_DETACH_DONE || (check_media && ticket->media!=hub_detach_media(media))) return 0;
    for (int i=0;i<16;i++) if (safe_loads[i].controller==safe_cached_decks[deck].controller &&
        safe_loads[i].deck==deck && !safe_loads[i].loading && (safe_loads[i].epoch==safe_cached_decks[deck].epoch || (check_media && safe_loads[i].failed))) return 1;
    return 0;
}
static int safe_cached_known(int deck) {
    uintptr_t owner=safe_native_interface(NULL,NULL);
    if (!owner) return 0;
    pthread_mutex_lock(&safe_lock);
    int known=safe_cached_valid_locked(deck,0,0) && safe_detach_pool.slots[safe_cached_decks[deck].ticket].owner==owner;
    pthread_mutex_unlock(&safe_lock);
    return known;
}
static uint64_t safe_detach_complete_hook(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    if (!lf_runtime_ready()) return safe_original_detach_complete(a,b,c,d,e,f,g,h);
    /* Native AsyncTask header and onDisconnectCompleted payload, exact tag only.
     * Its flags choose native PCM/cache cleanup, not a generic success result. */
    uintptr_t owner=*(uintptr_t *)(a+0x20);
    uint64_t serial=*(uint64_t *)(a+0x10),media=0;
    memcpy(&media,(void *)(a+0x28),6);
    uint64_t result=safe_original_detach_complete(a,b,c,d,e,f,g,h);
    pthread_mutex_lock(&safe_lock);
    int index=hub_detach_complete(&safe_detach_pool,owner,media,serial);
    pthread_mutex_unlock(&safe_lock);
    if (index>=0) lf_core->log(EJECT_ID,"safe-eject native-complete ticket=%d media=%#lx serial=%llu",index,media,(unsigned long long)serial);
    return result;
}
/* This connection token is local to this process, independent of a source label,
 * pathname, content ID or recycled native source slot. Caller holds safe_lock. */
static uint64_t safe_native_connection_locked(int source) {
    for (int i=0;i<SAFE_RECORDS;i++) if ((safe_records[i].item.state==1 || safe_records[i].item.state==3) &&
        (safe_records[i].sources&(1u<<source))) return safe_records[i].connection;
    return 0;
}
static int safe_native_detach(unsigned sources,int *started,char *reason,size_t size) {
    if (!sources) return 0;
    uintptr_t controller=safe_controller,api=0,entry=0;
    uintptr_t owner=safe_native_interface(&api,&entry);
    if (!owner) {
        snprintf(reason,size,"Cannot identify the USB disconnect service. Leave it connected.");return -1;
    }
    int indices[4],count=0,new_proofs=0,failed=0;
    for (int deck=0;deck<4;deck++) {
        uint64_t id[2]={0};unsigned char kind=0;
        int loaded=((unsigned char (*)(uintptr_t,int,void *,void *))0xbc5ed0)(controller,deck,id,&kind);
        if (!loaded) continue; /* safe_busy checks unknown SDK-loaded states. */
        int source=safe_source_identity(id[0]);
        if (source<0 || !(sources&(1u<<source))) continue;
        pthread_mutex_lock(&safe_lock);
        if (safe_cached_valid_locked(deck,id[0],1) && safe_detach_pool.slots[safe_cached_decks[deck].ticket].owner==owner) {
            pthread_mutex_unlock(&safe_lock);continue; /* Already retained from an earlier connection. */
        }
        /* Native deck unload can finish before the final cache notification.
         * A retry must resume that pending ticket even though the load ledger
         * has already cleared its source. */
        if (safe_cached_decks[deck].valid) {
            int old=safe_cached_decks[deck].ticket;
            struct hub_detach_ticket *ticket=&safe_detach_pool.slots[old];
            int same_load=0;
            for (int i=0;i<16;i++) if (safe_loads[i].controller==safe_cached_decks[deck].controller &&
                safe_loads[i].epoch==safe_cached_decks[deck].epoch && !safe_loads[i].loading) same_load=1;
            if (same_load && ticket->phase==HUB_DETACH_PENDING && ticket->owner==owner &&
                ticket->media==hub_detach_media(id[0]) && ticket->connection==safe_native_connection_locked(source) &&
                !hub_detach_retain(&safe_detach_pool,old)) {
                indices[count++]=old;pthread_mutex_unlock(&safe_lock);continue;
            }
        }
        struct safe_load *load=NULL;
        for (int i=0;i<16;i++) if (safe_loads[i].controller && safe_loads[i].deck==deck &&
            safe_loads[i].source==source && !safe_loads[i].loading && !safe_loads[i].failed) load=&safe_loads[i];
        int origin_matches=0;
        if(load) for(int i=0;i<SAFE_RECORDS;i++)
            if(probe_load_origin_matches(load->origin_valid,&load->origin,
                &safe_records[i].item,safe_records[i].sources,source)) origin_matches=1;
        if(!origin_matches) load=NULL;
        uint64_t connection=safe_native_connection_locked(source);
        uint64_t fence=__atomic_load_n((uint64_t *)(owner+0x20),__ATOMIC_ACQUIRE);
        int index=(load && load->epoch) ? hub_detach_acquire(&safe_detach_pool,owner,id[0],connection,fence):-1;
        if (index<0 || !load || !load->epoch) {
            pthread_mutex_unlock(&safe_lock);
            snprintf(reason,size,"Cannot identify the loaded track on deck %d. Leave USB connected.",deck+1);
            failed=1;goto cleanup;
        }
        indices[count++]=index;
        safe_cached_forget_locked(deck);
        if (hub_detach_retain(&safe_detach_pool,index)) {
            pthread_mutex_unlock(&safe_lock);snprintf(reason,size,"USB playback tracking is full.");failed=1;goto cleanup;
        }
        safe_cached_decks[deck].controller=load->controller;
        safe_cached_decks[deck].epoch=load->epoch;
            safe_cached_decks[deck].content_id=load->content_id;
        safe_cached_decks[deck].ticket=index;
        safe_cached_decks[deck].valid=1;new_proofs|=1<<deck;
        pthread_mutex_unlock(&safe_lock);
    }
    for (int i=0;i<count;i++) {
        int index=indices[i];
        pthread_mutex_lock(&safe_lock);
        int submit=hub_detach_submit(&safe_detach_pool,index);
        uint64_t media=safe_detach_pool.slots[index].media;
        if (safe_detach_pool.slots[index].submitted) *started=1;
        pthread_mutex_unlock(&safe_lock);
        if (submit<0) {snprintf(reason,size,"USB playback tracking changed. Leave it connected.");failed=1;goto cleanup;}
        if (submit) {
            lf_core->log(EJECT_ID,"safe-eject native-detach ticket=%d media=%#lx",index,media);
            ((void (*)(uintptr_t,const void *,unsigned))entry)(api,&media,0);
        }
    }
    uint64_t deadline=eject_clock_ms()+60000;
    for (;;) {
        int pending=0;
        pthread_mutex_lock(&safe_lock);
        for (int i=0;i<count;i++) if (safe_detach_pool.slots[indices[i]].phase!=HUB_DETACH_DONE) pending=1;
        pthread_mutex_unlock(&safe_lock);
        if (!pending) break;
        if (eject_clock_ms()>deadline) {
            snprintf(reason,size,"USB tracks are still closing. Leave it connected and retry.");failed=1;goto cleanup;
        }
        usleep(100000);
    }
cleanup:
    pthread_mutex_lock(&safe_lock);
    /* Failed preparation cancels only work that was never submitted. A timeout
     * leaves the pending ticket/proof alive so retry cannot queue it twice. */
    if (failed) for (int deck=0;deck<4;deck++) if (new_proofs&(1<<deck)) {
        int index=safe_cached_decks[deck].ticket;
        if (!safe_detach_pool.slots[index].submitted) safe_cached_forget_locked(deck);
    }
    for (int i=0;i<count;i++) {
        hub_detach_release(&safe_detach_pool,indices[i]);
        hub_detach_cancel_unsubmitted(&safe_detach_pool,indices[i]);
    }
    pthread_mutex_unlock(&safe_lock);
    return failed ? -1:0;
}
#endif

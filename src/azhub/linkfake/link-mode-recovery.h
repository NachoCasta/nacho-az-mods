/* SPDX-License-Identifier: GPL-3.0-only */
/* Exact-firmware mode adapter. Observe the native deferred mode task's
 * completion; never infer the user's setting from the mod-maintained cache.
 * The native callback only publishes an epoch. Injector-owned announcement
 * state is updated on the injector thread, outside an active safe eject.
 */
static fn8 probe_mode_complete_original;
static fn8 probe_mode_request_original,probe_mode_network_original;
static int probe_mode_transition_active;
static pthread_mutex_t probe_mode_cache_mutex=PTHREAD_MUTEX_INITIALIZER;
static unsigned char probe_network_mode;
static int probe_network_mode_known;
static void probe_mode_maintain_cache(void) {
    /* Serialize the entire check/write with transition entry. An atomic
     * busy check alone permits a previously admitted writer to run later. */
    pthread_mutex_lock(&probe_mode_cache_mutex);
    if (!probe_mode_transition_active) community_enable_internal_link_unlocked();
    pthread_mutex_unlock(&probe_mode_cache_mutex);
}
static uint64_t probe_mode_request(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    uintptr_t owner=*(uintptr_t *)(a+0x18);
    pthread_mutex_lock(&probe_mode_cache_mutex);
    if (*(unsigned char *)(owner+0x4a1) || !*(unsigned char *)(owner+0x4a0) ||
        *(unsigned char *)(owner+0x4a4)!=*(unsigned char *)(a+0x20))
        probe_mode_transition_active=1;
    pthread_mutex_unlock(&probe_mode_cache_mutex);
    return probe_mode_request_original(a,b,c,d,e,f,g,h);
}
static uint64_t probe_mode_network(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    /* Preserve the native setter's change detection despite the separate
     * library-cache override. The shadow comes from native requests, never
     * from polling the cache as a proxy for the persisted user setting. */
    if(probe_network_mode_known)
        *(volatile unsigned char *)0x3f86a08=probe_network_mode;
    uint64_t result=probe_mode_network_original(a,b,c,d,e,f,g,h);
    probe_network_mode=(unsigned char)b;probe_network_mode_known=1;
    return result;
}
static uint64_t probe_mode_epoch;
static unsigned probe_mode_value;
static uint64_t probe_mode_complete(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    uintptr_t owner=*(uintptr_t *)(a+0x18);
    uint64_t result=probe_mode_complete_original(a,b,c,d,e,f,g,h);
    if(lf_runtime_ready() && owner && !*(unsigned char *)(owner+0x4a1) &&
        !*(unsigned char *)(owner+0x4a3)) {
        pthread_mutex_lock(&probe_mode_cache_mutex);
        probe_mode_transition_active=0;
        unsigned mode=*(unsigned char *)(owner+0x4a4);
        __atomic_store_n(&probe_mode_value,mode,__ATOMIC_RELAXED);
        uint64_t epoch=__atomic_add_fetch(&probe_mode_epoch,1,__ATOMIC_RELEASE);
        pthread_mutex_unlock(&probe_mode_cache_mutex);
        lf_log("mode recovery: native completion mode=%u epoch=%llu",mode,(unsigned long long)epoch);
    }
    return result;
}
static void probe_mode_reset_sources(time_t *ready_at,int *waiting) {
    static uint64_t handled;
    /* Keep transition entry out until the replay state update is committed.
     * Lock order: mode cache, then source; neither lock spans native calls. */
    pthread_mutex_lock(&probe_mode_cache_mutex);
    uint64_t epoch=__atomic_load_n(&probe_mode_epoch,__ATOMIC_ACQUIRE);
    if(probe_mode_transition_active || !epoch || epoch==handled) {
        pthread_mutex_unlock(&probe_mode_cache_mutex);
        return;
    }
    unsigned mode=__atomic_load_n(&probe_mode_value,__ATOMIC_RELAXED);
    pthread_mutex_lock(&safe_source_mutex);
    for(int i=0;i<nfakes;i++) if((fakes[i].eject_reserved && !fakes[i].eject_done) ||
        fakes[i].unmount_pending) {
        pthread_mutex_unlock(&safe_source_mutex);
        pthread_mutex_unlock(&probe_mode_cache_mutex);
        return;
    }
    *waiting=-1;*ready_at=time(NULL)+2;
    for(int i=0;i<nfakes;i++) {
        struct fake *source=&fakes[i];
        /* The supported five-source transition withdraws USB2/USB3 rows too.
         * Replay every present owned slot after native completion; the epoch
         * guard prevents repeated replay of the same completed transition.
         * Reserved/ejected sources must remain withdrawn. */
        if(source->eject_reserved || !source->present) continue;
        __atomic_store_n(&source->link_ready,0,__ATOMIC_RELEASE);
        __atomic_store_n(&source->announced_at,0,__ATOMIC_RELEASE);
        __atomic_store_n(&source->queried_at,0,__ATOMIC_RELEASE);
        __atomic_store_n(&source->notify_pending,1,__ATOMIC_RELEASE);
    }
    handled=epoch;
    pthread_mutex_unlock(&safe_source_mutex);
    pthread_mutex_unlock(&probe_mode_cache_mutex);
    lf_log("mode recovery: announcements invalidated mode=%u epoch=%llu",
        mode,(unsigned long long)epoch);
}
static void probe_mode_recovery_install(void) {
    probe_mode_complete_original=make_hook(0x140a088,probe_mode_complete);
    probe_mode_request_original=make_hook(0x1408ef0,probe_mode_request);
    probe_mode_network_original=make_hook(0x13fa7a8,probe_mode_network);
}

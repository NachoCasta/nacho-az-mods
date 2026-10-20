/* SPDX-License-Identifier: GPL-3.0-only */
/* Included by the exact-firmware mode adapter after its state declarations. */
#include "link-member-retention.h"
#ifndef LF_MEMBER_CALL_SITE
#define LF_MEMBER_CALL_SITE() ((uintptr_t)__builtin_return_address(0))
#endif
#ifndef LF_MEMBER_STRING_RELEASE
#define LF_MEMBER_STRING_RELEASE(value) ((void (*)(void *))0x2469aa0)(value)
#endif
static fn8 probe_member_disconnect_original,probe_member_clear_original;
static uint32_t probe_member_active_snapshot(void) {
    if(!lf_runtime_ready()) return 0;
    pthread_mutex_lock(&probe_mode_cache_mutex);
    int active=probe_mode_transition_active;
    pthread_mutex_unlock(&probe_mode_cache_mutex);
    if(!active) return 0;
    uint32_t mask=0;
    pthread_mutex_lock(&safe_source_mutex);
    for(int i=0;i<nfakes;i++) {
        struct fake *source=&fakes[i];
        if((source->player==7 || source->player==8) && source->present &&
            !source->eject_reserved && !source->unmount_pending)
            mask|=UINT32_C(1)<<source->player;
    }
    pthread_mutex_unlock(&safe_source_mutex);
    return mask;
}
static int probe_member_table(uintptr_t owner,uintptr_t *begin,size_t *count) {
    uintptr_t first=*(uintptr_t *)(owner+0x380),last=*(uintptr_t *)(owner+0x388);
    uintptr_t capacity=*(uintptr_t *)(owner+0x390);
    if(!first || last<first || capacity<last || (last-first)%HUB_MEMBER_RECORD_SIZE ||
        last-first>HUB_MEMBER_RETENTION_MAX*HUB_MEMBER_RECORD_SIZE) return 0;
    *begin=first;*count=(last-first)/HUB_MEMBER_RECORD_SIZE;
    return 1;
}
static uint64_t probe_member_disconnect(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    uintptr_t caller=LF_MEMBER_CALL_SITE();
    /* Only the two native mode-setter call sites qualify. Ordinary timeout,
     * peer loss and safe-eject paths retain their original behavior. */
    uint32_t active=(caller==0x13fa9b4 || caller==0x13fafec) && b?
        probe_member_active_snapshot():0;
    int retain=0;
    if(active) {
        pthread_mutex_t *mutex=(pthread_mutex_t *)(a+0x3c8);
        pthread_mutex_lock(mutex);
        uintptr_t begin;size_t count;
        if(probe_member_table(a,&begin,&count)) for(size_t i=0;i<count;i++) {
            const unsigned char *row=(const unsigned char *)(begin+i*HUB_MEMBER_RECORD_SIZE);
            if(row[1]==*(unsigned char *)b && hub_member_is_owned(row,active)) retain=1;
        }
        pthread_mutex_unlock(mutex);
    }
    if(retain) { lf_log("mode continuity: retained member disconnect number=%u",*(unsigned char *)b);return 0; }
    return probe_member_disconnect_original(a,b,c,d,e,f,g,h);
}
static void probe_member_release_record(void *record) {
    LF_MEMBER_STRING_RELEASE((unsigned char *)record+0x40);
    LF_MEMBER_STRING_RELEASE((unsigned char *)record+0x30);
}
static uint64_t probe_member_clear(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    uint32_t active=LF_MEMBER_CALL_SITE()==0x13f0770?
        probe_member_active_snapshot():0;
    if(!active) return probe_member_clear_original(a,b,c,d,e,f,g,h);
    /* -fexceptions makes cleanup run if a native listener unwinds through this
     * C frame. No native callbacks run under the table or mod state locks. */
    struct hub_member_removed removed __attribute__((cleanup(hub_member_removed_release)))=
        {.release_record=probe_member_release_record};
    pthread_mutex_t *mutex=(pthread_mutex_t *)(a+0x3c8);
    pthread_mutex_lock(mutex);
    uintptr_t begin;size_t count,kept=0;
    int result=probe_member_table(a,&begin,&count)?
        hub_member_partition((unsigned char *)begin,count,active,&removed,&kept):-1;
    uint32_t retained=0;
    if(result==1) {
        *(uintptr_t *)(a+0x388)=begin+kept*HUB_MEMBER_RECORD_SIZE;
        for(size_t i=0;i<kept;i++) retained|=UINT32_C(1)<<
            *(unsigned char *)(begin+i*HUB_MEMBER_RECORD_SIZE+1);
    }
    pthread_mutex_unlock(mutex);
    if(result!=1) return probe_member_clear_original(a,b,c,d,e,f,g,h);
    __atomic_store_n(&probe_mode_retained_mask,retained,__ATOMIC_RELEASE);
    for(size_t row=0;row<removed.count;row++) {
        /* Match native reverse listener order and re-read registration state
         * after each callback; listener registration may change during dispatch. */
        for(int i=*(int *)(a+0x3c0)-1;i>=0;i--) {
            if(i>=*(int *)(a+0x3c0)) continue;
            uintptr_t listener=(*(uintptr_t **)(a+0x3b0))[i];
            fn8 notify=*(fn8 *)(*(uintptr_t *)listener+0x28);
            notify(listener,(uintptr_t)removed.records[row],b&255,0,0,0,0,0);
        }
    }
    lf_log("mode continuity: retained mask=%#x removed external=%zu",retained,removed.count);
    return 0;
}
static void probe_member_continuity_install(void) {
    probe_member_disconnect_original=make_hook(0x13e0448,probe_member_disconnect);
    probe_member_clear_original=make_hook(0x13e18d8,probe_member_clear);
}

/* SPDX-License-Identifier: GPL-3.0-only */
/* Every virtual library owns its TaggedTrackCacheService as well as its SQL
 * cache. Native cache methods still receive the same LocalMediaID internally;
 * receiver selection prevents mount/unmount from touching another stick. */
static void *hub_tag_cache_originals[8] __attribute__((used));
static pthread_mutex_t hub_tag_create_lock=PTHREAD_MUTEX_INITIALIZER;
__attribute__((used,noinline)) static uint64_t hub_tag_cache_this(uint64_t original,uint64_t method,uint64_t caller) {
    char name[32]={0};
    pthread_getname_np(pthread_self(),name,sizeof name);
    for (int i=0;i<nfakes;i++) {
        char music[24],info[24];
        snprintf(music,sizeof music,"HubLib%d",i);
        snprintf(info,sizeof info,"HubInfo%d",i);
        if (strcmp(name,music) && strcmp(name,info) &&
            !(hub_info_library && hub_info_library==fakes[i].srv_music)) continue;
        uint64_t cache=fakes[i].tag_cache;
        if (cache) return cache;
        pthread_mutex_lock(&hub_tag_create_lock);
        cache=fakes[i].tag_cache;
        if (!cache) {
            cache=(uint64_t)calloc(1,0xc0);
            if (cache) {
                /* Prefix of native ctor1828db8, before its GLOBAL service-map
                 * registration. Private instances must not replace that map. */
                *(uint64_t *)cache=0x2c6d0f8;
                ((void (*)(uint64_t))0x2489f50)(cache+8);
                *(uint64_t *)(cache+0xb8)=100;
                fakes[i].tag_cache=cache;
                lf_log("private tagged cache source=%d service=%#lx original=%#lx method=%lu caller=%#lx thread=%s",i,cache,original,method,caller,name);
            }
        }
        pthread_mutex_unlock(&hub_tag_create_lock);
        return cache ? cache : original;
    }
    return original;
}
#include "tag-cache-abi.h"
static void hub_tag_cache_install(void) {
    /* The first three methods have a 16-byte source-kind check/early RET
     * which the SDK relocator refuses. Hook the body before its prologue;
     * x0/receiver and real arguments are still intact at these boundaries. */
    const uintptr_t addresses[]={0x1829d38,0x1828978,0x1829620,0x1829990,0x18282d0,0x18285b0,0x1827ff0,0x1827d20};
    void *entries[]={hub_tag_0,hub_tag_1,hub_tag_2,hub_tag_3,hub_tag_4,hub_tag_5,hub_tag_6,hub_tag_7};
    for (unsigned i=0;i<8;i++) hub_tag_cache_originals[i]=make_hook(addresses[i],entries[i]);
}

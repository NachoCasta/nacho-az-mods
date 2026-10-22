/* SPDX-License-Identifier: GPL-3.0-only */
/* Explicit emulator research: observe native metadata; do not modify it. */
#ifndef HUB_SOURCE_PROPERTIES_NATIVE_H
#define HUB_SOURCE_PROPERTIES_NATIVE_H
#include "source-properties.h"
struct hub_native_property_blob {unsigned char bytes[0x94];};
typedef struct hub_native_property_blob (*hub_native_property_fn)(uintptr_t);
static hub_native_property_fn hub_native_property_original;
static unsigned hub_native_property_observations;
static fn8 hub_native_mount_task_original;
static fn8 hub_native_storage_update_original;
static fn8 hub_native_storage_replace_original;
static unsigned hub_native_storage_updates;
static uint64_t hub_native_storage_replace_observe(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
                                                   uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    if (lf_runtime_ready() && b && c &&
        __atomic_fetch_add(&hub_native_storage_updates,1,__ATOMIC_RELAXED)<32) {
        const uintptr_t *info=(const uintptr_t *)(uintptr_t)c;
        lf_log("native storage replace manager=%#lx media=%#lx info=%#lx owner=%#lx caller=%p",
            (unsigned long)a,(unsigned long)*(const uint64_t *)(uintptr_t)b,
            (unsigned long)info[0],(unsigned long)info[1],__builtin_return_address(0));
    }
    return hub_native_storage_replace_original(a,b,c,d,e,f,g,h);
}
static uint64_t hub_native_storage_update_observe(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
                                                  uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    /* Preserve all native arguments and ownership. The update takes a pointer
     * to a shared information object in x4; observing its two words does not
     * acquire, release or mutate that object. */
    if (lf_runtime_ready() && e &&
        __atomic_fetch_add(&hub_native_storage_updates,1,__ATOMIC_RELAXED)<32) {
        const uintptr_t *info=(const uintptr_t *)(uintptr_t)e;
        lf_log("native storage update manager=%#lx storage=%#lx member=%#lx flags=%#lx info=%#lx owner=%#lx caller=%p",
            (unsigned long)a,(unsigned long)b,(unsigned long)c,(unsigned long)d,
            (unsigned long)info[0],(unsigned long)info[1],__builtin_return_address(0));
    }
    return hub_native_storage_update_original(a,b,c,d,e,f,g,h);
}
/* AArch64 returns this aggregate through x8, matching SqliteMountPoint's native
 * getter. The original owns the mount point and fills a plain 0x94-byte value. */
static struct hub_native_property_blob hub_native_property_observe(uintptr_t mount) {
    struct hub_native_property_blob result=hub_native_property_original(mount);
    if (mount && lf_runtime_ready() &&
        __atomic_fetch_add(&hub_native_property_observations,1,__ATOMIC_RELAXED)<32) {
        struct hub_source_properties properties;
        hub_source_properties_decode(result.bytes,sizeof result.bytes,&properties);
        const char *path=jstr(mount+0x10);int source=-1;
        for (int i=0;i<nfakes;i++) if (fakes[i].present && !strcmp(fakes[i].dir,path)) {source=i;break;}
        lf_log("native source properties mount=%#lx source=%d path=%.255s name_valid=%d name=%.127s color_valid=%d color=%u",
            (unsigned long)mount,source,path,properties.name_valid,properties.name,
            properties.color_valid,properties.color);
    }
    return result;
}
/* Observe after this source's native mount task, on its own library worker.
 * The information server retains the mount point. The next queued unmount
 * cannot run until this task returns. No native USB server is consulted. */
static uint64_t hub_native_mount_task_observe(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
                                             uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    uint64_t result=hub_native_mount_task_original(a,b,c,d,e,f,g,h);
    if (!a || !lf_runtime_ready()) return result;
    uintptr_t server=*(uintptr_t *)(uintptr_t)a;
    int source=-1;
    for (int i=0;i<nfakes;i++) if (fakes[i].present && fakes[i].srv_music==server) {source=i;break;}
    if (source<0) return result;
    uintptr_t commands=*(uintptr_t *)(server+0x1d0);
    uintptr_t info=commands ? *(uintptr_t *)(commands+0x10):0;
    uintptr_t point=info ? *(uintptr_t *)info:0;
    if (!point || *(uintptr_t *)point!=0x2baf7e8 || strcmp(jstr(point+0x10),fakes[source].dir))
        return result;
    (void)hub_native_property_observe(point);
    return result;
}
static int hub_native_property_observer_install(void) {
    hub_native_property_original=(hub_native_property_fn)make_hook(0x134f210,(fn8)hub_native_property_observe);
    hub_native_mount_task_original=make_hook(0x1376c98,hub_native_mount_task_observe);
    hub_native_storage_update_original=make_hook(0x111d6a0,hub_native_storage_update_observe);
    hub_native_storage_replace_original=make_hook(0x111d2a0,hub_native_storage_replace_observe);
    return hub_native_property_original && hub_native_mount_task_original &&
        hub_native_storage_update_original && hub_native_storage_replace_original ? 0:-93;
}
#endif

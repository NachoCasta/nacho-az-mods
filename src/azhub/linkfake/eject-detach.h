/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HUB_EJECT_DETACH_H
#define HUB_EJECT_DETACH_H
#include <stdint.h>
#include <limits.h>
#define HUB_DETACH_SLOTS 16
#define HUB_DETACH_PENDING 1
#define HUB_DETACH_DONE 2
/* Callers serialize every operation. Full opaque MediaID preserves byte 5:
 * physical source matching may normalize it, asynchronous completion must not.
 * Native task serials belong to one verified controller/process. */
struct hub_detach_ticket {
    uintptr_t owner;
    uint64_t media,connection,fence;
    unsigned references;
    int phase,submitted;
};
struct hub_detach_pool { struct hub_detach_ticket slots[HUB_DETACH_SLOTS]; };
static uint64_t hub_detach_media(uint64_t media) { return media & UINT64_C(0x0000ffffffffffff); }
/* An old unload callback must not clear a newer request's admission guard.
 * State zero is the native branch that actually notifies unload completion. */
static int hub_detach_unload_matches(int native_state,int loading,uint64_t active,uint64_t completed) {
    return native_state==0 && !loading && hub_detach_media(active) && hub_detach_media(active)==hub_detach_media(completed);
}
/* One reference belongs to the waiting worker. Cached deck proofs retain their
 * own references. A timeout releases the worker but never discards pending work. */
static int hub_detach_acquire(struct hub_detach_pool *pool,uintptr_t owner,uint64_t media,uint64_t connection,uint64_t fence) {
    media=hub_detach_media(media);
    if (!owner || !media || !connection || fence==UINT64_MAX) return -1;
    int free_slot=-1,same_media=-1;
    for (int i=0;i<HUB_DETACH_SLOTS;i++) {
        struct hub_detach_ticket *slot=&pool->slots[i];
        if (slot->phase && slot->owner==owner && slot->media==media) {
            if (slot->connection==connection) {
                if (slot->references==UINT_MAX) return -1;
                slot->references++;return i;
            }
            if (slot->phase==HUB_DETACH_PENDING || slot->references) return -1;
            same_media=i;
        }
        if (!slot->phase || (slot->phase==HUB_DETACH_DONE && !slot->references))
            if (free_slot<0) free_slot=i;
    }
    if (same_media>=0) free_slot=same_media;
    if (free_slot<0) return -1;
    pool->slots[free_slot]=(struct hub_detach_ticket){.owner=owner,.media=media,.connection=connection,.fence=fence,.references=1,.phase=HUB_DETACH_PENDING};
    return free_slot;
}
static int hub_detach_retain(struct hub_detach_pool *pool,int index) {
    if (index<0 || index>=HUB_DETACH_SLOTS) return -1;
    struct hub_detach_ticket *slot=&pool->slots[index];
    if (!slot->phase || slot->references==UINT_MAX) return -1;
    slot->references++;return 0;
}
static void hub_detach_release(struct hub_detach_pool *pool,int index) {
    if (index>=0 && index<HUB_DETACH_SLOTS && pool->slots[index].references)
        pool->slots[index].references--;
}
/* Publish submitted before calling the native enqueue entry, which can finish
 * on another thread before returning. Retry never queues the same ticket twice. */
static int hub_detach_submit(struct hub_detach_pool *pool,int index) {
    if (index<0 || index>=HUB_DETACH_SLOTS || !pool->slots[index].phase) return -1;
    struct hub_detach_ticket *slot=&pool->slots[index];
    if (slot->submitted || slot->phase==HUB_DETACH_DONE) return 0;
    slot->submitted=1;return 1;
}
static void hub_detach_cancel_unsubmitted(struct hub_detach_pool *pool,int index) {
    if (index<0 || index>=HUB_DETACH_SLOTS) return;
    struct hub_detach_ticket *slot=&pool->slots[index];
    if (slot->phase==HUB_DETACH_PENDING && !slot->submitted && !slot->references)
        *slot=(struct hub_detach_ticket){0};
}
static int hub_detach_complete(struct hub_detach_pool *pool,uintptr_t owner,uint64_t media,uint64_t serial) {
    media=hub_detach_media(media);
    for (int i=0;i<HUB_DETACH_SLOTS;i++) {
        struct hub_detach_ticket *slot=&pool->slots[i];
        if (slot->phase==HUB_DETACH_PENDING && slot->submitted && slot->owner==owner && slot->media==media && serial>slot->fence) {
            slot->phase=HUB_DETACH_DONE;return i;
        }
    }
    return -1;
}
#endif

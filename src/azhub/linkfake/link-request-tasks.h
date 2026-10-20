/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef LINK_REQUEST_TASKS_H
#define LINK_REQUEST_TASKS_H
#include <stdint.h>
#include <string.h>
/* Callers serialize access. Never hold that lock across firmware calls.
 * A registry entry owns a value copy, not a pointer into native task storage.
 */
#define PROBE_REQUEST_TASKS 64
struct probe_request_scope {
    uintptr_t controller;
    uint64_t media;
    struct eject_item origin;
    int valid;
    uint64_t epoch;
    unsigned char load_type;
    uint32_t content_id;
    uintptr_t id_owner;
    uint64_t id_epoch;
};
struct probe_request_task {
    uintptr_t task;
    uint64_t serial;
    struct probe_request_scope scope;
};
struct probe_request_tasks { struct probe_request_task entries[PROBE_REQUEST_TASKS]; };
/* Controllers live for the native process lifetime. Epochs distinguish even
 * identical locations submitted again while an older task is still queued. */
struct probe_request_head { uintptr_t controller; uint64_t epoch; struct probe_request_scope scope; };
struct probe_request_heads { struct probe_request_head entries[16]; };
struct probe_id_operation { uintptr_t owner; uint64_t epoch; int pending; struct probe_request_scope retry; };
struct probe_id_operations { struct probe_id_operation entries[16]; };
static uint64_t probe_id_begin(struct probe_id_operations *ops,uintptr_t owner) {
    for(int i=0;i<16;i++) if(ops->entries[i].owner==owner || !ops->entries[i].owner) {
        struct probe_id_operation *op=&ops->entries[i];
        if(!owner || op->epoch==UINT64_MAX) return 0;
        op->owner=owner;op->pending=0;
        return ++op->epoch;
    }
    return 0;
}
static int probe_id_defer(struct probe_id_operations *ops,const struct probe_request_scope *scope) {
    for(int i=0;i<16;i++) if(ops->entries[i].owner==scope->id_owner &&
        scope->id_epoch && ops->entries[i].epoch==scope->id_epoch) {
        ops->entries[i].pending=1;ops->entries[i].retry=*scope;return 1;
    }
    return 0;
}
static uint64_t probe_id_epoch(const struct probe_id_operations *ops,uintptr_t owner) {
    for(int i=0;i<16;i++) if(ops->entries[i].owner==owner) return ops->entries[i].epoch;
    return 0;
}
static uint64_t probe_request_next(struct probe_request_heads *heads,uintptr_t controller) {
    for(int i=0;i<16;i++) if(heads->entries[i].controller==controller || !heads->entries[i].controller) {
        struct probe_request_head *head=&heads->entries[i];
        if(head->epoch==UINT64_MAX || !controller) return 0;
        head->controller=controller;
        return ++head->epoch;
    }
    return 0;
}
static int probe_request_current(const struct probe_request_heads *heads,
    const struct probe_request_scope *scope) {
    for(int i=0;i<16;i++) if(heads->entries[i].controller==scope->controller)
        return scope->epoch && heads->entries[i].epoch==scope->epoch;
    return 0;
}
/* The full native media instance is retained here, unlike source selection,
 * which intentionally strips its instance bits. A newer head with a different
 * key cannot admit an older download record merely because content IDs match. */
static int probe_request_remember(struct probe_request_heads *heads,
    const struct probe_request_scope *scope) {
    for(int i=0;i<16;i++) if(heads->entries[i].controller==scope->controller &&
        scope->epoch && heads->entries[i].epoch==scope->epoch) {
        struct probe_request_scope *old=&heads->entries[i].scope;
        /* Native media-instance reuse across physical replacements is ambiguous.
         * Preserve that ambiguity as an admission failure rather than allowing
         * a newer equal-key request to lend its physical identity to old work. */
        int collision=old->epoch && old->media==scope->media &&
            old->content_id==scope->content_id &&
            (!old->valid || !scope->valid || old->origin.generation!=scope->origin.generation ||
             old->origin.devnum!=scope->origin.devnum || strcmp(old->origin.usb,scope->origin.usb));
        *old=*scope;
        if(collision) old->valid=0;
        return !collision;
    }
    return 0;
}
static int probe_request_admission(const struct probe_request_heads *heads,
    uintptr_t controller,uint64_t media,uint32_t content,struct probe_request_scope *out) {
    for(int i=0;i<16;i++) if(heads->entries[i].controller==controller) {
        const struct probe_request_scope *scope=&heads->entries[i].scope;
        if(!scope->valid || !scope->epoch || scope->epoch!=heads->entries[i].epoch ||
            scope->media!=media || scope->content_id!=content) return 0;
        if(out) *out=*scope;
        return 1;
    }
    return 0;
}
static uint64_t probe_request_task_serial(const struct probe_request_tasks *pool,
    uintptr_t task,uintptr_t controller,uint64_t media) {
    for(int i=0;i<PROBE_REQUEST_TASKS;i++) {
        const struct probe_request_task *entry=&pool->entries[i];
        if(task && entry->task==task && entry->scope.controller==controller &&
            entry->scope.media==media) return entry->serial;
    }
    return 0;
}
static int probe_request_task_bind(struct probe_request_tasks *pool,uintptr_t task,
    uint64_t serial,const struct probe_request_scope *scope) {
    if(!task || !serial || !scope || !scope->controller) return 0;
    int free_slot=-1;
    for(int i=0;i<PROBE_REQUEST_TASKS;i++) {
        if(pool->entries[i].task==task) return 0;
        if(!pool->entries[i].task && free_slot<0) free_slot=i;
    }
    if(free_slot<0) return 0;
    pool->entries[free_slot]=(struct probe_request_task){task,serial,*scope};
    return 1;
}
static int probe_request_task_take(struct probe_request_tasks *pool,uintptr_t task,
    uint64_t serial,uintptr_t controller,struct probe_request_scope *out) {
    for(int i=0;i<PROBE_REQUEST_TASKS;i++) {
        struct probe_request_task *entry=&pool->entries[i];
        if(entry->task!=task || entry->serial!=serial ||
            entry->scope.controller!=controller || !task) continue;
        if(out) *out=entry->scope;
        memset(entry,0,sizeof *entry);
        return 1;
    }
    return 0;
}
/* Move a pre-reserved slot to a native task atomically under the caller's lock.
 * The reservation prevents concurrent posting from consuming its capacity. */
static int probe_request_task_rebind(struct probe_request_tasks *pool,uintptr_t reservation,
    uintptr_t controller,uintptr_t task,uint64_t serial) {
    if(!task || !serial) return 0;
    for(int i=0;i<PROBE_REQUEST_TASKS;i++) {
        struct probe_request_task *entry=&pool->entries[i];
        if(entry->task==reservation && entry->serial==1 && entry->scope.controller==controller) {
            entry->task=task;entry->serial=serial;return 1;
        }
    }
    return 0;
}
static int probe_request_task_id_take(struct probe_request_tasks *pool,uintptr_t task,
    uint64_t serial,uintptr_t owner,struct probe_request_scope *out) {
    for(int i=0;i<PROBE_REQUEST_TASKS;i++) {
        struct probe_request_task *entry=&pool->entries[i];
        if(task && entry->task==task && entry->serial==serial && entry->scope.id_owner==owner) {
            if(out) *out=entry->scope;
            memset(entry,0,sizeof *entry);return 1;
        }
    }
    return 0;
}
#endif

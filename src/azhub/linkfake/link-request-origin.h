/* SPDX-License-Identifier: GPL-3.0-only */
/* Native request provenance and targeted Instant Doubles cancellation.
 * Final request-head admission is independent from early task cancellation.
 * Accepted-operation epochs prevent delayed cleanup from cancelling newer work.
 * Native post/free/destructor paths retain task and allocation ownership. */
static fn8 probe_queue_common_original,probe_queue_allow_original;
static fn8 probe_queue_post_original;
#include "link-request-tasks.h"
static fn8 probe_queue_destroy_original,probe_queue_delete_original;
static fn8 probe_allow_destroy_original,probe_allow_delete_original;
static struct probe_request_tasks probe_request_tasks;
static struct probe_request_heads probe_request_heads;
static struct probe_id_operations probe_id_operations;
static fn8 probe_id_start_original,probe_queue_pop_original;
static fn8 probe_id_failure_destroy_original,probe_id_failure_delete_original;
static __thread const struct probe_request_scope *probe_id_cancel_scope;
static pthread_mutex_t probe_request_tasks_lock=PTHREAD_MUTEX_INITIALIZER;
/* Submission scopes are thread-local and restored after reentrant submission.
 * Overflow removes early cancellation; the independent final head guard still
 * requires captured physical provenance before admitting any hub request.
 * Async task ownership is retained by task, native serial and controller.
 * Task registry exhaustion loses early cancellation only. Final admission does
 * not trust registry membership or a late read of the current physical source.
 */
static __thread struct probe_request_scope probe_request_scopes[8];
static __thread unsigned probe_request_depth;
static uintptr_t probe_id_owner(uintptr_t controller) {
    if(!controller) return 0;
    int count=*(int *)(controller+0xc80);
    uintptr_t *listeners=*(uintptr_t **)(controller+0xc70);
    if(listeners && count>=0 && count<=32) for(int i=0;i<count;i++) {
        uintptr_t listener=listeners[i],table=listener ? *(uintptr_t *)listener:0;
        if(table && *(uintptr_t *)(table+0x10)==0xaad328 && listener>=0xe0 &&
            *(uintptr_t *)(listener-0xe0+0x110)==controller) return listener-0xe0;
    }
    return 0;
}
static int probe_queue_origin(uint64_t media,struct eject_item *origin) {
    int source=safe_source_identity(media);
    char path[256]={0};
    pthread_mutex_lock(&safe_source_mutex);
    if(source>=0 && source<nfakes && fakes[source].present)
        snprintf(path,sizeof path,"%s",fakes[source].dir);
    pthread_mutex_unlock(&safe_source_mutex);
    return *path && safe_usb_identity(path,origin);
}
static void probe_request_begin(uintptr_t controller,uintptr_t track,uintptr_t options) {
    unsigned depth=probe_request_depth++;
    if(depth>=8) return;
    struct probe_request_scope *scope=&probe_request_scopes[depth];
    memset(scope,0,sizeof *scope);
    scope->controller=controller;scope->media=*(uint64_t *)track;
    scope->content_id=*(uint32_t *)(track+8);
    scope->load_type=options ? *(unsigned char *)options:0;
    if(scope->load_type==5) {
        scope->id_owner=probe_id_owner(controller);
        pthread_mutex_lock(&probe_request_tasks_lock);
        scope->id_epoch=probe_id_epoch(&probe_id_operations,scope->id_owner);
        pthread_mutex_unlock(&probe_request_tasks_lock);
    }
    pthread_mutex_lock(&probe_request_tasks_lock);
    scope->epoch=probe_request_next(&probe_request_heads,controller);
    pthread_mutex_unlock(&probe_request_tasks_lock);
    scope->valid=probe_queue_origin(scope->media,&scope->origin);
    pthread_mutex_lock(&probe_request_tasks_lock);
    probe_request_remember(&probe_request_heads,scope);
    pthread_mutex_unlock(&probe_request_tasks_lock);
}
static int probe_request_admission_origin(uintptr_t controller,uintptr_t record,
    struct eject_item *origin) {
    struct probe_request_scope scope={0};
    uint64_t media=*(uint64_t *)(record+0x38);
    uint32_t content=*(uint32_t *)(record+0x40);
    pthread_mutex_lock(&probe_request_tasks_lock);
    int found=probe_request_admission(&probe_request_heads,controller,media,content,&scope);
    pthread_mutex_unlock(&probe_request_tasks_lock);
    struct eject_item now={0};
    int valid=found && probe_queue_origin(media,&now) && safe_same(&scope.origin,&now);
    if(valid) *origin=scope.origin;
    return valid;
}
static int probe_queue_stale(uintptr_t owner,uint64_t media) {
    if(!probe_request_depth || probe_request_depth>8) return 0;
    struct probe_request_scope *scope=&probe_request_scopes[probe_request_depth-1];
    if(scope->controller!=owner || scope->media!=media || !scope->valid) return 0;
    struct eject_item now={0};
    return !probe_queue_origin(media,&now) || !safe_same(&scope->origin,&now);
}
static void probe_id_cancel(const struct probe_request_scope *scope) {
    struct probe_request_scope cancelled=*scope;
    uintptr_t reservation=(uintptr_t)&cancelled;
    pthread_mutex_lock(&probe_request_tasks_lock);
    int current=scope->id_owner && scope->id_epoch &&
        scope->id_epoch==probe_id_epoch(&probe_id_operations,scope->id_owner);
    int reserved=current && probe_request_task_bind(&probe_request_tasks,reservation,1,&cancelled);
    if(current && !reserved) probe_id_defer(&probe_id_operations,scope);
    pthread_mutex_unlock(&probe_request_tasks_lock);
    if(reserved) {
        const struct probe_request_scope *previous=probe_id_cancel_scope;
        probe_id_cancel_scope=&cancelled;
        ((void (*)(uintptr_t))0xaad018)(scope->id_owner);
        probe_id_cancel_scope=previous;
        pthread_mutex_lock(&probe_request_tasks_lock);
        probe_request_task_take(&probe_request_tasks,reservation,1,scope->controller,NULL);
        pthread_mutex_unlock(&probe_request_tasks_lock);
    }
}
static void probe_queue_tick(void) {
    eject_tick();
    if(!lf_runtime_ready()) return;
    for(int i=0;i<16;i++) {
        struct probe_request_scope scope={0};
        pthread_mutex_lock(&probe_request_tasks_lock);
        int pending=probe_id_operations.entries[i].pending;
        if(pending) { scope=probe_id_operations.entries[i].retry;probe_id_operations.entries[i].pending=0; }
        pthread_mutex_unlock(&probe_request_tasks_lock);
        if(pending) probe_id_cancel(&scope);
    }
}
/* No allowance state has been written at either closure entry. Leave the
 * native executor in charge of destroying its payload, and deliver only the
 * existing exact-location browser cleanup. The broad native recording-denied
 * broadcast would also reset an unrelated Instant Doubles operation and enqueue
 * a misleading GUI recording alert. Those listeners are deliberately excluded.
 * Type5 cleanup targets only its associated native loader operation. */
static int probe_queue_cancel(uintptr_t owner,uintptr_t location,uintptr_t options) {
    struct probe_request_scope *scope=&probe_request_scopes[probe_request_depth-1];
    pthread_mutex_lock(&probe_request_tasks_lock);
    int current=probe_request_current(&probe_request_heads,scope);
    pthread_mutex_unlock(&probe_request_tasks_lock);
    if(current && scope->load_type==5) {
        probe_id_cancel(scope);
    }
    int count=*(int *)(owner+0xc80);
    uintptr_t *listeners=*(uintptr_t **)(owner+0xc70);
    if(current && listeners && count>=0 && count<=32) {
        /* The native failed-precheck callback ignores the failure value after
         * comparing the complete location, including its auxiliary vector. */
        unsigned char denied=8;
        for(int i=count-1;i>=0;i--) {
            uintptr_t listener=listeners[i],table=listener ? *(uintptr_t *)listener:0;
            uintptr_t callback=table ? *(uintptr_t *)(table+0x10):0;
            if(callback==0xa96b40) {
                ((void (*)(uintptr_t,uintptr_t,uintptr_t,const unsigned char *))callback)
                    (listener,location,options,&denied);
            }
        }
        safe_message("USB changed while loading. Select the track again.");
    }
    return 1;
}
static uint64_t probe_id_start(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    uint64_t result=probe_id_start_original(a,b,c,d,e,f,g,h);
    /* shouldExecuteInstantDoubles returns true immediately before the caller
     * clears old active state and prepares the new operation. Rejected/no-op
     * attempts must not invalidate an older operation's pending cleanup. */
    if(lf_runtime_ready() && a && (result&0xff)) {
        uintptr_t owner=a;
        pthread_mutex_lock(&probe_request_tasks_lock);
        probe_id_begin(&probe_id_operations,owner);
        pthread_mutex_unlock(&probe_request_tasks_lock);
    }
    return result;
}
static uint64_t probe_queue_pop(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    uintptr_t caller=(uintptr_t)__builtin_return_address(0);
    for(;;) {
        uint64_t result=probe_queue_pop_original(a,b,c,d,e,f,g,h);
        if(!result || !lf_runtime_ready() || caller!=0x237d67c || !b) return result;
        uintptr_t task=*(uintptr_t *)b;
        if(!task || *(uintptr_t *)task!=0x2a04640) return result;
        uintptr_t owner=*(uintptr_t *)(task+0x18);
        uint64_t serial=*(uint64_t *)(task+0x10);
        struct probe_request_scope scope={0};
        pthread_mutex_lock(&probe_request_tasks_lock);
        int captured=probe_request_task_id_take(&probe_request_tasks,task,serial,owner,&scope);
        int current=!captured || scope.id_epoch==probe_id_epoch(&probe_id_operations,owner);
        pthread_mutex_unlock(&probe_request_tasks_lock);
        if(current) return result;
        /* Pop has completed its lock-free CAS and retains no lock. Follow the
         * native executeTasks cancellation branch: freeTask(box,task), without
         * executing or publishing that task's serial as completed work. Native
         * freeTask calls its destructor and matching allocator exactly once. */
        ((void (*)(uintptr_t,uintptr_t))0x237e9f0)(a-0x38,task);
        *(uintptr_t *)b=0;
    }
}
static uint64_t probe_id_failure_destroy(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    pthread_mutex_lock(&probe_request_tasks_lock);
    probe_request_task_id_take(&probe_request_tasks,a,*(uint64_t *)(a+0x10),*(uintptr_t *)(a+0x18),NULL);
    pthread_mutex_unlock(&probe_request_tasks_lock);
    return probe_id_failure_destroy_original(a,b,c,d,e,f,g,h);
}
static uint64_t probe_id_failure_delete(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    pthread_mutex_lock(&probe_request_tasks_lock);
    probe_request_task_id_take(&probe_request_tasks,a,*(uint64_t *)(a+0x10),*(uintptr_t *)(a+0x18),NULL);
    pthread_mutex_unlock(&probe_request_tasks_lock);
    return probe_id_failure_delete_original(a,b,c,d,e,f,g,h);
}
static void probe_request_end(void) {
    if(probe_request_depth) --probe_request_depth;
}
static uint64_t probe_queue_post(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    uint64_t serial=0;uintptr_t root=0;int registered=0;
    if(lf_runtime_ready() && probe_id_cancel_scope && b && *(uintptr_t *)b==0x2a04640 &&
        a==probe_id_cancel_scope->id_owner+8 && *(uintptr_t *)(b+0x18)==probe_id_cancel_scope->id_owner) {
        root=probe_id_cancel_scope->controller;serial=*(uint64_t *)(b+0x10);
        pthread_mutex_lock(&probe_request_tasks_lock);
        registered=probe_request_task_rebind(&probe_request_tasks,(uintptr_t)probe_id_cancel_scope,root,b,serial);
        pthread_mutex_unlock(&probe_request_tasks_lock);
    } else if(lf_runtime_ready() && probe_request_depth && probe_request_depth<=8 && b) {
        struct probe_request_scope *scope=&probe_request_scopes[probe_request_depth-1];
        uintptr_t table=*(uintptr_t *)b;
        int common=table==0x29f7218 && a==scope->controller;
        int allow=table==0x29f0588 && a==scope->controller+0x628;
        uintptr_t owner_offset=common ? 0x238:0x18;
        uintptr_t media_offset=common ? 0x240:0x20;
        if((common || allow) && *(uintptr_t *)(b+owner_offset)==a && scope->media==*(uint64_t *)(b+media_offset)) {
            root=scope->controller;
            serial=*(uint64_t *)(b+0x10);
            pthread_mutex_lock(&probe_request_tasks_lock);
            registered=probe_request_task_bind(&probe_request_tasks,b,serial,scope);
            pthread_mutex_unlock(&probe_request_tasks_lock);
        }
    }
    uint64_t result=probe_queue_post_original(a,b,c,d,e,f,g,h);
    /* Native submission may execute/destroy the task before returning. Only
     * saved scalar keys are used here, never the potentially freed task. */
    if(!result && registered) {
        pthread_mutex_lock(&probe_request_tasks_lock);
        probe_request_task_take(&probe_request_tasks,b,serial,root,NULL);
        pthread_mutex_unlock(&probe_request_tasks_lock);
    }
    return result;
}
static int probe_queue_take(uintptr_t task,int allow,struct probe_request_scope *scope) {
    if(!task) return 0;
    uint64_t serial=*(uint64_t *)(task+0x10);
    uintptr_t controller=allow ? *(uintptr_t *)(task+0x18)-0x628:*(uintptr_t *)(task+0x238);
    pthread_mutex_lock(&probe_request_tasks_lock);
    int found=probe_request_task_take(&probe_request_tasks,task,serial,controller,scope);
    pthread_mutex_unlock(&probe_request_tasks_lock);
    return found;
}
static uint64_t probe_queue_destroy(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    probe_queue_take(a,0,NULL);
    return probe_queue_destroy_original(a,b,c,d,e,f,g,h);
}
static uint64_t probe_queue_delete(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    probe_queue_take(a,0,NULL);
    return probe_queue_delete_original(a,b,c,d,e,f,g,h);
}
static uint64_t probe_allow_destroy(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    probe_queue_take(a,1,NULL);
    return probe_allow_destroy_original(a,b,c,d,e,f,g,h);
}
static uint64_t probe_allow_delete(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    probe_queue_take(a,1,NULL);
    return probe_allow_delete_original(a,b,c,d,e,f,g,h);
}
static uint64_t probe_queue_common_body(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    if(lf_runtime_ready() && a &&
        probe_queue_stale(*(uintptr_t *)(a+0x220),*(uint64_t *)(a+0x228)) &&
        probe_queue_cancel(*(uintptr_t *)(a+0x220),a+0x228,a)) return 0;
    return probe_queue_common_original(a,b,c,d,e,f,g,h);
}
static uint64_t probe_queue_common(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    struct probe_request_scope scope={0};int found=0;
    if(lf_runtime_ready() && a>=0x18) {
        uintptr_t task=a-0x18,owner=*(uintptr_t *)(a+0x220);
        uint64_t media=*(uint64_t *)(a+0x228);
        pthread_mutex_lock(&probe_request_tasks_lock);
        uint64_t serial=probe_request_task_serial(&probe_request_tasks,task,owner,media);
        /* Only inspect the task header after matching a registered allocation.
         * A direct call's stack payload has no preceding task header.
         * The executing native task still owns the registered allocation here.
         */
        if(serial && serial==*(uint64_t *)(task+0x10))
            found=probe_request_task_take(&probe_request_tasks,task,serial,owner,&scope);
        pthread_mutex_unlock(&probe_request_tasks_lock);
    }
    if(found) {
        unsigned depth=probe_request_depth++;
        if(depth<8) { probe_request_scopes[depth]=scope; }
    }
    uint64_t result=probe_queue_common_body(a,b,c,d,e,f,g,h);
    if(found) probe_request_end();
    return result;
}
static uint64_t probe_queue_allow(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    struct probe_request_scope scope={0};int found=0;
    if(lf_runtime_ready() && a>=0x18) {
        uintptr_t task=a-0x18,owner=*(uintptr_t *)a-0x628;
        uint64_t media=*(uint64_t *)(a+8);
        pthread_mutex_lock(&probe_request_tasks_lock);
        uint64_t serial=probe_request_task_serial(&probe_request_tasks,task,owner,media);
        if(serial && serial==*(uint64_t *)(task+0x10))
            found=probe_request_task_take(&probe_request_tasks,task,serial,owner,&scope);
        pthread_mutex_unlock(&probe_request_tasks_lock);
        if(found) {
            unsigned depth=probe_request_depth++;
            if(depth<8) { probe_request_scopes[depth]=scope; }
        }
        if(probe_queue_stale(*(uintptr_t *)a-0x628,*(uint64_t *)(a+8)) &&
            probe_queue_cancel(*(uintptr_t *)a-0x628,a+8,a+0x48)) {
            if(found) probe_request_end();
            return 0;
        }
    }
    uint64_t result=probe_queue_allow_original(a,b,c,d,e,f,g,h);
    if(found) probe_request_end();
    return result;
}
static void probe_queue_install(void) {
    probe_id_start_original=make_hook(0xaae298,probe_id_start);
    probe_queue_pop_original=make_hook(0x237f900,probe_queue_pop);
    probe_id_failure_destroy_original=make_hook(0xaab598,probe_id_failure_destroy);
    probe_id_failure_delete_original=make_hook(0xaab818,probe_id_failure_delete);
    probe_allow_destroy_original=make_hook(0xa692e0,probe_allow_destroy);
    probe_allow_delete_original=make_hook(0xa693c0,probe_allow_delete);
    probe_queue_destroy_original=make_hook(0xa7c848,probe_queue_destroy);
    probe_queue_delete_original=make_hook(0xa7c9d8,probe_queue_delete);
    probe_queue_post_original=make_hook(0x6d1838,probe_queue_post);
    probe_queue_common_original=make_hook(0xa77390,probe_queue_common);
    probe_queue_allow_original=make_hook(0xa69db0,probe_queue_allow);
}

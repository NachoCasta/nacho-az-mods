/* SPDX-License-Identifier: GPL-3.0-only */
/* Emulator-only request provenance and cancellation experiment.
 * The generator's kernel guard is mandatory.
 * Arm one stage by creating its /run/azhub-queue-delay-{common,allow} marker.
 * A successful unlink consumes the marker once. The final admission guard uses
 * captured request origin; queued task tracing is not its authority.
 */
static fn8 probe_queue_common_original,probe_queue_allow_original;
static fn8 probe_queue_post_original;
static fn8 probe_queue_needs_async_original;
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
/* Diagnostic scopes are thread-local and restored after reentrant submission.
 * Overflow leaves tracing unavailable; the independent final head guard still
 * requires captured physical provenance before admitting any hub request.
 * Async task ownership is retained by task, native serial and controller.
 * Task registry exhaustion loses early cancellation only. Final admission does
 * not trust registry membership or a late read of the current physical source.
 */
static __thread struct probe_request_scope probe_request_scopes[8];
static __thread unsigned probe_request_depth;
static __thread int probe_request_force_async[8];
static void probe_queue_note(const char *,uintptr_t,uintptr_t,uint64_t);
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
    int saved=errno;
    probe_request_force_async[depth]=!unlink("/run/azhub-queue-force-async");
    scope->force_allow_async=!unlink("/run/azhub-queue-force-allow-async");
    errno=saved;
    lf_log("load queue: submission owner=%#lx media=%#lx origin_valid=%d generation=%llu devnum=%u depth=%u type=%u id_owner=%#lx id_epoch=%llu",
        controller,scope->media,scope->valid,(unsigned long long)scope->origin.generation,scope->origin.devnum,depth,
        scope->load_type,scope->id_owner,(unsigned long long)scope->id_epoch);
}
static int probe_request_admission_origin(uintptr_t controller,uintptr_t record,
    struct eject_item *origin) {
    struct probe_request_scope scope={0};
    uint64_t media=*(uint64_t *)(record+0x38);
    uint32_t content=*(uint32_t *)(record+0x40);
    probe_queue_note("final",record,controller,media);
    pthread_mutex_lock(&probe_request_tasks_lock);
    int found=probe_request_admission(&probe_request_heads,controller,media,content,&scope);
    pthread_mutex_unlock(&probe_request_tasks_lock);
    struct eject_item now={0};
    int valid=found && probe_queue_origin(media,&now) && safe_same(&scope.origin,&now);
    if(valid) *origin=scope.origin;
    lf_log("load queue: final provenance=%d owner=%#lx media=%#lx content=%u epoch=%llu origin_generation=%llu current_generation=%llu",
        valid,controller,media,content,(unsigned long long)scope.epoch,
        (unsigned long long)scope.origin.generation,(unsigned long long)now.generation);
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
    lf_log("load queue: Instant Doubles targeted cleanup owner=%#lx epoch=%llu current=%d reserved=%d thread=%#lx active=%u",
        scope->id_owner,(unsigned long long)scope->id_epoch,current,reserved,(unsigned long)pthread_self(),
        scope->id_owner ? *(unsigned char *)(scope->id_owner+0x184):0);
}
static void probe_queue_tick(void) {
    eject_tick();
    if(!lf_runtime_ready()) return;
    /* Diagnostic caller for the isolated emulator. It uses native PlayerID
     * objects and the native asynchronous API; it is not UI-flow acceptance. */
    for(int deck=0;deck<2;deck++) {
        const char *marker=deck ? "/run/azhub-id-load-2-from-1":"/run/azhub-id-load-1-from-2";
        int saved=errno,armed=!unlink(marker);errno=saved;
        if(armed) {
            uintptr_t controllers[2]={0};
            pthread_mutex_lock(&safe_lock);
            for(int j=0;j<16;j++) if(safe_loads[j].controller && safe_loads[j].deck>=0 && safe_loads[j].deck<2)
                controllers[safe_loads[j].deck]=safe_loads[j].controller;
            pthread_mutex_unlock(&safe_lock);
            uintptr_t target=probe_id_owner(controllers[deck]),source=probe_id_owner(controllers[1-deck]);
            lf_log("load queue: diagnostic Instant Doubles target=%#lx source=%#lx",target,source);
            if(target && source) ((void (*)(uintptr_t,uintptr_t))0xaaedb8)(target,source+0x104);
        }
    }
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
    int count=*(int *)(owner+0xc80),notified=0;
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
                notified++;
            }
        }
        safe_message("USB changed while loading. Select the track again.");
    }
    lf_log("load queue: cancelled source-change owner=%#lx media=%#lx epoch=%llu current=%d browser_cleanup=%d",
        owner,scope->media,(unsigned long long)scope->epoch,current,notified);
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
        uint64_t epoch=probe_id_begin(&probe_id_operations,owner);
        pthread_mutex_unlock(&probe_request_tasks_lock);
        lf_log("load queue: Instant Doubles operation owner=%#lx epoch=%llu thread=%#lx",owner,(unsigned long long)epoch,(unsigned long)pthread_self());
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
        lf_log("load queue: Instant Doubles cleanup dequeue owner=%#lx serial=%llu captured=%d current=%d thread=%#lx",
            owner,(unsigned long long)serial,captured,current,(unsigned long)pthread_self());
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
static uint64_t probe_queue_needs_async(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    if(lf_runtime_ready() && probe_id_cancel_scope && a==probe_id_cancel_scope->id_owner+8) {
        int saved=errno;
        int force=!unlink("/run/azhub-queue-force-id-cleanup-async");
        errno=saved;
        if(force) return 1;
    }
    if(lf_runtime_ready() && probe_request_depth && probe_request_depth<=8) {
        unsigned depth=probe_request_depth-1;
        if(probe_request_force_async[depth] && probe_request_scopes[depth].controller==a) {
            probe_request_force_async[depth]=0;
            lf_log("load queue: force native async owner=%#lx",a);
            return 1;
        }
        if(probe_request_scopes[depth].force_allow_async &&
            probe_request_scopes[depth].controller+0x628==a) {
            probe_request_scopes[depth].force_allow_async=0;
            lf_log("load queue: force native allowance async owner=%#lx",a);
            return 1;
        }
    }
    return probe_queue_needs_async_original(a,b,c,d,e,f,g,h);
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
        lf_log("load queue: Instant Doubles cleanup submission task=%#lx serial=%llu epoch=%llu registered=%d",
            b,(unsigned long long)serial,(unsigned long long)probe_id_cancel_scope->id_epoch,registered);
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
            lf_log("load queue: async submission task=%#lx serial=%llu owner=%#lx media=%#lx origin_valid=%d generation=%llu devnum=%u",
                b,(unsigned long long)*(uint64_t *)(b+0x10),a,*(uint64_t *)(b+media_offset),
                scope->valid,(unsigned long long)scope->origin.generation,scope->origin.devnum);
            lf_log("load queue: task registration=%d",registered);
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
static void probe_queue_note(const char *stage,uintptr_t payload,uintptr_t owner,uint64_t media) {
    lf_log("load queue: stage=%s payload=%#lx owner=%#lx media=%#lx",stage,payload,owner,media);
    char marker[80];
    snprintf(marker,sizeof marker,"/run/azhub-queue-delay-%s",stage);
    int saved=errno;
    if(!unlink(marker)) {
        struct eject_item before={0},after={0};
        int before_valid=probe_queue_origin(media,&before);
        lf_log("load queue: delayed stage=%s media=%#lx",stage,media);
        /* Bounded delay, only in the isolated emulator research build. */
        struct timespec remaining={.tv_sec=8};
        while(nanosleep(&remaining,&remaining) && errno==EINTR) {}
        int after_valid=probe_queue_origin(media,&after);
        lf_log("load queue: origin stage=%s before_valid=%d generation=%llu devnum=%u after_valid=%d generation=%llu devnum=%u same=%d",
            stage,before_valid,(unsigned long long)before.generation,before.devnum,
            after_valid,(unsigned long long)after.generation,after.devnum,
            before_valid && after_valid && safe_same(&before,&after));
        lf_log("load queue: resumed stage=%s media=%#lx",stage,media);
    }
    errno=saved;
}
static uint64_t probe_queue_common_body(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    if(lf_runtime_ready() && a) {
        if(probe_request_depth && probe_request_depth<=8) {
            struct probe_request_scope *scope=&probe_request_scopes[probe_request_depth-1];
            lf_log("load queue: common submission_scope=%d origin_valid=%d generation=%llu devnum=%u",
                scope->controller==*(uintptr_t *)(a+0x220) && scope->media==*(uint64_t *)(a+0x228),
                scope->valid,(unsigned long long)scope->origin.generation,scope->origin.devnum);
        }
        probe_queue_note("common",a,*(uintptr_t *)(a+0x220),*(uint64_t *)(a+0x228));
        if(probe_queue_stale(*(uintptr_t *)(a+0x220),*(uint64_t *)(a+0x228)) &&
            probe_queue_cancel(*(uintptr_t *)(a+0x220),a+0x228,a)) return 0;
        /* Explicit fault injection to validate the native rejection contract.
         * This is not an origin check and is never armed by normal operation.
         * Result1 follows the native allowance-denied listener notification.
         * The wrapper copies location/options if it needs to queue the reply.
         */
        int saved=errno;
        int reject=!unlink("/run/azhub-queue-reject-common");
        errno=saved;
        if(reject) {
            uint32_t result=1;
            uintptr_t owner=*(uintptr_t *)(a+0x220);
            /* Enumerate only the exact native pre-load listeners. This trace
             * supports auditing cancellation effects beyond the GUI handler.
             * No guessed result code or callback suppression is introduced.
             */
            int count=*(int *)(owner+0xc80);
            uintptr_t *listeners=*(uintptr_t **)(owner+0xc70);
            if(listeners && count>=0 && count<=32)
                for(int i=0;i<count;i++) {
                    uintptr_t listener=listeners[i];
                    uintptr_t table=listener ? *(uintptr_t *)listener:0;
                    lf_log("load queue: listener index=%d object=%#lx table=%#lx callback=%#lx",
                        i,listener,table,table ? *(uintptr_t *)(table+0x10):0);
                }
            lf_log("load queue: native rejection owner=%#lx media=%#lx",owner,*(uint64_t *)(a+0x228));
            ((void (*)(uintptr_t,uintptr_t,uintptr_t,const uint32_t *))0xa7de38)(owner,a+0x228,a,&result);
            return 0;
        }
    }
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
        if(depth<8) { probe_request_scopes[depth]=scope;probe_request_force_async[depth]=0; }
        lf_log("load queue: execute provenance=1 media=%#lx",scope.media);
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
            if(depth<8) { probe_request_scopes[depth]=scope;probe_request_force_async[depth]=0; }
            lf_log("load queue: allowance execute provenance=1 media=%#lx generation=%llu devnum=%u",
                scope.media,(unsigned long long)scope.origin.generation,scope.origin.devnum);
        }
        probe_queue_note("allow",a,*(uintptr_t *)a,*(uint64_t *)(a+8));
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
    probe_queue_needs_async_original=make_hook(0x6d0bb8,probe_queue_needs_async);
    probe_queue_destroy_original=make_hook(0xa7c848,probe_queue_destroy);
    probe_queue_delete_original=make_hook(0xa7c9d8,probe_queue_delete);
    probe_queue_post_original=make_hook(0x6d1838,probe_queue_post);
    probe_queue_common_original=make_hook(0xa77390,probe_queue_common);
    probe_queue_allow_original=make_hook(0xa69db0,probe_queue_allow);
}

/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#define LF_SAFE_EJECT 1
#define EJECT_MAX 16
#include "eject-ui-model.h"
#include "link-request-tasks.h"
static struct probe_request_tasks pool;
static struct probe_request_heads heads;
static struct probe_id_operations ids;
int main(void) {
    struct probe_request_scope origin={.controller=10,.media=22,.valid=1,
        .origin={.generation=100,.devnum=4,.usb="/sys/usb/2-1.3"}},out={0};
    assert(probe_request_task_bind(&pool,1000,1,&origin));
    assert(probe_request_task_serial(&pool,1000,10,22)==1);
    assert(!probe_request_task_serial(&pool,1000,10,23));
    assert(!probe_request_task_serial(&pool,1000,11,22));
    origin.origin.generation=101;
    assert(!probe_request_task_bind(&pool,1000,2,&origin));
    assert(!probe_request_task_take(&pool,1000,2,10,&out));
    assert(!probe_request_task_take(&pool,1000,1,11,&out));
    assert(probe_request_task_take(&pool,1000,1,10,&out));
    assert(!probe_request_task_serial(&pool,1000,10,22));
    assert(out.origin.generation==100 && out.media==22 && out.valid);
    assert(!probe_request_task_take(&pool,1000,1,10,&out));
    /* Reused task address cannot be cleared by an old cancellation key. */
    assert(probe_request_task_bind(&pool,1000,2,&origin));
    assert(!probe_request_task_take(&pool,1000,1,10,NULL));
    assert(probe_request_task_take(&pool,1000,2,10,&out));
    assert(out.origin.generation==101);
    origin.epoch=probe_request_next(&heads,origin.controller);
    assert(origin.epoch==1 && probe_request_current(&heads,&origin));
    struct probe_request_scope newer=origin;
    newer.epoch=probe_request_next(&heads,newer.controller);
    assert(newer.epoch==2 && probe_request_current(&heads,&newer));
    assert(!probe_request_current(&heads,&origin));
    assert(probe_request_remember(&heads,&newer));
    assert(probe_request_admission(&heads,newer.controller,newer.media,newer.content_id,&out));
    assert(out.origin.generation==newer.origin.generation);
    assert(!probe_request_admission(&heads,newer.controller,newer.media+1,newer.content_id,&out));
    assert(!probe_request_admission(&heads,newer.controller,newer.media,newer.content_id+1,&out));
    newer.epoch=probe_request_next(&heads,newer.controller);
    newer.origin.generation++;
    assert(!probe_request_remember(&heads,&newer));
    assert(!probe_request_admission(&heads,newer.controller,newer.media,newer.content_id,&out));
    /* Equal-key ambiguity stays denied on further retries; a distinct native
     * media instance may establish a fresh submission head. */
    newer.epoch=probe_request_next(&heads,newer.controller);
    assert(!probe_request_remember(&heads,&newer));
    newer.epoch=probe_request_next(&heads,newer.controller);newer.media++;
    assert(probe_request_remember(&heads,&newer));
    assert(probe_request_admission(&heads,newer.controller,newer.media,newer.content_id,&out));
    heads.entries[0].epoch=UINT64_MAX;
    assert(!probe_request_next(&heads,origin.controller));
    assert(!probe_request_current(&heads,&newer));
    for(unsigned i=0;i<PROBE_REQUEST_TASKS;i++)
        assert(probe_request_task_bind(&pool,1000+i,1,&origin));
    assert(!probe_request_task_bind(&pool,2000,1,&origin));
    for(unsigned i=0;i<PROBE_REQUEST_TASKS;i++)
        assert(probe_request_task_take(&pool,1000+i,1,10,NULL));
    for(unsigned i=1;i<10000;i++) {
        assert(probe_request_task_bind(&pool,1000,i,&origin));
        assert(probe_request_task_take(&pool,1000,i,10,&out));
    }
    origin.id_owner=50;origin.id_epoch=probe_id_begin(&ids,50);
    assert(origin.id_epoch==1);
    assert(probe_request_task_bind(&pool,5000,1,&origin));
    /* New operation begins between checking the old operation and posting its
     * cleanup. Even a later native task serial retains the original epoch. */
    assert(probe_id_begin(&ids,50)==2);
    assert(probe_request_task_rebind(&pool,5000,10,6000,100));
    assert(!probe_request_task_take(&pool,5000,1,10,NULL));
    assert(probe_request_task_id_take(&pool,6000,100,50,&out));
    assert(out.id_epoch!=probe_id_epoch(&ids,50));
    assert(!probe_id_defer(&ids,&origin));
    origin.id_epoch=2;
    assert(probe_id_defer(&ids,&origin));
    assert(ids.entries[0].pending && ids.entries[0].retry.id_epoch==2);
    assert(probe_id_begin(&ids,50)==3 && !ids.entries[0].pending);
    origin.id_epoch=3;
    for(unsigned i=0;i<PROBE_REQUEST_TASKS;i++)
        assert(probe_request_task_bind(&pool,1000+i,1,&origin));
    assert(!probe_request_task_bind(&pool,5000,1,&origin));
    assert(probe_id_defer(&ids,&origin));
    assert(probe_request_task_take(&pool,1000,1,10,NULL));
    struct probe_request_scope retry=ids.entries[0].retry;
    assert(probe_request_task_bind(&pool,5000,1,&retry));
    assert(probe_request_task_rebind(&pool,5000,10,6000,101));
    /* Execution and destruction can finish before native post returns. A
     * caller's saved reservation key must not retire another native task. */
    assert(probe_request_task_id_take(&pool,6000,101,50,&out));
    assert(out.id_epoch==probe_id_epoch(&ids,50));
    assert(!probe_request_task_take(&pool,5000,1,10,NULL));
    assert(probe_request_task_bind(&pool,6000,102,&origin));
    assert(!probe_request_task_id_take(&pool,6000,101,50,NULL));
    assert(probe_request_task_id_take(&pool,6000,102,50,NULL));
    for(unsigned i=1;i<PROBE_REQUEST_TASKS;i++)
        assert(probe_request_task_take(&pool,1000+i,1,10,NULL));
    assert(!probe_request_task_bind(&pool,0,1,&origin));
    assert(!probe_request_task_bind(&pool,1000,0,&origin));
    assert(!probe_request_task_bind(&pool,1000,1,NULL));
    origin.controller=0;
    assert(!probe_request_task_bind(&pool,1000,1,&origin));
    puts("request task ownership tests passed");
}

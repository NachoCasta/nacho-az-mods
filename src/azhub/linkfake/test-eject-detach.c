/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "eject-detach.h"
int main(void) {
    struct hub_detach_pool pool={0};
    uint64_t first=UINT64_C(0x010200000202),replacement=UINT64_C(0x020200000202);
    assert(hub_detach_unload_matches(0,0,first,first));
    assert(!hub_detach_unload_matches(0,1,first,first));
    assert(!hub_detach_unload_matches(6,0,first,first));
    assert(!hub_detach_unload_matches(7,1,first,first));
    assert(!hub_detach_unload_matches(0,0,replacement,first));
    assert(!hub_detach_unload_matches(0,0,0,0));
    int a=hub_detach_acquire(&pool,123,first,1,100);assert(a>=0);
    assert(hub_detach_complete(&pool,123,first,101)<0); /* Not submitted yet. */
    assert(hub_detach_submit(&pool,a)==1 && hub_detach_submit(&pool,a)==0);
    assert(hub_detach_complete(&pool,124,first,101)<0); /* Different controller. */
    assert(hub_detach_complete(&pool,123,replacement,101)<0); /* Opaque identity differs. */
    assert(hub_detach_complete(&pool,123,first,100)<0); /* Previously queued completion. */
    hub_detach_release(&pool,a); /* Timeout: callback can still arrive. */
    assert(pool.slots[a].phase==HUB_DETACH_PENDING);
    assert(hub_detach_acquire(&pool,123,first,2,101)<0);
    assert(hub_detach_acquire(&pool,123,first,1,101)==a); /* Retry resumes the same ticket. */
    assert(!hub_detach_retain(&pool,a)); /* Cached deck owns a reference. */
    assert(hub_detach_complete(&pool,123,first,102)==a);
    assert(hub_detach_complete(&pool,123,first,103)<0);
    hub_detach_release(&pool,a);
    assert(hub_detach_acquire(&pool,123,first,2,103)<0); /* Old cached identity is pinned. */
    hub_detach_release(&pool,a);
    int next=hub_detach_acquire(&pool,123,first,2,103);assert(next>=0);assert(hub_detach_submit(&pool,next)==1);
    assert(hub_detach_complete(&pool,123,first,102)<0); /* Delayed event from old connection. */
    assert(hub_detach_complete(&pool,123,first,104)==next);hub_detach_release(&pool,next);
    for (unsigned cycle=3;cycle<100003;cycle++) {
        int slot=hub_detach_acquire(&pool,123,first,cycle,cycle+200);
        assert(slot>=0 && hub_detach_submit(&pool,slot)==1 && hub_detach_complete(&pool,123,first,cycle+201)==slot);
        hub_detach_release(&pool,slot);
    }
    pool=(struct hub_detach_pool){0};
    for (int i=0;i<HUB_DETACH_SLOTS;i++) assert(hub_detach_acquire(&pool,123,first+i,i+1,1)>=0);
    assert(hub_detach_acquire(&pool,123,first+100,100,1)<0);
    assert(hub_detach_acquire(&pool,0,first,1,1)<0);
    assert(hub_detach_acquire(&pool,123,0,1,1)<0);
    assert(hub_detach_acquire(&pool,123,first,1,UINT64_MAX)<0);
    assert(hub_detach_retain(&pool,-1)<0 && hub_detach_retain(&pool,HUB_DETACH_SLOTS)<0);
    pool=(struct hub_detach_pool){0};
    int cancelled=hub_detach_acquire(&pool,123,first,1,1);assert(cancelled>=0);
    hub_detach_release(&pool,cancelled);hub_detach_cancel_unsubmitted(&pool,cancelled);
    assert(pool.slots[cancelled].phase==0);
    cancelled=hub_detach_acquire(&pool,123,first,1,1);assert(hub_detach_submit(&pool,cancelled)==1);
    hub_detach_release(&pool,cancelled);hub_detach_cancel_unsubmitted(&pool,cancelled);
    assert(pool.slots[cancelled].phase==HUB_DETACH_PENDING);
    assert(hub_detach_submit(&pool,-1)<0);
    puts("detach tickets: identities, stale serials, timeout/retry, cached ownership, 100000 cycles and fixed capacity passed");
}

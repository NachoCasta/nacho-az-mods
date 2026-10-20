/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise the actual research adapter with mocked native callbacks. */
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdio.h>
#include <time.h>
#include <sched.h>
#include <string.h>
typedef uint64_t (*fn8)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
static pthread_mutex_t safe_source_mutex=PTHREAD_MUTEX_INITIALIZER;
static struct fake {int eject_reserved,eject_done,unmount_pending,present,slot,link_ready,notify_pending;time_t announced_at,queried_at;} fakes[8];
static int nfakes=8;
static atomic_int native_busy,violations,stop_writer,writes;
static void community_enable_internal_link_unlocked(void) {
    if(atomic_load(&native_busy)) atomic_fetch_add(&violations,1);
    sched_yield();
    if(atomic_load(&native_busy)) atomic_fetch_add(&violations,1);
    atomic_fetch_add(&writes,1);
}
#define lf_log(...) ((void)0)
static int lf_runtime_ready(void){return 1;}
static fn8 make_hook(uintptr_t address,fn8 hook){(void)address;return hook;}
#include "link-mode-recovery.h"
static uint64_t request(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;
    atomic_store(&native_busy,1);sched_yield();return 123;
}
static uint64_t complete(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;
    atomic_store(&native_busy,0);return 456;
}
static void *writer(void *unused) {
    (void)unused;
    while(!atomic_load(&stop_writer)) probe_mode_maintain_cache();
    return NULL;
}
int main(void) {
    _Alignas(8) unsigned char owner[0x500]={0},task[0x28]={0};
    uintptr_t address=(uintptr_t)owner;memcpy(task+0x18,&address,sizeof address);
    task[0x20]=1;owner[0x4a0]=1;
    probe_mode_request_original=request;probe_mode_complete_original=complete;
    pthread_t thread;assert(!pthread_create(&thread,NULL,writer,NULL));
    while(!atomic_load(&writes)) sched_yield();
    for(int i=0;i<20000;i++) {
        assert(probe_mode_request((uintptr_t)task,0,0,0,0,0,0,0)==123);
        assert(probe_mode_complete((uintptr_t)task,0,0,0,0,0,0,0)==456);
    }
    atomic_store(&stop_writer,1);assert(!pthread_join(thread,NULL));
    assert(!atomic_load(&violations));assert(atomic_load(&writes)>0);
    const int slots[]={3,7,3,7,8,8,2,2};
    for(int i=0;i<nfakes;i++) fakes[i]=(struct fake){.present=1,.slot=slots[i],.link_ready=1,.announced_at=9,.queried_at=9};
    time_t ready=77;int waiting=2;
    /* A completed epoch exists, but a newer native transition starts before
     * the injector handles it. Do not publish the old epoch during that work. */
    assert(probe_mode_request((uintptr_t)task,0,0,0,0,0,0,0)==123);
    probe_mode_reset_sources(&ready,&waiting);
    assert(ready==77 && waiting==2 && fakes[0].announced_at==9);
    assert(!fakes[0].notify_pending && fakes[2].link_ready==1);
    assert(probe_mode_complete((uintptr_t)task,0,0,0,0,0,0,0)==456);
    fakes[0].eject_reserved=1;probe_mode_reset_sources(&ready,&waiting);
    assert(ready==77 && waiting==2 && fakes[2].link_ready==1);
    fakes[0].eject_reserved=0;fakes[1].unmount_pending=1;probe_mode_reset_sources(&ready,&waiting);
    assert(ready==77 && waiting==2);
    fakes[1].unmount_pending=0;probe_mode_reset_sources(&ready,&waiting);
    assert(waiting==-1);
    for(int i=0;i<nfakes;i++) {
        assert(fakes[i].notify_pending && !fakes[i].link_ready);
        assert(!fakes[i].announced_at && !fakes[i].queried_at);
    }
    ready=88;waiting=1;probe_mode_reset_sources(&ready,&waiting);
    assert(ready==88 && waiting==1);
    /* Finalized eject keeps its reservation until physical disconnection.
     * It must stay withdrawn without blocking recovery of another source. */
    fakes[0].eject_reserved=1;fakes[0].eject_done=1;fakes[0].present=0;
    fakes[2].present=0;
    fakes[3].eject_reserved=1;fakes[3].eject_done=1;
    for(int i=0;i<nfakes;i++) {
        fakes[i].notify_pending=0;fakes[i].link_ready=1;
        fakes[i].announced_at=9;fakes[i].queried_at=9;
    }
    assert(probe_mode_request((uintptr_t)task,0,0,0,0,0,0,0)==123);
    assert(probe_mode_complete((uintptr_t)task,0,0,0,0,0,0,0)==456);
    probe_mode_reset_sources(&ready,&waiting);
    assert(waiting==-1 && !fakes[0].notify_pending && !fakes[0].present);
    assert(!fakes[2].notify_pending && !fakes[2].present);
    assert(!fakes[3].notify_pending && fakes[3].eject_reserved);
    for(int i=0;i<nfakes;i++) {
        if(i==0 || i==2 || i==3) {
            assert(fakes[i].link_ready && fakes[i].announced_at==9 && fakes[i].queried_at==9);
        } else {
            assert(fakes[i].notify_pending && !fakes[i].link_ready);
            assert(!fakes[i].announced_at && !fakes[i].queried_at);
        }
    }
    puts("PASS: 20000 native transition/write races; stale-epoch/eject deferral and all8 owned-slot replay with absent/reserved exclusion");
}

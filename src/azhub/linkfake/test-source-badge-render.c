/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise production badge callbacks with refcounted stand-ins for native Images. */
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
typedef uint64_t (*fn8)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
static struct { int player,slot; } fakes[8]={{7,3},{7,7},{8,3},{8,7},{7,8},{8,8},{7,2},{8,2}};
static int nfakes=8,ready=1,decode_calls,fail_decode,hook_calls,assign_calls,live_handles;
static int native_base_done,partial_failure,compose_successes;
static unsigned refs[25];
static int lf_runtime_ready(void) { return ready; }
static int safe_source_identity(uint64_t identity) {
    return identity>=100 && identity<(uint64_t)(100+nfakes) ? (int)identity-100:-1;
}
static int compose(int family,int number,uintptr_t *image) {
    assert(family>=0 && family<3 && number>=1 && number<=8);
    assert(native_base_done); /* Never read native resources from init(). */
    ++decode_calls;int id=family*8+number;
    if (id==fail_decode && !partial_failure) { *image=0; return 0; }
    assert(id<=24 && !refs[id]);*image=(uintptr_t)id;refs[id]=1;live_handles++;
    if (id==fail_decode) return 0;
    ++compose_successes;return 1;
}
static void destroy(uintptr_t *image) {
    if (*image && *image<=24) {
        assert(refs[*image]);
        if (!--refs[*image]) live_handles--;
    }
}
static void assign(uintptr_t *destination,const uintptr_t *source) {
    assert(native_base_done && *source && *source<=24 && refs[*source]);
    destroy(destination);*destination=*source;refs[*source]++;assign_calls++;
}
#define HUB_BADGE_COMPOSE compose
#define HUB_BADGE_ASSIGN assign
#define HUB_BADGE_DESTROY destroy
static uint64_t native_callback(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
static uint64_t native_base(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
static uint64_t native_root(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
static fn8 make_hook(uintptr_t address,fn8 replacement) {
    assert(replacement);hook_calls++;
    if (address==0x1c88820) return native_base;
    if (address==0x1c88900) return native_root;
    assert(address==0x1c88618);return native_callback;
}
#include "source-badge-render.h"
struct fixture {
    uintptr_t widget[40],controller[8],closure[2],storage;
    unsigned char description[16];
    int owned,family,number;
};
static struct fixture *current,*nested;
static int base_calls,root_calls,callback_calls;
static void initialize(struct fixture *fixture,int family,int number) {
    memset(fixture,0,sizeof *fixture);
    fixture->widget[0]=family==0 ? 0x2e34e68:family==1 ? 0x2e32658:family==2 ? 0x2e36388:123;
    fixture->controller[5]=(uintptr_t)fixture->widget;
    fixture->closure[0]=(uintptr_t)fixture->controller;
    fixture->closure[1]=(uint64_t)(99+number);
    fixture->storage=(uintptr_t)fixture->closure;
    memset(fixture->description,0xa5,sizeof fixture->description);
    fixture->owned=family>=0 && family<3 && number>=1 && number<=8 && ready;
    fixture->family=family;fixture->number=number;
}
static void check_description(uint64_t controller,uint64_t description) {
    assert(controller==(uintptr_t)current->controller);
    assert(description);
    unsigned char expected[16];memcpy(expected,current->description,sizeof expected);
    if (current->owned && source_badge_images[current->family][current->number-1]) {
        expected[5]=0;memset(expected+8,0,8);
        assert(description!=(uintptr_t)current->description);
    } else assert(description==(uintptr_t)current->description);
    assert(!memcmp((void *)(uintptr_t)description,expected,sizeof expected));
    for (unsigned i=0;i<sizeof current->description;i++) assert(current->description[i]==0xa5);
}
static uint64_t native_base(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    assert(c==3 && d==4 && e==5 && f==6 && g==7 && h==8);
    check_description(a,b);base_calls++;
    destroy(&current->widget[26]);current->widget[26]=1000; /* Native image refresh. */
    native_base_done=1;return 0xbace;
}
static uint64_t native_root(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    assert(c==3 && d==4 && e==5 && f==6 && g==7 && h==8);
    check_description(a,b);root_calls++;return 0xf00d;
}
static void render_current(void) {
    native_base_done=0;int before=assign_calls;
    assert(source_badge_base((uintptr_t)current->controller,(uintptr_t)current->description,3,4,5,6,7,8)==0xbace);
    int composed=current->owned && source_badge_images[current->family][current->number-1];
    assert(assign_calls==before+!!composed);
    uintptr_t expected=composed ? (uintptr_t)(current->family*8+current->number):1000;
    assert(current->widget[26]==expected);
    assert(source_badge_root((uintptr_t)current->controller,(uintptr_t)current->description,3,4,5,6,7,8)==0xf00d);
}
static uint64_t native_callback(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    assert(a==(uintptr_t)&current->storage && b==2 && c==3 && d==4 && e==5 && f==6 && g==7 && h==8);
    callback_calls++;render_current();
    if (nested) {
        struct fixture *outer=current,*inner=nested;nested=NULL;current=inner;
        assert(source_badge_callback((uintptr_t)&inner->storage,2,3,4,5,6,7,8)==0xcafe);
        current=outer;render_current(); /* Inner callback must restore outer ownership. */
    }
    return 0xcafe;
}
static void run(struct fixture *fixture) {
    current=fixture;
    assert(source_badge_callback((uintptr_t)&fixture->storage,2,3,4,5,6,7,8)==0xcafe);
}
static void release_widget(struct fixture *fixture) { destroy(&fixture->widget[26]);fixture->widget[26]=0; }
int main(void) {
    assert(source_badge_install()==-90 && !hook_calls);
    assert(!source_badge_prepare() && !decode_calls && !live_handles);
    assert(!source_badge_prepare() && !decode_calls);
    assert(!source_badge_install() && hook_calls==3);
    struct fixture fixture;
    /* A failed lazy image keeps native fields and never retires a neighbor.
     * Partial decoder outputs are released, and failures are not retried per frame. */
    for (int failure=1;failure<=24;failure++) {
        decode_calls=0;fail_decode=failure;partial_failure=failure&1;
        initialize(&fixture,(failure-1)/8,(failure-1)%8+1);run(&fixture);
        assert(decode_calls==1 && !live_handles && fixture.widget[26]==1000);
        run(&fixture);assert(decode_calls==1 && fixture.widget[26]==1000);
        int neighbor=failure%24+1;
        initialize(&fixture,(neighbor-1)/8,(neighbor-1)%8+1);run(&fixture);
        assert(decode_calls==2 && live_handles==1 && fixture.widget[26]==(uintptr_t)neighbor);
        release_widget(&fixture);source_badge_discard_images();assert(!live_handles);
        for (int i=1;i<=24;i++) assert(!refs[i]);
        assert(!source_badge_prepare());
    }
    decode_calls=0;fail_decode=0;
    /* Another callback's in-flight slot falls back without waiting or composing. */
    source_badge_attempted[0][0]=1;
    initialize(&fixture,0,1);run(&fixture);assert(!decode_calls && fixture.widget[26]==1000);
    source_badge_attempted[0][0]=0;
    /* Every family/number selects a distinct cached handle and owns a reference. */
    for (int family=0;family<3;family++) for (int number=1;number<=8;number++) {
        initialize(&fixture,family,number);run(&fixture);
        assert(refs[family*8+number]==2);release_widget(&fixture);
        assert(live_handles==family*8+number);
    }
    assert(decode_calls==24);
    /* The incoming identity wins over stale controller state, including reuse. */
    initialize(&fixture,0,1);run(&fixture);
    fixture.number=8;fixture.closure[1]=107;run(&fixture);
    assert(refs[1]==1 && refs[8]==2);
    /* Native source and unknown widget paths keep the original description. */
    fixture.owned=0;fixture.closure[1]=999;run(&fixture);assert(fixture.widget[26]==1000);
    release_widget(&fixture);
    initialize(&fixture,3,1);run(&fixture);assert(fixture.widget[26]==1000);
    ready=0;initialize(&fixture,0,1);run(&fixture);assert(fixture.widget[26]==1000);ready=1;
    initialize(&fixture,0,1);fixture.storage=0;fixture.owned=0;run(&fixture);
    assert(fixture.widget[26]==1000);
    initialize(&fixture,1,1);fixture.owned=0;fakes[0].player=99;run(&fixture);
    assert(fixture.widget[26]==1000);fakes[0].player=7;
    /* A nested native identity must not inherit or erase outer hub ownership. */
    struct fixture inner;
    initialize(&fixture,2,4);initialize(&inner,1,0);nested=&inner;run(&fixture);
    assert(fixture.widget[26]==20 && inner.widget[26]==1000);release_widget(&fixture);
    initialize(&fixture,0,2);initialize(&inner,1,7);nested=&inner;run(&fixture);
    assert(fixture.widget[26]==2 && inner.widget[26]==15);
    release_widget(&fixture);release_widget(&inner);
    /* No callback scope means no claim, even for a recognized widget. */
    initialize(&fixture,0,1);fixture.owned=0;current=&fixture;render_current();
    assert(fixture.widget[26]==1000);
    assert(base_calls==root_calls+compose_successes && callback_calls>24 && live_handles==24);
    source_badge_discard_images();assert(!live_handles);
    for (int i=1;i<=24;i++) assert(!refs[i]);
    puts("source badge rendering: resource-free init, deferred composition, native failure fallback, ownership and image lifetime pass");
    return 0;
}

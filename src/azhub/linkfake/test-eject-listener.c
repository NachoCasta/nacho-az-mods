/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include "eject-listener.h"
static uintptr_t vtable[4];
static void *complete_later(void *argument) {
    struct safe_listener *listener=argument;
    assert(listener->vtable==vtable && listener->source==3 && listener->kind==1);
    safe_listener_complete(listener);
    return NULL;
}
int main(void) {
    assert(!safe_listener_acquire(-1,0,vtable));
    assert(!safe_listener_acquire(8,0,vtable));
    assert(!safe_listener_acquire(0,2,vtable));
    struct safe_listener *first[8][2];
    for (int source=0;source<8;source++) for (int kind=0;kind<2;kind++) {
        first[source][kind]=safe_listener_acquire(source,kind,vtable);
        assert(first[source][kind]);
        assert(!safe_listener_acquire(source,kind,vtable));
    }
    /* A timed-out producer cannot reuse a listener until its delayed completion. */
    pthread_t thread;assert(!pthread_create(&thread,NULL,complete_later,first[3][1]));
    assert(!pthread_join(thread,NULL));
    assert(safe_listener_acquire(3,1,vtable)==first[3][1]);
    for (int source=0;source<8;source++) for (int kind=0;kind<2;kind++) safe_listener_complete(first[source][kind]);
    for (unsigned round=0;round<100000;round++)
        for (int source=0;source<8;source++) for (int kind=0;kind<2;kind++) {
            struct safe_listener *listener=safe_listener_acquire(source,kind,vtable);
            assert(listener==first[source][kind] && !listener->done);
            assert(!safe_listener_acquire(source,kind,vtable));
            safe_listener_complete(listener);
        }
    puts("eject listeners: fixed 16 slots, pending retry exclusion, delayed completion, bounds and 100000 reconnect cycles passed");
}

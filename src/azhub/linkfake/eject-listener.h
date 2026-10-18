/* SPDX-License-Identifier: GPL-3.0-only */
/* Fixed storage for one outstanding native unmount per source/server. */
#ifndef HUB_EJECT_LISTENER_H
#define HUB_EJECT_LISTENER_H
#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>
struct safe_listener {
    uintptr_t *vtable;
    _Atomic int done;
    int source,kind,claimed;
};
static struct safe_listener safe_listener_pool[8][2];
/* The eject worker is the only producer. A native task calls its completion
 * once and never dereferences the listener afterward on the exact supported
 * firmware. Keep a pending slot occupied across timeouts/retries. */
static struct safe_listener *safe_listener_acquire(int source,int kind,uintptr_t *vtable) {
    if (source<0 || source>=8 || kind<0 || kind>=2) return NULL;
    struct safe_listener *listener=&safe_listener_pool[source][kind];
    if (listener->claimed && !atomic_load_explicit(&listener->done,memory_order_acquire)) return NULL;
    if (!listener->claimed) {
        listener->vtable=vtable;
        listener->source=source;
        listener->kind=kind;
        listener->claimed=1;
    }
    atomic_store_explicit(&listener->done,0,memory_order_release);
    return listener;
}
static void safe_listener_complete(struct safe_listener *listener) {
    atomic_store_explicit(&listener->done,1,memory_order_release);
}
#endif

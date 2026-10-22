/* SPDX-License-Identifier: GPL-3.0-only */
/* Single-controller operation ordering after channel authentication. Generation
 * reservation precedes asynchronous work; failed operations stay consumed.
 * A drain acknowledgement means worker join AND network resources retired. */
#ifndef HUB_PUBLIC_BROKER_GENERATION_H
#define HUB_PUBLIC_BROKER_GENERATION_H
#include <stdint.h>
#include <errno.h>
enum hub_public_broker_phase { HUB_BROKER_EMPTY,HUB_BROKER_STARTING,
    HUB_BROKER_ACTIVE,HUB_BROKER_DRAINING };
struct hub_public_broker_generation_slot { uint64_t generation;unsigned phase; };
struct hub_public_broker_generations {
    struct hub_public_broker_generation_slot slots[8];
    int stopping;
};
static int hub_public_broker_generation_source(struct hub_public_broker_generations *g,
        unsigned slot,uint64_t generation) {
    if (!g || slot>=8 || !generation) { errno=EINVAL;return -1; }
    struct hub_public_broker_generation_slot *s=g->slots+slot;
    if (g->stopping) { errno=ESHUTDOWN;return -1; }
    if (generation<=s->generation) { errno=ESTALE;return -1; }
    if (s->phase!=HUB_BROKER_EMPTY) { errno=EBUSY;return -1; }
    s->generation=generation;s->phase=HUB_BROKER_STARTING;return 0;
}
static int hub_public_broker_generation_ready(struct hub_public_broker_generations *g,
        unsigned slot,uint64_t generation) {
    if (!g || slot>=8 || !generation) { errno=EINVAL;return -1; }
    struct hub_public_broker_generation_slot *s=g->slots+slot;
    if (s->generation!=generation || s->phase!=HUB_BROKER_STARTING || g->stopping) {
        errno=ESTALE;return -1;
    }
    s->phase=HUB_BROKER_ACTIVE;return 0;
}
static int hub_public_broker_generation_detach(struct hub_public_broker_generations *g,
        unsigned slot,uint64_t generation) {
    if (!g || slot>=8 || !generation) { errno=EINVAL;return -1; }
    struct hub_public_broker_generation_slot *s=g->slots+slot;
    if (s->generation!=generation || s->phase==HUB_BROKER_EMPTY) { errno=ESTALE;return -1; }
    s->phase=HUB_BROKER_DRAINING;return 0;
}
static int hub_public_broker_generation_drained(struct hub_public_broker_generations *g,
        unsigned slot,uint64_t generation) {
    if (!g || slot>=8 || !generation) { errno=EINVAL;return -1; }
    struct hub_public_broker_generation_slot *s=g->slots+slot;
    if (s->generation!=generation || s->phase!=HUB_BROKER_DRAINING) { errno=ESTALE;return -1; }
    s->phase=HUB_BROKER_EMPTY;return 0;
}
static void hub_public_broker_generation_stop(struct hub_public_broker_generations *g) {
    g->stopping=1;
    for (unsigned i=0;i<8;++i) if (g->slots[i].phase!=HUB_BROKER_EMPTY)
        g->slots[i].phase=HUB_BROKER_DRAINING;
}
#endif

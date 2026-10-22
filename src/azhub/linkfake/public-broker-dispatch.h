/* SPDX-License-Identifier: GPL-3.0-only */
/* Controller adapter after peer/token authentication. Namespace authorization
 * and network ownership are mandatory external operations, not wire decoding.
 * Poll off the UI thread. Never acknowledge detach until phase becomes EMPTY. */
#ifndef HUB_PUBLIC_BROKER_DISPATCH_H
#define HUB_PUBLIC_BROKER_DISPATCH_H
#include "public-broker-generation.h"
#include "public-broker-source.h"
#include "public-broker-namespace.h"
#include "public-source-manager.h"
struct hub_public_broker_dispatch_ops {
    /* Validate the source inventory, pinned root and network namespace kind.
     * Retain owned duplicates on success. Failure must clean partial resources.
     * The endpoint manager's callbacks use this slot's prepared network state. */
    int (*prepare)(void *,unsigned,const struct hub_public_broker_source *,int,int);
    /* Called after worker join and endpoint close. 0 retired, 1 pending,
     * -1 fault (not drained). Must support repeated polls until retirement. */
    int (*release)(void *,unsigned);
    /* ALLOCATE receives a verified root; success starts asynchronous owned
     * allocation. ready returns0 pending,1 usable,-1 loss/failure and is polled
     * through ACTIVE. release must cancel/join all pending allocation work. */
    int (*allocate)(void *,unsigned,const struct hub_public_broker_source *,int);
    int (*ready)(void *,unsigned);
};
struct hub_public_broker_dispatch {
    struct hub_public_broker_generations generations;
    struct hub_public_source_manager *manager;
    struct hub_public_broker_dispatch_ops ops;
    void *context;
    int prepared[8],error[8],allocated[8],pending_root[8];
};
static int hub_public_broker_dispatch_init(struct hub_public_broker_dispatch *d,
        struct hub_public_source_manager *manager,struct hub_public_broker_dispatch_ops ops,void *context) {
    if (!d || !manager || (!ops.prepare && (!ops.allocate || !ops.ready)) || !ops.release) { errno=EINVAL;return -1; }
    memset(d,0,sizeof *d);d->manager=manager;d->ops=ops;d->context=context;
    for (unsigned i=0;i<8;++i) d->pending_root[i]=-1;
    return 0;
}
static int hub_public_broker_dispatch_retire(struct hub_public_broker_dispatch *d,unsigned slot) {
    struct hub_public_broker_generation_slot *s=d->generations.slots+slot;
    if (hub_public_broker_generation_detach(&d->generations,slot,s->generation)<0) return -1;
    if (d->pending_root[slot]>=0) { close(d->pending_root[slot]);d->pending_root[slot]=-1; }
    if (hub_public_source_manager_withdraw(d->manager,slot)<0) return -1;
    return hub_public_source_manager_request(d->manager,slot,NULL,0,0);
}
/* Frame and transferred FD remain caller-owned even when dispatch fails. */
static int hub_public_broker_dispatch_command(struct hub_public_broker_dispatch *d,
        const struct hub_public_broker_frame *frame) {
    if (!d || !hub_public_broker_valid(frame)) { errno=EPROTO;return -1; }
    unsigned slot=frame->slot;
    if (frame->op==HUB_BROKER_STOP) {
        hub_public_broker_generation_stop(&d->generations);
        for (unsigned i=0;i<8;++i) if (d->generations.slots[i].phase!=HUB_BROKER_EMPTY)
            if (hub_public_broker_dispatch_retire(d,i)<0) return -1;
        return 0;
    }
    if (frame->op==HUB_BROKER_DETACH) {
        if (hub_public_broker_generation_detach(&d->generations,slot,frame->generation)<0) return -1;
        return hub_public_broker_dispatch_retire(d,slot);
    }
    if (frame->op!=HUB_BROKER_SOURCE && frame->op!=HUB_BROKER_ALLOCATE) { errno=EPROTO;return -1; }
    if ((frame->op==HUB_BROKER_SOURCE && !d->ops.prepare) ||
            (frame->op==HUB_BROKER_ALLOCATE && (!d->ops.allocate || !d->ops.ready))) { errno=ENOTSUP;return -1; }
    struct hub_public_broker_source source;
    if (hub_public_broker_source_decode(frame->payload,frame->length,&source)<0 ||
            hub_public_broker_generation_source(&d->generations,slot,frame->generation)<0) return -1;
    d->error[slot]=0;
    int root=frame->op==HUB_BROKER_ALLOCATE ? hub_public_broker_source_pin(&source,frame->descriptors[0]):hub_public_broker_source_open(&source);
    if (root<0) goto failed;
    if (frame->op==HUB_BROKER_ALLOCATE) {
        if (d->ops.allocate(d->context,slot,&source,root)<0) {
            int error=errno;close(root);errno=error;goto failed;
        }
        d->allocated[slot]=d->prepared[slot]=1;d->pending_root[slot]=root;return 0;
    }
    if (hub_public_broker_namespace_validate(frame->descriptors[0])<0) {
        int error=errno;close(root);errno=error;goto failed;
    }
    if (d->ops.prepare(d->context,slot,&source,root,frame->descriptors[0])<0) {
        int error=errno;close(root);errno=error;goto failed;
    }
    d->prepared[slot]=1;
    int result=hub_public_source_manager_request_fd(d->manager,slot,root,frame->generation),error=errno;
    close(root);
    if (result<0) { errno=error;goto failed; }
    return 0;
failed: {
    int error=errno ? errno:EIO;d->error[slot]=error;
    hub_public_broker_dispatch_retire(d,slot);errno=error;return -1;
    }
}
/* Return 1 for a ready/drained transition, 0 still pending, -1 controller fault.
 * Worker failure retires that source; neighboring slots remain untouched. */
static int hub_public_broker_dispatch_poll(struct hub_public_broker_dispatch *d,unsigned slot) {
    if (!d || slot>=8) { errno=EINVAL;return -1; }
    struct hub_public_broker_generation_slot *s=d->generations.slots+slot;
    if (s->phase==HUB_BROKER_EMPTY) return 0;
    if (d->allocated[slot] && s->phase!=HUB_BROKER_DRAINING) {
        int ready=d->ops.ready(d->context,slot);
        if (ready<0 || (!ready && d->pending_root[slot]<0)) {
            d->error[slot]=ready<0 && errno ? errno:EADDRNOTAVAIL;
            if (hub_public_broker_dispatch_retire(d,slot)<0) return -1;
        } else if (!ready) return 0;
        else if (d->pending_root[slot]>=0) {
            int result=hub_public_source_manager_request_fd(d->manager,slot,d->pending_root[slot],s->generation),error=errno;
            close(d->pending_root[slot]);d->pending_root[slot]=-1;
            if (result<0) {
                d->error[slot]=error;if (hub_public_broker_dispatch_retire(d,slot)<0) return -1;
            }
        }
    }
    if (hub_public_source_manager_poll(d->manager,slot)<0) return -1;
    struct hub_public_managed_source *source=d->manager->sources+slot;
    if (s->phase!=HUB_BROKER_DRAINING && source->error) {
        d->error[slot]=source->error;
        if (hub_public_broker_dispatch_retire(d,slot)<0) return -1;
    }
    /* A clean but unexpected end is still source loss. Do not leave an ACTIVE
     * generation advertised in status after the manager has closed endpoints. */
    if (s->phase!=HUB_BROKER_DRAINING && !source->worker.started &&
            source->attempted==source->requested) {
        if (!d->error[slot]) d->error[slot]=EPIPE;
        if (hub_public_broker_dispatch_retire(d,slot)<0) return -1;
    }
    if (s->phase==HUB_BROKER_STARTING && source->worker.started &&
            hub_public_source_worker_state(&source->worker)==HUB_PUBLIC_WORKER_RUNNING) {
        if (hub_public_broker_generation_ready(&d->generations,slot,s->generation)<0) return -1;
        return 1;
    }
    if (s->phase!=HUB_BROKER_DRAINING || !hub_public_source_manager_drained(d->manager,slot)) return 0;
    if (d->prepared[slot]) {
        int result=d->ops.release(d->context,slot);
        if (result) return result<0 ? -1:0;
        d->prepared[slot]=0;
    }
    if (hub_public_broker_generation_drained(&d->generations,slot,s->generation)<0) return -1;
    d->allocated[slot]=0;
    return 1;
}
#endif

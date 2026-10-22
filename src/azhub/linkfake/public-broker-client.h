/* SPDX-License-Identifier: GPL-3.0-only */
/* Single-controller inventory reconciliation after authenticated HELLO.
 * Sending is the commit point. Channel loss never proves remote retirement. */
#ifndef HUB_PUBLIC_BROKER_CLIENT_H
#define HUB_PUBLIC_BROKER_CLIENT_H
#include "public-broker-source.h"
#include "public-broker-generation.h"
struct hub_public_broker_client_slot {
    struct hub_public_broker_source desired;
    int root,wanted,phase,detach_sent,error;
    uint64_t generation,identity;
    dev_t device;
    ino_t inode;
};
struct hub_public_broker_client {
    struct hub_public_broker_client_slot slots[8];
    unsigned char token[16];
    int lost;
};
static void hub_public_broker_client_init(struct hub_public_broker_client *c,const unsigned char token[16]) {
    memset(c,0,sizeof *c);memcpy(c->token,token,16);
    for (unsigned i=0;i<8;++i) c->slots[i].root=-1;
}
/* Caller retains root. NULL withdraws immediately, including queued recovery. */
static int hub_public_broker_client_desire(struct hub_public_broker_client *c,unsigned slot,
        const struct hub_public_broker_source *source,int root) {
    if (!c || slot>=8) { errno=EINVAL;return -1; }
    struct hub_public_broker_client_slot *s=c->slots+slot;
    int pinned=-1;
    if (source && (pinned=hub_public_broker_source_pin(source,root))<0) return -1;
    if (source && (!s->wanted || s->desired.identity!=source->identity ||
            s->desired.device!=source->device || s->desired.inode!=source->inode)) s->error=0;
    if (s->root>=0) close(s->root);
    s->root=pinned;s->wanted=source!=NULL;
    if (source) s->desired=*source;
    return 0;
}
static int hub_public_broker_client_send(struct hub_public_broker_client *c,int peer,unsigned slot) {
    if (!c || slot>=8 || c->lost) { errno=ENOTCONN;return -1; }
    struct hub_public_broker_client_slot *s=c->slots+slot;
    struct hub_public_broker_frame f={.slot=slot};memcpy(f.token,c->token,16);
    int replace=s->wanted && (s->identity!=s->desired.identity || s->device!=s->desired.device || s->inode!=s->desired.inode);
    if (s->phase!=HUB_BROKER_EMPTY) {
        if (s->detach_sent || s->phase==HUB_BROKER_DRAINING || (s->wanted && !replace)) return 0;
        f.op=HUB_BROKER_DETACH;f.generation=s->generation;
    } else {
        /* ENETRESET identifies changed DHCP configuration after owned retirement.
         * ESTALE remains a hard root/identity failure. Eject clears wanted first. */
        if (!s->wanted || (s->error && s->error!=EADDRINUSE && s->error!=ENETRESET)) return 0;
        if (s->generation==UINT64_MAX) { errno=EOVERFLOW;return -1; }
        f.op=HUB_BROKER_ALLOCATE;f.generation=s->generation+1;f.count=1;f.descriptors[0]=s->root;
        if (hub_public_broker_source_encode(&s->desired,f.payload,sizeof f.payload,&f.length)<0) return -1;
    }
    if (hub_public_broker_send(peer,&f)<0) {
        if (errno==EAGAIN || errno==EWOULDBLOCK || errno==EINTR) return 0;
        c->lost=1;return -1;
    }
    if (f.op==HUB_BROKER_DETACH) s->detach_sent=1;
    else {
        s->generation=f.generation;s->phase=HUB_BROKER_STARTING;s->detach_sent=0;s->error=0;
        s->identity=s->desired.identity;s->device=s->desired.device;s->inode=s->desired.inode;
    }
    return 1;
}
/* Frames must already pass peer/token authentication. Stale statuses cannot
 * retire a newer publication or change its unmount gate. */
static int hub_public_broker_client_status(struct hub_public_broker_client *c,const struct hub_public_broker_frame *f) {
    if (!c || !f || c->lost || f->op!=HUB_BROKER_STATUS || f->slot>=8 || f->count ||
            f->length!=1 || f->payload[0]>HUB_BROKER_DRAINING || memcmp(c->token,f->token,16)) { errno=EPROTO;return -1; }
    struct hub_public_broker_client_slot *s=c->slots+f->slot;
    if (!f->generation || f->generation!=s->generation) { errno=ESTALE;return -1; }
    unsigned phase=f->payload[0];
    /* Session error responses and change notifications can repeat EMPTY. */
    if ((s->phase==HUB_BROKER_EMPTY && phase!=HUB_BROKER_EMPTY) || (s->phase==HUB_BROKER_DRAINING && phase!=HUB_BROKER_EMPTY && phase!=HUB_BROKER_DRAINING) ||
            (s->phase==HUB_BROKER_ACTIVE && phase==HUB_BROKER_STARTING)) { errno=ESTALE;return -1; }
    s->phase=phase;s->error=f->error;
    return 0;
}
static int hub_public_broker_client_drained(const struct hub_public_broker_client *c,unsigned slot) {
    return c && slot<8 && c->slots[slot].phase==HUB_BROKER_EMPTY;
}
static void hub_public_broker_client_close(struct hub_public_broker_client *c) {
    for (unsigned i=0;i<8;++i) if (c->slots[i].root>=0) { close(c->slots[i].root);c->slots[i].root=-1; }
    c->lost=1;
}
#endif

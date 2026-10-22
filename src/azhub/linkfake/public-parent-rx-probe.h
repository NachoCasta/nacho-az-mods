/* SPDX-License-Identifier: GPL-3.0-only */
/* Explicit diagnostic build only. A protocol-zero packet socket receives no
 * payload. Its membership owns one temporary parent receive-mode reference;
 * close or process exit releases that reference without restoring stale flags. */
#ifndef HUB_PUBLIC_PARENT_RX_PROBE_H
#define HUB_PUBLIC_PARENT_RX_PROBE_H
#include "public-parent-rx.h"
struct hub_public_parent_rx_probe { int phase,fd;uint64_t since; };
static void hub_public_parent_rx_probe_poll(struct hub_public_parent_rx_probe *p,uint64_t now,int ready,
        const char *name,int (*acquire)(const char *),int (*release)(int)) {
    if(!p->phase && ready) {p->phase=1;p->since=now;p->fd=-1;}
    if(p->phase==1 && now>=p->since && now-p->since>=10000) {
        p->fd=acquire(name);p->phase=p->fd>=0 ? 2:3;p->since=now;
        fprintf(stderr,"parent RX diagnostic membership interface=%s acquired=%d errno=%d\n",name,p->fd>=0,p->fd>=0 ? 0:errno);
    }
    if(p->phase==2 && now>=p->since && now-p->since>=60000) {
        release(p->fd);p->fd=-1;p->phase=3;
        fprintf(stderr,"parent RX diagnostic membership released after60s\n");
    }
}
#endif

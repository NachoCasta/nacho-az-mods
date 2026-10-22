/* SPDX-License-Identifier: GPL-3.0-only */
/* Standalone-broker allocation controller: anonymous namespace -> owned MACVLAN
 * -> supervised native DHCP. A grant is NOT ARP/configuration/public readiness.
 * Default SIGCHLD/exclusive child reaping required, never call inside EP147.
 * Zero-initialize and poll through cancellation before releasing storage. */
#ifndef HUB_PUBLIC_NETWORK_ACQUIRE_H
#define HUB_PUBLIC_NETWORK_ACQUIRE_H
#include "public-network-namespace.h"
#include "public-network-link.h"
#include "public-dhcp-acquire.h"
struct hub_public_network_acquire {
    struct hub_public_namespace_create namespace;
    struct hub_public_network_link link;
    struct hub_public_dhcp_acquire dhcp;
    struct hub_public_network_link_spec spec;
    char helper[1024],hook[1024];
    int phase,namespace_fd,stopping,error,complete,allow_no_offer,no_offer;
};
static int hub_public_network_acquire_start(struct hub_public_network_acquire *a,
        const struct hub_public_network_link_spec *spec,const char *helper,const char *hook) {
    if (!a || !spec || !helper || !hook || helper[0]!='/' || hook[0]!='/' ||
            strnlen(helper,1024)>=1024 || strnlen(hook,1024)>=1024 ||
            !spec->parent[0] || strnlen(spec->parent,IFNAMSIZ)==IFNAMSIZ ||
            strncmp(spec->name,"azpub",5) || strnlen(spec->name,IFNAMSIZ)==IFNAMSIZ || (spec->mac[0]&3)!=2) { errno=EINVAL;return -1; }
    if (a->phase) { errno=EBUSY;return -1; }
    memset(a,0,sizeof *a);a->namespace_fd=-1;a->spec=*spec;strcpy(a->helper,helper);strcpy(a->hook,hook);
    if (hub_public_namespace_create_start(&a->namespace)<0) return -1;
    a->phase=1;return 0;
}
static int hub_public_network_acquire_allow_no_offer(struct hub_public_network_acquire *a) {
    if (!a || a->phase!=1 || a->stopping || a->complete) { errno=EINVAL;return -1; }
    a->allow_no_offer=1;return 0;
}
static void hub_public_network_acquire_cancel(struct hub_public_network_acquire *a,int error,uint64_t now) {
    if (!a || !a->phase || a->complete) return;
    a->stopping=1;if (!a->error) a->error=error ? error:ECANCELED;
    if (a->phase==3) hub_public_dhcp_acquire_cancel(&a->dhcp,a->error,now);
}
static int hub_public_network_acquire_fail(struct hub_public_network_acquire *a,int error) {
    if (!a->error) a->error=error ? error:EIO;
    if (a->namespace_fd>=0) { close(a->namespace_fd);a->namespace_fd=-1; }
    a->complete=1;errno=a->error;return -1;
}
/* 0 pending,1 complete native grant,2 explicitly permitted complete no-offer
 * with owned namespace retained for a separately validated IPv4LL candidate,
 * -1 terminal failure. No-offer never supplies a fabricated grant.
 * Successful owner must drain future endpoints before closing namespace_fd.
 * Failure closes namespace only after creation/link threads and DHCP are done. */
static int hub_public_network_acquire_poll(struct hub_public_network_acquire *a,uint64_t now) {
    if (!a || !a->phase) { errno=EINVAL;return -1; }
    if (a->complete) { if (a->error) { errno=a->error;return -1; }return a->no_offer ? 2:1; }
    if (a->phase==1) {
        int result=hub_public_namespace_create_poll(&a->namespace);
        if (result==1) return 0;
        if (result<0 && a->namespace.started) { a->stopping=1;if (!a->error) a->error=errno;return 0; }
        if (result<0) return hub_public_network_acquire_fail(a,errno);
        a->namespace_fd=hub_public_namespace_create_take(&a->namespace);
        if (a->namespace_fd<0 || a->stopping) return hub_public_network_acquire_fail(a,a->error ? a->error:errno);
        a->spec.namespace_fd=a->namespace_fd;
        if (hub_public_network_link_start(&a->link,&a->spec)<0) return hub_public_network_acquire_fail(a,errno);
        a->phase=2;return 0;
    }
    if (a->phase==2) {
        int result=hub_public_network_link_poll(&a->link);
        if (result==1) return 0;
        if (result<0 && a->link.started) { a->stopping=1;if (!a->error) a->error=errno;return 0; }
        if (result<0 || a->stopping) return hub_public_network_acquire_fail(a,a->error ? a->error:errno);
        if (hub_public_dhcp_acquire_begin(&a->dhcp,a->helper,a->namespace_fd,a->spec.name,a->hook,a->spec.mac,now)<0)
            return hub_public_network_acquire_fail(a,errno);
        a->phase=3;return 0;
    }
    int result=hub_public_dhcp_acquire_poll(&a->dhcp,now);
    if (!result) return 0;
    if (result<0 && !a->stopping && !a->error && a->allow_no_offer && hub_public_dhcp_acquire_no_offer(&a->dhcp)) {
        a->no_offer=a->complete=1;return 2;
    }
    if (result<0 || a->stopping) return hub_public_network_acquire_fail(a,a->error ? a->error:errno);
    a->complete=1;return 1;
}
#endif

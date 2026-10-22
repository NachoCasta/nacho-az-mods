/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef AZHUB_PORT_TOPOLOGY_H
#define AZHUB_PORT_TOPOLOGY_H
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
/* USB device nodes are nested below their parent hub in sysfs. Reading maxchild
 * distinguishes a hub from a leaf without depending on block enumeration order.
 * Empty leaf ports consume a number. Capacity is bounded to the eight UI labels. */
struct hub_port_route { unsigned bus,root,depth,ports[7]; };
static int hub_port_route_parse(const char *path,struct hub_port_route *out) {
    struct hub_port_route best={0};
    for (const char *p=path;*p;) {
        while (*p=='/') p++;
        const char *end=strchr(p,'/');if (!end) end=p+strlen(p);
        unsigned bus=0,root=0;int used=0;
        if (*p>='0' && *p<='9' && sscanf(p,"%u-%u%n",&bus,&root,&used)==2 && bus && root) {
            const char *q=p+used;struct hub_port_route next={.bus=bus,.root=root};
            while (q<end && *q=='.' && next.depth<7) {
                unsigned port=0;int n=0;
                if (q[1]<'0' || q[1]>'9' || sscanf(q+1,"%u%n",&port,&n)!=1 || !port || port>255) break;
                next.ports[next.depth++]=port;q+=n+1;
            }
            if (q==end && next.depth>=best.depth) best=next;
        }
        p=end;
    }
    if (!out || !best.depth) return 0;
    *out=best;return 1;
}
typedef int (*hub_port_children_fn)(void *,const struct hub_port_route *);
/* Return a flattened one-based physical leaf-port index, or zero if the route
 * is invalid, exceeds capacity, or required hub topology cannot be established. */
static unsigned hub_port_leaf_count(struct hub_port_route *hub,
        hub_port_children_fn children,void *context,unsigned budget) {
    int count=children(context,hub);
    if (count<1 || count>255 || hub->depth>=7) return 0;
    unsigned total=0;
    for (int port=1;port<=count;port++) {
        struct hub_port_route child=*hub;child.ports[child.depth++]=(unsigned)port;
        int nested=children(context,&child);
        if (nested<0) return 0;
        unsigned leaves=nested>0 ? hub_port_leaf_count(&child,children,context,budget):1;
        if (!leaves) return 0;
        total+=leaves;if (total>=budget) return budget;
    }
    return total;
}
static unsigned hub_port_number(const struct hub_port_route *route,
        hub_port_children_fn children,void *context) {
    if (!route || !route->depth || route->depth>7 || !children) return 0;
    struct hub_port_route hub={.bus=route->bus,.root=route->root};
    unsigned before=0;
    for (unsigned level=0;level<route->depth;level++) {
        int count=children(context,&hub);unsigned selected=route->ports[level];
        if (count<1 || count>255 || !selected || selected>(unsigned)count) return 0;
        for (unsigned port=1;port<selected;port++) {
            struct hub_port_route child=hub;child.ports[child.depth++]=port;
            int nested=children(context,&child);
            if (nested<0) return 0;
            unsigned leaves=nested>0 ? hub_port_leaf_count(&child,children,context,9):1;
            if (!leaves || (before+=leaves)>=8) return 0;
        }
        hub.ports[hub.depth++]=selected;
    }
    if (children(context,&hub)!=0) return 0; /* A hub itself is not a USB source. */
    return before+1;
}
/* The context is the resolved native root-hub node (for example .../usb2/2-1),
 * not /sys/class/block. Descendants are named 2-1.2 and 2-1.2.3. Missing child
 * nodes are empty ports; malformed attributes or inaccessible nodes fail closed. */
struct hub_port_sysfs { char root[PATH_MAX]; };
static int hub_port_sysfs_init(struct hub_port_sysfs *context,const char *resolved,
                               const struct hub_port_route *route) {
    if (!context || !resolved || !route) return 0;
    char component[64];int n=snprintf(component,sizeof component,"%u-%u",route->bus,route->root);
    if (n<1 || n>=(int)sizeof component) return 0;
    for (const char *p=resolved;*p;) {
        while (*p=='/') p++;
        const char *end=strchr(p,'/');if (!end) end=p+strlen(p);
        if ((size_t)(end-p)==(size_t)n && !memcmp(p,component,(size_t)n)) {
            size_t length=(size_t)(end-resolved);
            if (length>=sizeof context->root) return 0;
            memcpy(context->root,resolved,length);context->root[length]=0;return 1;
        }
        p=end;
    }
    return 0;
}
static int hub_port_sysfs_children(void *opaque,const struct hub_port_route *route) {
    struct hub_port_sysfs *context=opaque;
    if (!context || !route || route->depth>7) return -1;
    char path[PATH_MAX],name[96];
    int n=snprintf(name,sizeof name,"%u-%u",route->bus,route->root);
    if (n<1 || n>=(int)sizeof name) return -1;
    /* Build the nested directory path without mutating the shared context. */
    if (snprintf(path,sizeof path,"%s",context->root)>=(int)sizeof path) return -1;
    n=snprintf(name,sizeof name,"%u-%u",route->bus,route->root);
    for (unsigned level=0;level<route->depth;level++) {
        int added=snprintf(name+n,sizeof name-(size_t)n,".%u",route->ports[level]);
        if (added<1 || added>=(int)sizeof name-n) return -1;
        n+=added;size_t length=strlen(path);
        int written=snprintf(path+length,sizeof path-length,"/%s",name);
        if (written<1 || written>=(int)(sizeof path-length)) return -1;
    }
    size_t length=strlen(path);
    if (length+10>=sizeof path) return -1;
    memcpy(path+length,"/maxchild",10);
    FILE *file=fopen(path,"re");
    if (!file) return errno==ENOENT ? 0:-1;
    unsigned count=0;char extra=0;
    int parsed=fscanf(file,"%u %c",&count,&extra);int failed=ferror(file);fclose(file);
    if (failed || parsed!=1 || count>255) return -1;
    return (int)count;
}
#endif

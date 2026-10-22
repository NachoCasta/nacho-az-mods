/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "port-topology.h"
static int nested;
static int children(void *unused,const struct hub_port_route *r) {
    (void)unused;
    if (!r->depth) return 4;
    if (nested && r->depth==1 && r->ports[0]==2) return nested==2 ? 8:4;
    return 0;
}
int main(void) {
    struct hub_port_route route;
    assert(hub_port_route_parse("/sys/usb2/2-1/2-1.4/2-1.4:1.0/block/sdb",&route));
    assert(route.depth==1 && route.ports[0]==4);
    assert(hub_port_number(&route,children,0)==4); /* Ports1..3 may be empty. */
    nested=1;assert(hub_port_number(&route,children,0)==7);
    assert(hub_port_route_parse("/sys/usb2/2-1/2-1.2/2-1.2.3/block/sdc",&route));
    assert(hub_port_number(&route,children,0)==4);
    /* Reverse insertion order cannot change the result: no occupancy input. */
    for (unsigned port=4;port;port--) {
        route.depth=2;route.ports[0]=2;route.ports[1]=port;
        assert(hub_port_number(&route,children,0)==port+1);
    }
    nested=2;route.depth=2;route.ports[0]=2;route.ports[1]=7;
    assert(hub_port_number(&route,children,0)==8);
    route.ports[1]=8;assert(!hub_port_number(&route,children,0));
    route.depth=1;route.ports[0]=3;assert(!hub_port_number(&route,children,0));
    assert(!hub_port_route_parse("/sys/usb2/2-1/2-1:1.0",&route));
    assert(!hub_port_route_parse("/sys/usb2/2-1.0",&route));
    route.depth=1;route.ports[0]=5;assert(!hub_port_number(&route,children,0));
    char temporary[]="/tmp/azhub-topology-XXXXXX";assert(mkdtemp(temporary));
    char root[PATH_MAX],hub[PATH_MAX],root_attribute[PATH_MAX],hub_attribute[PATH_MAX];
    assert(snprintf(root,sizeof root,"%s/2-1",temporary)<(int)sizeof root);
    assert(!mkdir(root,0700));
    assert(snprintf(hub,sizeof hub,"%s/2-1.2",root)<(int)sizeof hub);
    assert(!mkdir(hub,0700));
    assert(snprintf(root_attribute,sizeof root_attribute,"%s/maxchild",root)<(int)sizeof root_attribute);
    assert(snprintf(hub_attribute,sizeof hub_attribute,"%s/maxchild",hub)<(int)sizeof hub_attribute);
    FILE *f=fopen(root_attribute,"w");assert(f);fputs("4\n",f);fclose(f);
    f=fopen(hub_attribute,"w");assert(f);fputs("4\n",f);fclose(f);
    struct hub_port_sysfs context;
    assert(hub_port_route_parse("/sys/usb2/2-1/2-1.2/2-1.2.3/block/sdc",&route));
    assert(hub_port_sysfs_init(&context,hub,&route));
    assert(!strcmp(context.root,root));
    assert(hub_port_number(&route,hub_port_sysfs_children,&context)==4);
    route.depth=1;route.ports[0]=4;
    assert(hub_port_number(&route,hub_port_sysfs_children,&context)==7);
    f=fopen(hub_attribute,"w");assert(f);fputs("broken\n",f);fclose(f);
    assert(!hub_port_number(&route,hub_port_sysfs_children,&context));
    unlink(hub_attribute);rmdir(hub);unlink(root_attribute);rmdir(root);rmdir(temporary);
    puts("Physical port flattening, sysfs nesting, invalid topology and empty-port reservation passed");
}

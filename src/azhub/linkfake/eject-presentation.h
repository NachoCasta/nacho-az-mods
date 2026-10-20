/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HUB_EJECT_PRESENTATION_H
#define HUB_EJECT_PRESENTATION_H
#include <stdlib.h>
#include <string.h>
#include "eject-ui-model.h"
static int hub_eject_same_row(const struct eject_item *a,const struct eject_item *b) {
#ifdef LF_SAFE_EJECT
    return a->port==b->port && a->generation==b->generation && a->devnum==b->devnum && !strcmp(a->usb,b->usb);
#else
    return a->port==b->port && !strcmp(a->disk,b->disk) && !strcmp(a->topology,b->topology);
#endif
}
static int hub_eject_row_order(const void *left,const void *right) {
    const struct eject_item *a=left,*b=right;
    int an=a->display_number>0 ? a->display_number:INT_MAX;
    int bn=b->display_number>0 ? b->display_number:INT_MAX;
    if (an!=bn) return an<bn ? -1:1;
    int order=strcmp(a->label,b->label);
    if (order) return order;
    if (a->port!=b->port) return a->port<b->port ? -1:1;
    order=strcmp(a->topology,b->topology);
    return order ? order:strcmp(a->disk,b->disk);
}
static void hub_eject_sort(struct eject_inventory *inventory) {
    qsort(inventory->items,inventory->count,sizeof inventory->items[0],hub_eject_row_order);
}
static int hub_eject_step(int selected,int count,int direction) {
    if (count<=0) return -1;
    if (selected<0 || selected>=count) return direction>0 ? 0:count-1;
    return (selected+(direction>0 ? 1:count-1))%count;
}
/* Preserve the selected physical connection; removal clears selection so a
 * refresh cannot silently substitute a different ejection target. */
static int hub_eject_refresh(struct eject_inventory *shown,const struct eject_inventory *live,int port,int selected) {
    struct eject_item previous={0};
    int had_selection=selected>=0 && selected<shown->count;
    if (had_selection) previous=shown->items[selected];
    memset(shown,0,sizeof *shown);
    for (int i=0;i<live->count;i++) if (live->items[i].port==port)
        shown->items[shown->count++]=live->items[i];
    hub_eject_sort(shown);
    for (int i=0;had_selection && i<shown->count;i++)
        if (hub_eject_same_row(&previous,&shown->items[i])) return i;
    return -1;
}
#endif

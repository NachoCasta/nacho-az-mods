/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef LINK_LOAD_ORIGIN_PROBE_H
#define LINK_LOAD_ORIGIN_PROBE_H
#include <string.h>
/* Included after eject-ui-model.h. Source numbers select a reservation, while
 * sysfs generation, USB address and physical path establish its connection.
 * A recycled sdX name, source slot or label cannot establish that identity. */
static int probe_load_origin_matches(int valid,const struct eject_item *origin,
    const struct eject_item *reserved,unsigned sources,int source) {
    if(!valid || !origin || !reserved || source<0 || source>=8 ||
        !(sources&(1u<<source)) || (reserved->state!=1 && reserved->state!=3) ||
        !origin->generation || !origin->devnum || !origin->usb[0]) return 0;
    return origin->generation==reserved->generation &&
        origin->devnum==reserved->devnum && !strcmp(origin->usb,reserved->usb);
}
#endif

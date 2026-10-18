/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "eject-gesture.h"
int main(void) {
    struct hub_stop_gesture g={0};
    assert(hub_stop_event(&g,1,1,100)==HUB_STOP_NONE);
    assert(hub_stop_event(&g,1,1,150)==HUB_STOP_NONE);
    assert(hub_stop_event(&g,0,1,300)==HUB_STOP_SELECT);
    assert(hub_stop_poll(&g,3000)==HUB_STOP_NONE);
    assert(hub_stop_event(&g,1,1,4000)==HUB_STOP_NONE);
    assert(hub_stop_poll(&g,5499)==HUB_STOP_NONE);
    assert(hub_stop_poll(&g,5500)==HUB_STOP_ALL);
    assert(hub_stop_poll(&g,5600)==HUB_STOP_NONE);
    assert(hub_stop_event(&g,0,1,5700)==HUB_STOP_NONE);
    /* A late UI tick must not turn a long hold into a short click. */
    assert(hub_stop_event(&g,1,1,6000)==HUB_STOP_NONE);
    assert(hub_stop_event(&g,0,1,8000)==HUB_STOP_ALL);
    /* Never deliver half a native pair after hub insertion/removal. */
    assert(hub_stop_event(&g,1,0,9000)==HUB_STOP_FORWARD);
    assert(hub_stop_event(&g,0,1,9200)==HUB_STOP_FORWARD);
    assert(hub_stop_event(&g,1,1,10000)==HUB_STOP_NONE);
    assert(hub_stop_event(&g,0,0,10200)==HUB_STOP_SELECT);
    puts("USB STOP gesture ownership and single-action checks passed");
}

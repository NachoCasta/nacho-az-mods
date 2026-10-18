/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HUB_EJECT_GESTURE_H
#define HUB_EJECT_GESTURE_H
#include <stdint.h>

enum hub_stop_action { HUB_STOP_NONE, HUB_STOP_FORWARD, HUB_STOP_SELECT, HUB_STOP_ALL };
struct hub_stop_gesture {
    int down, captured, long_sent;
    uint64_t started;
};
/* Decide ownership on DOWN and retain it through UP, even if topology changes.
 * The prototype's 1500 ms threshold is a UI choice, not a native timer claim. */
#define HUB_STOP_HOLD_MS 1500u
static enum hub_stop_action hub_stop_poll(struct hub_stop_gesture *g, uint64_t now) {
    if (g->down && g->captured && !g->long_sent && now-g->started>=HUB_STOP_HOLD_MS) {
        g->long_sent=1;
        return HUB_STOP_ALL;
    }
    return HUB_STOP_NONE;
}
static enum hub_stop_action hub_stop_event(struct hub_stop_gesture *g, int down,
                                            int has_hub, uint64_t now) {
    if (down) {
        if (!g->down) {
            g->down=1; g->captured=has_hub; g->long_sent=0; g->started=now;
        }
        return g->captured ? HUB_STOP_NONE : HUB_STOP_FORWARD;
    }
    if (!g->down) return HUB_STOP_FORWARD;
    enum hub_stop_action action=HUB_STOP_FORWARD;
    if (g->captured) {
        action=hub_stop_poll(g,now);
        if (action==HUB_STOP_NONE && !g->long_sent) action=HUB_STOP_SELECT;
    }
    g->down=0;
    return action;
}
#endif

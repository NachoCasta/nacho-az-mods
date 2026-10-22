/* SPDX-License-Identifier: GPL-3.0-only */
/* Reusable controller inventory after authenticated helper HELLO. Caller owns
 * SDK launch/lifetime, native inventory snapshots and shutdown authorization.
 * This component neither enters namespaces nor creates/reaps processes. */
#ifndef HUB_PUBLIC_SHARING_INVENTORY_H
#define HUB_PUBLIC_SHARING_INVENTORY_H
#include "public-broker-inventory.h"
#include "public-player-reconcile.h"
#include "public-source-mac.h"
struct hub_public_sharing_view { const char *root;int present,reserved; };
struct hub_public_sharing_inventory {
    struct hub_public_broker_client client;
    struct hub_public_broker_inventory observed;
    struct hub_public_player_pool players;
    struct hub_public_player_reconcile selection;
    unsigned char parent[6];
    int error[8];
    uint64_t retired_generation[8];
    unsigned retired_error[8];
    int quiescing,stopped;
};
static int hub_public_sharing_inventory_init(struct hub_public_sharing_inventory *state,
        const unsigned char token[16],const unsigned char parent[6]) {
    unsigned char checked[6];
    if (!state || !token || hub_public_source_mac(parent,0,checked)<0) { errno=EINVAL;return -1; }
    memset(state,0,sizeof *state);memcpy(state->parent,parent,6);
    hub_public_broker_client_init(&state->client,token);return 0;
}
/* Receive a bounded batch, then withdraw all absent/reserved roots before
 * selecting any new publication. A per-source filesystem/capacity error cannot
 * stop another source. A channel fault preserves all outstanding drain gates. */
static int hub_public_sharing_inventory_poll(struct hub_public_sharing_inventory *state,int peer,
        const struct hub_public_sharing_view views[8],uint64_t now) {
    if (!state || !views || state->client.lost) { errno=ENOTCONN;return -1; }
    if (state->stopped) return 1;
    if (hub_public_player_clock(&state->players,now)<0) return -1;
    for (unsigned i=0;i<8;++i) {
        struct hub_public_broker_frame frame;
        int received=hub_public_broker_receive_token(peer,state->client.token,&frame);
        if (!received) break;
        if (received<0) { state->client.lost=1;return -1; }
        if (frame.op==HUB_BROKER_STATUS && !frame.slot && !frame.generation && !frame.count &&
                frame.length==1 && frame.payload[0]==4 && !frame.error) {
            hub_public_broker_dispose(&frame);
            for (unsigned slot=0;slot<8;++slot) if (!hub_public_broker_client_drained(&state->client,slot)) {
                state->client.lost=1;errno=EPROTO;return -1;
            }
            state->quiescing=state->stopped=1;return 1;
        }
        int result=hub_public_broker_client_status(&state->client,&frame),error=errno;
        if (!result && frame.payload[0]==HUB_BROKER_EMPTY) {
            state->retired_generation[frame.slot]=frame.generation;
            state->retired_error[frame.slot]=frame.error;
        }
        if (!result && frame.payload[0]==HUB_BROKER_DRAINING && !frame.error &&
                state->client.slots[frame.slot].wanted && !state->client.slots[frame.slot].detach_sent) state->quiescing=1;
        hub_public_broker_dispose(&frame);
        if (result<0 && error!=ESTALE) { state->client.lost=1;errno=error;return -1; }
    }
    if (state->quiescing) return 0;
    for (unsigned slot=0;slot<8;++slot) if (!views[slot].present || views[slot].reserved) {
        if (hub_public_broker_inventory_withdraw(&state->observed,&state->client,slot)<0) return -1;
        unsigned unused;
        if (hub_public_player_reconcile(&state->selection,&state->players,&state->client,
                slot,views[slot].present,views[slot].reserved,now,&unused)<0) return -1;
        if (hub_public_broker_client_send(&state->client,peer,slot)<0) return -1;
    }
    for (unsigned slot=0;slot<8;++slot) if (views[slot].present && !views[slot].reserved) {
        unsigned player=0;int selected=hub_public_player_reconcile(&state->selection,&state->players,
            &state->client,slot,1,0,now,&player);
        if (selected<0) return -1;
        if (!selected) { state->error[slot]=ENOSPC;continue; }
        struct hub_public_broker_source declared={.identity=1,.device=1,.inode=1,.player=player};
        int result=-1;errno=EINVAL;
        if (views[slot].root && strlen(views[slot].root)<sizeof declared.directory) {
            strcpy(declared.directory,views[slot].root);
            snprintf(declared.interface,sizeof declared.interface,"azpub%u",slot);
            snprintf(declared.label,sizeof declared.label,"Hub USB %u",slot+1);
            if (hub_public_source_mac(state->parent,slot,declared.mac)<0) return -1;
            result=hub_public_broker_inventory_observe(&state->observed,&state->client,slot,&declared,1,0);
        }
        state->error[slot]=result<0 ? errno:0;
        if (result<0) {
            if (hub_public_broker_inventory_withdraw(&state->observed,&state->client,slot)<0) return -1;
            unsigned unused;
            if (hub_public_player_reconcile(&state->selection,&state->players,&state->client,
                    slot,0,1,now,&unused)<0) return -1;
        }
        if (hub_public_broker_client_send(&state->client,peer,slot)<0) return -1;
    }
    return 0;
}
#endif

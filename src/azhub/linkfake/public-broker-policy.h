/* SPDX-License-Identifier: GPL-3.0-only */
/* Candidate helper policy independent of emulator disk letters. Trusted parent
 * identity is obtained by the helper, never accepted from a SOURCE request. */
#ifndef HUB_PUBLIC_BROKER_POLICY_H
#define HUB_PUBLIC_BROKER_POLICY_H
#include "public-broker-root-policy.h"
#include "public-source-mac.h"
#include "public-player-pool.h"
static int hub_public_broker_identity_policy(unsigned slot,const struct hub_public_broker_source *source,
        const unsigned char parent_mac[6]) {
    if (slot>=8 || !hub_public_broker_source_valid(source)) { errno=EINVAL;return -1; }
    unsigned char expected[6];if (hub_public_source_mac(parent_mac,slot,expected)<0) return -1;
    char interface[16],label[32];snprintf(interface,sizeof interface,"azpub%u",slot);
    snprintf(label,sizeof label,"Hub USB %u",slot+1);
    if (source->player<HUB_PUBLIC_PLAYER_FIRST || source->player>=HUB_PUBLIC_PLAYER_FIRST+HUB_PUBLIC_PLAYER_COUNT ||
            memcmp(source->mac,expected,6) || strcmp(source->interface,interface) || strcmp(source->label,label)) {
        errno=EPERM;return -1;
    }
    return 0;
}
static int hub_public_broker_policy(unsigned slot,const struct hub_public_broker_source *source,int root,
        const unsigned char parent_mac[6]) {
    if (hub_public_broker_identity_policy(slot,source,parent_mac)<0) return -1;
    return hub_public_broker_root_policy(source,root,"/media/usb","/proc/mounts","/sys/class/block");
}
#endif

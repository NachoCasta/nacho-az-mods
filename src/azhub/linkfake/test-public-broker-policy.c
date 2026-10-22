/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include "public-broker-policy.h"
int main(void) {
    unsigned char parent[6]={0x52,0x54,0,0x12,0x34,0x56},other[6]={0x52,0x54,0,0x12,0x34,0x57};
    for (unsigned slot=0;slot<8;++slot) {
        struct hub_public_broker_source source={.identity=1,.device=1,.inode=1,.player=41};
        strcpy(source.directory,"/media/usb/sdx9");
        snprintf(source.interface,sizeof source.interface,"azpub%u",slot);
        snprintf(source.label,sizeof source.label,"Hub USB %u",slot+1);
        assert(!hub_public_source_mac(parent,slot,source.mac));
        for (unsigned player=41;player<=48;++player) { source.player=player;assert(!hub_public_broker_identity_policy(slot,&source,parent)); }
        assert(hub_public_broker_identity_policy(slot,&source,other)<0);
        assert(hub_public_broker_identity_policy((slot+1)%8,&source,parent)<0);
        source.player=7;assert(hub_public_broker_identity_policy(slot,&source,parent)<0);source.player=41;
        source.mac[5]^=8;assert(hub_public_broker_identity_policy(slot,&source,parent)<0);source.mac[5]^=8;
        source.label[0]='X';assert(hub_public_broker_identity_policy(slot,&source,parent)<0);
    }
    puts("PASS: controller-derived source identities, eight-source separation, bounded alternative players and foreign-controller/native identity refusal");
}

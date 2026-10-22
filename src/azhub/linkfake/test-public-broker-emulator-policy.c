/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include "public-broker-emulator-policy.h"
int main(void) {
    for (unsigned slot=0;slot<8;++slot) {
        struct hub_public_broker_source source={.identity=1,.device=1,.inode=1,.player=41+slot,
            .mac={2,0,0,0,0x12,(unsigned char)(3+slot)}};
        snprintf(source.directory,sizeof source.directory,"/media/usb/sd%c1",'c'+slot);
        snprintf(source.interface,sizeof source.interface,"azpub%u",slot);
        snprintf(source.label,sizeof source.label,"Hub USB %u",slot+1);
        assert(!hub_public_broker_emulator_policy(slot,&source));
        assert(hub_public_broker_emulator_policy((slot+1)%8,&source)<0);
        ++source.player;assert(hub_public_broker_emulator_policy(slot,&source)<0);--source.player;
        source.mac[5]^=0x10;assert(hub_public_broker_emulator_policy(slot,&source)<0);source.mac[5]^=0x10;
        strcpy(source.directory,"/media/usb/sda1");assert(hub_public_broker_emulator_policy(slot,&source)<0);
        strcpy(source.directory,"/tmp/azhub-sdk-library");
        assert(slot ? hub_public_broker_emulator_policy(slot,&source)<0 : !hub_public_broker_emulator_policy(slot,&source));
    }
    puts("PASS: eight distinct fixture root/MAC/player/interface/label declarations, cross-slot rejection and native USB exclusion");
}

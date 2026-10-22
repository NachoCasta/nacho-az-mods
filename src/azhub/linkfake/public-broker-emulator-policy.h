/* SPDX-License-Identifier: GPL-3.0-only */
/* Declared isolated fixture identities only; not production player/MAC policy. */
#ifndef HUB_PUBLIC_BROKER_EMULATOR_POLICY_H
#define HUB_PUBLIC_BROKER_EMULATOR_POLICY_H
#include "public-broker-source.h"
static int hub_public_broker_emulator_policy(unsigned slot,const struct hub_public_broker_source *source) {
    if (slot>=8 || !hub_public_broker_source_valid(source)) { errno=EINVAL;return -1; }
    char root[64],interface[16],label[32];
    snprintf(root,sizeof root,"/media/usb/sd%c1",'c'+slot);
    snprintf(interface,sizeof interface,"azpub%u",slot);
    snprintf(label,sizeof label,"Hub USB %u",slot+1);
    unsigned char mac[6]={2,0,0,0,0x12,(unsigned char)(3+slot)};
    int legacy_alias=!slot && !strcmp(source->directory,"/tmp/azhub-sdk-library");
    if ((!legacy_alias && strcmp(source->directory,root)) || strcmp(source->interface,interface) ||
            source->player!=41+slot || memcmp(source->mac,mac,6) || strcmp(source->label,label)) {
        errno=EPERM;return -1;
    }
    return 0;
}
#endif

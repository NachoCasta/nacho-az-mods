/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef NUSB_SOURCE_REFRESH_H
#define NUSB_SOURCE_REFRESH_H
#include <stdint.h>
#include <string.h>
#include "name-policy.h"
#ifndef NUSB_ROOT_VTABLE
#define NUSB_ROOT_VTABLE ((uintptr_t)0x2e09098)
#endif
#ifndef NUSB_NAME_LISTENER_VTABLE
#define NUSB_NAME_LISTENER_VTABLE ((uintptr_t)0x2e0b538)
#endif
#ifndef NUSB_NAME_LISTENER_METHOD
#define NUSB_NAME_LISTENER_METHOD ((uintptr_t)0x1c305c8)
#endif
/* Caller is the native information-change task on JUCE's message thread. Its
 * closure owns a live dataset context; no widget pointer survives this call.
 * Dispatch only the registered Source controller's presentation callback. */
static int nusb_refresh_source_names(uintptr_t root,const unsigned char *source_id) {
    uint32_t kind=0;memcpy(&kind,source_id,sizeof kind);
    if(!root || *(uintptr_t *)root!=NUSB_ROOT_VTABLE || kind!=1 || nusb_native_port(source_id+4)<0)return 0;
    int count=*(int *)(root+0x1b8),capacity=*(int *)(root+0x1b0);
    if(count<0 || count>64 || capacity<count)return 0;
    int refreshed=0;
    for(int index=count-1;index>=0;--index) {
        int current=*(int *)(root+0x1b8);
        if(current<0 || current>64)return refreshed;
        if(index>=current)continue;
        uintptr_t *listeners=*(uintptr_t **)(root+0x1a8);
        if(!listeners)return refreshed;
        uintptr_t listener=listeners[index];
        if(!listener || *(uintptr_t *)listener!=NUSB_NAME_LISTENER_VTABLE ||
           *(uintptr_t *)NUSB_NAME_LISTENER_VTABLE!=NUSB_NAME_LISTENER_METHOD)continue;
        ((void (*)(uintptr_t,const void *,int))NUSB_NAME_LISTENER_METHOD)(listener,source_id,1);
        ++refreshed;
    }
    return refreshed;
}
#endif

/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
static uintptr_t root_vtable[1],name_vtable[1],foreign_vtable[1];
static unsigned calls;static unsigned char expected_id[12];
static void changed(uintptr_t,const void *source,int full) {assert(full==1 && !memcmp(source,expected_id,12));++calls;}
#define NUSB_ROOT_VTABLE ((uintptr_t)root_vtable)
#define NUSB_NAME_LISTENER_VTABLE ((uintptr_t)name_vtable)
#define NUSB_NAME_LISTENER_METHOD ((uintptr_t)changed)
#include "../source-refresh.h"
int main() {
    alignas(8) unsigned char root[0x300]={0};
    uintptr_t good=(uintptr_t)name_vtable,foreign=(uintptr_t)foreign_vtable;
    uintptr_t listeners[]={(uintptr_t)&good,0,(uintptr_t)&foreign,(uintptr_t)&good};
    *(uintptr_t *)root=NUSB_ROOT_VTABLE;*(uintptr_t **)(root+0x1a8)=listeners;
    *(int *)(root+0x1b0)=4;*(int *)(root+0x1b8)=4;name_vtable[0]=NUSB_NAME_LISTENER_METHOD;
    uint32_t kind=1;memcpy(expected_id,&kind,4);expected_id[4]=1;expected_id[8]=2;expected_id[9]=1;
    unsigned char saved[12];memcpy(saved,expected_id,12);
    assert(nusb_refresh_source_names((uintptr_t)root,expected_id)==2 && calls==2);
    assert(!memcmp(saved,expected_id,12));
    expected_id[4]=2;assert(!nusb_refresh_source_names((uintptr_t)root,expected_id));expected_id[4]=1;
    kind=0;memcpy(expected_id,&kind,4);assert(!nusb_refresh_source_names((uintptr_t)root,expected_id));kind=1;memcpy(expected_id,&kind,4);

    *(int *)(root+0x1b8)=-1;assert(!nusb_refresh_source_names((uintptr_t)root,expected_id));
    *(int *)(root+0x1b8)=65;assert(!nusb_refresh_source_names((uintptr_t)root,expected_id));
    *(int *)(root+0x1b8)=4;*(int *)(root+0x1b0)=3;assert(!nusb_refresh_source_names((uintptr_t)root,expected_id));
    *(int *)(root+0x1b0)=4;name_vtable[0]=0;assert(!nusb_refresh_source_names((uintptr_t)root,expected_id));
    assert(calls==2);puts("Native title bridge: exact native identity, full SourceID preservation, registered/foreign listeners and count/capacity guards pass");
}

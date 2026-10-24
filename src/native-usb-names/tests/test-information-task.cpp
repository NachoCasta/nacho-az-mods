/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual native-task bridge with owned fake objects; no firmware is executed. */
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
static uintptr_t root_vt[1],name_vt[1];
static void changed(uintptr_t,const void *,int);
#define NUSB_ROOT_VTABLE ((uintptr_t)root_vt)
#define NUSB_NAME_LISTENER_VTABLE ((uintptr_t)name_vt)
#define NUSB_NAME_LISTENER_METHOD ((uintptr_t)changed)
#include "../native-names.cpp"
static unsigned forwards,calls;static unsigned char expected_id[12];
static void changed(uintptr_t,const void *source,int full) {
    assert(full==1 && !memcmp(source,expected_id,12));++calls;
    applied_epoch[0].store(epochs[0].load());
}
static void forwarded(uintptr_t) {++forwards;}
static void quiet_log(const char *,const char *,...) {}
int main() {
    az_core_v1 core={};core.size=sizeof core;core.log=quiet_log;api=&core;
    original_information_task=forwarded;active.store(true);
    alignas(8) unsigned char root[0x300]={0},closure[32]={0};
    uintptr_t listener=(uintptr_t)name_vt,list[]={ (uintptr_t)&listener };
    *(uintptr_t *)root=NUSB_ROOT_VTABLE;*(uintptr_t **)(root+0x1a8)=list;
    *(int *)(root+0x1b0)=1;*(int *)(root+0x1b8)=1;name_vt[0]=NUSB_NAME_LISTENER_METHOD;
    *(uintptr_t *)(root+0x2b0)=0x123456;*(uintptr_t *)closure=(uintptr_t)root;
    unsigned char media[]={1,0,0,0,2,1,0,0};uint32_t kind=1;
    memcpy(expected_id,&kind,4);memcpy(expected_id+4,media,8);
    memcpy(closure+8,expected_id,12);memcpy(closure+20,media,8);
    slots[0].epoch=1;slots[0].done=1;slots[0].manager=0x123456;memcpy(slots[0].media,media,8);
    epochs[0].store(1);pending_refresh[0].store(1);notified_epoch[0].store(1);
    information_task_hook((const NativeInformationClosure *)closure);
    assert(forwards==1 && calls==1 && !pending_refresh[0].load());
    assert(!memcmp(closure+8,expected_id,12) && !memcmp(closure+20,media,8));
    epochs[0].store(2);pending_refresh[0].store(1);
    information_task_hook((const NativeInformationClosure *)closure);assert(forwards==2 && calls==1 && pending_refresh[0].load()==1);
    slots[0].epoch=2;slots[0].done=2;pending_refresh[0].store(2);applied_epoch[0].store(0);
    closure[16]=0x22; // SourceID belongs to USB2 while the event belongs to USB1.
    information_task_hook((const NativeInformationClosure *)closure);assert(forwards==3 && calls==1);closure[16]=2;
    closure[20]=2;information_task_hook((const NativeInformationClosure *)closure);assert(forwards==4 && calls==1);closure[20]=1;
    *(uintptr_t *)(root+0x2b0)=0x777777;
    information_task_hook((const NativeInformationClosure *)closure);assert(forwards==5 && calls==1);*(uintptr_t *)(root+0x2b0)=0x123456;
    pthread_mutex_lock(&state_lock);information_task_hook((const NativeInformationClosure *)closure);pthread_mutex_unlock(&state_lock);
    assert(forwards==6 && calls==1 && pending_refresh[0].load()==2 && !notified_epoch[0].load());
    information_task_hook((const NativeInformationClosure *)closure);assert(forwards==7 && calls==2 && !pending_refresh[0].load());
    active.store(false);information_task_hook((const NativeInformationClosure *)closure);assert(forwards==8 && calls==2);
    puts("Actual information task: original forwarding, stale/foreign/mismatched identities, ownership, contention retry and SourceID preservation pass");
}

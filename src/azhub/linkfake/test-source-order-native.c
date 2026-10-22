/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <string.h>
typedef uint64_t (*fn8)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
static int ready=1,calls;
static unsigned char *observed_rows;
static uint64_t expected_first=101;
static struct {int player,slot;} fakes[3]={{7,5},{7,1},{8,3}};
static int lf_runtime_ready(void) {return ready;}
static int safe_source_identity(uint64_t key) {return key>=100 && key<=102 ? (int)key-100:-1;}
static int hub_source_display_number(int player,int slot) {(void)player;return slot;}
static uint64_t native_refresh(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
                               uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    assert(a && b==11 && c==12 && d==13 && e==14 && f==15 && g==16 && h==17);
    uint64_t first;memcpy(&first,observed_rows+48,sizeof first);
    assert(first==expected_first); /* Native refresh must see the final order. */
    calls++;return 0x5678;
}
static fn8 make_hook(uintptr_t address,fn8 replacement);
static uintptr_t model_vtable[3]={0,0,0x1c33a70};
#define HUB_SOURCE_MODEL_VTABLE ((uintptr_t)model_vtable)
#include "source-order-native.h"
static fn8 make_hook(uintptr_t address,fn8 replacement) {
    assert(address==0x1c30340 && replacement==hub_source_refresh_ordered);return native_refresh;
}
static void refresh(unsigned char *list) {
    assert(hub_source_refresh_ordered((uintptr_t)list,11,12,13,14,15,16,17)==0x5678);
}
int main(void) {
    unsigned char list[0x110]={0},model[32]={0},rows[5][40]={0};
    uintptr_t buffer=(uintptr_t)rows,vt=(uintptr_t)model_vtable;
    int count=5;
    uintptr_t model_pointer=(uintptr_t)model;memcpy(list+0x108,&model_pointer,sizeof model_pointer);
    memcpy(model,&vt,sizeof vt);memcpy(model+8,&buffer,sizeof buffer);
    memcpy(model+0x10,&count,4);memcpy(model+0x18,&count,4);
    for (int i=0;i<3;i++) {
        uint32_t kind=1;uint64_t key=100+i;
        memcpy(rows[i+1]+4,&kind,4);memcpy(rows[i+1]+8,&key,8);
        memset(rows[i+1]+16,40+i,24);
    }
    rows[0][0]=10;rows[4][0]=20;
    unsigned char original[sizeof rows];memcpy(original,rows,sizeof rows);
    observed_rows=(unsigned char *)rows;
    assert(!hub_source_order_install());refresh(list);
    assert(calls==1 && !memcmp(rows[0],original,40) && !memcmp(rows[4],original+160,40));
    assert(!memcmp(rows[1],original+80,40) && !memcmp(rows[2],original+120,40));
    assert(!memcmp(rows[3],original+40,40));
    ready=0;expected_first=100;memcpy(rows,original,sizeof rows);refresh(list);
    assert(calls==2 && !memcmp(rows,original,sizeof rows));
    ready=1;int capacity=1;memcpy(model+0x10,&capacity,4);refresh(list);
    assert(calls==3 && !memcmp(rows,original,sizeof rows));
    uintptr_t unknown_vtable[3]={0};vt=(uintptr_t)unknown_vtable;
    memcpy(model+0x10,&count,4);memcpy(model,&vt,sizeof vt);refresh(list);
    assert(calls==4 && !memcmp(rows,original,sizeof rows));
    vt=(uintptr_t)model_vtable;memcpy(model,&vt,sizeof vt);
    model_vtable[2]=0xfeedbeef; /* Core-owned getNumRows replacement. */
    uintptr_t wrapper[4]={0},wrapper_vt[3]={0,0,0x234eaf0};
    wrapper[0]=(uintptr_t)wrapper_vt;wrapper[3]=(uintptr_t)model;
    uintptr_t wrapped=(uintptr_t)wrapper;memcpy(list+0x108,&wrapped,sizeof wrapped);
    expected_first=101;
    refresh(list);assert(calls==5 && !memcmp(rows[1],original+80,40));
    return 0;
}

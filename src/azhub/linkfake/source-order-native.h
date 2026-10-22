/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HUB_SOURCE_ORDER_NATIVE_H
#define HUB_SOURCE_ORDER_NATIVE_H
#include <stdint.h>
#include "source-order.h"
#ifndef HUB_SOURCE_ORDER_LOG
#define HUB_SOURCE_ORDER_LOG(...) ((void)0)
#endif
#ifndef HUB_SOURCE_MODEL_VTABLE
#define HUB_SOURCE_MODEL_VTABLE ((uintptr_t)0x2e0b900)
#endif
static fn8 hub_source_refresh_original;
static int hub_source_row_number(const unsigned char *row) {
    uint32_t kind;uint64_t identity;
    memcpy(&kind,row+4,sizeof kind);
    if (kind!=1) return 0;
    memcpy(&identity,row+8,sizeof identity);
    int source=safe_source_identity(identity);
    return source<0 ? 0:hub_source_display_number(fakes[source].player,fakes[source].slot);
}
static void hub_source_model_order(uintptr_t model) {
    uintptr_t rows;int capacity,count;
    memcpy(&rows,(const void *)(model+8),sizeof rows);
    memcpy(&capacity,(const void *)(model+0x10),sizeof capacity);
    memcpy(&count,(const void *)(model+0x18),sizeof count);
    if (count>0 && count<=128 && capacity>=count)
        hub_source_order_rows((void *)rows,(size_t)count,hub_source_row_number);
}
static uintptr_t hub_source_list_model(uintptr_t list) {
    uintptr_t model=0,vtable=0,method=0;
    memcpy(&model,(const void *)(list+0x108),sizeof model);
    if (!model) return 0;
    memcpy(&vtable,(const void *)model,sizeof vtable);
    if (!vtable) return 0;
    if (vtable==HUB_SOURCE_MODEL_VTABLE) return model;
    memcpy(&method,(const void *)(vtable+0x10),sizeof method);
    HUB_SOURCE_ORDER_LOG("source order model=%#lx vtable=%#lx method=%#lx",
        (unsigned long)model,(unsigned long)vtable,(unsigned long)method);
    if (method==0x234eaf0) {
        memcpy(&model,(const void *)(model+0x18),sizeof model);
        if (!model) return 0;
        memcpy(&vtable,(const void *)model,sizeof vtable);
        if (!vtable) return 0;
        memcpy(&method,(const void *)(vtable+0x10),sizeof method);
    }
    /* Core replaces getNumRows in this exact table to add its menu row.
     * Validate the table identity, not that core-owned method's code address. */
    return vtable==HUB_SOURCE_MODEL_VTABLE ? model:0;
}
static uint64_t hub_source_refresh_ordered(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
                                         uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    if (lf_runtime_ready() && a) {
        uintptr_t model=hub_source_list_model((uintptr_t)a);
        if (model) hub_source_model_order(model);
    }
    return hub_source_refresh_original(a,b,c,d,e,f,g,h);
}
static int hub_source_order_install(void) {
    /* AZ-MODS owns model and controller add. Source's list refresh is separate;
     * validate its model interface before touching presentation-only rows. */
    hub_source_refresh_original=make_hook(0x1c30340,hub_source_refresh_ordered);
    return hub_source_refresh_original ? 0:-92;
}
#endif

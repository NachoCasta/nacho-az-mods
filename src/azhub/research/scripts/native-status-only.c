/* SPDX-License-Identifier: GPL-3.0-only */
/* Emulator-only native reference observer. No hub/library/network/input setup. */
#include "azmod-mod.h"
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static const az_core_v1 *lf_core;
static int lf_hook_failures;
typedef uint64_t (*fn8)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
static void lf_log(const char *format,...) {
    char text[768];va_list args;va_start(args,format);vsnprintf(text,sizeof text,format,args);va_end(args);
    lf_core->log("usb-hub-support","%s",text);
}
static void *make_hook(uintptr_t address,void *replacement) {
    void *original=NULL;
    if(lf_core->claim_hook("usb-hub-support",address,replacement,&original)) {
        ++lf_hook_failures;return NULL;
    }
    return original;
}
#include "../../linkfake/native-status-observer.h"
static int observer_init(const az_core_v1 *core) {
    if(!core || core->abi!=AZ_MOD_ABI || !AZ_HAS(core,claim_hook)) return -1;
    lf_core=core;return native_status_observer_install();
}
static const char *const firmware[]={"ep147-9e9454e357389ece",NULL};
static const az_mod_v1 observer={.abi=AZ_MOD_ABI,.size=sizeof(az_mod_v1),
    .id="usb-hub-support",.fw_tags=firmware,.init=observer_init};
AZ_MOD_EXPORT const az_mod_v1 *az_mod_v1_entry(void) { return &observer; }

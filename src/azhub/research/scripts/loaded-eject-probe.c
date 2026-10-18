/* SPDX-License-Identifier: GPL-3.0-only */
/* Emulator-only observation of native disconnect and unload modes. */
#include "azmod-mod.h"
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>
typedef unsigned long long (*fn8)(unsigned long long,unsigned long long,unsigned long long,unsigned long long,unsigned long long,unsigned long long,unsigned long long,unsigned long long);
static const az_core_v1 *core;
static fn8 disconnect_original,unload_original;
static unsigned long long disconnect_hook(unsigned long long a,unsigned long long b,unsigned long long c,unsigned long long d,unsigned long long e,unsigned long long f,unsigned long long g,unsigned long long h) {
    unsigned long long media=0; if (b) memcpy(&media,(void *)b,6);
    core->log("loaded-eject-probe","disconnect object=%#llx media=%#llx flag=%u caller=%#lx",a,media,(unsigned)c,(unsigned long)__builtin_return_address(0));
    return disconnect_original(a,b,c,d,e,f,g,h);
}
static unsigned long long unload_hook(unsigned long long a,unsigned long long b,unsigned long long c,unsigned long long d,unsigned long long e,unsigned long long f,unsigned long long g,unsigned long long h) {
    unsigned long long media=0; if (b) memcpy(&media,(void *)(b+0x10),6);
    core->log("loaded-eject-probe","exec-unload controller=%#llx media=%#llx mode=%u notify=%u",a,media,b ? *(unsigned char *)(b+0x20):255,b ? *(unsigned char *)(b+0x28):255);
    return unload_original(a,b,c,d,e,f,g,h);
}
static int init(const az_core_v1 *api) {
    struct utsname u;
    if (uname(&u) || strcmp(u.release,"6.6.138-dirty") || access("/dev/subucom_ctrl",F_OK)) return -1;
    if (!api || api->abi!=AZ_MOD_ABI || !AZ_HAS(api,claim_hook)) return -2;
    core=api;
    int r=core->claim_hook("loaded-eject-probe",0xa59b60,(void *)disconnect_hook,(void **)&disconnect_original);
    if (r) return r;
    return core->claim_hook("loaded-eject-probe",0xa77110,(void *)unload_hook,(void **)&unload_original);
}
static const char *const firmware[]={"ep147-9e9454e357389ece",0};
static const az_mod_v1 mod={.abi=AZ_MOD_ABI,.size=sizeof(az_mod_v1),.id="loaded-eject-probe",.fw_tags=firmware,.init=init};
AZ_MOD_EXPORT const az_mod_v1 *az_mod_v1_entry(void) { return &mod; }

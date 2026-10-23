/* SPDX-License-Identifier: GPL-3.0-only */
/* Emulator-only fault control; never packaged with Native USB Names.
 * Changes a returned presentation String, never stored properties or media. */
#include "azmod-mod.h"
#include <atomic>
#include <stdint.h>
#include <string.h>
#include <sys/utsname.h>
#define ID "nachocasta-name-fixture"
static const az_core_v1 *api;
static std::atomic<int> mode(1),port(2);
static int (*original)(uintptr_t,uintptr_t,const unsigned char *);
static void mode_change(int value,void *) {mode.store(value);}
static void port_change(int value,void *) {port.store(value);}
static int get_name(uintptr_t self,uintptr_t result,const unsigned char *media) {
    int status=original(self,result,media),m=mode.load(),p=port.load();
    if(!m || !media || media[0]!=1 || media[1] || media[2] || media[3] ||
       (media[4]!=2 && media[4]!=0x22) || (p<2 && media[4]!=(p ? 0x22:2)))return status;
    uintptr_t temporary=0;
    try {
        ((void (*)(void *,const char *))0x24a1180)(&temporary,m==3 ? "\x01":"");
        ((void (*)(void *,void *))0x247d330)((void *)result,&temporary);
        ((void (*)(void *))0x2469aa0)(&temporary);
        return m==1 ? 0:1;
    } catch(...) {api->log(ID,"fixture substitution failed");return status;}
}
static int init(const az_core_v1 *core) {
    api=core;struct utsname u;
    if(uname(&u) || strcmp(u.release,"6.6.138-dirty") || strcmp(core->fw_tag(),"ep147-9e9454e357389ece"))return -1;
    const uint32_t expected[]={0xa9bb7bfd,0x910003fd,0xa9025bf5,0xaa0203f5};
    if(memcmp((const void *)0x111c1e0,expected,sizeof expected))return -2;
    static const char *const choices[]={"Native value","Unavailable","Empty","Invalid control"};
    static const char *const ports[]={"USB1","USB2","Both"};
    az_row_v1 row={};row.size=sizeof row;row.kind=AZ_ROW_CHOICE;row.key="input";row.label="EMU name input";row.choices=choices;row.n_choices=4;row.default_choice=1;row.on_change=mode_change;
    row.help="Emulator-only presentation fault control. Does not rewrite metadata. Never install on hardware.";
    if(core->register_row(ID,&row)<0)return -3;
    row.key="port";row.label="EMU name port";row.choices=ports;row.n_choices=3;row.default_choice=2;row.on_change=port_change;
    if(core->register_row(ID,&row)<0)return -4;
    mode.store(core->cfg_get(ID,"input",1));port.store(core->cfg_get(ID,"port",2));
    if(core->claim_hook(ID,0x111c1e0,(void *)get_name,(void **)&original))return -5;
    core->log(ID,"Emulator presentation fault input=%d port=%d; metadata untouched",mode.load(),port.load());return 0;
}
static const char *const tags[]={"ep147-9e9454e357389ece",0};
static const az_mod_v1 mod={AZ_MOD_ABI,sizeof(az_mod_v1),ID,tags,init,0,0};
AZ_MOD_EXPORT const az_mod_v1 *az_mod_v1_entry(void) {return &mod;}

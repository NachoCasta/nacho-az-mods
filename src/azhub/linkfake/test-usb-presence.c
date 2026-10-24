/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include "azmod-mod.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "usb-presence.h"
static char directory[128];
static int present;
static const char *assets(const char *id) {
    assert(!strcmp(id,"usb-hub-support"));
    return present ? directory : NULL;
}
static void create_asset(const char *name,int populated) {
    char path[256];snprintf(path,sizeof path,"%s/%s",directory,name);
    FILE *file=fopen(path,"w");assert(file);
    if(populated) assert(fputs("fixture",file)>=0);
    assert(!fclose(file));
}
int main(void) {
    az_core_v1 core={.abi=AZ_MOD_ABI,.size=sizeof core,.assets_dir=assets};
    strcpy(directory,"/tmp/azhub-presence.XXXXXX");assert(mkdtemp(directory));
    assert(community_require_usb(&core)==-40);
    present=1;assert(community_require_usb(&core)==-40);
    create_asset("mount-hub.sh",1);assert(community_require_usb(&core)==-40);
    create_asset("azhub.ko",0);assert(community_require_usb(&core)==-40);
    create_asset("azhub.ko",1);assert(community_require_usb(&core)==-40);
    create_asset("recover-hubs.sh",0);assert(community_require_usb(&core)==-40);
    create_asset("recover-hubs.sh",1);assert(!community_require_usb(&core));
    present=0;assert(community_require_usb(&core)==-40);
    char path[256];snprintf(path,sizeof path,"%s/mount-hub.sh",directory);assert(!unlink(path));
    snprintf(path,sizeof path,"%s/azhub.ko",directory);assert(!unlink(path));
    snprintf(path,sizeof path,"%s/recover-hubs.sh",directory);assert(!unlink(path));assert(!rmdir(directory));
    core.assets_dir=NULL;assert(community_require_usb(&core)==-40);
    puts("USB presence initialization gate passed");
}

/* SPDX-License-Identifier: GPL-3.0-only */
/* Community SDK port: automatic hub discovery; hardware acceptance pending. */
#define _GNU_SOURCE
#include "azmod-mod.h"
static const az_core_v1 *lf_core;
static int lf_hook_failures;
#define LF_COMMUNITY 1
#define LF_NEXT RTLD_DEFAULT
static void community_discovery_poll(void);
#include "linkfake.c"
#include "discovery.h"
#include "kernel-asset.h"
#include "mount-worker.h"

static const char *community_mount_table(void) {
    const char *fixture=getenv("AZHUB_DISCOVERY_MOUNTS");
    return fixture ? fixture : "/proc/mounts";
}
static void community_enable_internal_link(void) {
    /* Exact-firmware writable cache, distinct from the persisted user setting.
     * Keep local library/member machinery active without selecting Link mode
     * (which disables local decks 3/4). Physical Link packets stay filtered. */
    volatile unsigned char *enabled=(volatile unsigned char *)0x3f86a08;
    if (!*enabled) {
        *enabled=1;
        lf_log("internal Link library cache enabled; user setting unchanged");
    }
}
static void community_refresh_presence(const char *config) {
    unsigned char seen[MAX_FAKES]={0};
    FILE *file=fopen(config,"r");
    if (!file) return; /* A failed scan must not withdraw all sources. */
    char line[512],ip[32],directory[256]; int player,slot;
    while (fgets(line,sizeof line,file)) {
        if (sscanf(line,"%d %31s %d %255s",&player,ip,&slot,directory)!=4) continue;
        for (int i=0;i<nfakes;i++) if (!strcmp(fakes[i].dir,directory)) seen[i]=1;
    }
    fclose(file);
    for (int i=0;i<nfakes;i++) {
        struct fake *source=&fakes[i];
        if (!seen[i] && source->present) {
            source->present=0;
            source->notify_pending=0;
            source->mount_at=0;
            source->announced_at=source->queried_at=0;
            source->unmount_pending=1;
            lf_log("hub source absent player %d slot %d path=%s",source->player,source->slot,source->dir);
        } else if (seen[i] && !source->present && !source->unmount_pending) {
            source->present=1;
            source->notify_pending=1;
            lf_log("hub source returned player %d slot %d path=%s",source->player,source->slot,source->dir);
        }
    }
}
static void community_discovery_poll(void) {
    community_enable_internal_link();
    if (!getenv("LINKFAKE_AUTODISCOVER")) return;
    int count=azhub_discover(community_mount_table(),"/sys/class/block","/tmp/azhub-discovered.conf");
    if (count>=0) {
        community_refresh_presence("/tmp/azhub-discovered.conf");
        int previous=nfakes;
        linkfake_load_config();
        if (nfakes>previous) lf_log("discovery published %d hub sources (previous %d)",(int)nfakes,previous);
    }
}

static int community_init(const az_core_v1 *core) {
    if (!core || core->abi!=AZ_MOD_ABI || !AZ_HAS(core,claim_hook)) return -1;
    lf_core=core;
    int kernel_result=community_prepare_kernel(core);
    if (kernel_result) return kernel_result;
    const char *config="/home/root/settings/azhub.conf";
    if (access(config,R_OK)) {
        config="/tmp/azhub-discovered.conf";
        if (azhub_discover(community_mount_table(),"/sys/class/block",config)<0) return -2;
        setenv("LINKFAKE_AUTODISCOVER","1",1);
    }
    setenv("LINKFAKE_CONF",config,1);
    setenv("LINKFAKE_TCP_LOOP","1",1);
    setenv("LINKFAKE_REQ","5",1);
    setenv("LINKFAKE_SETUP_AS_TARGET","1",1);
    setenv("LINKFAKE_PROBE","1",1);
    setenv("LINKFAKE_HUBSRV","1",1);
    setenv("LINKFAKE_SLOT_CAPS","1",1);
    setenv("LINKFAKE_LATE_SERVERS","1",1);
    setenv("LINKFAKE_LOCAL_ONLY","1",1);
    setenv("LINKFAKE_BROWSER_GUARD","1",1);
    setenv("LINKFAKE_PRIVATE_TAG_CACHE","1",1);
    unsetenv("LINKFAKE_SLOT");
    /* EP147 PLT only: libc itself and the core's own imports stay untouched. */
    const struct { uintptr_t address; void *hook; } entries[]={
        {0x425c60,msgsnd},{0x425e90,send},{0x426170,read},
        {0x4267b0,sendto},{0x426840,recvfrom},{0x426860,close},
        {0x427430,recv},{0x4275f0,connect},{0x428620,write}
    };
    for (unsigned i=0;i<sizeof entries/sizeof entries[0];i++)
        make_hook(entries[i].address,entries[i].hook);
    if (lf_hook_failures) return -3;
    linkfake_init();
    if (lf_hook_failures) return -4;
    if (!AZ_HAS(core,thread_start) || !AZ_HAS(core,assets_dir)) return -27;
    if (core->thread_start("hub-libraries","HubMount",community_mount_worker,NULL)) return -28;
    return (nfakes || getenv("LINKFAKE_AUTODISCOVER")) ? 0 : -5;
}
static const char *const hub_firmware[]={"ep147-9e9454e357389ece",NULL};
static const az_mod_v1 hub_mod={
    .abi=AZ_MOD_ABI,.size=sizeof(az_mod_v1),.id="hub-libraries",
    .fw_tags=hub_firmware,.init=community_init
};
AZ_MOD_EXPORT const az_mod_v1 *az_mod_v1_entry(void) { return &hub_mod; }

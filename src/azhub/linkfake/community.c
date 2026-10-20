/* SPDX-License-Identifier: GPL-3.0-only */
/* Community SDK port: automatic hub discovery; hardware acceptance pending. */
#define _GNU_SOURCE
#include "azmod-mod.h"
#ifdef LF_SAFE_EJECT
#include <pthread.h>
#include <time.h>
static pthread_mutex_t safe_mount_mutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t safe_source_mutex=PTHREAD_MUTEX_INITIALIZER;
static int safe_eject_blocked_device(const char *device);
static int safe_eject_blocked_path(const char *path);
static int safe_eject_prepare(void);
struct fake;
static int safe_server_begin(int source);
static void safe_server_end(int source);
static int safe_eject_source_poll(struct fake *source,time_t now,time_t *ready_at,int waiting);
#endif
static const az_core_v1 *lf_core;
static _Atomic int community_ready; /* 0 preparing, 1 running, -1 aborted */
static int lf_hook_failures;
#define LF_COMMUNITY 1
#define LF_NEXT RTLD_DEFAULT
static void community_discovery_poll(void);
#include "linkfake.c"
#include "discovery.h"
#include "kernel-asset.h"
#include "usb-presence.h"
#include "mount-worker.h"
#ifdef LF_EJECT_UI
#include "eject-ui.h"
#endif
#ifdef LF_SAFE_EJECT
#include "safe-eject.h"
#include "link-cached-status.h"
#ifdef LF_EJECT_UI
#include "source-badge-render.h"
#endif
#endif

static const char *community_mount_table(void) {
    const char *fixture=getenv("AZHUB_DISCOVERY_MOUNTS");
    return fixture ? fixture : "/proc/mounts";
}
static void probe_mode_maintain_cache(void);
static void community_enable_internal_link(void) { probe_mode_maintain_cache(); }
static void community_enable_internal_link_unlocked(void) {
    /* Exact-firmware writable cache, distinct from the persisted user setting.
     * Keep local library/member machinery active without selecting Link mode
     * (which disables local decks 3/4). Private hub transport is admitted separately from physical Link. */
    volatile unsigned char *enabled=(volatile unsigned char *)0x3f86a08;
    if (!*enabled) {
        *enabled=1;
        lf_log("internal Link library cache enabled; user setting unchanged");
    }
}
static void community_refresh_presence(const char *config) {
    unsigned char seen[MAX_FAKES]={0};
    FILE *file=fopen(config,"re");
    if (!file) return; /* A failed scan must not withdraw all sources. */
    char line[512],ip[32],directory[256]; int player,slot;
    while (fgets(line,sizeof line,file)) {
        if (sscanf(line,"%d %31s %d %255s",&player,ip,&slot,directory)!=4) continue;
        for (int i=0;i<nfakes;i++) if (!strcmp(fakes[i].dir,directory)) seen[i]=1;
    }
    fclose(file);
    for (int i=0;i<nfakes;i++) {
        struct fake *source=&fakes[i];
#ifdef LF_SAFE_EJECT
        if (source->eject_reserved) continue;
#endif
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
#ifdef LF_SAFE_EJECT
    pthread_mutex_lock(&safe_source_mutex);
#endif
    int count=azhub_discover(community_mount_table(),"/sys/class/block","/tmp/azhub-discovered.conf");
    if (count>=0) {
        community_refresh_presence("/tmp/azhub-discovered.conf");
        int previous=nfakes;
        linkfake_load_config();
        if (nfakes>previous) lf_log("discovery published %d hub sources (previous %d)",(int)nfakes,previous);
    }
#ifdef LF_SAFE_EJECT
    pthread_mutex_unlock(&safe_source_mutex);
#endif
}

/* Workers have not crossed readiness when initialization aborts. Publish a
 * terminal state so they exit, and close the unpublished loopback sockets. */
static int community_abort(int result) {
    __atomic_store_n(&community_ready,-1,__ATOMIC_RELEASE);
    for (int i=0;i<nfakes;i++) if (fakes[i].sock>=0) {
        close(fakes[i].sock);fakes[i].sock=-1;
    }
    return result;
}
#include "link-private-admission.h"

#include "link-library-ownership.h"

#include "link-mode-recovery.h"

#include "link-request-origin.h"

static int community_init(const az_core_v1 *core) {
    if (!core || core->abi!=AZ_MOD_ABI || !AZ_HAS(core,claim_hook)) return -1;
    lf_core=core;
    if (community_require_usb(core)) {
        core->log("usb-hub-support","matching USB package assets unavailable; initialization refused");
        return -40;
    }
    hub_mount_restore_fault();
#ifdef LF_SAFE_EJECT
    int safe_prepare_result=safe_eject_prepare();
    if (safe_prepare_result) return safe_prepare_result;
#endif
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
    unsetenv("LINKFAKE_LOCAL_ONLY");
    setenv("LINKFAKE_BROWSER_GUARD","1",1);
    setenv("LINKFAKE_PRIVATE_TAG_CACHE","1",1);
    unsetenv("LINKFAKE_SLOT");
    if (!AZ_HAS(core,thread_start) || !AZ_HAS(core,assets_dir) ||
        !AZ_HAS(core,launch) || !AZ_HAS(core,launch_alive) || !AZ_HAS(core,launch_stop)) return -27;
    /* Prepare fallible resources before the irreversible hook phase. Workers
     * remain idle until every claim succeeds. */
#if defined(LF_SAFE_EJECT) && defined(LF_EJECT_UI)
    if (source_badge_prepare()) return community_abort(-90);
#endif
    linkfake_init();
    if (lf_hook_failures) return community_abort(-4);
    if (hub_tag_cache_prepare()) return community_abort(-30);
    if (core->thread_start("usb-hub-support","HubMount",community_mount_worker,NULL)) return community_abort(-28);
    if ((nfakes || getenv("LINKFAKE_AUTODISCOVER")) &&
        core->thread_start("usb-hub-support","HubTransport",injector,NULL)) return community_abort(-29);
#ifdef LF_EJECT_UI
    int eject_result=eject_init();
    if (eject_result) return community_abort(eject_result);
#endif
    /* EP147 PLT only: libc itself and the core's own imports stay untouched. */
    const struct { uintptr_t address; void *hook; } entries[]={
        {0x425c60,msgsnd},{0x425e90,send},{0x426170,read},
        {0x4267b0,sendto},{0x426840,recvfrom},{0x426860,close},
        {0x427430,recv},{0x4275f0,connect},{0x428620,write}
    };
    for (unsigned i=0;i<sizeof entries/sizeof entries[0];i++)
        make_hook(entries[i].address,entries[i].hook);
    if (lf_hook_failures) return community_abort(-3);
    if (getenv("LINKFAKE_PROBE")) probe_install();
    if (lf_hook_failures) return community_abort(-4);
#ifdef LF_EJECT_UI
    eject_result=eject_install_hooks();
    if (eject_result) return community_abort(eject_result);
#endif
    probe_private_admission_install();
    if (lf_hook_failures) return community_abort(-36);
    probe_library_ownership_install();
    if (lf_hook_failures) return community_abort(-37);
    probe_mode_recovery_install();
    if (lf_hook_failures) return community_abort(-38);
    probe_queue_install();
    if (lf_hook_failures) return community_abort(-39);
#if defined(LF_SAFE_EJECT) && defined(LF_EJECT_UI)
    int badge_result=source_badge_install();
    if (badge_result || lf_hook_failures) return community_abort(-91);
#endif
    __atomic_store_n(&community_ready,1,__ATOMIC_RELEASE);
#ifdef LF_SAFE_EJECT
    /* Keep the selector/coordinator available with no active libraries, including
     * persisted safe states and an optional empty legacy configuration. */
    return 0;
#else
    return (nfakes || getenv("LINKFAKE_AUTODISCOVER")) ? 0 : -5;
#endif
}
static const char *const hub_firmware[]={"ep147-9e9454e357389ece",NULL};
static const az_mod_v1 hub_mod={
    .abi=AZ_MOD_ABI,.size=sizeof(az_mod_v1),.id="usb-hub-support",
    .fw_tags=hub_firmware,.init=community_init
#ifdef LF_EJECT_UI
    ,.tick=probe_queue_tick
#endif
};
AZ_MOD_EXPORT const az_mod_v1 *az_mod_v1_entry(void) { return &hub_mod; }

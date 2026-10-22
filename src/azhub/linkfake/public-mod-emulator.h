/* SPDX-License-Identifier: GPL-3.0-only */
/* Explicit emulator integration gate. This consumes an externally owned test
 * namespace; production network allocation is intentionally not supplied here.
 * All controller calls run on HubTransport, never on the UI/eject worker. */
#ifndef HUB_PUBLIC_MOD_EMULATOR_H
#define HUB_PUBLIC_MOD_EMULATOR_H
#include <sys/utsname.h>
/* Public datagrams must not update private identity observations or traverse
 * the native UDP ingress adapter. These sockets have their own peer policy. */
#define sendto(fd,b,n,f,a,l) syscall(SYS_sendto,fd,b,n,f,a,l)
#define recvmsg(fd,m,f) syscall(SYS_recvmsg,fd,m,f)
#include "public-network-endpoint.h"
#undef sendto
#undef recvmsg
static struct hub_public_source_manager public_mod_manager;
static struct hub_public_network_spec public_mod_specs[HUB_PUBLIC_MANAGED_SOURCES];
static int public_mod_enabled;
static unsigned public_mod_count;
struct public_mod_observation {
    int seen,reported,last_state,last_error,last_bound,last_wanted;
    uint64_t incarnation,last_incarnation;
    dev_t device;
    ino_t inode;
};
static struct public_mod_observation public_mod_observations[HUB_PUBLIC_MANAGED_SOURCES];
#ifndef HUB_PUBLIC_MOD_REPORT
#define HUB_PUBLIC_MOD_REPORT(slot,incarnation,state,error,bound,wanted) ((void)0)
#endif
static void public_mod_report(unsigned slot) {
    struct public_mod_observation *observation=public_mod_observations+slot;
    struct hub_public_managed_source *source=public_mod_manager.sources+slot;
    int state=hub_public_source_worker_state(&source->worker);
    if (!observation->reported || state!=observation->last_state || source->error!=observation->last_error ||
            source->bound!=observation->last_bound || source->wanted!=observation->last_wanted ||
            observation->incarnation!=observation->last_incarnation) {
        HUB_PUBLIC_MOD_REPORT(slot,observation->incarnation,state,source->error,source->bound,source->wanted);
        observation->reported=1; observation->last_state=state; observation->last_error=source->error;
        observation->last_bound=source->bound; observation->last_wanted=source->wanted;
        observation->last_incarnation=observation->incarnation;
    }
}
static int public_mod_peer(void *unused,const struct sockaddr_in *peer) {
    (void)unused;
    return peer && peer->sin_family==AF_INET && peer->sin_addr.s_addr==inet_addr("192.0.2.2");
}
static int public_mod_configured_count(const char *configured) {
    if (!configured) return 1;
    if (configured[0]<'1' || configured[0]>'8' || configured[1]) { errno=EINVAL; return -1; }
    return configured[0]-'0';
}
static int public_mod_prepare(void) {
    const char *enabled=getenv("AZHUB_PUBLIC_EMULATOR");
    if (!enabled) return 0;
    struct utsname kernel;
    if (strcmp(enabled,"1") || uname(&kernel) || strcmp(kernel.release,"6.6.138-dirty")) {
        errno=EPERM; return -1;
    }
    int configured=public_mod_configured_count(getenv("AZHUB_PUBLIC_EMULATOR_COUNT"));
    if (configured<0) return -1;
    unsigned count=(unsigned)configured,opened=0;
    for (unsigned slot=0;slot<count;++slot) {
        struct hub_public_network_spec *spec=public_mod_specs+slot;
        char path[80];
        if (slot) snprintf(path,sizeof path,"/var/run/netns/azhub-pubtest%u",slot);
        else strcpy(path,"/var/run/netns/azhub-pubtest");
        spec->namespace_fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
        if (spec->namespace_fd<0) goto fail;
        ++opened;
        snprintf(spec->interface,sizeof spec->interface,"azpub%u",slot);
        spec->address.s_addr=htonl(0xc0000203u+slot);
        spec->mask.s_addr=inet_addr("255.255.255.0");
        spec->broadcast.s_addr=inet_addr("192.0.2.255");
        const unsigned char mac[6]={2,0,0,0,0x12,(unsigned char)(3+slot)}; memcpy(spec->mac,mac,6);
        spec->player=41+slot; snprintf(spec->label,sizeof spec->label,"Hub USB %u",slot+1);
        spec->allowed=public_mod_peer;
    }
    if (hub_public_source_manager_init(&public_mod_manager,
            (struct hub_public_source_manager_ops){hub_public_network_manager_open,hub_public_network_manager_close},
            public_mod_specs)<0) goto fail;
    public_mod_count=count; public_mod_enabled=1;
    return 0;
fail: {
    int error=errno;
    for (unsigned slot=0;slot<opened;++slot) {
        close(public_mod_specs[slot].namespace_fd); public_mod_specs[slot].namespace_fd=-1;
    }
    errno=error; return -1;
    }
}
static void public_mod_abort(void) {
    /* Initialization has not crossed community_ready, so no worker can exist. */
    if (public_mod_enabled) for (unsigned slot=0;slot<public_mod_count;++slot) {
        close(public_mod_specs[slot].namespace_fd); public_mod_specs[slot].namespace_fd=-1;
    }
    public_mod_enabled=0; public_mod_count=0;
}
static void public_mod_poll_source(unsigned slot) {
    struct fake *source=fakes+slot;
    struct public_mod_observation *observation=public_mod_observations+slot;
    struct stat status;
    int exists=slot<(unsigned)nfakes;
    int wanted=exists && source->present && !source->unmount_pending;
    if (exists && source->eject_reserved) {
        hub_public_source_manager_withdraw(&public_mod_manager,slot);
    } else {
        if (wanted && (stat(source->dir,&status) || !S_ISDIR(status.st_mode))) wanted=0;
        if (wanted && (!observation->seen || status.st_dev!=observation->device || status.st_ino!=observation->inode)) {
            if (observation->incarnation==UINT64_MAX) wanted=0;
            else { ++observation->incarnation; observation->device=status.st_dev; observation->inode=status.st_ino; }
        }
        observation->seen=wanted;
        hub_public_source_manager_request(&public_mod_manager,slot,wanted ? source->dir : NULL,
                observation->incarnation,wanted);
    }
    hub_public_source_manager_poll(&public_mod_manager,slot);
    public_mod_report(slot);
}
static void public_mod_poll(void) {
    if (!public_mod_enabled) return;
    for (unsigned slot=0;slot<public_mod_count;++slot) public_mod_poll_source(slot);
}
static int public_mod_drain(unsigned slot) {
    if (!public_mod_enabled || slot>=HUB_PUBLIC_MANAGED_SOURCES || slot>=public_mod_count) return 1;
    hub_public_source_manager_request(&public_mod_manager,slot,NULL,0,0);
    public_mod_observations[slot].seen=0;
    hub_public_source_manager_poll(&public_mod_manager,slot);
    public_mod_report(slot);
    return hub_public_source_manager_drained(&public_mod_manager,slot);
}
#endif

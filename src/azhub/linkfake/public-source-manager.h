/* SPDX-License-Identifier: GPL-3.0-only */
/* Single-controller ownership of source incarnations. Endpoint callbacks must
 * allocate fresh sockets for each open and release them only after join.
 * Keep this controller off the UI thread; request/poll never waits for staging.
 * The endpoint allocator, private libraries and unmount remain caller-owned. */
#ifndef HUB_PUBLIC_SOURCE_MANAGER_H
#define HUB_PUBLIC_SOURCE_MANAGER_H
#include "public-source-worker.h"
#define HUB_PUBLIC_MANAGED_SOURCES 8
struct hub_public_source_binding {
    unsigned player;
    int listener,udp,query_listener;
    uint16_t file_port,metadata_port;
    hub_public_rpc_peer_policy allowed;
    void *policy;
    int advertise;
    struct hub_public_provider_endpoint provider;
};
struct hub_public_source_manager_ops {
    int (*open)(void *,unsigned,struct hub_public_source_binding *);
    void (*close)(void *,unsigned,struct hub_public_source_binding *);
};
struct hub_public_managed_source {
    struct hub_public_source_worker worker;
    struct hub_public_source_binding binding;
    char directory[4096];
    uint64_t identity,requested,attempted;
    int wanted,bound,error,hold_withdrawn,directory_fd;
};
struct hub_public_source_manager {
    struct hub_public_managed_source sources[HUB_PUBLIC_MANAGED_SOURCES];
    struct hub_public_source_manager_ops ops;
    void *context;
};
static int hub_public_source_manager_init(struct hub_public_source_manager *manager,
        struct hub_public_source_manager_ops ops,void *context) {
    if (!manager || !ops.open || !ops.close) { errno=EINVAL; return -1; }
    memset(manager,0,sizeof *manager); manager->ops=ops; manager->context=context;
    for (unsigned i=0;i<HUB_PUBLIC_MANAGED_SOURCES;++i) manager->sources[i].directory_fd=-1;
    return 0;
}
/* Call once per observed presence/path/mount-identity change. Repeated identical
 * observations do not restart a failed source or disturb an existing source. */
static int hub_public_source_manager_request(struct hub_public_source_manager *manager,unsigned slot,
        const char *directory,uint64_t identity,int wanted) {
    if (slot>=HUB_PUBLIC_MANAGED_SOURCES || (wanted && (!directory || !*directory ||
            !identity || strnlen(directory,4096)>=4096))) { errno=EINVAL; return -1; }
    struct hub_public_managed_source *source=manager->sources+slot;
    wanted=!!wanted;
    if (source->wanted==wanted && (!wanted ||
            (source->identity==identity && !strcmp(source->directory,directory)))) return 0;
    if (source->requested==UINT64_MAX) { errno=EOVERFLOW; return -1; }
    if (source->directory_fd>=0) { close(source->directory_fd);source->directory_fd=-1; }
    ++source->requested; source->wanted=wanted; source->hold_withdrawn=0;
    if (wanted) { strcpy(source->directory,directory); source->identity=identity; }
    return 0;
}
/* Caller retains its verified directory FD. The manager owns a duplicate until
 * withdrawal/replacement, and the worker independently duplicates before start.
 * A repeated incarnation must refer to the same object; no silent substitution.
 * This API does not authorize the mount or an external protocol generation. */
static int hub_public_source_manager_request_fd(struct hub_public_source_manager *manager,unsigned slot,
        int directory,uint64_t identity) {
    if (slot>=HUB_PUBLIC_MANAGED_SOURCES || !identity) { errno=EINVAL;return -1; }
    struct stat state;int flags=fcntl(directory,F_GETFL);
    if (flags<0 || fstat(directory,&state)<0 || !S_ISDIR(state.st_mode) ||
            (flags&O_ACCMODE)!=O_RDONLY || (flags&O_PATH)) { errno=EINVAL;return -1; }
    struct hub_public_managed_source *source=manager->sources+slot;
    if (source->wanted && source->identity==identity) {
        /* After withdrawal the retained FD is gone, and identical requests must
         * not clear the hold or republish the ejected incarnation. */
        if (source->hold_withdrawn) return 0;
        struct stat existing;
        if (source->directory_fd<0 || fstat(source->directory_fd,&existing)<0 ||
                existing.st_dev!=state.st_dev || existing.st_ino!=state.st_ino) { errno=ESTALE;return -1; }
        return 0;
    }
    if (source->requested==UINT64_MAX) { errno=EOVERFLOW;return -1; }
    int owned=fcntl(directory,F_DUPFD_CLOEXEC,0);if (owned<0) return -1;
    if (source->directory_fd>=0) close(source->directory_fd);
    source->directory_fd=owned;source->directory[0]=0;
    ++source->requested;source->wanted=1;source->identity=identity;source->hold_withdrawn=0;
    return 0;
}
static int hub_public_source_manager_nonce(unsigned char nonce[16]) {
    int fd=open("/dev/urandom",O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
    if (fd<0) return -1;
    size_t filled=0;
    while (filled<16) {
        ssize_t got=read(fd,nonce+filled,16-filled);
        if (got<0 && errno==EINTR) continue;
        if (got<=0) { int error=got<0 ? errno : EIO; close(fd); errno=error; return -1; }
        filled+=(size_t)got;
    }
    return close(fd);
}
/* First retire public visibility/readers while retaining STALE RPC responses.
 * A subsequent wanted0 request stops/joins and releases the endpoints. */
static int hub_public_source_manager_withdraw(struct hub_public_source_manager *manager,unsigned slot) {
    if (slot>=HUB_PUBLIC_MANAGED_SOURCES) { errno=EINVAL; return -1; }
    struct hub_public_managed_source *source=manager->sources+slot;
    source->hold_withdrawn=1;
    if (source->directory_fd>=0) { close(source->directory_fd);source->directory_fd=-1; }
    if (source->worker.started) hub_public_source_worker_withdraw(&source->worker);
    return 0;
}
/* One source per call, bounded worker/join polling. A source that fails stays
 * retired for that request; a new observed incarnation is required to retry. */
static int hub_public_source_manager_poll(struct hub_public_source_manager *manager,unsigned slot) {
    if (slot>=HUB_PUBLIC_MANAGED_SOURCES) { errno=EINVAL; return -1; }
    struct hub_public_managed_source *source=manager->sources+slot;
    if (source->worker.started) {
        int state=hub_public_source_worker_state(&source->worker);
        if (!source->wanted || source->requested!=source->attempted ||
            (state==HUB_PUBLIC_WORKER_WITHDRAWN && !source->hold_withdrawn))
            hub_public_source_worker_stop(&source->worker);
        int joined=hub_public_source_worker_join_ready(&source->worker);
        if (joined<0) return -1;
        if (!joined) return 0;
        source->error=source->worker.error;
    }
    if (source->bound) {
        manager->ops.close(manager->context,slot,&source->binding); source->bound=0;
    }
    /* Eject can arrive before the first poll or while an older incarnation is
     * joining. Never publish a pending incarnation after its withdrawal. */
    if (!source->wanted || source->hold_withdrawn || source->attempted==source->requested) return 0;
    source->attempted=source->requested; source->error=0;
    unsigned char nonce[16];
    if (hub_public_source_manager_nonce(nonce)<0) { source->error=errno; return 0; }
    memset(&source->binding,0,sizeof source->binding);
    source->binding.listener=source->binding.udp=source->binding.query_listener=-1;
    /* A failed open must release any partial resources before returning. */
    if (manager->ops.open(manager->context,slot,&source->binding)<0) { source->error=errno; return 0; }
    source->bound=1;
    struct hub_public_source_binding *binding=&source->binding;
    if (hub_public_source_worker_start_root(&source->worker,source->directory,source->directory_fd,
            source->requested,nonce,binding->player,binding->listener,binding->udp,
            binding->file_port,binding->allowed,binding->policy,binding->query_listener,
            binding->metadata_port,binding->advertise ? &binding->provider : NULL)<0) {
        source->error=errno;
        manager->ops.close(manager->context,slot,binding); source->bound=0;
    }
    return 0;
}
/* True only after the worker joined AND its endpoint resources were released.
 * Native eject may use this as an additional gate, never as unmount proof. */
static int hub_public_source_manager_drained(const struct hub_public_source_manager *manager,unsigned slot) {
    return slot<HUB_PUBLIC_MANAGED_SOURCES && !manager->sources[slot].worker.started && !manager->sources[slot].bound;
}
#endif

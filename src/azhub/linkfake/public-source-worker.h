/* SPDX-License-Identifier: GPL-3.0-only */
/* Source-owned staging/transport thread. Zero-initialize before first start.
 * The controller owns bound sockets and peer-policy context until join succeeds.
 * Only request flags and acquire-loaded status cross the thread boundary.
 * No UI callback waits for staging or a blocking thread join. */
#ifndef HUB_PUBLIC_SOURCE_WORKER_H
#define HUB_PUBLIC_SOURCE_WORKER_H
#include <poll.h>
#include <time.h>
#include "public-source-service.h"
#include "public-db-port.h"
#include "public-provider-udp.h"
enum hub_public_worker_state {
    HUB_PUBLIC_WORKER_IDLE,HUB_PUBLIC_WORKER_BUILDING,HUB_PUBLIC_WORKER_RUNNING,
    HUB_PUBLIC_WORKER_WITHDRAWN,HUB_PUBLIC_WORKER_STOPPED,HUB_PUBLIC_WORKER_FAILED
};
struct hub_public_source_worker {
    pthread_t thread;
    int started,state,error,stop,withdraw;
    char directory[4096];
    int directory_fd;
    uint64_t generation;
    unsigned char nonce[16];
    unsigned player;
    int listener,udp,query_listener;
    uint16_t port,metadata_port;
    hub_public_rpc_peer_policy allowed;
    void *policy;
    int advertise;
    struct hub_public_provider_endpoint provider;
};
static uint64_t hub_public_worker_milliseconds(void) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC,&now)<0) return 0;
    return (uint64_t)now.tv_sec*1000+(uint64_t)now.tv_nsec/1000000;
}
static void *hub_public_worker_run(void *opaque) {
    struct hub_public_source_worker *worker=opaque;
    struct hub_public_root root;
    struct hub_db_port_service query;
    struct hub_public_provider_udp provider;
    int provider_started=0;
    hub_db_port_init(&query,worker->query_listener,worker->metadata_port,worker->allowed,worker->policy);
    struct hub_public_library_files *library=calloc(1,sizeof *library);
    struct hub_public_source_service *service=calloc(1,sizeof *service);
    int root_initialized=0,library_attempted=0,error=0;
    if (!library || !service) { error=ENOMEM; goto done; }
    if (hub_public_root_init(&root)<0) { error=errno; goto done; }
    root_initialized=1;
    int attached=worker->directory_fd>=0 ? hub_public_root_attach_fd(&root,worker->directory_fd,worker->generation) :
        hub_public_root_attach(&root,worker->directory,worker->generation);
    if (attached<0) { error=errno; goto done; }
    library_attempted=1;
    if (hub_public_library_files_init(library,&root,worker->generation,worker->nonce)<0) { error=errno; goto done; }
    for (;;) {
        if (__atomic_load_n(&worker->stop,__ATOMIC_ACQUIRE) ||
            __atomic_load_n(&worker->withdraw,__ATOMIC_ACQUIRE)) goto done;
        int result=hub_public_library_files_step(library);
        if (result<0) { error=errno; goto done; }
        if (result==1) break;
    }
    if (hub_public_source_service_init(service,library,worker->player,worker->listener,
            worker->udp,worker->port,worker->allowed,worker->policy)<0) { error=errno; goto done; }
    if (worker->advertise) {
        if (worker->provider.rpc_alias_enabled) service->rpc_alias=worker->provider.rpc_alias_fd;
        if (hub_public_export_set_group(&service->files.rpc,worker->provider.export_group)<0) { error=errno; goto done; }
        hub_public_provider_udp_init(&provider,&worker->provider,hub_public_worker_milliseconds(),worker->allowed,worker->policy);
        provider_started=1;
    }
    fprintf(stderr,"public source library ready player=%u generation=%llu tracks=%u metadata=%u\n",
        worker->player,(unsigned long long)worker->generation,library->tracks,worker->metadata_port);
    __atomic_store_n(&worker->state,HUB_PUBLIC_WORKER_RUNNING,__ATOMIC_RELEASE);
    while (!__atomic_load_n(&worker->stop,__ATOMIC_ACQUIRE)) {
        if (__atomic_load_n(&worker->withdraw,__ATOMIC_ACQUIRE) &&
            __atomic_load_n(&worker->state,__ATOMIC_RELAXED)!=HUB_PUBLIC_WORKER_WITHDRAWN) {
            hub_public_source_service_withdraw(service);
            __atomic_store_n(&worker->state,HUB_PUBLIC_WORKER_WITHDRAWN,__ATOMIC_RELEASE);
        }
        if (hub_public_source_service_step(service,hub_public_worker_milliseconds())<0) { error=errno; break; }
        if (worker->query_listener>=0 && hub_db_port_step(&query,hub_public_worker_milliseconds(),
                hub_public_library_ready_catalog(library)!=NULL)<0) { error=errno; break; }
        if (!hub_public_library_ready_catalog(library))
            __atomic_store_n(&worker->state,HUB_PUBLIC_WORKER_WITHDRAWN,__ATOMIC_RELEASE);
        if (provider_started) {
            if (hub_public_provider_udp_step(&provider,library->tracks,hub_public_worker_milliseconds(),
                    hub_public_library_ready_catalog(library)!=NULL)<0) { error=errno; break; }
            /* A player-number collision cannot be repaired by reacquiring the
             * same identity. Reserve EADDRINUSE recovery for allocator ACD. */
            if (provider.conflict) { error=EEXIST; break; }
        }
        poll(NULL,0,1);
    }
done:
    if (provider_started) hub_public_provider_withdraw(&provider.state);
    hub_db_port_destroy(&query);
    if (service) { hub_public_source_service_withdraw(service); hub_public_source_service_destroy(service); }
    if (library_attempted) hub_public_library_files_destroy(library);
    if (root_initialized) {
        hub_public_root_withdraw(&root);
        if (hub_public_root_destroy(&root)<0 && !error) error=errno;
    }
    if (worker->directory_fd>=0) { close(worker->directory_fd);worker->directory_fd=-1; }
    free(service); free(library);
    if (error) fprintf(stderr,"public source worker failed player=%u generation=%llu errno=%d\n",worker->player,(unsigned long long)worker->generation,error);
    worker->error=error;
    __atomic_store_n(&worker->state,error ? HUB_PUBLIC_WORKER_FAILED : HUB_PUBLIC_WORKER_STOPPED,__ATOMIC_RELEASE);
    return NULL;
}
/* nonce must be freshly generated for every incarnation, including reconnects.
 * Start/join belong to one controller thread; requests may come from others. */
static int hub_public_source_worker_start_root(struct hub_public_source_worker *worker,
        const char *directory,int directory_fd,uint64_t generation,const unsigned char nonce[16],
        unsigned player,int listener,int udp,uint16_t port,
        hub_public_rpc_peer_policy allowed,void *policy,int query_listener,uint16_t metadata_port,
        const struct hub_public_provider_endpoint *provider) {
    if (!worker || worker->started || (!directory && directory_fd<0) || !nonce || !generation || !allowed ||
        !player || player>255 || listener<0 || (udp>=0 && !port) ||
        (directory && strnlen(directory,4096)>=4096) || (query_listener>=0 && !metadata_port) ||
        (provider && (provider->discovery_fd<0 || provider->status_fd<0 || !provider->broadcast ||
            provider->identity.number!=player || query_listener<0 || udp<0 ||
            (provider->rpc_alias_enabled && (provider->rpc_alias_fd<0 || provider->rpc_alias_fd==udp))))) { errno=EINVAL; return -1; }
    int owned=-1;
    if (directory_fd>=0) {
        struct stat state;int flags=fcntl(directory_fd,F_GETFL);
        if (flags<0 || fstat(directory_fd,&state)<0 || !S_ISDIR(state.st_mode) ||
                (flags&O_ACCMODE)!=O_RDONLY || (flags&O_PATH)) { errno=EINVAL;return -1; }
        owned=fcntl(directory_fd,F_DUPFD_CLOEXEC,0);if (owned<0) return -1;
    }
    memset(worker,0,sizeof *worker);worker->directory_fd=owned;
    if (directory) strcpy(worker->directory,directory);
    worker->generation=generation; memcpy(worker->nonce,nonce,16);
    worker->player=player; worker->listener=listener; worker->udp=udp; worker->port=port;
    worker->query_listener=query_listener; worker->metadata_port=metadata_port;
    worker->allowed=allowed; worker->policy=policy;
    if (provider) { worker->provider=*provider; worker->advertise=1; }
    __atomic_store_n(&worker->state,HUB_PUBLIC_WORKER_BUILDING,__ATOMIC_RELEASE);
    int result=pthread_create(&worker->thread,NULL,hub_public_worker_run,worker);
    if (result) {
        if (owned>=0) close(owned);
        worker->directory_fd=-1;
        worker->error=result; worker->state=HUB_PUBLIC_WORKER_FAILED; errno=result; return -1;
    }
    worker->started=1; return 0;
}
/* A verified descriptor is duplicated before the thread starts; caller retains
 * ownership. NULL pathname means staging can never reopen a replacement path. */
static int hub_public_source_worker_start_configured_fd(struct hub_public_source_worker *worker,
        int directory_fd,uint64_t generation,const unsigned char nonce[16],
        unsigned player,int listener,int udp,uint16_t port,
        hub_public_rpc_peer_policy allowed,void *policy,int query_listener,uint16_t metadata_port,
        const struct hub_public_provider_endpoint *provider) {
    return hub_public_source_worker_start_root(worker,NULL,directory_fd,generation,nonce,player,
        listener,udp,port,allowed,policy,query_listener,metadata_port,provider);
}
static int hub_public_source_worker_start_configured(struct hub_public_source_worker *worker,
        const char *directory,uint64_t generation,const unsigned char nonce[16],
        unsigned player,int listener,int udp,uint16_t port,
        hub_public_rpc_peer_policy allowed,void *policy,int query_listener,uint16_t metadata_port,
        const struct hub_public_provider_endpoint *provider) {
    return hub_public_source_worker_start_root(worker,directory,-1,generation,nonce,player,
        listener,udp,port,allowed,policy,query_listener,metadata_port,provider);
}
static int hub_public_source_worker_start_endpoints(struct hub_public_source_worker *worker,
        const char *directory,uint64_t generation,const unsigned char nonce[16],
        unsigned player,int listener,int udp,uint16_t port,
        hub_public_rpc_peer_policy allowed,void *policy,int query_listener,uint16_t metadata_port) {
    return hub_public_source_worker_start_configured(worker,directory,generation,nonce,player,
        listener,udp,port,allowed,policy,query_listener,metadata_port,NULL);
}
static int hub_public_source_worker_start(struct hub_public_source_worker *worker,
        const char *directory,uint64_t generation,const unsigned char nonce[16],
        unsigned player,int listener,int udp,uint16_t port,
        hub_public_rpc_peer_policy allowed,void *policy) {
    return hub_public_source_worker_start_endpoints(worker,directory,generation,nonce,
        player,listener,udp,port,allowed,policy,-1,0);
}
static int hub_public_source_worker_state(const struct hub_public_source_worker *worker) {
    return __atomic_load_n(&worker->state,__ATOMIC_ACQUIRE);
}
static void hub_public_source_worker_withdraw(struct hub_public_source_worker *worker) {
    __atomic_store_n(&worker->withdraw,1,__ATOMIC_RELEASE);
}
static void hub_public_source_worker_stop(struct hub_public_source_worker *worker) {
    __atomic_store_n(&worker->stop,1,__ATOMIC_RELEASE);
}
/* 0 still working; 1 joined/already stopped; -1 join failure. Successful join
 * is the gate for releasing endpoint sockets, policy context and source mount. */
static int hub_public_source_worker_join_ready(struct hub_public_source_worker *worker) {
    if (!worker->started) return 1;
    int state=hub_public_source_worker_state(worker);
    if (state!=HUB_PUBLIC_WORKER_STOPPED && state!=HUB_PUBLIC_WORKER_FAILED) return 0;
    int result=pthread_join(worker->thread,NULL);
    if (result) { errno=result; return -1; }
    worker->started=0; return 1;
}
#endif

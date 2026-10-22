/* SPDX-License-Identifier: GPL-3.0-only */
/* One source-owned worker for metadata and file transport. The caller stages
 * the library and binds nonblocking sockets on its chosen isolated endpoint.
 * All functions run on that worker. Stop/join it before destroying the library.
 * Listener/datagram sockets are borrowed; accepted connections are owned here. */
#ifndef HUB_PUBLIC_SOURCE_SERVICE_H
#define HUB_PUBLIC_SOURCE_SERVICE_H
#include <netinet/tcp.h>
#include "public-db-menus.h"
#include "public-library-export.h"
#include "public-rpc-udp.h"
#define HUB_PUBLIC_SOURCE_CLIENTS 8
struct hub_public_source_service {
    struct hub_db_library_view view;
    struct hub_public_library_export files;
    struct hub_db_session sessions[HUB_PUBLIC_SOURCE_CLIENTS];
    struct hub_db_backend backends[HUB_PUBLIC_SOURCE_CLIENTS];
    unsigned char used[HUB_PUBLIC_SOURCE_CLIENTS];
    int listener,udp,rpc_alias,initialized;
    unsigned player;
    hub_public_rpc_peer_policy allowed;
    void *policy;
};
static int hub_public_source_service_init(struct hub_public_source_service *service,
        struct hub_public_library_files *library,unsigned player,int listener,int udp,
        uint16_t file_port,hub_public_rpc_peer_policy allowed,void *policy) {
    if (!service || !library || !allowed || !player || player>255 || listener<0 ||
        (udp>=0 && !file_port)) { errno=EINVAL; return -1; }
    memset(service,0,sizeof *service); service->rpc_alias=-1;
    if (!hub_public_library_ready_catalog(library)) { errno=EAGAIN; return -1; }
    if (hub_db_library_view_init(&service->view,library)<0) return -1;
    service->files.library=library; service->files.rpc.catalog=&library->catalog;
    service->files.rpc.mount_port=service->files.rpc.nfs_port=file_port;
    service->player=player; service->listener=listener; service->udp=udp;
    service->allowed=allowed; service->policy=policy; service->initialized=1;
    return 0;
}
static void hub_public_source_service_close_client(struct hub_public_source_service *service,unsigned slot) {
    if (!service->used[slot]) return;
    hub_db_session_destroy(service->sessions+slot);
    hub_db_backend_destroy(service->backends+slot); service->used[slot]=0;
}
/* Withdrawal prevents fresh work immediately and drops buffered TCP replies.
 * Keep UDP/catalog alive until endpoint shutdown so old handles return STALE.
 * Network advertisement withdrawal and actual unmount remain caller duties. */
static void hub_public_source_service_withdraw(struct hub_public_source_service *service) {
    if (!service || !service->initialized) return;
    hub_public_library_files_cancel(service->files.library);
    for (unsigned i=0;i<HUB_PUBLIC_SOURCE_CLIENTS;++i)
        hub_public_source_service_close_client(service,i);
}
static int hub_public_source_service_step(struct hub_public_source_service *service,uint64_t now) {
    if (!service || !service->initialized) { errno=EINVAL; return -1; }
    const int rpc_sockets[]={service->udp,service->rpc_alias};
    for (unsigned i=0;i<2;++i)
        if (rpc_sockets[i]>=0 && hub_public_rpc_udp_dispatch_once(rpc_sockets[i],
                hub_public_library_export_dispatch,&service->files,service->allowed,service->policy)==HUB_UDP_ERROR)
            return -1;
    struct sockaddr_in peer={0}; socklen_t length=sizeof peer;
    int fd=accept4(service->listener,(struct sockaddr *)&peer,&length,SOCK_NONBLOCK|SOCK_CLOEXEC);
    if (fd>=0) {
        unsigned slot=0; while (slot<HUB_PUBLIC_SOURCE_CLIENTS && service->used[slot]) ++slot;
        if (length!=sizeof peer || peer.sin_family!=AF_INET ||
            !service->allowed(service->policy,&peer) || slot==HUB_PUBLIC_SOURCE_CLIENTS ||
            !hub_public_library_ready_catalog(service->files.library)) close(fd);
        else {
            int enabled=1;
            if (setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,&enabled,sizeof enabled)<0) close(fd);
            else {
                struct hub_db_backend *backend=service->backends+slot;
                memset(backend,0,sizeof *backend); backend->view=&service->view;
                backend->connection=service->sessions+slot;
                struct hub_db_callbacks callbacks={hub_db_backend_begin,hub_db_backend_next,hub_db_backend_alive};
                if (hub_db_session_init(service->sessions+slot,fd,service->player,now,callbacks,backend)<0)
                    hub_db_session_destroy(service->sessions+slot);
                else service->used[slot]=1;
            }
        }
    } else if (errno!=EAGAIN && errno!=EWOULDBLOCK && errno!=EINTR) return -1;
    for (unsigned i=0;i<HUB_PUBLIC_SOURCE_CLIENTS;++i)
        if (service->used[i] && hub_db_session_step(service->sessions+i,now)<0)
            hub_public_source_service_close_client(service,i);
    return 0;
}
static void hub_public_source_service_destroy(struct hub_public_source_service *service) {
    if (!service || !service->initialized) return;
    for (unsigned i=0;i<HUB_PUBLIC_SOURCE_CLIENTS;++i)
        hub_public_source_service_close_client(service,i);
    hub_db_library_view_destroy(&service->view); service->initialized=0;
}
#endif

/* SPDX-License-Identifier: GPL-3.0-only */
/* Bounded nonblocking RemoteDBServer port lookup, owned by the source worker.
 * The bound listener is borrowed. Only admitted peers can obtain a port. */
#ifndef HUB_PUBLIC_DB_PORT_H
#define HUB_PUBLIC_DB_PORT_H
#include "public-rpc-udp.h"
#define HUB_DB_PORT_CLIENTS 8
struct hub_db_port_client {
    int fd;
    unsigned received,sent;
    uint64_t started;
    unsigned char query[19];
};
struct hub_db_port_service {
    int listener;
    uint16_t port;
    hub_public_rpc_peer_policy allowed;
    void *policy;
    struct hub_db_port_client clients[HUB_DB_PORT_CLIENTS];
};
static void hub_db_port_init(struct hub_db_port_service *service,int listener,uint16_t port,
        hub_public_rpc_peer_policy allowed,void *policy) {
    memset(service,0,sizeof *service); service->listener=listener; service->port=port;
    service->allowed=allowed; service->policy=policy;
    for (unsigned i=0;i<HUB_DB_PORT_CLIENTS;++i) service->clients[i].fd=-1;
}
static void hub_db_port_close(struct hub_db_port_client *client) {
    if (client->fd>=0) { shutdown(client->fd,SHUT_RDWR); close(client->fd); client->fd=-1; }
}
static void hub_db_port_destroy(struct hub_db_port_service *service) {
    for (unsigned i=0;i<HUB_DB_PORT_CLIENTS;++i) hub_db_port_close(service->clients+i);
}
static int hub_db_port_step(struct hub_db_port_service *service,uint64_t now,int ready) {
    static const unsigned char query[19]={0,0,0,15,'R','e','m','o','t','e','D','B','S','e','r','v','e','r',0};
    struct sockaddr_in peer={0}; socklen_t size=sizeof peer;
    int fd=accept4(service->listener,(struct sockaddr *)&peer,&size,SOCK_NONBLOCK|SOCK_CLOEXEC);
    if (fd>=0) {
        unsigned slot=0; while (slot<HUB_DB_PORT_CLIENTS && service->clients[slot].fd>=0) ++slot;
        if (!ready || !service->port || !service->allowed || size!=sizeof peer || peer.sin_family!=AF_INET ||
            !service->allowed(service->policy,&peer) || slot==HUB_DB_PORT_CLIENTS) close(fd);
        else service->clients[slot]=(struct hub_db_port_client){.fd=fd,.started=now};
    } else if (errno!=EAGAIN && errno!=EWOULDBLOCK && errno!=EINTR) return -1;
    for (unsigned i=0;i<HUB_DB_PORT_CLIENTS;++i) {
        struct hub_db_port_client *client=service->clients+i;
        if (client->fd<0) continue;
        if (!ready || now-client->started>=5000) { hub_db_port_close(client); continue; }
        if (client->received<sizeof query) {
            ssize_t got=recv(client->fd,client->query+client->received,sizeof query-client->received,MSG_DONTWAIT);
            if (!got) { hub_db_port_close(client); continue; }
            if (got<0) {
                if (errno!=EAGAIN && errno!=EWOULDBLOCK && errno!=EINTR) hub_db_port_close(client);
                continue;
            }
            client->received+=(unsigned)got;
            if (memcmp(client->query,query,client->received)) { hub_db_port_close(client); continue; }
        }
        if (client->received==sizeof query) {
            unsigned char reply[2]={(unsigned char)(service->port>>8),(unsigned char)service->port};
            ssize_t sent=send(client->fd,reply+client->sent,sizeof reply-client->sent,MSG_DONTWAIT|MSG_NOSIGNAL);
            if (sent<0) {
                if (errno!=EAGAIN && errno!=EWOULDBLOCK && errno!=EINTR) hub_db_port_close(client);
            } else {
                client->sent+=(unsigned)sent;
                if (client->sent==sizeof reply) hub_db_port_close(client);
            }
        }
    }
    return 0;
}
#endif

/* SPDX-License-Identifier: GPL-3.0-only */
/* Nonblocking transport on caller-bound per-source UDP50000/50002 sockets.
 * Neither this adapter nor its worker creates interfaces or chooses IP/MACs. */
#ifndef HUB_PUBLIC_PROVIDER_UDP_H
#define HUB_PUBLIC_PROVIDER_UDP_H
#include "public-provider-state.h"
#include "public-rpc-udp.h"
#include "public-player-discovery.h"
struct hub_public_provider_endpoint {
    int discovery_fd,status_fd;
    uint32_t broadcast; /* Network byte order; explicitly configured. */
    struct hub_public_provider_identity identity;
    char export_group[64];
    int rpc_alias_enabled,rpc_alias_fd;
};
struct hub_public_provider_udp {
    struct hub_public_provider_endpoint endpoint;
    struct hub_public_provider_state state;
    hub_public_rpc_peer_policy allowed;
    void *policy;
    int conflict;
};
static int hub_public_provider_udp_send(void *opaque,uint32_t peer,unsigned port,const void *data,size_t length) {
    struct hub_public_provider_udp *transport=opaque;
    struct sockaddr_in destination={.sin_family=AF_INET,.sin_port=htons((uint16_t)port),
        .sin_addr={.s_addr=peer ? peer : transport->endpoint.broadcast}};
    int fd=port==50000 ? transport->endpoint.discovery_fd : transport->endpoint.status_fd;
    ssize_t sent=sendto(fd,data,length,MSG_DONTWAIT|MSG_NOSIGNAL,(struct sockaddr *)&destination,sizeof destination);
    if (sent<0 && (errno==EAGAIN || errno==EWOULDBLOCK || errno==EINTR || errno==ENOBUFS)) return 0;
    return sent==(ssize_t)length ? 1 : -1;
}
static void hub_public_provider_udp_init(struct hub_public_provider_udp *transport,
        const struct hub_public_provider_endpoint *endpoint,uint64_t now,
        hub_public_rpc_peer_policy allowed,void *policy) {
    memset(transport,0,sizeof *transport); transport->endpoint=*endpoint;
    transport->allowed=allowed; transport->policy=policy;
    hub_public_provider_start(&transport->state,&endpoint->identity,now,hub_public_provider_udp_send,transport);
}
static int hub_public_provider_udp_receive(struct hub_public_provider_udp *transport,int discovery,
        uint32_t tracks,uint64_t now) {
    unsigned char bytes[2048]; struct sockaddr_in peer={0};
    struct iovec buffer={bytes,sizeof bytes}; struct msghdr message={0};
    message.msg_name=&peer; message.msg_namelen=sizeof peer; message.msg_iov=&buffer; message.msg_iovlen=1;
    ssize_t received=recvmsg(discovery ? transport->endpoint.discovery_fd : transport->endpoint.status_fd,&message,MSG_DONTWAIT);
    if (received<0) return errno==EAGAIN || errno==EWOULDBLOCK || errno==EINTR ? 0 : -1;
    if ((message.msg_flags&MSG_TRUNC) || message.msg_namelen!=sizeof peer || peer.sin_family!=AF_INET ||
        (discovery && ntohs(peer.sin_port)!=50000) || !transport->allowed ||
        !transport->allowed(transport->policy,&peer) || received<36 || memcmp(bytes,"Qspt1WmJOL",10)) return 0;
    if (discovery) {
        unsigned number=0; const unsigned char *mac=NULL;
        hub_public_player_claim(bytes,(size_t)received,&number,&mac);
        if (mac && number==transport->endpoint.identity.number && memcmp(mac,transport->endpoint.identity.mac,6)) {
            transport->conflict=1; hub_public_provider_withdraw(&transport->state);
        }
        return 0;
    }
    /* Native AZ status requests use ephemeral source ports. Replies go to its
     * well-known50002 receiver; peer admission is based on address/interface. */
    int result=hub_public_provider_receive(&transport->state,peer.sin_addr.s_addr,bytes,(size_t)received,tracks,now);
    return result<0 ? -1 : 0;
}
static int hub_public_provider_udp_step(struct hub_public_provider_udp *transport,uint32_t tracks,uint64_t now,int ready) {
    if (!ready) hub_public_provider_withdraw(&transport->state);
    if (hub_public_provider_udp_receive(transport,1,tracks,now)<0 ||
        hub_public_provider_udp_receive(transport,0,tracks,now)<0) return -1;
    return hub_public_provider_tick(&transport->state,now);
}
#endif

/* SPDX-License-Identifier: GPL-3.0-only */
/* One nonblocking UDP operation on an already owned/bound socket. Caller owns
 * interface/address selection, peer policy, scheduling and socket lifetime. */
#ifndef HUB_PUBLIC_RPC_UDP_H
#define HUB_PUBLIC_RPC_UDP_H
#include <sys/socket.h>
#include <netinet/in.h>
#include "public-rpc-read.h"
#define HUB_PUBLIC_RPC_UDP_REQUEST_MAX 4096
typedef int (*hub_public_rpc_peer_policy)(void *,const struct sockaddr_in *);
typedef void (*hub_public_rpc_datagram_dispatch)(void *,const void *,size_t,struct hub_public_rpc_reply *);
enum hub_public_udp_result { HUB_UDP_ERROR=-1,HUB_UDP_IDLE=0,HUB_UDP_DROPPED=1,HUB_UDP_REPLIED=2 };
static enum hub_public_udp_result hub_public_rpc_udp_dispatch_once(int fd,
        hub_public_rpc_datagram_dispatch dispatch,void *context,
        hub_public_rpc_peer_policy allowed,void *policy) {
    unsigned char request[HUB_PUBLIC_RPC_UDP_REQUEST_MAX];
    struct sockaddr_in peer={0};
    struct iovec buffer={request,sizeof request};
    struct msghdr message={0};
    message.msg_name=&peer; message.msg_namelen=sizeof peer;
    message.msg_iov=&buffer; message.msg_iovlen=1;
    ssize_t bytes=recvmsg(fd,&message,MSG_DONTWAIT);
    if (bytes<0) return errno==EAGAIN || errno==EWOULDBLOCK || errno==EINTR ? HUB_UDP_IDLE : HUB_UDP_ERROR;
    if ((message.msg_flags&MSG_TRUNC) || message.msg_namelen!=sizeof peer ||
        peer.sin_family!=AF_INET || !allowed || !allowed(policy,&peer) || !dispatch) return HUB_UDP_DROPPED;
    struct hub_public_rpc_reply reply;
    dispatch(context,request,(size_t)bytes,&reply);
    if (!reply.length) return HUB_UDP_DROPPED;
    /* Dispatch has released every file lease. A full send queue cannot hold a
     * mount or block shutdown; clients may retransmit an unanswered UDP call. */
    ssize_t sent=sendto(fd,reply.bytes,reply.length,MSG_DONTWAIT|MSG_NOSIGNAL,
                        (const struct sockaddr *)&peer,sizeof peer);
    if (sent<0 && (errno==EAGAIN || errno==EWOULDBLOCK || errno==EINTR || errno==ENOBUFS)) return HUB_UDP_DROPPED;
    return sent==(ssize_t)reply.length ? HUB_UDP_REPLIED : HUB_UDP_ERROR;
}
struct hub_public_rpc_read_context { struct hub_public_root *root; hub_public_file_resolver resolve; void *catalog; };
static void hub_public_rpc_read_datagram(void *opaque,const void *data,size_t length,struct hub_public_rpc_reply *reply) {
    struct hub_public_rpc_read_context *context=opaque;
    hub_public_rpc_read_dispatch(context->root,data,length,context->resolve,context->catalog,reply);
}
static enum hub_public_udp_result hub_public_rpc_udp_once(int fd,
        struct hub_public_root *root,hub_public_file_resolver resolve,void *catalog,
        hub_public_rpc_peer_policy allowed,void *policy) {
    struct hub_public_rpc_read_context context={root,resolve,catalog};
    return hub_public_rpc_udp_dispatch_once(fd,hub_public_rpc_read_datagram,&context,allowed,policy);
}
#endif

/* SPDX-License-Identifier: GPL-3.0-only */
/* Create an UP MACVLAN directly in a caller-owned empty namespace. No address
 * is selected/applied. Caller retains namespace ownership through endpoint
 * drain. Explicit deletion ACK supports immediate identity reuse without
 * depending on deferred namespace destruction. */
#ifndef HUB_PUBLIC_NETWORK_LINK_H
#define HUB_PUBLIC_NETWORK_LINK_H
#include <linux/if_link.h>
#include <net/if_arp.h>
#include "public-address-remove.h"
struct hub_public_network_link_spec { int namespace_fd;char parent[IFNAMSIZ],name[IFNAMSIZ];unsigned char parent_mac[6],mac[6]; };
struct hub_public_network_link { pthread_t thread;struct hub_public_network_link_spec spec;int started,done,error,removing; };
struct hub_public_network_link_packet { struct nlmsghdr header;struct ifinfomsg link;unsigned char attributes[256]; };
static int hub_public_network_link_attribute(struct hub_public_network_link_packet *p,unsigned type,const void *data,size_t size) {
    size_t offset=NLMSG_ALIGN(p->header.nlmsg_len),length=RTA_LENGTH(size);
    if (offset+RTA_ALIGN(length)>sizeof *p) { errno=EMSGSIZE;return -1; }
    struct rtattr *a=(void *)((unsigned char *)p+offset);a->rta_type=type;a->rta_len=length;
    if (size) memcpy(RTA_DATA(a),data,size);
    p->header.nlmsg_len=offset+RTA_ALIGN(length);return 0;
}
static int hub_public_network_link_packet(const struct hub_public_network_link_spec *s,unsigned parent,
        struct hub_public_network_link_packet *p) {
    if (!s || !p || !parent || s->namespace_fd<0 || strncmp(s->name,"azpub",5) ||
            strnlen(s->name,IFNAMSIZ)==IFNAMSIZ || (s->mac[0]&3)!=2) { errno=EINVAL;return -1; }
    memset(p,0,sizeof *p);p->header.nlmsg_type=RTM_NEWLINK;p->header.nlmsg_seq=1;
    p->header.nlmsg_flags=NLM_F_REQUEST|NLM_F_ACK|NLM_F_CREATE|NLM_F_EXCL;
    p->header.nlmsg_len=NLMSG_LENGTH(sizeof p->link);p->link.ifi_family=AF_UNSPEC;
    p->link.ifi_flags=IFF_UP;p->link.ifi_change=IFF_UP;
    if (hub_public_network_link_attribute(p,IFLA_LINK,&parent,4)<0 ||
            hub_public_network_link_attribute(p,IFLA_NET_NS_FD,&s->namespace_fd,4)<0 ||
            hub_public_network_link_attribute(p,IFLA_IFNAME,s->name,strlen(s->name)+1)<0 ||
            hub_public_network_link_attribute(p,IFLA_ADDRESS,s->mac,6)<0) return -1;
    size_t info=NLMSG_ALIGN(p->header.nlmsg_len);
    if (hub_public_network_link_attribute(p,IFLA_LINKINFO|NLA_F_NESTED,NULL,0)<0 ||
            hub_public_network_link_attribute(p,IFLA_INFO_KIND,"macvlan",8)<0) return -1;
    size_t data=NLMSG_ALIGN(p->header.nlmsg_len);
    if (hub_public_network_link_attribute(p,IFLA_INFO_DATA|NLA_F_NESTED,NULL,0)<0) return -1;
    uint32_t mode=MACVLAN_MODE_BRIDGE;
    if (hub_public_network_link_attribute(p,IFLA_MACVLAN_MODE,&mode,4)<0) return -1;
    ((struct rtattr *)((unsigned char *)p+data))->rta_len=p->header.nlmsg_len-data;
    ((struct rtattr *)((unsigned char *)p+info))->rta_len=p->header.nlmsg_len-info;return 0;
}
static int hub_public_network_link_remove_packet(unsigned index,struct hub_public_network_link_packet *p) {
    if (!index || !p) { errno=EINVAL;return -1; }
    memset(p,0,sizeof *p);p->header.nlmsg_type=RTM_DELLINK;p->header.nlmsg_seq=1;
    p->header.nlmsg_flags=NLM_F_REQUEST|NLM_F_ACK;
    p->header.nlmsg_len=NLMSG_LENGTH(sizeof p->link);p->link.ifi_family=AF_UNSPEC;p->link.ifi_index=index;
    return 0;
}
static void *hub_public_network_link_thread(void *opaque) {
    struct hub_public_network_link *op=opaque;int fd=-1,error=0;
    unsigned parent=if_nametoindex(op->spec.parent);if (!parent) goto failed;
    fd=socket(AF_INET,SOCK_DGRAM|SOCK_CLOEXEC,0);if (fd<0) goto failed;
    struct ifreq req={0};strcpy(req.ifr_name,op->spec.parent);
    if (ioctl(fd,SIOCGIFFLAGS,&req)<0 || !(req.ifr_flags&IFF_UP) || (req.ifr_flags&IFF_LOOPBACK)) { errno=ENODEV;goto failed; }
    if (ioctl(fd,SIOCGIFHWADDR,&req)<0 || req.ifr_hwaddr.sa_family!=ARPHRD_ETHER ||
            memcmp(req.ifr_hwaddr.sa_data,op->spec.parent_mac,6)) { errno=ESTALE;goto failed; }
    close(fd);fd=-1;
    struct hub_public_network_link_packet packet;
    if (op->removing) {
        if (setns(op->spec.namespace_fd,CLONE_NEWNET)<0) goto failed;
        unsigned index=if_nametoindex(op->spec.name);if (!index) { errno=ENODEV;goto failed; }
        fd=socket(AF_INET,SOCK_DGRAM|SOCK_CLOEXEC,0);if (fd<0) goto failed;
        memset(&req,0,sizeof req);strcpy(req.ifr_name,op->spec.name);
        if (ioctl(fd,SIOCGIFHWADDR,&req)<0 || req.ifr_hwaddr.sa_family!=ARPHRD_ETHER ||
                memcmp(req.ifr_hwaddr.sa_data,op->spec.mac,6)) { errno=ESTALE;goto failed; }
        close(fd);fd=-1;
        if (hub_public_network_link_remove_packet(index,&packet)<0) goto failed;
    } else if (hub_public_network_link_packet(&op->spec,parent,&packet)<0) goto failed;
    fd=socket(AF_NETLINK,SOCK_RAW|SOCK_CLOEXEC|SOCK_NONBLOCK,NETLINK_ROUTE);if (fd<0) goto failed;
    struct sockaddr_nl local={.nl_family=AF_NETLINK},kernel={.nl_family=AF_NETLINK};
    if (bind(fd,(void *)&local,sizeof local)<0 ||
            sendto(fd,&packet,packet.header.nlmsg_len,0,(void *)&kernel,sizeof kernel)!=(ssize_t)packet.header.nlmsg_len) goto failed;
    uint64_t deadline=hub_public_address_remove_ms()+2000;
    for (;;) {
        unsigned char bytes[4096];struct sockaddr_nl from={0};struct iovec iov={bytes,sizeof bytes};
        struct msghdr message={.msg_name=&from,.msg_namelen=sizeof from,.msg_iov=&iov,.msg_iovlen=1};
        ssize_t count=recvmsg(fd,&message,0);
        if (count>=0) {
            if (message.msg_flags&(MSG_TRUNC|MSG_CTRUNC) || message.msg_namelen!=sizeof from ||
                    from.nl_family!=AF_NETLINK || from.nl_pid || from.nl_groups) { errno=EPROTO;goto failed; }
            if (hub_public_address_remove_ack(bytes,count)<0) goto failed;
            break;
        }
        if (errno!=EAGAIN && errno!=EWOULDBLOCK && errno!=EINTR) goto failed;
        if (hub_public_address_remove_ms()>=deadline) { errno=ETIMEDOUT;goto failed; }
        struct pollfd wait={.fd=fd,.events=POLLIN};if (poll(&wait,1,20)<0 && errno!=EINTR) goto failed;
    }
    goto done;
failed: error=errno ? errno:EIO;
done:
    if (fd>=0) close(fd);
    close(op->spec.namespace_fd);op->spec.namespace_fd=-1;op->error=error;
    __atomic_store_n(&op->done,1,__ATOMIC_RELEASE);return NULL;
}
static int hub_public_network_link_operation_start(struct hub_public_network_link *op,const struct hub_public_network_link_spec *spec,int removing) {
    if (!op || !spec || op->started || !spec->parent[0] || strnlen(spec->parent,IFNAMSIZ)==IFNAMSIZ) { errno=EINVAL;return -1; }
    struct stat current,target;if (stat("/proc/self/ns/net",&current)<0 || fstat(spec->namespace_fd,&target)<0) return -1;
    if (current.st_dev==target.st_dev && current.st_ino==target.st_ino) { errno=EPERM;return -1; }
    struct hub_public_network_link_packet packet;if (hub_public_network_link_packet(spec,1,&packet)<0) return -1;
    int fd=fcntl(spec->namespace_fd,F_DUPFD_CLOEXEC,0);if (fd<0) return -1;
    memset(op,0,sizeof *op);op->spec=*spec;op->spec.namespace_fd=fd;op->removing=!!removing;
    int error=pthread_create(&op->thread,NULL,hub_public_network_link_thread,op);
    if (error) { close(fd);op->spec.namespace_fd=-1;errno=error;return -1; }op->started=1;return 0;
}
static int hub_public_network_link_start(struct hub_public_network_link *op,const struct hub_public_network_link_spec *spec) {
    return hub_public_network_link_operation_start(op,spec,0);
}
static int hub_public_network_link_remove_start(struct hub_public_network_link *op,const struct hub_public_network_link_spec *spec) {
    return hub_public_network_link_operation_start(op,spec,1);
}
static int hub_public_network_link_poll(struct hub_public_network_link *op) {
    if (!op || !op->started) { errno=EINVAL;return -1; }
    if (!__atomic_load_n(&op->done,__ATOMIC_ACQUIRE)) return 1;
    int error=pthread_join(op->thread,NULL);if (error) { errno=error;return -1; }op->started=0;
    if (op->error) { errno=op->error;return -1; }return 0;
}
#endif

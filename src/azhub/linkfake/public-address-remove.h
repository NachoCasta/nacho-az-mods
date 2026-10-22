/* SPDX-License-Identifier: GPL-3.0-only */
/* Exact IPv4 operations in an owned namespace, off the controller/UI thread.
 * Removal starts only after source workers/endpoints drain. Exclusive configure
 * starts only after caller-proven ARP claim and refuses existing interface IPv4.
 * Poll to terminal before releasing the operation or its containing memory. */
#ifndef HUB_PUBLIC_ADDRESS_REMOVE_H
#define HUB_PUBLIC_ADDRESS_REMOVE_H
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if_arp.h>
#include <poll.h>
#include <time.h>
#include "public-network-endpoint.h"
struct hub_public_address_remove {
    pthread_t thread;
    struct hub_public_network_spec spec;
    int started,done,error,configure;
};
struct hub_public_address_remove_packet {
    struct nlmsghdr header;
    struct ifaddrmsg address;
    unsigned char attributes[3*RTA_SPACE(4)];
};
static int hub_public_address_remove_packet(const struct hub_public_network_spec *spec,unsigned index,
        struct hub_public_address_remove_packet *packet) {
    uint32_t mask=ntohl(spec->mask.s_addr),inverse=~mask;
    if (!index || !mask || (inverse&(inverse+1)) || !spec->address.s_addr) { errno=EINVAL;return -1; }
    memset(packet,0,sizeof *packet);
    packet->header.nlmsg_len=NLMSG_LENGTH(sizeof packet->address);
    packet->header.nlmsg_type=RTM_DELADDR;packet->header.nlmsg_flags=NLM_F_REQUEST|NLM_F_ACK;
    packet->header.nlmsg_seq=1;packet->address.ifa_family=AF_INET;packet->address.ifa_index=index;
    while (mask) { ++packet->address.ifa_prefixlen;mask<<=1; }
    const unsigned types[]={IFA_LOCAL,IFA_ADDRESS};
    for (unsigned i=0;i<2;++i) {
        struct rtattr *attribute=(void *)((unsigned char *)packet+NLMSG_ALIGN(packet->header.nlmsg_len));
        attribute->rta_type=types[i];attribute->rta_len=RTA_LENGTH(4);
        memcpy(RTA_DATA(attribute),&spec->address.s_addr,4);
        packet->header.nlmsg_len=NLMSG_ALIGN(packet->header.nlmsg_len)+RTA_SPACE(4);
    }
    return 0;
}
static int hub_public_address_configure_packet(const struct hub_public_network_spec *spec,unsigned index,
        struct hub_public_address_remove_packet *packet) {
    if (hub_public_address_remove_packet(spec,index,packet)<0) return -1;
    uint32_t address=ntohl(spec->address.s_addr),mask=ntohl(spec->mask.s_addr);
    if ((spec->address.s_addr|~spec->mask.s_addr)!=spec->broadcast.s_addr ||
            (address&~mask)==0 || (address&~mask)==~mask || address>=0xe0000000u || address<0x01000000u) { errno=EINVAL;return -1; }
    packet->header.nlmsg_type=RTM_NEWADDR;packet->header.nlmsg_flags|=NLM_F_CREATE|NLM_F_EXCL;
    struct rtattr *attribute=(void *)((unsigned char *)packet+NLMSG_ALIGN(packet->header.nlmsg_len));
    attribute->rta_type=IFA_BROADCAST;attribute->rta_len=RTA_LENGTH(4);
    memcpy(RTA_DATA(attribute),&spec->broadcast.s_addr,4);
    packet->header.nlmsg_len=NLMSG_ALIGN(packet->header.nlmsg_len)+RTA_SPACE(4);return 0;
}
static int hub_public_address_configure_verify(const struct hub_public_network_spec *spec) {
    int fd=socket(AF_INET,SOCK_DGRAM|SOCK_CLOEXEC,0);if (fd<0) return -1;
    struct ifreq request={0};strcpy(request.ifr_name,spec->interface);int error=0;
    if (ioctl(fd,SIOCGIFFLAGS,&request)<0) error=errno;
    else if (!(request.ifr_flags&IFF_UP) || (request.ifr_flags&IFF_LOOPBACK)) error=EINVAL;
    if (!error && (ioctl(fd,SIOCGIFHWADDR,&request)<0 || request.ifr_hwaddr.sa_family!=ARPHRD_ETHER ||
            memcmp(request.ifr_hwaddr.sa_data,spec->mac,6))) error=ESTALE;
    if (!error) {
        if (!ioctl(fd,SIOCGIFADDR,&request)) error=EADDRINUSE;
        else if (errno!=EADDRNOTAVAIL) error=errno;
    }
    close(fd);if (error) { errno=error;return -1; }return 0;
}
static int hub_public_address_remove_ack(const void *bytes,size_t length) {
    if (length<NLMSG_LENGTH(sizeof(struct nlmsgerr))) { errno=EPROTO;return -1; }
    struct nlmsghdr header;memcpy(&header,bytes,sizeof header);
    if (header.nlmsg_len!=length || header.nlmsg_seq!=1 || header.nlmsg_type!=NLMSG_ERROR) { errno=EPROTO;return -1; }
    struct nlmsgerr ack;memcpy(&ack,(const unsigned char *)bytes+NLMSG_HDRLEN,sizeof ack);
    if (ack.error>0 || ack.error<-4095) { errno=EPROTO;return -1; }
    if (ack.error) { errno=-ack.error;return -1; }return 0;
}
static uint64_t hub_public_address_remove_ms(void) {
    struct timespec now;if (clock_gettime(CLOCK_MONOTONIC,&now)<0) return 0;
    return (uint64_t)now.tv_sec*1000+now.tv_nsec/1000000;
}
static void *hub_public_address_remove_thread(void *opaque) {
    struct hub_public_address_remove *op=opaque;int fd=-1,error=0;
    if (setns(op->spec.namespace_fd,CLONE_NEWNET)<0 ||
            (op->configure ? hub_public_address_configure_verify(&op->spec):hub_public_network_verify(&op->spec))<0) goto failed;
    unsigned index=if_nametoindex(op->spec.interface);if (!index) goto failed;
    struct hub_public_address_remove_packet packet;
    if ((op->configure ? hub_public_address_configure_packet(&op->spec,index,&packet):hub_public_address_remove_packet(&op->spec,index,&packet))<0) goto failed;
    fd=socket(AF_NETLINK,SOCK_RAW|SOCK_CLOEXEC|SOCK_NONBLOCK,NETLINK_ROUTE);if (fd<0) goto failed;
    struct sockaddr_nl local={.nl_family=AF_NETLINK},kernel={.nl_family=AF_NETLINK};
    if (bind(fd,(void *)&local,sizeof local)<0 ||
            sendto(fd,&packet,packet.header.nlmsg_len,0,(void *)&kernel,sizeof kernel)!=(ssize_t)packet.header.nlmsg_len) goto failed;
    uint64_t deadline=hub_public_address_remove_ms()+2000;
    for (;;) {
        unsigned char reply[4096];struct sockaddr_nl from={0};
        struct iovec iov={reply,sizeof reply};struct msghdr message={.msg_name=&from,.msg_namelen=sizeof from,.msg_iov=&iov,.msg_iovlen=1};
        ssize_t length=recvmsg(fd,&message,0);
        if (length>=0) {
            if (message.msg_flags&(MSG_TRUNC|MSG_CTRUNC) || message.msg_namelen!=sizeof from ||
                    from.nl_family!=AF_NETLINK || from.nl_pid || from.nl_groups) { errno=EPROTO;goto failed; }
            if (hub_public_address_remove_ack(reply,(size_t)length)<0) goto failed;
            break;
        }
        if (errno!=EAGAIN && errno!=EWOULDBLOCK && errno!=EINTR) goto failed;
        if (hub_public_address_remove_ms()>=deadline) { errno=ETIMEDOUT;goto failed; }
        struct pollfd wait={.fd=fd,.events=POLLIN};
        if (poll(&wait,1,20)<0 && errno!=EINTR) goto failed;
    }
    goto done;
failed:
    error=errno ? errno:EIO;
done:
    if (fd>=0) close(fd);
    close(op->spec.namespace_fd);op->spec.namespace_fd=-1;op->error=error;
    __atomic_store_n(&op->done,1,__ATOMIC_RELEASE);return NULL;
}
static int hub_public_address_operation_start(struct hub_public_address_remove *op,const struct hub_public_network_spec *spec,int configure) {
    if (!op || !spec || op->started || spec->namespace_fd<0 || !spec->interface[0] ||
            strnlen(spec->interface,IFNAMSIZ)==IFNAMSIZ || strncmp(spec->interface,"azpub",5) ||
            (spec->mac[0]&3)!=2) { errno=EINVAL;return -1; }
    struct stat current,target;
    if (stat("/proc/self/ns/net",&current)<0 || fstat(spec->namespace_fd,&target)<0) return -1;
    if (current.st_dev==target.st_dev && current.st_ino==target.st_ino) { errno=EPERM;return -1; }
    struct hub_public_address_remove_packet packet;
    if ((configure ? hub_public_address_configure_packet(spec,1,&packet):hub_public_address_remove_packet(spec,1,&packet))<0) return -1;
    int fd=fcntl(spec->namespace_fd,F_DUPFD_CLOEXEC,0);if (fd<0) return -1;
    memset(op,0,sizeof *op);op->spec=*spec;op->spec.namespace_fd=fd;op->configure=!!configure;
    int error=pthread_create(&op->thread,NULL,hub_public_address_remove_thread,op);
    if (error) { close(fd);op->spec.namespace_fd=-1;errno=error;return -1; }
    op->started=1;return 0;
}
static int hub_public_address_remove_start(struct hub_public_address_remove *op,const struct hub_public_network_spec *spec) {
    return hub_public_address_operation_start(op,spec,0);
}
static int hub_public_address_configure_start(struct hub_public_address_remove *op,const struct hub_public_network_spec *spec) {
    return hub_public_address_operation_start(op,spec,1);
}
/* 1 pending; 0 kernel-acknowledged removal and joined; -1 failed/unstarted.
 * A timeout is not removal proof; retain the enclosing network ownership. */
static int hub_public_address_remove_poll(struct hub_public_address_remove *op) {
    if (!op || !op->started) { errno=EINVAL;return -1; }
    if (!__atomic_load_n(&op->done,__ATOMIC_ACQUIRE)) return 1;
    int error=pthread_join(op->thread,NULL);if (error) { errno=error;return -1; }
    op->started=0;
    if (op->error) { errno=op->error;return -1; }return 0;
}
#endif

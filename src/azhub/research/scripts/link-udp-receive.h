/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef LINK_UDP_RECEIVE_H
#define LINK_UDP_RECEIVE_H
/* Research receive adapter. Caller must enable IP_PKTINFO before receiving and
 * remember whether the application requested it independently. No socket-option
 * mutation, address rewriting or admission decision is performed here.
 * Incoming metadata is collected in the same syscall as the payload, never by
 * peeking and then assuming the next datagram belongs to the same sender.
 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
struct probe_udp_metadata {
    struct sockaddr_storage source;
    socklen_t source_length;
    unsigned interface_index;
    struct in_addr destination;
    size_t copied_bytes;
    int control_incomplete;
};
typedef ssize_t (*probe_recvmsg_fn)(int,struct msghdr *,int);
/* Mirror Linux put_cmsg's bounded output convention, including partial data
 * after a complete cmsghdr and alignment padding counted in msg_controllen. */
static size_t probe_udp_control_copy(void *out,size_t capacity,const struct cmsghdr *header,
    const unsigned char *data,int *flags) {
    if (capacity<sizeof *header) {*flags|=MSG_CTRUNC;return 0;}
    size_t length=header->cmsg_len;
    if (length>capacity) {length=capacity;*flags|=MSG_CTRUNC;}
    struct cmsghdr copy=*header;copy.cmsg_len=length;
    memcpy(out,&copy,sizeof copy);
    if (length>sizeof copy) memcpy((unsigned char *)out+sizeof copy,data,length-sizeof copy);
    size_t used=CMSG_ALIGN(length);if (used>capacity) used=capacity;
    if (used>length) memset((unsigned char *)out+length,0,used-length);
    return used;
}
static ssize_t probe_udp_receive(probe_recvmsg_fn receive,int fd,struct msghdr *caller,
    int flags,int visible_pktinfo,struct probe_udp_metadata *metadata) {
    memset(metadata,0,sizeof *metadata);
    if (!caller) return receive(fd,caller,flags);
    size_t capacity=caller->msg_control ? caller->msg_controllen:0;
    /* Room for ordinary IPv4 ancillary records even when the caller supplied
     * no control buffer. Overflow remains detectable and cannot grant trust. */
    size_t extra=4096;
    if (capacity>SIZE_MAX-extra) {errno=ENOMEM;return -1;}
    unsigned char *control=malloc(capacity+extra);
    if (!control) {errno=ENOMEM;return -1;}
    struct msghdr incoming=*caller;
    incoming.msg_name=&metadata->source;incoming.msg_namelen=sizeof metadata->source;
    incoming.msg_control=control;incoming.msg_controllen=capacity+extra;
    ssize_t result=receive(fd,&incoming,flags);
    int saved_errno=errno;
    if (result<0) {free(control);errno=saved_errno;return result;}
    metadata->source_length=incoming.msg_namelen;
    metadata->control_incomplete=!!(incoming.msg_flags&MSG_CTRUNC);
    size_t remaining=(size_t)result;
    for (size_t i=0;i<caller->msg_iovlen && remaining;i++) {
        size_t part=caller->msg_iov[i].iov_len;
        if (part>remaining) part=remaining;
        metadata->copied_bytes+=part;remaining-=part;
    }
    if (caller->msg_name) {
        size_t take=caller->msg_namelen;
        if (take>incoming.msg_namelen) take=incoming.msg_namelen;
        if (take>sizeof metadata->source) take=sizeof metadata->source;
        memcpy(caller->msg_name,&metadata->source,take);
        caller->msg_namelen=incoming.msg_namelen;
    } else caller->msg_namelen=0;
    caller->msg_flags=incoming.msg_flags;
    size_t used=0;
    for (struct cmsghdr *c=CMSG_FIRSTHDR(&incoming);c;c=CMSG_NXTHDR(&incoming,c)) {
        size_t offset=(unsigned char *)c-control;
        struct cmsghdr header;memcpy(&header,c,sizeof header);
        if (header.cmsg_len<sizeof header || header.cmsg_len>incoming.msg_controllen-offset) {
            metadata->control_incomplete=1;caller->msg_flags|=MSG_CTRUNC;break;
        }
        int pktinfo=header.cmsg_level==IPPROTO_IP && header.cmsg_type==IP_PKTINFO;
        if (pktinfo && header.cmsg_len>=CMSG_LEN(sizeof(struct in_pktinfo))) {
            struct in_pktinfo info;memcpy(&info,CMSG_DATA(c),sizeof info);
            metadata->interface_index=info.ipi_ifindex;metadata->destination=info.ipi_addr;
        }
        if (pktinfo && !visible_pktinfo) continue;
        if (capacity-used<sizeof header) {caller->msg_flags|=MSG_CTRUNC;continue;}
        used+=probe_udp_control_copy((unsigned char *)caller->msg_control+used,capacity-used,
            &header,(unsigned char *)c+sizeof header,&caller->msg_flags);
    }
    caller->msg_controllen=used;
    free(control);errno=saved_errno;return result;
}
static ssize_t probe_udp_receive_from(probe_recvmsg_fn receive,int fd,void *buffer,size_t size,
    int flags,struct sockaddr *source,socklen_t *length,struct probe_udp_metadata *metadata) {
    /* Preserve recvfrom's invalid non-null name/null-length rejection. */
    if (source && !length) {errno=EFAULT;return -1;}
    struct iovec vector={buffer,size};
    struct msghdr message={.msg_name=source,.msg_namelen=source ? *length:0,.msg_iov=&vector,.msg_iovlen=1};
    ssize_t result=probe_udp_receive(receive,fd,&message,flags,0,metadata);
    if (result>=0 && source) *length=message.msg_namelen;
    return result;
}
#endif

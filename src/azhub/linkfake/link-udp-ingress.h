/* SPDX-License-Identifier: GPL-3.0-only */
/* Private UDP ingress integration with scoped sender provenance checks.
 * Included in place of the original recvfrom/recvmsg wrappers. Caller changes
 * to IP_PKTINFO after first reception and rejected MSG_PEEK remain open gates.
 */
#include <net/if.h>
#include "link-udp-receive.h"
#include "link-udp-policy.h"
struct probe_udp_endpoint { int fd,pin,visible; unsigned port,member; };
static struct probe_udp_endpoint probe_udp_receivers[16],probe_udp_senders[MAX_FAKES];
static pthread_mutex_t probe_udp_mutex=PTHREAD_MUTEX_INITIALIZER;
static void probe_udp_forget(int fd) {
    pthread_mutex_lock(&probe_udp_mutex);
    for(unsigned i=0;i<16;i++) if(probe_udp_receivers[i].port && probe_udp_receivers[i].fd==fd) {
        link_socket_close_pin(probe_udp_receivers[i].pin);memset(&probe_udp_receivers[i],0,sizeof probe_udp_receivers[i]);
    }
    for(unsigned i=0;i<MAX_FAKES;i++) if(probe_udp_senders[i].port && probe_udp_senders[i].fd==fd) {
        link_socket_close_pin(probe_udp_senders[i].pin);memset(&probe_udp_senders[i],0,sizeof probe_udp_senders[i]);
    }
    pthread_mutex_unlock(&probe_udp_mutex);
}
static void probe_udp_sender_add(int fd,unsigned member) {
    struct sockaddr_in address={0};socklen_t size=sizeof address;
    if(getsockname(fd,(void *)&address,&size) || address.sin_family!=AF_INET ||
        address.sin_addr.s_addr!=htonl(INADDR_LOOPBACK) || !probe_udp_private_number(member)) return;
    pthread_mutex_lock(&probe_udp_mutex);
    for(unsigned i=0;i<MAX_FAKES;i++) if(!probe_udp_senders[i].port) {
        int pin=fcntl(fd,F_DUPFD_CLOEXEC,0);
        if(pin>=0) probe_udp_senders[i]=(struct probe_udp_endpoint){fd,pin,0,ntohs(address.sin_port),member};
        break;
    }
    pthread_mutex_unlock(&probe_udp_mutex);
}
/* Return a per-call pin so a concurrent close cannot redirect the receive to
 * a reused descriptor. No registry lock is held across the receive syscall. */
static struct probe_udp_endpoint probe_udp_receiver(int fd) {
    struct probe_udp_endpoint result={.pin=-1};
    struct sockaddr_in address={0};socklen_t size=sizeof address;
    int type=0;socklen_t type_size=sizeof type;
    if(getsockname(fd,(void *)&address,&size) || address.sin_family!=AF_INET ||
        ntohs(address.sin_port)<50000 || ntohs(address.sin_port)>50002 ||
        getsockopt(fd,SOL_SOCKET,SO_TYPE,&type,&type_size) || type!=SOCK_DGRAM) return result;
    result.port=ntohs(address.sin_port);
    pthread_mutex_lock(&probe_udp_mutex);
    for(unsigned i=0;i<16;i++) if(probe_udp_receivers[i].port && probe_udp_receivers[i].fd==fd) {
        if(link_socket_same(fd,probe_udp_receivers[i].pin)) {
            result=probe_udp_receivers[i];result.pin=fcntl(result.pin,F_DUPFD_CLOEXEC,0);
            pthread_mutex_unlock(&probe_udp_mutex);return result;
        }
        link_socket_close_pin(probe_udp_receivers[i].pin);memset(&probe_udp_receivers[i],0,sizeof probe_udp_receivers[i]);
    }
    for(unsigned i=0;i<16;i++) if(!probe_udp_receivers[i].port) {
        int pin=fcntl(fd,F_DUPFD_CLOEXEC,0),visible=0,one=1;socklen_t option_size=sizeof visible;
        if(pin>=0 && !getsockopt(pin,IPPROTO_IP,IP_PKTINFO,&visible,&option_size) &&
            !setsockopt(pin,IPPROTO_IP,IP_PKTINFO,&one,sizeof one)) {
            probe_udp_receivers[i]=(struct probe_udp_endpoint){fd,pin,visible,ntohs(address.sin_port),0};
            result=probe_udp_receivers[i];result.pin=fcntl(pin,F_DUPFD_CLOEXEC,0);
        } else link_socket_close_pin(pin);
        break;
    }
    pthread_mutex_unlock(&probe_udp_mutex);return result;
}
static unsigned probe_udp_trusted(const struct probe_udp_metadata *metadata) {
    unsigned member=0,loopback=if_nametoindex("lo");
    const struct sockaddr_in *source=as_in((const void *)&metadata->source,metadata->source_length);
    if(!loopback || metadata->control_incomplete || metadata->interface_index!=loopback || !source ||
        source->sin_addr.s_addr!=htonl(INADDR_LOOPBACK)) return 0;
    pthread_mutex_lock(&probe_udp_mutex);
    for(unsigned i=0;i<MAX_FAKES;i++) if(probe_udp_senders[i].port==ntohs(source->sin_port) &&
        probe_udp_senders[i].member && link_socket_same(probe_udp_senders[i].fd,probe_udp_senders[i].pin)) {
        member=probe_udp_senders[i].member;break;
    }
    pthread_mutex_unlock(&probe_udp_mutex);return member;
}
static int probe_udp_admit(struct probe_udp_endpoint endpoint,const struct probe_udp_metadata *metadata,
    const uint8_t prefix[54],size_t copied,size_t length,unsigned *trusted) {
    *trusted=probe_udp_trusted(metadata);
    size_t required=length<54 ? length:54;
    int rejected=copied<required || probe_udp_reject(prefix,length,endpoint.port,*trusted);
    if(rejected) {
        static unsigned notes;
        if(__atomic_fetch_add(&notes,1,__ATOMIC_RELAXED)<24)
            lf_log("UDP ingress rejected port=%u interface=%u trusted=%u bytes=%zu",endpoint.port,metadata->interface_index,*trusted,length);
    }
    return !rejected;
}
ssize_t recvfrom(int fd,void *buffer,size_t size,int flags,struct sockaddr *source,socklen_t *length) {
    if(!lf_runtime_ready()) return real_recvfrom(fd,buffer,size,flags,source,length);
    struct probe_udp_endpoint endpoint=probe_udp_receiver(fd);
    if(endpoint.pin<0) {
        if(endpoint.port) {errno=ENOBUFS;return -1;}
        return real_recvfrom(fd,buffer,size,flags,source,length);
    }
    socklen_t capacity=source && length ? *length:0;
    struct probe_udp_metadata metadata;
    ssize_t result=probe_udp_receive_from(real_recvmsg,endpoint.pin,buffer,size,flags,source,length,&metadata);
    int saved_errno=errno;link_socket_close_pin(endpoint.pin);errno=saved_errno;
    if(result<=0) return result;
    uint8_t prefix[54]={0};size_t copied=metadata.copied_bytes<54 ? metadata.copied_bytes:54;
    memcpy(prefix,buffer,copied);unsigned trusted;
    if(!probe_udp_admit(endpoint,&metadata,prefix,copied,(size_t)result,&trusted)) {errno=EAGAIN;return -1;}
    if(trusted && source && length) {
        fix_source((void *)&metadata.source,metadata.source_length);
        if(capacity>metadata.source_length) capacity=metadata.source_length;
        memcpy(source,&metadata.source,capacity);
    }
    return result;
}
ssize_t recvmsg(int fd,struct msghdr *message,int flags) {
    if(!lf_runtime_ready()) return real_recvmsg(fd,message,flags);
    struct probe_udp_endpoint endpoint=probe_udp_receiver(fd);
    if(endpoint.pin<0) {
        if(endpoint.port) {errno=ENOBUFS;return -1;}
        return real_recvmsg(fd,message,flags);
    }
    socklen_t capacity=message && message->msg_name ? message->msg_namelen:0;
    struct probe_udp_metadata metadata;
    ssize_t result=probe_udp_receive(real_recvmsg,endpoint.pin,message,flags,endpoint.visible,&metadata);
    int saved_errno=errno;link_socket_close_pin(endpoint.pin);errno=saved_errno;
    if(result<=0) return result;
    uint8_t prefix[54]={0};size_t copied=0,remaining=metadata.copied_bytes;
    for(size_t i=0;i<message->msg_iovlen && copied<54 && remaining;i++) {
        size_t take=message->msg_iov[i].iov_len;if(take>54-copied) take=54-copied;if(take>remaining) take=remaining;
        memcpy(prefix+copied,message->msg_iov[i].iov_base,take);copied+=take;remaining-=take;
    }
    unsigned trusted;
    if(!probe_udp_admit(endpoint,&metadata,prefix,copied,(size_t)result,&trusted)) {errno=EAGAIN;return -1;}
    if(trusted && message->msg_name) {
        fix_source((void *)&metadata.source,metadata.source_length);
        if(capacity>metadata.source_length) capacity=metadata.source_length;
        memcpy(message->msg_name,&metadata.source,capacity);
    }
    return result;
}

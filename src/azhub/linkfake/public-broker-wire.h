/* SPDX-License-Identifier: GPL-3.0-only */
/* Bounded local broker packets, independent of EP147's socket adapters.
 * SOCK_SEQPACKET preserves one command and its descriptor references together.
 * Received descriptors are CLOEXEC and belong to the frame until disposed.
 * Peer/token checks are explicit helpers; decoding alone does not authenticate
 * a session, authorize namespaces/generations or execute operations. */
#ifndef HUB_PUBLIC_BROKER_WIRE_H
#define HUB_PUBLIC_BROKER_WIRE_H
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#define HUB_BROKER_HEADER 64
#define HUB_BROKER_PAYLOAD 4096
#define HUB_BROKER_FDS 2
enum hub_public_broker_op { HUB_BROKER_HELLO=1,HUB_BROKER_SOURCE,HUB_BROKER_DETACH,
    HUB_BROKER_STATUS,HUB_BROKER_STOP,HUB_BROKER_ALLOCATE };
struct hub_public_broker_frame {
    unsigned op,slot,error,count;
    uint64_t generation;
    unsigned char token[16];
    size_t length;
    unsigned char payload[HUB_BROKER_PAYLOAD];
    int descriptors[HUB_BROKER_FDS];
};
static void hub_public_broker_dispose(struct hub_public_broker_frame *frame) {
    if (!frame) return;
    for (unsigned i=0;i<frame->count && i<HUB_BROKER_FDS;++i)
        if (frame->descriptors[i]>=0) close(frame->descriptors[i]);
    frame->count=0;
    for (unsigned i=0;i<HUB_BROKER_FDS;++i) frame->descriptors[i]=-1;
}
static void hub_public_broker_put(unsigned char *p,uint64_t value,unsigned size) {
    while (size) { p[--size]=(unsigned char)value;value>>=8; }
}
static uint64_t hub_public_broker_get(const unsigned char *p,unsigned size) {
    uint64_t value=0;while (size--) value=(value<<8)|*p++;return value;
}
static int hub_public_broker_valid(const struct hub_public_broker_frame *frame) {
    if (!frame || frame->op<HUB_BROKER_HELLO || frame->op>HUB_BROKER_ALLOCATE || frame->slot>=8 ||
            frame->length>HUB_BROKER_PAYLOAD || frame->count>HUB_BROKER_FDS ||
            !memcmp(frame->token,"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",16)) return 0;
    if (frame->op!=HUB_BROKER_STATUS && frame->error) return 0;
    if (frame->op==HUB_BROKER_SOURCE || frame->op==HUB_BROKER_ALLOCATE)
        return frame->generation && frame->count==1 && frame->length;
    if (frame->count) return 0;
    if (frame->op==HUB_BROKER_DETACH) return frame->generation && !frame->length;
    return frame->op==HUB_BROKER_STATUS || !frame->length;
}
static int hub_public_broker_socket(int fd) {
    int type=0;socklen_t length=sizeof type;
    if (getsockopt(fd,SOL_SOCKET,SO_TYPE,&type,&length)<0) return -1;
    struct sockaddr_storage address;socklen_t size=sizeof address;
    if (type!=SOCK_SEQPACKET || getsockname(fd,(struct sockaddr *)&address,&size)<0 ||
            address.ss_family!=AF_UNIX) { errno=EPROTOTYPE;return -1; }
    return 0;
}
static int hub_public_broker_encode(const struct hub_public_broker_frame *frame,
        unsigned char header[HUB_BROKER_HEADER]) {
    if (!hub_public_broker_valid(frame)) { errno=EINVAL;return -1; }
    memset(header,0,HUB_BROKER_HEADER);memcpy(header,"AZHB",4);header[4]=1;
    header[5]=(unsigned char)frame->op;header[6]=(unsigned char)frame->count;
    hub_public_broker_put(header+8,frame->length,4);hub_public_broker_put(header+12,frame->slot,4);
    hub_public_broker_put(header+16,frame->generation,8);memcpy(header+24,frame->token,16);
    hub_public_broker_put(header+40,frame->error,4);return 0;
}
/* Success queues one complete message. Sender retains its descriptors. */
static int hub_public_broker_send(int fd,const struct hub_public_broker_frame *frame) {
    unsigned char header[HUB_BROKER_HEADER];
    if (hub_public_broker_socket(fd)<0 || hub_public_broker_encode(frame,header)<0) return -1;
    struct iovec vectors[2]={{header,sizeof header},{(void *)frame->payload,frame->length}};
    union { struct cmsghdr align;unsigned char bytes[CMSG_SPACE(HUB_BROKER_FDS*sizeof(int))]; } control={0};
    struct msghdr message={.msg_iov=vectors,.msg_iovlen=2};
    if (frame->count) {
        message.msg_control=control.bytes;message.msg_controllen=CMSG_SPACE(frame->count*sizeof(int));
        struct cmsghdr *c=CMSG_FIRSTHDR(&message);c->cmsg_level=SOL_SOCKET;c->cmsg_type=SCM_RIGHTS;
        c->cmsg_len=CMSG_LEN(frame->count*sizeof(int));memcpy(CMSG_DATA(c),frame->descriptors,frame->count*sizeof(int));
    }
    ssize_t sent=syscall(SYS_sendmsg,fd,&message,MSG_DONTWAIT|MSG_NOSIGNAL);
    if (sent==(ssize_t)(HUB_BROKER_HEADER+frame->length)) return 0;
    if (sent>=0) errno=EIO;
    return -1;
}
/*1 complete frame,0 would block,-1 invalid/closed/error. Output must not own
 * descriptors on entry. All newly received FDs close on ANY rejected packet. */
static int hub_public_broker_receive(int fd,struct hub_public_broker_frame *out) {
    if (!out) { errno=EINVAL;return -1; }
    if (hub_public_broker_socket(fd)<0) return -1;
    unsigned char bytes[HUB_BROKER_HEADER+HUB_BROKER_PAYLOAD];
    union { struct cmsghdr align;unsigned char bytes[CMSG_SPACE(HUB_BROKER_FDS*sizeof(int))]; } control={0};
    struct iovec vector={bytes,sizeof bytes};
    struct msghdr message={.msg_iov=&vector,.msg_iovlen=1,.msg_control=control.bytes,.msg_controllen=sizeof control.bytes};
    ssize_t size=syscall(SYS_recvmsg,fd,&message,MSG_DONTWAIT|MSG_CMSG_CLOEXEC);
    if (size<0) return errno==EAGAIN || errno==EWOULDBLOCK ? 0:-1;
    struct hub_public_broker_frame frame={.descriptors={-1,-1}};int invalid=0;
    for (struct cmsghdr *c=CMSG_FIRSTHDR(&message);c;c=CMSG_NXTHDR(&message,c)) {
        if (c->cmsg_level!=SOL_SOCKET || c->cmsg_type!=SCM_RIGHTS || c->cmsg_len<CMSG_LEN(0)) {
            invalid=1;continue;
        }
        size_t length=c->cmsg_len-CMSG_LEN(0);
        if (length%sizeof(int)) invalid=1;
        for (size_t i=0;i<length/sizeof(int);++i) {
            int received;memcpy(&received,(unsigned char *)CMSG_DATA(c)+i*sizeof(int),sizeof received);
            if (frame.count<HUB_BROKER_FDS) frame.descriptors[frame.count++]=received;
            else { close(received);invalid=1; }
        }
    }
    if (invalid || message.msg_flags&(MSG_TRUNC|MSG_CTRUNC) || size<HUB_BROKER_HEADER ||
            memcmp(bytes,"AZHB",4) || bytes[4]!=1 || bytes[7] || bytes[6]!=frame.count) goto rejected;
    for (unsigned i=44;i<HUB_BROKER_HEADER;++i) if (bytes[i]) goto rejected;
    frame.op=bytes[5];frame.length=(size_t)hub_public_broker_get(bytes+8,4);
    frame.slot=(unsigned)hub_public_broker_get(bytes+12,4);frame.generation=hub_public_broker_get(bytes+16,8);
    memcpy(frame.token,bytes+24,16);frame.error=(unsigned)hub_public_broker_get(bytes+40,4);
    if (frame.length!=(size_t)size-HUB_BROKER_HEADER || !hub_public_broker_valid(&frame)) goto rejected;
    memcpy(frame.payload,bytes+HUB_BROKER_HEADER,frame.length);*out=frame;return 1;
rejected:
    hub_public_broker_dispose(&frame);errno=size ? EPROTO:ECONNRESET;return -1;
}
/* Optional pid0 accepts a wrapper-launched peer only when the caller also
 * supplies a reviewed executable FD. Parent-side credentials are kernel data,
 * never a PID claimed inside a protocol message. */
static int hub_public_broker_peer(int fd,pid_t expected_pid,uid_t expected_uid,int executable) {
    if (expected_pid<0 || (expected_pid==0 && executable<0)) { errno=EINVAL;return -1; }
    if (hub_public_broker_socket(fd)<0) return -1;
    struct ucred peer;socklen_t size=sizeof peer;
    if (getsockopt(fd,SOL_SOCKET,SO_PEERCRED,&peer,&size)<0) return -1;
    if (size!=sizeof peer || peer.pid<=0 || peer.uid!=expected_uid ||
            (expected_pid && peer.pid!=expected_pid)) { errno=EACCES;return -1; }
    if (executable>=0) {
        char path[64];snprintf(path,sizeof path,"/proc/%ld/exe",(long)peer.pid);
        int actual=open(path,O_PATH|O_CLOEXEC);if (actual<0) return -1;
        struct stat wanted,observed;
        int okay=fstat(executable,&wanted)==0 && fstat(actual,&observed)==0 && S_ISREG(wanted.st_mode) &&
            wanted.st_dev==observed.st_dev && wanted.st_ino==observed.st_ino;
        close(actual);if (!okay) { errno=EACCES;return -1; }
    }
    return 0;
}
/* Check the per-launch capability on EVERY packet, including descriptor-bearing
 * ones. Caller still owns operation/generation/source authorization. */
static int hub_public_broker_receive_token(int fd,const unsigned char token[16],struct hub_public_broker_frame *out) {
    if (!token) { errno=EINVAL;return -1; }
    int result=hub_public_broker_receive(fd,out);if (result!=1) return result;
    unsigned difference=0;for (unsigned i=0;i<16;++i) difference|=token[i]^out->token[i];
    if (difference) { hub_public_broker_dispose(out);errno=EACCES;return -1; }
    return 1;
}
#endif

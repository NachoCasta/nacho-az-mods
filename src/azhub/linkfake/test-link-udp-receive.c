/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <net/if.h>
#include "link-udp-receive.h"
static int receiver,sender;
static struct sockaddr_in target;
static void send_packet(void) {
    const char text[]="0123456789abcdef";
    assert(sendto(sender,text,16,0,(void *)&target,sizeof target)==16);
}
static void pktinfo(int enabled) {assert(!setsockopt(receiver,IPPROTO_IP,IP_PKTINFO,&enabled,sizeof enabled));}
struct observation {ssize_t result;int flags;size_t control_size;unsigned types[8];size_t lengths[8];unsigned count;};
static struct observation run(int adapter,int visible,size_t capacity,int flags,size_t payload) {
    unsigned char data[32]={0},control[256];memset(control,0xa5,sizeof control);
    struct iovec vectors[2]={{data,payload/2},{data+payload/2,payload-payload/2}};
    struct sockaddr_storage address={0};
    struct msghdr message={.msg_name=&address,.msg_namelen=sizeof address,.msg_iov=vectors,.msg_iovlen=2,
        .msg_control=capacity ? control:NULL,.msg_controllen=capacity};
    pktinfo(adapter || visible);send_packet();
    struct probe_udp_metadata metadata;
    ssize_t result=adapter ? probe_udp_receive(recvmsg,receiver,&message,flags,visible,&metadata):recvmsg(receiver,&message,flags);
    assert(result==((flags&MSG_TRUNC) || payload>=16 ? 16:(ssize_t)payload));
    assert(!memcmp(data,"0123456789abcdef",payload<16 ? payload:16));
    assert(message.msg_namelen==sizeof(struct sockaddr_in));
    if (adapter) {
        assert(metadata.interface_index==if_nametoindex("lo"));
        assert(!metadata.control_incomplete);
        assert(metadata.copied_bytes==(payload<16 ? payload:16));
    }
    struct observation out={.result=result,.flags=message.msg_flags,.control_size=message.msg_controllen};
    for(struct cmsghdr *c=CMSG_FIRSTHDR(&message);c;c=CMSG_NXTHDR(&message,c)) {
        assert(out.count<8);out.types[out.count]=c->cmsg_type;out.lengths[out.count++]=c->cmsg_len;
    }
    if(flags&MSG_PEEK) {char consumed[16];assert(recv(receiver,consumed,sizeof consumed,0)==16);assert(!memcmp(consumed,"0123456789abcdef",16));}
    return out;
}
int main(void) {
    receiver=socket(AF_INET,SOCK_DGRAM,0);sender=socket(AF_INET,SOCK_DGRAM,0);assert(receiver>=0 && sender>=0);
    memset(&target,0,sizeof target);target.sin_family=AF_INET;target.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    assert(!bind(receiver,(void *)&target,sizeof target));socklen_t length=sizeof target;
    assert(!getsockname(receiver,(void *)&target,&length));
    int one=1;assert(!setsockopt(receiver,SOL_SOCKET,SO_TIMESTAMP,&one,sizeof one));
    assert(!setsockopt(receiver,IPPROTO_IP,IP_RECVTTL,&one,sizeof one));
    for(int visible=0;visible<=1;visible++) for(size_t capacity=0;capacity<=128;capacity++)
        for(unsigned mode=0;mode<4;mode++) {
            int flags=(mode&1 ? MSG_TRUNC:0)|(mode&2 ? MSG_PEEK:0);
            struct observation native=run(0,visible,capacity,flags,7);
            struct observation adapted=run(1,visible,capacity,flags,7);
            assert(native.result==adapted.result && native.flags==adapted.flags && native.control_size==adapted.control_size);
            assert(native.count==adapted.count);
            for(unsigned i=0;i<native.count;i++) assert(native.types[i]==adapted.types[i] && native.lengths[i]==adapted.lengths[i]);
        }
    (void)run(1,0,128,0,32);
    (void)run(1,0,128,MSG_TRUNC,0);
    pktinfo(1);send_packet();char payload[32];unsigned char short_name[3]={0};length=sizeof short_name;
    struct probe_udp_metadata metadata;
    assert(probe_udp_receive_from(recvmsg,receiver,payload,sizeof payload,0,(void *)short_name,&length,&metadata)==16);
    assert(length==sizeof(struct sockaddr_in) && metadata.interface_index==if_nametoindex("lo"));
    send_packet();length=99;
    assert(probe_udp_receive_from(recvmsg,receiver,payload,sizeof payload,0,NULL,&length,&metadata)==16 && length==99);
    errno=0;assert(probe_udp_receive_from(recvmsg,receiver,payload,sizeof payload,0,(void *)short_name,NULL,&metadata)==-1 && errno==EFAULT);
    errno=0;assert(probe_udp_receive_from(recvmsg,receiver,payload,sizeof payload,MSG_DONTWAIT,NULL,NULL,&metadata)==-1 && (errno==EAGAIN || errno==EWOULDBLOCK));
    close(receiver);close(sender);
    puts("PASS: 1032 native/adapter comparisons, scatter buffers, control truncation, timestamp/TTL, peek, MSG_TRUNC and recvfrom names");
    return 0;
}

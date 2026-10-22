/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <sys/utsname.h>
#include "public-address-remove.h"
static int live(int argc,char **argv) {
    struct utsname kernel;assert(!uname(&kernel) && !strcmp(kernel.release,"6.6.138-dirty"));
    assert(argc==3 && !strncmp(argv[2],"/var/run/netns/azhub-rm-",23));
    struct hub_public_network_spec spec={.namespace_fd=open(argv[2],O_RDONLY|O_CLOEXEC),
        .interface="azpub7",.mac={2,0,0,0,0x12,0x73}};
    assert(spec.namespace_fd>=0);
    inet_pton(AF_INET,"192.0.2.3",&spec.address);inet_pton(AF_INET,"255.255.255.0",&spec.mask);
    inet_pton(AF_INET,"192.0.2.255",&spec.broadcast);
    struct hub_public_address_remove op={0};assert(!hub_public_address_remove_start(&op,&spec));close(spec.namespace_fd);
    uint64_t deadline=hub_public_address_remove_ms()+5000;int result;
    while ((result=hub_public_address_remove_poll(&op))==1) {
        assert(hub_public_address_remove_ms()<deadline);poll(NULL,0,1);
    }
    if (result) fprintf(stderr,"removal failed: %s\n",strerror(errno));
    assert(!result && !op.started && op.spec.namespace_fd==-1);
    puts("PASS: kernel-acknowledged exact IPv4 removal in isolated emulator namespace");return 0;
}
int main(int argc,char **argv) {
    if (argc>1) return live(argc,argv);
    struct hub_public_network_spec spec={.namespace_fd=open("/proc/self/ns/net",O_RDONLY|O_CLOEXEC),
        .interface="azpub0",.mac={2,0,0,0,0,3}};
    assert(spec.namespace_fd>=0);inet_pton(AF_INET,"192.0.2.3",&spec.address);
    inet_pton(AF_INET,"255.255.255.0",&spec.mask);
    struct hub_public_address_remove_packet packet;
    assert(!hub_public_address_remove_packet(&spec,77,&packet));
    assert(packet.header.nlmsg_type==RTM_DELADDR && packet.address.ifa_index==77 && packet.address.ifa_prefixlen==24);
    int remaining=packet.header.nlmsg_len-NLMSG_LENGTH(sizeof packet.address),count=0;
    for (struct rtattr *a=IFA_RTA(&packet.address);RTA_OK(a,remaining);a=RTA_NEXT(a,remaining)) {
        assert(RTA_PAYLOAD(a)==4 && !memcmp(RTA_DATA(a),&spec.address.s_addr,4));++count;
    }
    assert(!remaining && count==2);
    inet_pton(AF_INET,"192.0.2.255",&spec.broadcast);
    assert(!hub_public_address_configure_packet(&spec,77,&packet));
    assert(packet.header.nlmsg_type==RTM_NEWADDR && (packet.header.nlmsg_flags&NLM_F_EXCL));
    spec.broadcast.s_addr^=1;assert(hub_public_address_configure_packet(&spec,77,&packet)<0);spec.broadcast.s_addr^=1;
    struct { struct nlmsghdr h;struct nlmsgerr e; } reply={.h={.nlmsg_len=NLMSG_LENGTH(sizeof(struct nlmsgerr)),.nlmsg_type=NLMSG_ERROR,.nlmsg_seq=1}};
    assert(!hub_public_address_remove_ack(&reply,sizeof reply));
    reply.e.error=-EADDRNOTAVAIL;assert(hub_public_address_remove_ack(&reply,sizeof reply)<0 && errno==EADDRNOTAVAIL);
    reply.e.error=1;assert(hub_public_address_remove_ack(&reply,sizeof reply)<0 && errno==EPROTO);
    reply.e.error=0;reply.h.nlmsg_seq=2;assert(hub_public_address_remove_ack(&reply,sizeof reply)<0);
    reply.h.nlmsg_seq=1;
    for (size_t n=0;n<sizeof reply;++n) assert(hub_public_address_remove_ack(&reply,n)<0);
    struct hub_public_address_remove op={0};
    assert(hub_public_address_remove_start(&op,&spec)<0 && errno==EPERM);
    close(spec.namespace_fd);spec.namespace_fd=open("/proc/self/ns/mnt",O_RDONLY|O_CLOEXEC);assert(spec.namespace_fd>=0);
    assert(!hub_public_address_remove_start(&op,&spec));
    int result;while ((result=hub_public_address_remove_poll(&op))==1) poll(NULL,0,1);
    assert(result<0 && !op.started && op.spec.namespace_fd==-1);close(spec.namespace_fd);
    inet_pton(AF_INET,"255.0.255.0",&spec.mask);assert(hub_public_address_remove_packet(&spec,77,&packet)<0);
    puts("PASS: exact DELADDR request, strict ACK/error parsing, native namespace refusal and wrong-type async cleanup; no address mutation");
}

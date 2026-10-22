/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <sys/utsname.h>
#include "public-network-link.h"
#include "public-network-namespace.h"
#include "public-address-arp-namespace.h"
struct verify { int fd,error,configured,absent; };
static void *verify(void *opaque) {
    struct verify *v=opaque;if (setns(v->fd,CLONE_NEWNET)<0) { v->error=errno;return NULL; }
    if (v->absent) { assert(!if_nametoindex("azpub6"));return NULL; }
    int fd=socket(AF_INET,SOCK_DGRAM|SOCK_CLOEXEC,0);assert(fd>=0);
    struct ifreq req={0};strcpy(req.ifr_name,"azpub6");
    assert(!ioctl(fd,SIOCGIFFLAGS,&req) && (req.ifr_flags&IFF_UP));
    assert(!ioctl(fd,SIOCGIFHWADDR,&req));const unsigned char expected[6]={2,0,0,0,0x12,0x76};
    assert(!memcmp(req.ifr_hwaddr.sa_data,expected,6));
    if (v->configured) {
        assert(!ioctl(fd,SIOCGIFADDR,&req) && ((struct sockaddr_in *)&req.ifr_addr)->sin_addr.s_addr==inet_addr("192.0.2.3"));
    } else assert(ioctl(fd,SIOCGIFADDR,&req)<0 && errno==EADDRNOTAVAIL);
    close(fd);return NULL;
}
int main(int argc,char **argv) {
    struct hub_public_network_link_spec spec={.namespace_fd=9,.parent="eth0",.name="azpub6",
        .parent_mac={0x52,0x54,0,0x12,0x34,0x56},.mac={2,0,0,0,0x12,0x76}};
    struct hub_public_network_link_packet packet;assert(!hub_public_network_link_packet(&spec,2,&packet));
    assert(packet.header.nlmsg_type==RTM_NEWLINK && (packet.header.nlmsg_flags&NLM_F_EXCL));
    int length=packet.header.nlmsg_len-NLMSG_LENGTH(sizeof packet.link),attributes=0;
    for (struct rtattr *a=IFLA_RTA(&packet.link);RTA_OK(a,length);a=RTA_NEXT(a,length)) ++attributes;
    assert(!length && attributes==5);
    spec.mac[0]=1;assert(hub_public_network_link_packet(&spec,2,&packet)<0);spec.mac[0]=2;
    assert(hub_public_network_link_remove_packet(0,&packet)<0);
    assert(!hub_public_network_link_remove_packet(7,&packet));
    assert(packet.header.nlmsg_type==RTM_DELLINK && packet.link.ifi_index==7 &&
        packet.header.nlmsg_flags==(NLM_F_REQUEST|NLM_F_ACK) && packet.header.nlmsg_len==NLMSG_LENGTH(sizeof packet.link));
    if (argc>1) {
        struct utsname kernel;assert(argc==2 && !strcmp(argv[1],"--emulator") && !uname(&kernel) && !strcmp(kernel.release,"6.6.138-dirty"));
        struct stat before,after;assert(!stat("/proc/self/ns/net",&before));
        struct hub_public_namespace_create ns={0};assert(!hub_public_namespace_create_start(&ns));
        int result;while ((result=hub_public_namespace_create_poll(&ns))==1) poll(NULL,0,1);
        assert(!result);spec.namespace_fd=hub_public_namespace_create_take(&ns);assert(spec.namespace_fd>=0);
        struct hub_public_network_link op={0};assert(!hub_public_network_link_start(&op,&spec));
        while ((result=hub_public_network_link_poll(&op))==1) poll(NULL,0,1);
        if (result) fprintf(stderr,"link create: %s\n",strerror(errno));
        assert(!result);
        struct verify check={.fd=spec.namespace_fd};pthread_t thread;assert(!pthread_create(&thread,NULL,verify,&check));
        assert(!pthread_join(thread,NULL) && !check.error);
        struct hub_public_arp_namespace factory={0};
        assert(!hub_public_arp_namespace_start(&factory,spec.namespace_fd,spec.name,spec.mac));
        while ((result=hub_public_arp_namespace_poll(&factory))==1) poll(NULL,0,1);
        assert(!result);struct hub_public_arp transport={.fd=-1};assert(!hub_public_arp_namespace_take(&factory,&transport));
        assert((fcntl(transport.fd,F_GETFL)&O_NONBLOCK) && (fcntl(transport.fd,F_GETFD)&FD_CLOEXEC));
        struct hub_public_acd arp;const unsigned char candidate[4]={192,0,2,3};uint64_t now=hub_public_address_remove_ms();
        assert(!hub_public_acd_init(&arp,candidate,spec.mac,now,0));assert(hub_public_acd_poll(&arp,now)==HUB_ACD_PROBE);
        uint64_t send_deadline=now+2000;
        while (hub_public_arp_send(&transport,&arp)<0) {
            assert(errno==EAGAIN || errno==EWOULDBLOCK || errno==EINTR);assert(hub_public_address_remove_ms()<send_deadline);poll(NULL,0,1);
        }
        hub_public_arp_close(&transport);
        struct hub_public_network_spec address={.namespace_fd=spec.namespace_fd,.interface="azpub6"};memcpy(address.mac,spec.mac,6);
        inet_pton(AF_INET,"192.0.2.3",&address.address);inet_pton(AF_INET,"255.255.255.0",&address.mask);
        inet_pton(AF_INET,"192.0.2.255",&address.broadcast);
        struct hub_public_address_remove operation={0};assert(!hub_public_address_configure_start(&operation,&address));
        while ((result=hub_public_address_remove_poll(&operation))==1) poll(NULL,0,1);
        assert(!result);check.configured=1;assert(!pthread_create(&thread,NULL,verify,&check) && !pthread_join(thread,NULL));
        assert(!hub_public_address_configure_start(&operation,&address));
        while ((result=hub_public_address_remove_poll(&operation))==1) poll(NULL,0,1);
        assert(result<0 && errno==EADDRINUSE);
        assert(!hub_public_address_remove_start(&operation,&address));
        while ((result=hub_public_address_remove_poll(&operation))==1) poll(NULL,0,1);
        assert(!result);check.configured=0;assert(!pthread_create(&thread,NULL,verify,&check) && !pthread_join(thread,NULL));
        spec.mac[5]^=1;
        assert(!hub_public_network_link_remove_start(&op,&spec));
        while ((result=hub_public_network_link_poll(&op))==1) poll(NULL,0,1);
        assert(result<0 && errno==ESTALE);spec.mac[5]^=1;
        assert(!pthread_create(&thread,NULL,verify,&check) && !pthread_join(thread,NULL));
        assert(!hub_public_network_link_remove_start(&op,&spec));
        while ((result=hub_public_network_link_poll(&op))==1) poll(NULL,0,1);
        assert(!result);check.absent=1;
        assert(!pthread_create(&thread,NULL,verify,&check) && !pthread_join(thread,NULL));
        assert(!hub_public_network_link_start(&op,&spec));
        while ((result=hub_public_network_link_poll(&op))==1) poll(NULL,0,1);
        assert(!result);check.absent=0;
        assert(!pthread_create(&thread,NULL,verify,&check) && !pthread_join(thread,NULL));
        assert(!hub_public_network_link_remove_start(&op,&spec));
        while ((result=hub_public_network_link_poll(&op))==1) poll(NULL,0,1);
        assert(!result);close(spec.namespace_fd);
        assert(!stat("/proc/self/ns/net",&after) && before.st_ino==after.st_ino);
        puts("PASS: owned namespace/MACVLAN, namespace-bound ARP probe from controller, configure/remove and native namespace preservation");
    } else {
        struct hub_public_network_link op={0};spec.namespace_fd=open("/proc/self/ns/net",O_RDONLY|O_CLOEXEC);assert(spec.namespace_fd>=0);
        assert(hub_public_network_link_start(&op,&spec)<0 && errno==EPERM);
        assert(hub_public_network_link_remove_start(&op,&spec)<0 && errno==EPERM);close(spec.namespace_fd);
        puts("PASS: bounded nested MACVLAN request and native namespace refusal; no network mutation");
    }
}

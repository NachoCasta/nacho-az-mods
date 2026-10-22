/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <sys/utsname.h>
#include "public-network-acquire.h"
#include "public-network-ready.h"
#include "public-link-local.h"
#include "../research/scripts/allocated-library-probe.h"
struct echo { int ns,error,link_local,no_default_route;struct hub_public_dhcp_event event; };
static void *roundtrip(void *opaque) {
    struct echo *e=opaque;if (setns(e->ns,CLONE_NEWNET)<0) { e->error=errno;return NULL; }
    if (e->link_local) {
        FILE *routes=fopen("/proc/thread-self/net/route","re");assert(routes);char line[512];int found=0;
        assert(fgets(line,sizeof line,routes));
        while (fgets(line,sizeof line,routes)) {
            fprintf(stderr,"owned route: %s",line);
            char interface[16];unsigned destination,gateway,flags,mask;
            assert(sscanf(line,"%15s %x %x %x %*u %*u %*u %x",interface,&destination,&gateway,&flags,&mask)==5);
            assert(gateway==0 && (destination || mask));
            if (!strcmp(interface,"azpub0") && destination==inet_addr("169.254.0.0") && mask==inet_addr("255.255.0.0")) found=1;
        }
        assert(!fclose(routes) && found);e->no_default_route=1;
    }
    int fd=socket(AF_INET,SOCK_DGRAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0);assert(fd>=0);
    struct sockaddr_in local={.sin_family=AF_INET,.sin_addr=e->event.address},peer={.sin_family=AF_INET,.sin_port=htons(49000),.sin_addr=e->event.server};
    assert(!bind(fd,(void *)&local,sizeof local) && !connect(fd,(void *)&peer,sizeof peer));
    const char message[]="AZHUB-ADDRESS-READY";assert(send(fd,message,sizeof message-1,0)==sizeof message-1);
    struct pollfd wait={.fd=fd,.events=POLLIN};assert(poll(&wait,1,3000)==1);
    char reply[32];assert(recv(fd,reply,sizeof reply,0)==16 && !memcmp(reply,"AZHUB-ADDRESS-OK",16));close(fd);return NULL;
}
#include "../research/scripts/broker-allocated-library-probe.h"
static int broker_roundtrip(int ns,const struct hub_public_dhcp_event *event) {
    struct echo echo={.ns=ns,.event=*event};pthread_t thread;
    assert(!pthread_create(&thread,NULL,roundtrip,&echo) && !pthread_join(thread,NULL));return echo.error;
}
int main(int argc,char **argv) {
    int emulator=(argc==4 || argc==5) && !strcmp(argv[1],"--emulator"),ready_mode=argc==5;
    int library=ready_mode && !strcmp(argv[4],"--library");
    int link_local=ready_mode && !strcmp(argv[4],"--link-local"),direct_route=0;
    struct hub_public_network_acquire a={0};
    struct hub_public_network_link_spec spec={.parent="eth0",.name="azpub0",
        .parent_mac={0x52,0x54,0,0x12,0x34,0x56},.mac={2,0,0,0,0x12,3}};
    if (!emulator) {
        assert(argc==1);
        assert(!hub_public_network_acquire_start(&a,&spec,"/not/launched","/not/launched"));
        assert(!hub_public_network_acquire_allow_no_offer(&a));
        assert(hub_public_network_acquire_start(&a,&spec,"/not/launched","/not/launched")<0 && errno==EBUSY);
        hub_public_network_acquire_cancel(&a,ECANCELED,hub_public_address_remove_ms());
        int result;while (!(result=hub_public_network_acquire_poll(&a,hub_public_address_remove_ms()))) poll(NULL,0,1);
        assert(result<0 && a.complete && a.namespace_fd==-1 && !a.namespace.started && !a.link.started);
        /* Synthetic completed native no-offer boundary. No namespace entry or
         * network mutation: only a duplicate of the current namespace is owned. */
        for (unsigned allowed=0;allowed<2;++allowed) {
            memset(&a,0,sizeof a);a.phase=3;a.allow_no_offer=allowed;
            a.namespace_fd=open("/proc/self/ns/net",O_RDONLY|O_CLOEXEC);assert(a.namespace_fd>=0);
            a.dhcp=(struct hub_public_dhcp_acquire){.pid=0,.fd=-1,.complete=1,.reaped=1,.eof=1,
                .error=ENODATA,.status=256,.deconfigured=1,.lease_failed=1,.records=2};
            int owned=a.namespace_fd;result=hub_public_network_acquire_poll(&a,1);
            if (allowed) {
                assert(result==2 && a.complete && a.no_offer && !a.error && a.namespace_fd==owned);
                assert(!a.dhcp.bound && !a.dhcp.proposal.address.s_addr && fcntl(owned,F_GETFD)>=0);
                assert(hub_public_network_acquire_poll(&a,2)==2);close(owned);a.namespace_fd=-1;
            } else assert(result<0 && a.complete && a.namespace_fd==-1 && fcntl(owned,F_GETFD)<0);
        }
        puts("PASS: allocation controller cancellation waits for namespace creator and releases ownership; no DHCP launched");return 0;
    }
    struct utsname kernel;assert(!uname(&kernel) && !strcmp(kernel.release,"6.6.138-dirty"));
    if (argc==5 && !strcmp(argv[4],"--broker-library")) return broker_allocated_library_probe(argv[2],argv[3],broker_roundtrip) ? 1:0;
    struct stat before,after;assert(!stat("/proc/self/ns/net",&before));
    assert(!hub_public_network_acquire_start(&a,&spec,argv[3],argv[2]));
    if (link_local) assert(!hub_public_network_acquire_allow_no_offer(&a));
    uint64_t deadline=hub_public_address_remove_ms()+25000;int result;
    while (!(result=hub_public_network_acquire_poll(&a,hub_public_address_remove_ms()))) {
        assert(hub_public_address_remove_ms()<deadline);poll(NULL,0,1);
    }
    if (result<0) fprintf(stderr,"allocation failed: %s\n",strerror(errno));
    assert(result==(link_local ? 2:1) && a.complete && a.namespace_fd>=0 && a.dhcp.complete && a.dhcp.reaped);
    if (link_local) assert(hub_public_dhcp_acquire_no_offer(&a.dhcp));
    else assert(a.dhcp.proposal.address.s_addr==inet_addr("192.0.2.3"));
    if (ready_mode) {
        assert(library || link_local || !strcmp(argv[4],"--ready"));struct hub_public_network_lease lease={0};
        if (link_local) {
            int entropy=open("/dev/urandom",O_RDONLY|O_CLOEXEC);assert(entropy>=0);struct in_addr address;
            do { assert(!hub_public_link_local_sample(entropy,&address)); } while (address.s_addr==inet_addr("169.254.2.2"));
            close(entropy);assert(!hub_public_lease_link_local(&lease,address,hub_public_address_remove_ms()));
        } else assert(!hub_public_lease_offer(&lease,&a.dhcp.proposal,a.dhcp.started,hub_public_address_remove_ms()));
        struct hub_public_network_spec address={.namespace_fd=a.namespace_fd,.interface="azpub0",
            .address=lease.current.address,.mask=lease.current.mask,.broadcast=lease.current.broadcast};memcpy(address.mac,spec.mac,6);
        struct hub_public_network_ready ready={0};assert(!hub_public_network_ready_start(&ready,&lease,&address,hub_public_address_remove_ms()));
        do {
            result=hub_public_network_ready_poll(&ready,hub_public_address_remove_ms(),500,(const unsigned char (*)[6])&spec.parent_mac,1);
            if (result<0) fprintf(stderr,"ready failed: %s\n",strerror(errno));
            assert(result>=0 && hub_public_address_remove_ms()<deadline);poll(NULL,0,1);
        } while (!result || ready.arp.state!=HUB_ACD_ACTIVE);
        assert(lease.state==HUB_LEASE_READY && ready.applied);
        struct echo echo={.ns=a.namespace_fd,.event=lease.current,.link_local=link_local};pthread_t thread;
        if (link_local) echo.event.server.s_addr=inet_addr("169.254.2.2");
        assert(!pthread_create(&thread,NULL,roundtrip,&echo) && !pthread_join(thread,NULL) && !echo.error);
        direct_route=echo.no_default_route;
        if (library) assert(!allocated_library_probe(&ready,spec.parent_mac));
        while ((result=hub_public_network_ready_quiesce(&ready))==1) poll(NULL,0,1);
        assert(!result && lease.state==HUB_LEASE_DRAINING);
        assert(!hub_public_lease_drained(&lease,lease.generation,hub_public_address_remove_ms()));
        struct hub_public_address_remove removal={0};assert(!hub_public_address_remove_start(&removal,&address));
        while ((result=hub_public_address_remove_poll(&removal))==1) poll(NULL,0,1);
        assert(!result && !hub_public_lease_removed(&lease,lease.generation,hub_public_address_remove_ms()));
        assert(lease.state==HUB_LEASE_IDLE && !lease.configured);
    }
    assert(!stat("/proc/self/ns/net",&after) && before.st_ino==after.st_ino);
    if (link_local) {
        struct hub_public_network_link removal={0};a.spec.namespace_fd=a.namespace_fd;
        assert(!hub_public_network_link_remove_start(&removal,&a.spec));
        while ((result=hub_public_network_link_poll(&removal))==1) poll(NULL,0,1);
        assert(!result);close(a.namespace_fd);a.namespace_fd=-1;
        assert(direct_route);
        puts("{\"native_no_offer\":true,\"IPv4LL_configured_and_retired\":true,\"direct_UDP_roundtrip\":true,\"no_default_route\":true,\"owned_link_deleted\":true}");return 0;
    }
    printf("{\"allocated_namespace_interface_native_DHCP\":true,\"address\":\"192.0.2.3\",\"started_ms\":%llu,\"lease_seconds\":%u,\"not_public_ready\":%s,\"configured_and_retired\":%s,\"allocated_library_served\":%s}\n",
        (unsigned long long)a.dhcp.started,a.dhcp.proposal.lease_seconds,ready_mode ? "false":"true",ready_mode ? "true":"false",library ? "true":"false");
    close(a.namespace_fd);a.namespace_fd=-1;return 0;
}

/* SPDX-License-Identifier: GPL-3.0-only */
/* Read-only library research harness; advertised mode requires isolated TAP. */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/utsname.h>
struct hub_db_message;
static void research_db_request(const struct hub_db_message *request);
#define HUB_DB_OBSERVE_REQUEST(request) research_db_request(request)
#include "../../linkfake/public-network-endpoint.h"
static void research_db_request(const struct hub_db_message *request) {
    fprintf(stderr,"{\"event\":\"db_request\",\"type\":%u,\"args\":[",request->type);
    for (unsigned i=0;i<request->count;++i) {
        if (i) fputc(',',stderr);
        if (request->args[i].kind==HUB_DB_NUMBER) fprintf(stderr,"%u",request->args[i].number);
        else fputs("null",stderr);
    }
    fputs("]}\n",stderr);
}
static int loopback_peer(void *unused,const struct sockaddr_in *peer) {
    (void)unused; return peer->sin_addr.s_addr==htonl(INADDR_LOOPBACK);
}
static int native_emulator_peer(void *unused,const struct sockaddr_in *peer) {
    (void)unused; return peer->sin_addr.s_addr==htonl(0xc000020f);
}
static int software_peer(void *unused,const struct sockaddr_in *peer) {
    (void)unused; return peer->sin_addr.s_addr==htonl(0xc0000202);
}
static const char *provider_interface="azpub1";
static int isolated_provider_namespace(int emulator) {
    if (emulator) {
        struct utsname system;
        if (uname(&system)<0 || strcmp(system.release,"6.6.138-dirty")) return 0;
    }
    struct stat self,initial;
    if (stat("/proc/self/ns/net",&self)<0 || stat("/proc/1/ns/net",&initial)<0 ||
        (self.st_dev==initial.st_dev && self.st_ino==initial.st_ino)) return 0;
    struct ifaddrs *interfaces;
    if (getifaddrs(&interfaces)<0) return 0;
    int safe=1,found=0;
    for (struct ifaddrs *item=interfaces;item;item=item->ifa_next) {
        if (!strcmp(item->ifa_name,"lo")) continue;
        if (strcmp(item->ifa_name,provider_interface) && (emulator || strcmp(item->ifa_name,"azlink0"))) safe=0;
        if (item->ifa_addr && item->ifa_addr->sa_family==AF_INET) {
            const struct sockaddr_in *address=(const struct sockaddr_in *)item->ifa_addr;
            if (strcmp(item->ifa_name,provider_interface) || address->sin_addr.s_addr!=htonl(emulator ? 0xc0000203 : 0xc0000202)) safe=0;
            ++found;
        }
    }
    freeifaddrs(interfaces); return safe && found==1;
}
static int isolated_loopback_namespace(void) {
    struct stat self,initial;
    if (stat("/proc/self/ns/net",&self)<0 || stat("/proc/1/ns/net",&initial)<0 ||
        (self.st_dev==initial.st_dev && self.st_ino==initial.st_ino)) return 0;
    struct ifaddrs *interfaces;
    if (getifaddrs(&interfaces)<0) return 0;
    int safe=1;
    for (struct ifaddrs *item=interfaces;item;item=item->ifa_next)
        if ((item->ifa_flags&IFF_UP) && strcmp(item->ifa_name,"lo")) safe=0;
    freeifaddrs(interfaces); return safe;
}
static uint64_t milliseconds(void) {
    struct timespec now; assert(!clock_gettime(CLOCK_MONOTONIC,&now));
    return (uint64_t)now.tv_sec*1000+(uint64_t)now.tv_nsec/1000000;
}
int main(int argc,char **argv) {
    uint64_t lifetime=120000;
    const char *seconds=getenv("AZHUB_RESEARCH_SECONDS");
    if (seconds) {
        char *end; unsigned long value=strtoul(seconds,&end,10);
        if (!*seconds || *end || value<5 || value>600) return 2;
        lifetime=(uint64_t)value*1000;
    }
    int emulator=argc==3 && !strcmp(argv[2],"--emulator-provider");
    if (emulator) provider_interface="azpub0";
    int advertise=emulator || (argc==3 && !strcmp(argv[2],"--isolated-provider"));
    int fixed_port=advertise || (argc==3 && !strcmp(argv[2],"--isolated-combined"));
    if (!(argc==2 || (argc==3 && !strcmp(argv[2],"--combined")) || fixed_port) ||
        (advertise ? !isolated_provider_namespace(emulator) : (fixed_port && !isolated_loopback_namespace()))) return 2;
    unsigned char nonce[16]; assert(!hub_public_source_manager_nonce(nonce));
    struct hub_public_source_binding binding={.listener=-1,.udp=-1,.query_listener=-1};
    binding.provider.discovery_fd=binding.provider.status_fd=binding.provider.rpc_alias_fd=-1;
    binding.player=41; binding.allowed=loopback_peer;
    struct hub_public_source_manager *manager=NULL;
    struct hub_public_network_spec specs[HUB_PUBLIC_MANAGED_SOURCES]={0};
    if (advertise) {
        struct hub_public_network_spec spec={.namespace_fd=open("/proc/self/ns/net",O_RDONLY|O_CLOEXEC),
            .address={.s_addr=htonl(emulator ? 0xc0000203 : 0xc0000202)},
            .mask={.s_addr=htonl(0xffffff00)},.broadcast={.s_addr=htonl(0xc00002ff)},
            .player=41,.allowed=emulator ? software_peer : native_emulator_peer};
        assert(spec.namespace_fd>=0); strcpy(spec.interface,provider_interface); strcpy(spec.label,"Hub USB1 Research");
        const unsigned char mac[]={2,0,0,0,emulator ? 18 : 17,emulator ? 3 : 41}; memcpy(spec.mac,mac,6);
        struct stat before,after; assert(!stat("/proc/self/ns/net",&before));
        specs[0]=spec; manager=calloc(1,sizeof *manager); assert(manager);
        struct hub_public_source_manager_ops ops={hub_public_network_manager_open,hub_public_network_manager_close};
        assert(!hub_public_source_manager_init(manager,ops,specs));
        assert(!hub_public_source_manager_request(manager,0,argv[1],1,1));
        assert(!hub_public_source_manager_poll(manager,0));
        binding=manager->sources[0].binding;
        int result=manager->sources[0].error ? -1 : 0,error=manager->sources[0].error;
        assert(!stat("/proc/self/ns/net",&after) && before.st_ino==after.st_ino && before.st_dev==after.st_dev);
        if (result<0) { fprintf(stderr,"Endpoint open failed: %d\n",error); close(specs[0].namespace_fd); free(manager); return 1; }
    } else {
        binding.listener=socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0); assert(binding.listener>=0);
        struct sockaddr_in address={.sin_family=AF_INET,.sin_addr={.s_addr=htonl(INADDR_LOOPBACK)}};
        assert(!bind(binding.listener,(struct sockaddr *)&address,sizeof address) && !listen(binding.listener,8));
        socklen_t size=sizeof address; assert(!getsockname(binding.listener,(struct sockaddr *)&address,&size));
        binding.metadata_port=ntohs(address.sin_port);
        if (argc==3) {
            binding.udp=socket(AF_INET,SOCK_DGRAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0); assert(binding.udp>=0);
            address.sin_port=fixed_port ? htons(111) : 0;
            assert(!bind(binding.udp,(struct sockaddr *)&address,sizeof address));
            size=sizeof address; assert(!getsockname(binding.udp,(struct sockaddr *)&address,&size));
            binding.file_port=ntohs(address.sin_port);
        }
        if (fixed_port) {
            binding.query_listener=socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0); assert(binding.query_listener>=0);
            address.sin_port=htons(12523);
            assert(!bind(binding.query_listener,(struct sockaddr *)&address,sizeof address) && !listen(binding.query_listener,8));
        }
    }
    struct hub_public_source_worker standalone={0};
    struct hub_public_source_worker *worker=manager ? &manager->sources[0].worker : &standalone;
    if (!manager) assert(!hub_public_source_worker_start_configured(worker,argv[1],1,nonce,binding.player,
        binding.listener,binding.udp,binding.file_port,binding.allowed,binding.policy,
        binding.query_listener,binding.metadata_port,NULL));
    while (hub_public_source_worker_state(worker)==HUB_PUBLIC_WORKER_BUILDING) {
        if (manager) assert(!hub_public_source_manager_poll(manager,0));
        poll(NULL,0,1);
    }
    int ready=hub_public_source_worker_state(worker)==HUB_PUBLIC_WORKER_RUNNING;
    if (ready) {
        if (binding.udp>=0) printf("%u %u\n",binding.metadata_port,binding.file_port);
        else printf("%u\n",binding.metadata_port);
        fflush(stdout);
    }
    uint64_t started=milliseconds(); int stop=0,withdrawn_reported=0;
    while (ready && !stop && milliseconds()-started<lifetime) {
        struct pollfd command={.fd=STDIN_FILENO,.events=POLLIN};
        if (poll(&command,1,0)>0 && (command.revents&POLLIN)) {
            char value; if (read(STDIN_FILENO,&value,1)==1) {
                if (value=='Q') stop=1;
                if (value=='E') {
                    if (manager) assert(!hub_public_source_manager_withdraw(manager,0));
                    else hub_public_source_worker_withdraw(worker);
                }
            }
        }
        if (manager) assert(!hub_public_source_manager_poll(manager,0));
        int state=hub_public_source_worker_state(worker);
        if (state==HUB_PUBLIC_WORKER_WITHDRAWN && !withdrawn_reported) {
            puts("WITHDRAWN"); fflush(stdout); withdrawn_reported=1;
        }
        if (state==HUB_PUBLIC_WORKER_FAILED || state==HUB_PUBLIC_WORKER_STOPPED) break;
        poll(NULL,0,1);
    }
    int error;
    if (manager) {
        assert(!hub_public_source_manager_request(manager,0,NULL,0,0));
        while (!hub_public_source_manager_drained(manager,0)) {
            assert(!hub_public_source_manager_poll(manager,0)); poll(NULL,0,1);
        }
        error=manager->sources[0].error;
        close(specs[0].namespace_fd); free(manager);
    } else {
        hub_public_source_worker_stop(worker);
        int joined;
        while (!(joined=hub_public_source_worker_join_ready(worker))) poll(NULL,0,1);
        assert(joined==1); error=worker->error; hub_public_network_close(&binding);
    }
    if (error) fprintf(stderr,"Source worker failed: %d\n",error);
    return ready && !error ? 0 : 1;
}

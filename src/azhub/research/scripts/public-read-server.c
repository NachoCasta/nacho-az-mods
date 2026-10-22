/* SPDX-License-Identifier: GPL-3.0-only */
/* Isolated research harness for the C READ service, not a mod daemon. */
#define _GNU_SOURCE
#include <stdlib.h>
#include <poll.h>
#include <time.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/utsname.h>
#include "../../linkfake/public-rpc-udp.h"
#include "../../linkfake/public-file-catalog.h"
#include "../../linkfake/public-rpc-export.h"
#include "../../linkfake/public-library-export.h"
static unsigned current_peer,read_sequence;
static struct { unsigned port,first,last,requests; size_t bytes; } peer_reads[16];
struct measured_dispatch { hub_public_rpc_datagram_dispatch dispatch; void *context; };
static void measured(void *opaque,const void *data,size_t length,struct hub_public_rpc_reply *reply) {
    struct measured_dispatch *target=opaque;
    target->dispatch(target->context,data,length,reply);
    struct hub_public_rpc_call call;
    if (!getenv("AZ_PUBLIC_RPC_STATS") || hub_public_rpc_decode(data,length,&call)!=HUB_RPC_CALL ||
        call.program!=100003 || call.procedure!=6 || reply->length<100 ||
        hub_public_rpc_word(reply->bytes+20) || hub_public_rpc_word(reply->bytes+24)) return;
    unsigned sequence=++read_sequence;
    for (unsigned i=0;i<16;++i) if (!peer_reads[i].port || peer_reads[i].port==current_peer) {
        peer_reads[i].port=current_peer;
        if (!peer_reads[i].first) peer_reads[i].first=sequence;
        peer_reads[i].last=sequence; ++peer_reads[i].requests;
        peer_reads[i].bytes+=hub_public_rpc_word(reply->bytes+96); break;
    }
}
static int loopback_only(void *unused,const struct sockaddr_in *peer) {
    (void)unused; current_peer=ntohs(peer->sin_port);
    return peer->sin_addr.s_addr==htonl(INADDR_LOOPBACK);
}
static int emulator_peer_only(void *unused,const struct sockaddr_in *peer) {
    (void)unused; current_peer=ntohs(peer->sin_port);
    return peer->sin_addr.s_addr==htonl(0xc0000202U);
}
static int isolated_emulator_link(void) {
    struct utsname system;
    struct stat self,initial;
    if (uname(&system)<0 || strcmp(system.release,"6.6.138-dirty") ||
        stat("/proc/self/ns/net",&self)<0 || stat("/proc/1/ns/net",&initial)<0 ||
        (self.st_dev==initial.st_dev && self.st_ino==initial.st_ino)) return 0;
    struct ifaddrs *interfaces;
    if (getifaddrs(&interfaces)<0) return 0;
    int safe=1,found=0;
    for (struct ifaddrs *item=interfaces;item;item=item->ifa_next) {
        if (!(item->ifa_flags&IFF_UP) || !strcmp(item->ifa_name,"lo")) continue;
        if (strcmp(item->ifa_name,"azpub0")) { safe=0; continue; }
        if (item->ifa_addr && item->ifa_addr->sa_family==AF_INET) {
            const struct sockaddr_in *address=(const struct sockaddr_in *)item->ifa_addr;
            if (address->sin_addr.s_addr!=htonl(0xc0000203U)) safe=0;
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
int main(int argc,char **argv) {
    int portmapper=argc==4 && !strcmp(argv[3],"--isolated-portmapper");
    int emulator_link=argc==4 && !strcmp(argv[3],"--emulator-link");
    if ((argc!=3 && !portmapper && !emulator_link) ||
        (portmapper && !isolated_loopback_namespace()) || (emulator_link && !isolated_emulator_link())) return 2;
    int library_mode=!strcmp(argv[2],"--library");
    int result=1,fd=-1;
    struct hub_public_root root;
    if (hub_public_root_init(&root)<0) return 1;
    struct hub_public_library_files *library=library_mode ? calloc(1,sizeof *library) : NULL;
    struct hub_public_file_catalog *catalog=library_mode ? (library ? &library->catalog : NULL) : calloc(1,sizeof *catalog);
    int initialized=0;
    if (!catalog || hub_public_root_attach(&root,argv[1],1)<0) goto done;
    unsigned char nonce[16],handle[32];
    int random=open("/dev/urandom",O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
    if (random<0) goto done;
    size_t have=0;
    while (have<sizeof nonce) {
        ssize_t got=read(random,nonce+have,sizeof nonce-have);
        if (got<0 && errno==EINTR) continue;
        if (got<=0) { close(random); goto done; }
        have+=(size_t)got;
    }
    close(random);
    if (library_mode) {
        if (hub_public_library_files_init(library,&root,1,nonce)<0) goto done;
        int stage;
        while ((stage=hub_public_library_files_step(library))==0) {}
        if (stage<0 || !hub_public_library_ready_catalog(library)) goto done;
        if (!hub_public_catalog_find(catalog,"",0,handle)) goto done;
        fprintf(stderr,"staged tracks=%u nodes=%u missing_audio=%u\n",library->tracks,catalog->count,library->missing_audio);
    } else {
    if (hub_public_catalog_init(catalog,&root,1,nonce)<0) goto done;
    initialized=1;
    if (hub_public_catalog_register_node(catalog,"",0,1,handle)<0) goto done;
    size_t path_length=strlen(argv[2]);
    if (!path_length || path_length>=HUB_PUBLIC_PATH_MAX) goto done;
    char relative[HUB_PUBLIC_PATH_MAX]; memcpy(relative,argv[2],path_length+1);
    unsigned char temporary[32];
    for (size_t i=0;i<path_length;++i) if (relative[i]=='/') {
        relative[i]=0;
        if (hub_public_catalog_register_node(catalog,relative,i,1,temporary)<0) goto done;
        relative[i]='/';
    }
    if (hub_public_catalog_register(catalog,relative,path_length,temporary)<0) goto done;
    }
    fd=socket(AF_INET,SOCK_DGRAM|SOCK_CLOEXEC,0);
    struct sockaddr_in address={0}; address.sin_family=AF_INET;
    address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    if (portmapper || emulator_link) address.sin_port=htons(111);
    if (emulator_link) {
        address.sin_addr.s_addr=htonl(0xc0000203U);
        if (fd<0 || setsockopt(fd,SOL_SOCKET,SO_BINDTODEVICE,"azpub0",7)<0) goto done;
    }
    if (fd<0 || bind(fd,(struct sockaddr *)&address,sizeof address)<0) goto done;
    socklen_t length=sizeof address;
    if (getsockname(fd,(struct sockaddr *)&address,&length)<0) goto done;
    struct hub_public_export export={.catalog=catalog,.mount_port=ntohs(address.sin_port),.nfs_port=ntohs(address.sin_port)};
    struct hub_public_library_export library_export={library,export};
    struct measured_dispatch dispatch={library ? hub_public_library_export_dispatch : hub_public_rpc_export_dispatch,
        library ? (void *)&library_export : (void *)&export};
    printf("%u ",ntohs(address.sin_port));
    for (unsigned i=0;i<32;++i) printf("%02x",handle[i]);
    puts(""); fflush(stdout);
    /* Bound abandoned research processes; stdin closes when the owner exits. */
    struct timespec start,now; clock_gettime(CLOCK_MONOTONIC,&start);
    for (;;) {
        clock_gettime(CLOCK_MONOTONIC,&now);
        if (now.tv_sec-start.tv_sec>=120) goto done;
        struct pollfd ready[2]={{fd,POLLIN,0},{STDIN_FILENO,POLLIN,0}};
        int count=poll(ready,2,250);
        if (count<0) { if (errno==EINTR) continue; goto done; }
        if (ready[1].revents&(POLLIN|POLLHUP|POLLERR)) {
            char command; ssize_t bytes=read(STDIN_FILENO,&command,1);
            if (bytes!=1 || command=='Q') { result=0; break; }
            if (command=='E') {
                if (library) hub_public_library_files_cancel(library);
                else hub_public_catalog_withdraw(catalog);
                if (!hub_public_root_drained(&root)) goto done;
                puts("WITHDRAWN"); fflush(stdout);
            }
        }
        if (ready[0].revents&POLLIN) {
            if (hub_public_rpc_udp_dispatch_once(fd,
                    measured,&dispatch,
                                       emulator_link ? emulator_peer_only : loopback_only,NULL)==HUB_UDP_ERROR) goto done;
        }
    }
done:
    if (getenv("AZ_PUBLIC_RPC_STATS")) for (unsigned i=0;i<16 && peer_reads[i].port;++i)
        fprintf(stderr,"RPC_READ_PEER port=%u first=%u last=%u requests=%u bytes=%zu\n",
            peer_reads[i].port,peer_reads[i].first,peer_reads[i].last,peer_reads[i].requests,peer_reads[i].bytes);
    if (fd>=0) close(fd);
    if (library) { hub_public_library_files_destroy(library); free(library); }
    else {
        if (initialized) { hub_public_catalog_withdraw(catalog); hub_public_catalog_destroy(catalog); }
        free(catalog);
    }
    hub_public_root_withdraw(&root); hub_public_root_destroy(&root);
    return result;
}

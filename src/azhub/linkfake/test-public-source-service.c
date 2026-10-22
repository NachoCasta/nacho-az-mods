/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include "public-source-service.h"
static int allowed(void *context,const struct sockaddr_in *peer) {
    return *(int *)context && peer->sin_addr.s_addr==htonl(INADDR_LOOPBACK);
}
static unsigned clients(struct hub_public_source_service *service) {
    unsigned count=0;
    for (unsigned i=0;i<HUB_PUBLIC_SOURCE_CLIENTS;++i) count+=service->used[i]!=0;
    return count;
}
static int connect_client(struct sockaddr_in *address) {
    int fd=socket(AF_INET,SOCK_STREAM|SOCK_CLOEXEC,0);
    assert(fd>=0 && !connect(fd,(struct sockaddr *)address,sizeof *address)); return fd;
}
int main(void) {
    char directory[]="/tmp/azhub-source-service.XXXXXX"; assert(mkdtemp(directory));
    struct hub_public_root roots[2]; struct hub_public_library_files libraries[2]={0};
    struct hub_public_source_service *services=calloc(2,sizeof *services); assert(services);
    int listeners[2],permit=1; struct sockaddr_in addresses[2];
    for (unsigned i=0;i<2;++i) {
        assert(!hub_public_root_init(roots+i) && !hub_public_root_attach(roots+i,directory,i+1));
        unsigned char nonce[16]={0}; nonce[0]=(unsigned char)(i+1);
        assert(!hub_public_catalog_init(&libraries[i].catalog,roots+i,i+1,nonce)); libraries[i].initialized=1;
        assert(!hub_public_track_index_init(&libraries[i].metadata,i+1));
        assert(!hub_entity_index_init(&libraries[i].entities,i+1));
        assert(!hub_public_track_index_seal(&libraries[i].metadata));
        assert(!hub_entity_index_seal(&libraries[i].entities)); libraries[i].state=HUB_LIBRARY_READY;
        listeners[i]=socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0); assert(listeners[i]>=0);
        addresses[i]=(struct sockaddr_in){.sin_family=AF_INET,.sin_addr={.s_addr=htonl(INADDR_LOOPBACK)}};
        assert(!bind(listeners[i],(struct sockaddr *)(addresses+i),sizeof addresses[i]) && !listen(listeners[i],16));
        socklen_t length=sizeof addresses[i]; assert(!getsockname(listeners[i],(struct sockaddr *)(addresses+i),&length));
        assert(!hub_public_source_service_init(services+i,libraries+i,41+i,listeners[i],-1,0,allowed,&permit));
    }
    int peers[9];
    for (unsigned i=0;i<9;++i) {
        peers[i]=connect_client(addresses); assert(!hub_public_source_service_step(services,1));
        assert(clients(services)==(i<8 ? i+1 : 8));
    }
    char byte; assert(recv(peers[8],&byte,1,MSG_DONTWAIT)==0);
    int neighbor=connect_client(addresses+1); assert(!hub_public_source_service_step(services+1,1));
    assert(clients(services+1)==1);
    int rpc[2],udp_peer=socket(AF_INET,SOCK_DGRAM,0); assert(udp_peer>=0);
    struct sockaddr_in rpc_addresses[2];
    for (unsigned i=0;i<2;++i) {
        rpc[i]=socket(AF_INET,SOCK_DGRAM|SOCK_NONBLOCK,0); assert(rpc[i]>=0);
        rpc_addresses[i]=(struct sockaddr_in){.sin_family=AF_INET,.sin_addr={.s_addr=htonl(INADDR_LOOPBACK)}};
        assert(!bind(rpc[i],(struct sockaddr *)(rpc_addresses+i),sizeof rpc_addresses[i]));
        socklen_t size=sizeof rpc_addresses[i]; assert(!getsockname(rpc[i],(struct sockaddr *)(rpc_addresses+i),&size));
    }
    services->udp=rpc[0]; services->rpc_alias=rpc[1]; services->files.rpc.nfs_port=2049;
    unsigned char call[56]={0};
    const uint32_t words[]={17,0,2,100000,2,3,0,0,0,0,100003,2,17,0};
    for (unsigned i=0;i<14;++i) hub_public_rpc_store(call+i*4,words[i]);
    for (unsigned i=0;i<2;++i)
        assert(sendto(udp_peer,call,sizeof call,0,(struct sockaddr *)(rpc_addresses+i),sizeof rpc_addresses[i])==sizeof call);
    assert(!hub_public_source_service_step(services,2));
    for (unsigned i=0;i<2;++i) {
        unsigned char reply[32]; assert(recv(udp_peer,reply,sizeof reply,MSG_DONTWAIT)==28);
        assert(hub_public_rpc_word(reply+24)==2049);
    }
    hub_public_source_service_withdraw(services);
    assert(!clients(services) && clients(services+1)==1 && hub_public_library_ready_catalog(libraries+1));
    for (unsigned i=0;i<8;++i) assert(recv(peers[i],&byte,1,MSG_DONTWAIT)==0);
    int rejected=connect_client(addresses); assert(!hub_public_source_service_step(services,2));
    assert(!clients(services) && recv(rejected,&byte,1,MSG_DONTWAIT)==0); close(rejected);
    permit=0; rejected=connect_client(addresses+1); assert(!hub_public_source_service_step(services+1,2));
    assert(clients(services+1)==1 && recv(rejected,&byte,1,MSG_DONTWAIT)==0); close(rejected);
    for (unsigned i=0;i<9;++i) close(peers[i]); close(neighbor);
    for (unsigned i=0;i<2;++i) {
        hub_public_source_service_destroy(services+i); hub_public_source_service_destroy(services+i);
        assert(fcntl(listeners[i],F_GETFD)>=0); close(listeners[i]);
        hub_public_library_files_destroy(libraries+i); assert(!hub_public_root_destroy(roots+i));
    }
    free(services); assert(!rmdir(directory));
    for (unsigned i=0;i<2;++i) { assert(fcntl(rpc[i],F_GETFD)>=0); close(rpc[i]); }
    close(udp_peer);
    puts("PASS: source service client cap, peer admission, withdrawal, neighbor isolation and borrowed listener lifetime");
    return 0;
}

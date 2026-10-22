/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include "public-source-worker.h"
static int allowed(void *unused,const struct sockaddr_in *peer) {
    (void)unused; return peer->sin_addr.s_addr==htonl(INADDR_LOOPBACK);
}
static int bound_socket(int type,unsigned ip,unsigned port) {
    int fd=socket(AF_INET,type|SOCK_NONBLOCK|SOCK_CLOEXEC,0);assert(fd>=0);
    struct sockaddr_in address={.sin_family=AF_INET,.sin_addr={.s_addr=htonl(ip)},.sin_port=htons(port)};
    assert(!bind(fd,(struct sockaddr *)&address,sizeof address));
    if (type==SOCK_STREAM) assert(!listen(fd,8));return fd;
}
static void await_state(struct hub_public_source_worker *worker,int expected) {
    uint64_t deadline=hub_public_worker_milliseconds()+5000;
    while (hub_public_source_worker_state(worker)!=expected) {
        assert(hub_public_source_worker_state(worker)!=HUB_PUBLIC_WORKER_FAILED);
        assert(hub_public_worker_milliseconds()<deadline); poll(NULL,0,1);
    }
}
static void join_worker(struct hub_public_source_worker *worker) {
    uint64_t deadline=hub_public_worker_milliseconds()+5000; int result;
    while (!(result=hub_public_source_worker_join_ready(worker))) {
        assert(hub_public_worker_milliseconds()<deadline); poll(NULL,0,1);
    }
    assert(result==1 && !worker->started);
}
int main(void) {
    char directory[]="/tmp/azhub-source-worker.XXXXXX"; assert(mkdtemp(directory));
    char pioneer[512],rekordbox[512],pdb[512];
    snprintf(pioneer,sizeof pioneer,"%s/PIONEER",directory); assert(!mkdir(pioneer,0700));
    snprintf(rekordbox,sizeof rekordbox,"%s/PIONEER/rekordbox",directory); assert(!mkdir(rekordbox,0700));
    snprintf(pdb,sizeof pdb,"%s/PIONEER/rekordbox/export.pdb",directory);
    /* Empty track table with one valid page; other entity tables are absent. */
    unsigned char bytes[1024]={0}; bytes[5]=2; bytes[8]=1; bytes[36]=1; bytes[40]=1; bytes[516]=1;
    int fd=open(pdb,O_CREAT|O_EXCL|O_WRONLY,0600); assert(fd>=0);
    assert(write(fd,bytes,sizeof bytes)==sizeof bytes && !close(fd));
    int listener=socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0); assert(listener>=0);
    struct sockaddr_in address={.sin_family=AF_INET,.sin_addr={.s_addr=htonl(INADDR_LOOPBACK)}};
    assert(!bind(listener,(struct sockaddr *)&address,sizeof address) && !listen(listener,8));
    struct hub_public_source_worker worker={0}; unsigned char nonce[16]={1};
    for (unsigned cycle=0;cycle<8;++cycle) {
        nonce[0]=(unsigned char)(cycle+1);
        assert(!hub_public_source_worker_start(&worker,directory,cycle+1,nonce,41,listener,-1,0,allowed,NULL));
        if (cycle%2) {
            await_state(&worker,HUB_PUBLIC_WORKER_RUNNING);
            assert(!hub_public_source_worker_join_ready(&worker));
            hub_public_source_worker_withdraw(&worker);
            await_state(&worker,HUB_PUBLIC_WORKER_WITHDRAWN);
        }
        hub_public_source_worker_stop(&worker); join_worker(&worker);
        assert(!worker.error && fcntl(listener,F_GETFD)>=0);
    }
    /* A verified FD must survive replacement of the configured pathname. */
    int pinned=open(directory,O_RDONLY|O_DIRECTORY|O_CLOEXEC);assert(pinned>=0);
    char moved[256];snprintf(moved,sizeof moved,"%s-moved",directory);
    assert(!rename(directory,moved) && !mkdir(directory,0700));
    assert(!hub_public_source_worker_start_configured_fd(&worker,pinned,50,nonce,41,listener,-1,0,allowed,NULL,-1,0,NULL));
    close(pinned);
    await_state(&worker,HUB_PUBLIC_WORKER_RUNNING);
    hub_public_source_worker_stop(&worker);join_worker(&worker);assert(!worker.error);
    assert(!rmdir(directory) && !rename(moved,directory));
    assert(hub_public_source_worker_start_configured_fd(&worker,-1,51,nonce,41,listener,-1,0,allowed,NULL,-1,0,NULL)<0);
    /* Real discovery collision crosses the worker boundary as a hard identity
     * error, not an automatically retried address-conflict grant. */
    struct hub_public_provider_endpoint provider={.discovery_fd=bound_socket(SOCK_DGRAM,0x7f000002,50000),
        .status_fd=bound_socket(SOCK_DGRAM,0x7f000002,50002),.broadcast=htonl(0x7f000001)};
    unsigned char ip[4]={127,0,0,2},mac[6]={2,0,0,0,0,41};
    assert(!hub_public_provider_identity_init(&provider.identity,41,ip,mac,"Hub USB 1"));
    int rpc=bound_socket(SOCK_DGRAM,0x7f000002,0),query=bound_socket(SOCK_STREAM,0x7f000002,0);
    int peer=bound_socket(SOCK_DGRAM,0x7f000001,50000);
    assert(!hub_public_source_worker_start_configured(&worker,directory,52,nonce,41,listener,rpc,111,allowed,NULL,query,12345,&provider));
    await_state(&worker,HUB_PUBLIC_WORKER_RUNNING);
    struct hub_public_provider_identity competitor=provider.identity;competitor.mac[5]=42;
    unsigned char claim[54];size_t length=hub_public_provider_discovery(&competitor,2,6,claim);
    struct sockaddr_in destination={.sin_family=AF_INET,.sin_addr={.s_addr=htonl(0x7f000002)},.sin_port=htons(50000)};
    assert(sendto(peer,claim,length,0,(struct sockaddr *)&destination,sizeof destination)==(ssize_t)length);
    join_worker(&worker);assert(worker.error==EEXIST && hub_public_source_worker_state(&worker)==HUB_PUBLIC_WORKER_FAILED);
    close(peer);close(query);close(rpc);close(provider.discovery_fd);close(provider.status_fd);
    assert(!unlink(pdb));
    assert(!hub_public_source_worker_start(&worker,directory,99,nonce,41,listener,-1,0,allowed,NULL));
    join_worker(&worker); assert(worker.error==ENOENT && hub_public_source_worker_state(&worker)==HUB_PUBLIC_WORKER_FAILED);
    close(listener); assert(!rmdir(rekordbox) && !rmdir(pioneer) && !rmdir(directory));
    puts("PASS: source worker staging, withdrawal, stop/join, reconnect, hard player-collision retirement and missing-PDB cleanup");
    return 0;
}

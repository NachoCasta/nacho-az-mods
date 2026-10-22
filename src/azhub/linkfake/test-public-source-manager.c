/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include "public-source-manager.h"
struct fixture {
    struct hub_public_source_manager *manager;
    unsigned opened[8],closed[8];
    struct sockaddr_in addresses[8];
    int fail_open;
};
static int allowed(void *unused,const struct sockaddr_in *peer) {
    (void)unused; return peer->sin_addr.s_addr==htonl(INADDR_LOOPBACK);
}
static int open_endpoint(void *opaque,unsigned slot,struct hub_public_source_binding *binding) {
    struct fixture *fixture=opaque;
    assert(!fixture->manager->sources[slot].bound && !fixture->manager->sources[slot].worker.started);
    ++fixture->opened[slot];
    if (!slot && fixture->fail_open) { errno=EADDRINUSE; return -1; }
    binding->listener=socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0); assert(binding->listener>=0);
    struct sockaddr_in address={.sin_family=AF_INET,.sin_addr={.s_addr=htonl(INADDR_LOOPBACK)}};
    assert(!bind(binding->listener,(struct sockaddr *)&address,sizeof address) && !listen(binding->listener,8));
    socklen_t length=sizeof address; assert(!getsockname(binding->listener,(struct sockaddr *)&address,&length));
    fixture->addresses[slot]=address; binding->player=41+slot; binding->allowed=allowed;
    return 0;
}
static void close_endpoint(void *opaque,unsigned slot,struct hub_public_source_binding *binding) {
    struct fixture *fixture=opaque;
    assert(!fixture->manager->sources[slot].worker.started);
    assert(!close(binding->listener)); binding->listener=-1; ++fixture->closed[slot];
}
static void wait_running(struct hub_public_source_manager *manager,unsigned slot) {
    uint64_t deadline=hub_public_worker_milliseconds()+5000;
    while (!manager->sources[slot].worker.started ||
            hub_public_source_worker_state(&manager->sources[slot].worker)!=HUB_PUBLIC_WORKER_RUNNING ||
            manager->sources[slot].attempted!=manager->sources[slot].requested) {
        assert(!hub_public_source_manager_poll(manager,slot));
        assert(!manager->sources[slot].error && hub_public_worker_milliseconds()<deadline); poll(NULL,0,1);
    }
}
static void wait_drained(struct hub_public_source_manager *manager,unsigned slot) {
    uint64_t deadline=hub_public_worker_milliseconds()+5000;
    while (!hub_public_source_manager_drained(manager,slot)) {
        assert(!hub_public_source_manager_poll(manager,slot));
        assert(hub_public_worker_milliseconds()<deadline); poll(NULL,0,1);
    }
}
int main(void) {
    char directory[]="/tmp/azhub-source-manager.XXXXXX"; assert(mkdtemp(directory));
    char pioneer[512],rekordbox[512],pdb[512];
    snprintf(pioneer,sizeof pioneer,"%s/PIONEER",directory); assert(!mkdir(pioneer,0700));
    snprintf(rekordbox,sizeof rekordbox,"%s/PIONEER/rekordbox",directory); assert(!mkdir(rekordbox,0700));
    snprintf(pdb,sizeof pdb,"%s/PIONEER/rekordbox/export.pdb",directory);
    unsigned char bytes[1024]={0}; bytes[5]=2; bytes[8]=1; bytes[36]=1; bytes[40]=1; bytes[516]=1;
    int fd=open(pdb,O_CREAT|O_EXCL|O_WRONLY,0600); assert(fd>=0 && write(fd,bytes,sizeof bytes)==sizeof bytes && !close(fd));
    struct hub_public_source_manager *manager=calloc(1,sizeof *manager); assert(manager);
    struct fixture fixture={.manager=manager};
    assert(!hub_public_source_manager_init(manager,(struct hub_public_source_manager_ops){open_endpoint,close_endpoint},&fixture));
    /* A queued source must not become visible after eject beats its first poll. */
    assert(!hub_public_source_manager_request(manager,2,directory,1,1));
    assert(!hub_public_source_manager_withdraw(manager,2));
    for (unsigned i=0;i<20;++i) assert(!hub_public_source_manager_poll(manager,2));
    assert(!fixture.opened[2] && hub_public_source_manager_drained(manager,2));
    assert(!hub_public_source_manager_request(manager,2,directory,1,1));
    assert(!hub_public_source_manager_poll(manager,2) && !fixture.opened[2]);
    assert(!hub_public_source_manager_request(manager,2,directory,2,1)); wait_running(manager,2);
    assert(fixture.opened[2]==1);
    assert(!hub_public_source_manager_request(manager,2,NULL,0,0)); wait_drained(manager,2);
    for (unsigned slot=0;slot<2;++slot) {
        assert(!hub_public_source_manager_request(manager,slot,directory,1,1)); wait_running(manager,slot);
    }
    uint64_t generation=manager->sources[0].worker.generation;
    unsigned char nonce[16]; memcpy(nonce,manager->sources[0].worker.nonce,16);
    assert(!hub_public_source_manager_request(manager,0,directory,1,1));
    assert(!hub_public_source_manager_poll(manager,0) && fixture.opened[0]==1);
    assert(!hub_public_source_manager_request(manager,0,directory,2,1)); wait_running(manager,0);
    assert(fixture.opened[0]==2 && fixture.closed[0]==1 && manager->sources[0].worker.generation>generation);
    assert(memcmp(nonce,manager->sources[0].worker.nonce,16));
    assert(hub_public_source_worker_state(&manager->sources[1].worker)==HUB_PUBLIC_WORKER_RUNNING && fixture.closed[1]==0);
    assert(!hub_public_source_manager_withdraw(manager,0));
    uint64_t deadline=hub_public_worker_milliseconds()+5000;
    while (hub_public_source_worker_state(&manager->sources[0].worker)!=HUB_PUBLIC_WORKER_WITHDRAWN) {
        assert(!hub_public_source_manager_poll(manager,0));
        assert(hub_public_worker_milliseconds()<deadline); poll(NULL,0,1);
    }
    assert(!hub_public_source_manager_drained(manager,0) && manager->sources[0].bound);
    assert(!hub_public_source_manager_request(manager,0,NULL,0,0)); wait_drained(manager,0);
    fixture.fail_open=1;
    assert(!hub_public_source_manager_request(manager,0,directory,3,1)); assert(!hub_public_source_manager_poll(manager,0));
    assert(manager->sources[0].error==EADDRINUSE && hub_public_source_manager_drained(manager,0));
    for (unsigned i=0;i<20;++i) assert(!hub_public_source_manager_poll(manager,0));
    assert(fixture.opened[0]==3); fixture.fail_open=0;
    assert(!hub_public_source_manager_request(manager,0,directory,4,1)); wait_running(manager,0);
    for (unsigned slot=0;slot<2;++slot) { assert(!hub_public_source_manager_request(manager,slot,NULL,0,0)); wait_drained(manager,slot); }
    assert(fixture.closed[0]==3 && fixture.closed[1]==1);
    int pinned=open(directory,O_RDONLY|O_DIRECTORY|O_CLOEXEC);assert(pinned>=0);
    assert(!hub_public_source_manager_request_fd(manager,3,pinned,10));
    assert(!hub_public_source_manager_request_fd(manager,3,pinned,10));
    uint64_t unchanged=manager->sources[3].requested;
    assert(!hub_public_source_manager_withdraw(manager,3));
    assert(manager->sources[3].directory_fd==-1);
    assert(!hub_public_source_manager_request_fd(manager,3,pinned,10));
    assert(manager->sources[3].requested==unchanged);
    assert(!hub_public_source_manager_poll(manager,3) && !fixture.opened[3]);
    assert(!hub_public_source_manager_request_fd(manager,3,pinned,11));close(pinned);
    char moved[256];snprintf(moved,sizeof moved,"%s-moved",directory);
    assert(!rename(directory,moved) && !mkdir(directory,0700));
    int replacement=open(directory,O_RDONLY|O_DIRECTORY|O_CLOEXEC);assert(replacement>=0);
    assert(hub_public_source_manager_request_fd(manager,3,replacement,11)<0 && errno==ESTALE);
    close(replacement);wait_running(manager,3);
    assert(!hub_public_source_manager_request(manager,3,NULL,0,0));wait_drained(manager,3);
    assert(manager->sources[3].directory_fd==-1 && fixture.opened[3]==1 && fixture.closed[3]==1);
    assert(!rmdir(directory) && !rename(moved,directory));
    free(manager); assert(!unlink(pdb) && !rmdir(rekordbox) && !rmdir(pioneer) && !rmdir(directory));
    puts("PASS: manager join-before-endpoint-close/reopen, fresh generation/nonce, unchanged observation, neighbor isolation and bounded failure retry");
    return 0;
}

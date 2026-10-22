/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <dirent.h>
#include "public-broker-allocator.h"
static uint64_t clock_now(void *context) { return context ? *(uint64_t *)context:hub_public_address_remove_ms(); }
static unsigned jitter(void *unused) { (void)unused;return 500; }
static int authorize(void *unused,unsigned slot,const struct hub_public_broker_source *source,int root) {
    (void)unused;assert(slot<8 && source->identity && fcntl(root,F_GETFD)>=0);return 0;
}
static int allowed(void *unused,const struct sockaddr_in *peer) { (void)unused;(void)peer;return 0; }
static unsigned descriptors(void) {
    DIR *directory=opendir("/proc/self/fd");assert(directory);unsigned count=0;struct dirent *entry;
    while ((entry=readdir(directory))) if (strcmp(entry->d_name,".") && strcmp(entry->d_name,"..")) ++count;
    closedir(directory);return count;
}
int main(void) {
    unsigned baseline=descriptors();char path[]="/tmp/azhub-allocator-XXXXXX";assert(mkdtemp(path));
    int root=open(path,O_RDONLY|O_DIRECTORY|O_CLOEXEC);assert(root>=0);struct stat stat;assert(!fstat(root,&stat));
    struct hub_public_source_manager *manager=calloc(1,sizeof *manager);assert(manager);
    struct hub_public_broker_allocator allocator={.manager=manager,.parent="eth0",.parent_mac={0x52,0x54,0,0x12,0x34,0x56},
        .helper="/not/launched",.hook="/not/launched",.now=clock_now,.jitter=jitter,.authorize=authorize,.allowed=allowed};
    assert(!hub_public_source_manager_init(manager,(struct hub_public_source_manager_ops){hub_public_broker_allocator_open,hub_public_broker_allocator_close},&allocator));
    struct hub_public_broker_dispatch dispatch;assert(!hub_public_broker_dispatch_init(&dispatch,manager,
        (struct hub_public_broker_dispatch_ops){.release=hub_public_broker_allocator_release,.allocate=hub_public_broker_allocator_allocate,.ready=hub_public_broker_allocator_ready},&allocator));
    struct hub_public_broker_source source={.identity=1,.device=stat.st_dev,.inode=stat.st_ino,.player=41,
        .mac={2,0,0,0,0x12,3},.interface="azpub0",.label="Hub USB 1"};strcpy(source.directory,path);
    struct hub_public_broker_frame frame={.op=HUB_BROKER_ALLOCATE,.generation=1,.count=1,.token={1},.descriptors={root,-1}};
    for (unsigned i=0;i<8;++i) {
        frame.slot=i;source.mac[5]=3+i;source.player=41+i;
        assert(!hub_public_broker_source_encode(&source,frame.payload,sizeof frame.payload,&frame.length));
        assert(!hub_public_broker_dispatch_command(&dispatch,&frame));
        if (!i) assert(hub_public_broker_allocator_allocate(&allocator,7,&source,root)<0 && errno==EEXIST);
    }
    source.player=41;
    struct hub_public_broker_frame stop={.op=HUB_BROKER_STOP,.token={1}};
    assert(!hub_public_broker_dispatch_command(&dispatch,&stop));uint64_t deadline=clock_now(NULL)+5000;
    for (unsigned i=0;i<8;++i) while (dispatch.generations.slots[i].phase!=HUB_BROKER_EMPTY) {
        assert(hub_public_broker_dispatch_poll(&dispatch,i)>=0);assert(clock_now(NULL)<deadline);poll(NULL,0,1);
    }
    for (unsigned i=0;i<8;++i) {
        assert(!allocator.sources[i].phase && !allocator.sources[i].acquire.namespace.started && !manager->sources[i].worker.started);
        assert(dispatch.pending_root[i]==-1 && !manager->sources[i].bound);
    }
    /* Backoff survives source disposal and a new publication generation. */
    memset(allocator.retry,0,sizeof allocator.retry);uint64_t now=10999;allocator.context=&now;
    assert(!hub_public_acd_retry_declined(&allocator.retry[0],1000));
    assert(!hub_public_broker_dispatch_init(&dispatch,manager,(struct hub_public_broker_dispatch_ops){.release=hub_public_broker_allocator_release,
        .allocate=hub_public_broker_allocator_allocate,.ready=hub_public_broker_allocator_ready},&allocator));
    frame.slot=0;assert(!hub_public_broker_dispatch_command(&dispatch,&frame));
    assert(allocator.sources[0].phase==3 && !allocator.sources[0].acquire.phase && allocator.sources[0].acquire.namespace_fd==-1);
    assert(!hub_public_broker_dispatch_poll(&dispatch,0) && !manager->sources[0].worker.started);
    /* Unplug/eject during the wait must release only the pinned root, without
     * launching DHCP or closing an unrelated descriptor (including stdin). */
    int witness=dup(root);assert(witness>=0);
    assert(!hub_public_broker_dispatch_command(&dispatch,&stop));
    while (dispatch.generations.slots[0].phase!=HUB_BROKER_EMPTY)
        assert(hub_public_broker_dispatch_poll(&dispatch,0)>=0);
    assert(fcntl(witness,F_GETFD)>=0 && !allocator.sources[0].phase);
    assert(allocator.retry[0].conflicts==1 && allocator.retry[0].not_before==11000);
    close(witness);
    assert(!hub_public_broker_dispatch_init(&dispatch,manager,(struct hub_public_broker_dispatch_ops){.release=hub_public_broker_allocator_release,
        .allocate=hub_public_broker_allocator_allocate,.ready=hub_public_broker_allocator_ready},&allocator));
    frame.generation=2;assert(!hub_public_broker_dispatch_command(&dispatch,&frame));
    assert(allocator.sources[0].phase==3 && !allocator.sources[0].acquire.phase);
    now=11000;assert(!hub_public_broker_dispatch_poll(&dispatch,0));
    assert(allocator.sources[0].phase==1 && allocator.sources[0].acquire.namespace.started);
    assert(!hub_public_broker_dispatch_command(&dispatch,&stop));deadline=clock_now(NULL)+5000;
    while (dispatch.generations.slots[0].phase!=HUB_BROKER_EMPTY) {
        assert(hub_public_broker_dispatch_poll(&dispatch,0)>=0 && clock_now(NULL)<deadline);poll(NULL,0,1);
    }
    assert(allocator.retry[0].conflicts==1 && allocator.retry[0].not_before==11000);
    /* A link/acquisition collision must not impersonate observed ARP conflict. */
    allocator.sources[0].phase=1;
    allocator.sources[0].acquire.phase=2;allocator.sources[0].acquire.complete=1;
    allocator.sources[0].acquire.error=EADDRINUSE;
    assert(hub_public_broker_allocator_ready(&allocator,0)<0 && errno==EEXIST);
    assert(allocator.last_acquire_phase[0]==2 && allocator.last_acquire_error[0]==EADDRINUSE);
    memset(&allocator.sources[0],0,sizeof allocator.sources[0]);
    /* Link retirement can take multiple polls after address removal. Do not
     * finalize an already-IDLE lease again when a later operation is pending. */
    struct hub_public_broker_allocated_source *retiring=allocator.sources;
    retiring->phase=2;retiring->acquire.complete=1;retiring->acquire.phase=3;
    int namespace_fd=open("/proc/self/ns/net",O_RDONLY|O_CLOEXEC);assert(namespace_fd>=0);
    retiring->spec.namespace_fd=retiring->acquire.namespace_fd=namespace_fd;
    retiring->lease.state=HUB_LEASE_REMOVING;retiring->lease.generation=1;retiring->lease.configured=1;
    retiring->ready.applied=1;retiring->removal.state=2;retiring->removal.declared=retiring->spec;
    struct stat ns;assert(!fstat(namespace_fd,&ns));retiring->removal.device=ns.st_dev;retiring->removal.inode=ns.st_ino;
    retiring->link_removal.done=1;retiring->link_removal.error=EBUSY;
    assert(hub_public_broker_allocator_release(&allocator,0)<0 && errno==EBUSY);
    assert(retiring->lease.state==HUB_LEASE_IDLE && !retiring->ready.applied);
    assert(hub_public_broker_allocator_release(&allocator,0)<0 && errno==EBUSY);
    assert(fcntl(namespace_fd,F_GETFD)>=0);close(namespace_fd);
    memset(retiring,0,sizeof *retiring);
    struct hub_public_address_inventory native={0};
    struct in_addr candidate={inet_addr("192.0.2.3")},different={inet_addr("192.0.2.4")};
    allocator.sources[4].phase=2;allocator.sources[4].spec.address=candidate;
    assert(hub_public_broker_allocator_candidate(&allocator,0,candidate,&native)<0 && errno==EADDRNOTAVAIL);
    assert(!hub_public_broker_allocator_candidate(&allocator,0,different,&native));
    assert(allocator.sources[4].phase==2 && allocator.sources[4].spec.address.s_addr==candidate.s_addr);
    allocator.sources[4].phase=1;allocator.sources[4].acquire.complete=1;allocator.sources[4].acquire.dhcp.bound=1;
    allocator.sources[4].acquire.dhcp.proposal.address=candidate;
    assert(!hub_public_broker_allocator_candidate(&allocator,0,candidate,&native));
    assert(hub_public_broker_allocator_candidate(&allocator,5,candidate,&native)<0 && errno==EADDRNOTAVAIL);
    allocator.sources[5].phase=2;allocator.sources[5].spec.address=candidate;
    assert(!hub_public_broker_allocator_candidate(&allocator,5,candidate,&native));
    allocator.sources[5].phase=0;
    allocator.sources[4].acquire.error=EIO;
    assert(!hub_public_broker_allocator_candidate(&allocator,5,candidate,&native));
    assert(!hub_public_address_inventory_add(&native,different));
    assert(hub_public_broker_allocator_candidate(&allocator,0,different,&native)<0 && errno==EADDRNOTAVAIL);
    int entropy[2];assert(!pipe(entropy));uint16_t choices[]={0,1,2};
    assert(write(entropy[1],choices,sizeof choices)==sizeof choices);close(entropy[1]);
    allocator.allow_link_local=1;allocator.entropy_fd=entropy[0];
    allocator.sources[6].acquire.dhcp=(struct hub_public_dhcp_acquire){.fd=-1,.complete=1,.reaped=1,.eof=1,
        .error=ENODATA,.status=256,.deconfigured=1,.lease_failed=1,.records=2};
    assert(!hub_public_address_inventory_add(&native,(struct in_addr){inet_addr("169.254.1.0")}));
    allocator.sources[4].phase=2;allocator.sources[4].spec.address.s_addr=inet_addr("169.254.1.1");
    assert(!hub_public_broker_allocator_link_local(&allocator,6,&native,20000));
    assert(allocator.sources[6].lease.origin==HUB_ADDRESS_LINK_LOCAL && !allocator.sources[6].lease.configured);
    assert(allocator.sources[6].lease.current.address.s_addr==inet_addr("169.254.1.2"));
    allocator.sources[6].acquire.dhcp.bound=1;
    assert(hub_public_broker_allocator_link_local(&allocator,6,&native,20001)<0 && errno==EPROTO);
    close(entropy[0]);
    close(root);free(manager);assert(!rmdir(path) && descriptors()==baseline);
    puts("PASS: real allocator callbacks cancel eight pending root capabilities, join namespace creators and release all descriptors before STOP completion");
}

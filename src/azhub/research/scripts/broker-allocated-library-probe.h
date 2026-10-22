/* SPDX-License-Identifier: GPL-3.0-only */
/* Explicit single-source emulator trial through the real ALLOCATE callbacks.
 * Runner markers are research coordination, not production broker activation. */
#ifndef HUB_BROKER_ALLOCATED_LIBRARY_PROBE_H
#define HUB_BROKER_ALLOCATED_LIBRARY_PROBE_H
#include "../../linkfake/public-broker-allocator.h"
static uint64_t broker_library_now(void *unused) { (void)unused;return hub_public_address_remove_ms(); }
static unsigned broker_library_jitter(void *unused) { (void)unused;return 500; }
static int broker_library_authorize(void *unused,unsigned slot,const struct hub_public_broker_source *source,int root) {
    (void)unused;(void)root;
    if (slot || strcmp(source->directory,"/media/usb/sdc1") || source->player!=41 || strcmp(source->interface,"azpub0")) { errno=EPERM;return -1; }
    return 0;
}
static int broker_library_peer(void *unused,const struct sockaddr_in *peer) { (void)unused;return peer->sin_addr.s_addr==inet_addr("192.0.2.2"); }
static int broker_allocated_library_probe(const char *hook,const char *helper,
        int (*reachability)(int,const struct hub_public_dhcp_event *)) {
    const char *control=getenv("AZHUB_ADDRESS_LIBRARY_CONTROL");
    if (!control || strncmp(control,"/tmp/azhub-dhcp-",16) || strlen(control)>180 || strlen(helper)>=1024 || strlen(hook)>=1024) return -1;
    for (const char *p=control+16;*p;++p) if (!((*p>='a' && *p<='z') || (*p>='0' && *p<='9') || *p=='-')) return -1;
    char marker[256],done[256];snprintf(marker,sizeof marker,"%s.ready",control);snprintf(done,sizeof done,"%s.done",control);
    if (!access(marker,F_OK) || !access(done,F_OK)) return -1;
    struct hub_public_source_manager *manager=calloc(1,sizeof *manager);
    struct hub_public_broker_allocator *allocator=calloc(1,sizeof *allocator);
    if (!manager || !allocator) { free(manager);free(allocator);return -1; }
    allocator->manager=manager;strcpy(allocator->parent,"eth0");strcpy(allocator->helper,helper);strcpy(allocator->hook,hook);
    const unsigned char parent[6]={0x52,0x54,0,0x12,0x34,0x56};memcpy(allocator->parent_mac,parent,6);
    allocator->now=broker_library_now;allocator->jitter=broker_library_jitter;allocator->authorize=broker_library_authorize;allocator->allowed=broker_library_peer;
    assert(!hub_public_source_manager_init(manager,(struct hub_public_source_manager_ops){hub_public_broker_allocator_open,hub_public_broker_allocator_close},allocator));
    struct hub_public_broker_dispatch dispatch;
    assert(!hub_public_broker_dispatch_init(&dispatch,manager,(struct hub_public_broker_dispatch_ops){.allocate=hub_public_broker_allocator_allocate,
        .ready=hub_public_broker_allocator_ready,.release=hub_public_broker_allocator_release},allocator));
    int root=open("/media/usb/sdc1",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);assert(root>=0);struct stat stat;assert(!fstat(root,&stat));
    struct hub_public_broker_source source={.identity=1,.device=stat.st_dev,.inode=stat.st_ino,.directory="/media/usb/sdc1",
        .interface="azpub0",.mac={2,0,0,0,0x12,3},.player=41,.label="Hub USB 1"};
    struct hub_public_broker_frame command={.op=HUB_BROKER_ALLOCATE,.generation=1,.count=1,.token={1},.descriptors={root,-1}};
    assert(!hub_public_broker_source_encode(&source,command.payload,sizeof command.payload,&command.length));
    assert(!hub_public_broker_dispatch_command(&dispatch,&command));close(root);
    uint64_t deadline=broker_library_now(NULL)+180000;int published=0,result=-1;
    while (broker_library_now(NULL)<deadline) {
        int step=hub_public_broker_dispatch_poll(&dispatch,0);
        if (step<0 || dispatch.error[0] || dispatch.generations.slots[0].phase==HUB_BROKER_EMPTY) break;
        if (!published && dispatch.generations.slots[0].phase==HUB_BROKER_ACTIVE && allocator->sources[0].ready.arp.state==HUB_ACD_ACTIVE) {
            assert(!reachability(allocator->sources[0].spec.namespace_fd,&allocator->sources[0].lease.current));
            char message[256];int length=snprintf(message,sizeof message,"{\"ready\":true,\"address\":\"192.0.2.3\",\"player\":41,\"metadata_port\":%u}\n",manager->sources[0].binding.metadata_port);
            int fd=open(marker,O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);if (fd<0) break;
            int okay=write(fd,message,length)==length;close(fd);if (!okay) break;published=1;
        }
        if (published && !access(done,F_OK)) { result=0;break; }
        poll(NULL,0,1);
    }
    struct hub_public_broker_frame stop={.op=HUB_BROKER_STOP,.token={1}};
    if (hub_public_broker_dispatch_command(&dispatch,&stop)<0) _exit(75);
    deadline=broker_library_now(NULL)+15000;
    while (dispatch.generations.slots[0].phase!=HUB_BROKER_EMPTY) {
        if (hub_public_broker_dispatch_poll(&dispatch,0)<0 || broker_library_now(NULL)>=deadline) _exit(76);
        poll(NULL,0,1);
    }
    if (dispatch.error[0]) result=-1;
    assert(!allocator->sources[0].phase && !manager->sources[0].worker.started && !manager->sources[0].bound);
    free(manager);free(allocator);
    if (!result) puts("{\"allocated_namespace_interface_native_DHCP\":true,\"address\":\"192.0.2.3\",\"not_public_ready\":false,\"configured_and_retired\":true,\"allocated_library_served\":true,\"broker_allocate_callbacks\":true}");
    return result;
}
#endif

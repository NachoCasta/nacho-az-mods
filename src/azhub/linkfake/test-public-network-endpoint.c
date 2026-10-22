/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <dirent.h>
#include <sys/utsname.h>
#include "public-network-endpoint.h"
static int allowed(void *unused,const struct sockaddr_in *peer) {
    (void)unused; return peer->sin_addr.s_addr==htonl(0xc0000202);
}
static unsigned descriptors(void) {
    DIR *directory=opendir("/proc/self/fd"); assert(directory);
    unsigned count=0; struct dirent *entry;
    while ((entry=readdir(directory))) if (strcmp(entry->d_name,".") && strcmp(entry->d_name,"..")) ++count;
    closedir(directory); return count;
}
int main(int argc,char **argv) {
    /* Explicit local-emulator fixture only; not a real-AZ network writer. */
    assert(argc==2 && !strcmp(argv[1],"/var/run/netns/azhub-pubtest"));
    struct utsname system; assert(!uname(&system) && !strcmp(system.release,"6.6.138-dirty"));
    struct hub_public_network_spec spec={.namespace_fd=open(argv[1],O_RDONLY|O_CLOEXEC),
        .address={.s_addr=htonl(0xc0000203)},.mask={.s_addr=htonl(0xffffff00)},
        .broadcast={.s_addr=htonl(0xc00002ff)},.player=41,.allowed=allowed,
        .interface="azpub0",.label="Hub endpoint test",.mac={2,0,0,0,18,3}};
    assert(spec.namespace_fd>=0);
    struct stat initial,target,after; assert(!stat("/proc/self/ns/net",&initial) && !fstat(spec.namespace_fd,&target));
    assert(initial.st_ino!=target.st_ino || initial.st_dev!=target.st_dev);
    unsigned before=descriptors();
    struct hub_public_source_binding binding,failed;
    assert(!hub_public_network_open(&spec,&binding)); assert(descriptors()==before+6);
    assert(!stat("/proc/self/ns/net",&after) && initial.st_ino==after.st_ino && initial.st_dev==after.st_dev);
    assert(binding.file_port==111 && binding.metadata_port && binding.provider.rpc_alias_enabled);
    assert(hub_public_network_open(&spec,&failed)<0 && errno==EADDRINUSE && descriptors()==before+6);
    hub_public_network_close(&binding); assert(descriptors()==before);
    spec.mac[5]=4; assert(hub_public_network_open(&spec,&failed)<0 && errno==EINVAL && descriptors()==before);
    spec.mac[5]=3; spec.broadcast.s_addr=htonl(0xc00002fe);
    assert(hub_public_network_open(&spec,&failed)<0 && errno==EINVAL && descriptors()==before);
    spec.broadcast.s_addr=htonl(0xc00002ff);
    assert(!hub_public_network_open(&spec,&binding)); hub_public_network_close(&binding);
    assert(descriptors()==before && !stat("/proc/self/ns/net",&after) && initial.st_ino==after.st_ino);
    close(spec.namespace_fd);
    puts("PASS: cross-namespace endpoint creation, controller namespace preservation, partial failure cleanup, validation and fresh reopen");
    return 0;
}

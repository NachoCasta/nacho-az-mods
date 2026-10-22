/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include "public-network-namespace.h"
#include "public-broker-namespace.h"
int main(int argc,char **argv) {
    int emulator=argc==2 && !strcmp(argv[1],"--emulator");
    if (emulator) { struct utsname kernel;assert(!uname(&kernel) && !strcmp(kernel.release,"6.6.138-dirty")); }
    else assert(argc==1);
    struct stat before,after;assert(!stat("/proc/self/ns/net",&before));
    for (unsigned i=0;i<8;++i) {
        struct hub_public_namespace_create op={0};assert(!hub_public_namespace_create_start(&op));
        assert(hub_public_namespace_create_start(&op)<0 && errno==EBUSY);
        int result;while ((result=hub_public_namespace_create_poll(&op))==1) poll(NULL,0,1);
        if (result<0) {
            assert(!emulator && (errno==EPERM || errno==EACCES));
            assert(!op.started && op.fd==-1);
        } else {
            assert(hub_public_namespace_create_start(&op)<0 && errno==EBUSY);
            int fd=hub_public_namespace_create_take(&op);assert(fd>=0);
            assert(fcntl(fd,F_GETFD)&FD_CLOEXEC);struct stat created;assert(!fstat(fd,&created));
            assert(before.st_dev!=created.st_dev || before.st_ino!=created.st_ino);
            assert(!hub_public_broker_namespace_validate(fd));close(fd);
            assert(hub_public_namespace_create_take(&op)<0);
        }
        assert(!stat("/proc/self/ns/net",&after) && before.st_dev==after.st_dev && before.st_ino==after.st_ino);
    }
    puts("PASS: namespace creator joins, owned FD survives thread, controller namespace unchanged; privilege failures clean");
}

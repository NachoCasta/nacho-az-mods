/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <poll.h>
#include "public-address-arp-namespace.h"
int main(void) {
    struct hub_public_arp_namespace op={0};const unsigned char mac[6]={2,0,0,0,0x12,3};
    int fd=open("/proc/self/ns/net",O_RDONLY|O_CLOEXEC);assert(fd>=0);
    assert(hub_public_arp_namespace_start(&op,fd,"azpub0",mac)<0 && errno==EPERM);close(fd);
    fd=open("/proc/self/ns/mnt",O_RDONLY|O_CLOEXEC);assert(fd>=0);
    assert(!hub_public_arp_namespace_start(&op,fd,"azpub0",mac));close(fd);
    assert(hub_public_arp_namespace_start(&op,0,"azpub0",mac)<0 && errno==EBUSY);
    int result;while ((result=hub_public_arp_namespace_poll(&op))==1) poll(NULL,0,1);
    assert(result<0 && !op.started && op.namespace_fd==-1 && op.transport.fd==-1);
    struct hub_public_arp transport={.fd=-1};assert(hub_public_arp_namespace_take(&op,&transport)<0);
    puts("PASS: ARP namespace factory refuses native namespace and cleans wrong-kind asynchronous failure");
}

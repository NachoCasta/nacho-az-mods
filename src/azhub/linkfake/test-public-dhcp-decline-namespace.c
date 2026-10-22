/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include "public-dhcp-decline-namespace.h"
int main(void) {
    struct hub_public_decline_namespace op={0};struct hub_public_arp transport={.fd=open("/dev/null",O_RDONLY|O_CLOEXEC),.index=1,.mac={2,0,0,0,0x12,3}};
    assert(transport.fd>=0);struct hub_public_acd conflict={.state=HUB_ACD_CONFLICT,.mac={2,0,0,0,0x12,3},.address={192,0,2,3}};
    struct hub_public_dhcp_event grant={.kind=HUB_DHCP_BOUND};inet_pton(AF_INET,"192.0.2.3",&grant.address);inet_pton(AF_INET,"192.0.2.2",&grant.server);
    int ns=open("/proc/self/ns/net",O_RDONLY|O_CLOEXEC);assert(ns>=0);
    assert(hub_public_decline_namespace_start(&op,ns,&transport,&conflict,&grant)<0 && errno==EPERM);close(ns);
    ns=open("/proc/self/ns/mnt",O_RDONLY|O_CLOEXEC);assert(ns>=0);
    assert(!hub_public_decline_namespace_start(&op,ns,&transport,&conflict,&grant));
    int result;while ((result=hub_public_decline_namespace_poll(&op))==1) poll(NULL,0,1);
    assert(result<0 && !op.started && op.namespace_fd==-1 && op.transport.fd==-1);
    assert(fcntl(ns,F_GETFD)>=0 && fcntl(transport.fd,F_GETFD)>=0);close(ns);close(transport.fd);
    puts("PASS: namespace DECLINE refuses native namespace and joins wrong-kind failure without releasing caller descriptors");
}

/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include "public-network-ready.h"
int main(void) {
    const char *line="1\tbound\tazpub0\t192.0.2.3\t255.255.255.0\t60\t192.0.2.2\n";
    struct hub_public_dhcp_event offer;assert(!hub_public_dhcp_event_parse(line,strlen(line),"azpub0",&offer));
    for (unsigned trial=0;trial<2;++trial) {
        struct hub_public_network_lease lease={0};assert(!hub_public_lease_offer(&lease,&offer,100,200));
        struct hub_public_network_spec spec={.namespace_fd=open("/proc/self/ns/mnt",O_RDONLY|O_CLOEXEC),
            .interface="azpub0",.mac={2,0,0,0,0x12,3},.address=offer.address,.mask=offer.mask,.broadcast=offer.broadcast};assert(spec.namespace_fd>=0);
        struct hub_public_network_ready ready={0};assert(!hub_public_network_ready_start(&ready,&lease,&spec,300));
        int result;
        if (trial) { result=hub_public_network_ready_poll(&ready,100,0,NULL,0);assert(result<0); }
        else while (!(result=hub_public_network_ready_poll(&ready,300,0,NULL,0))) poll(NULL,0,1);
        assert(result<0 && lease.state==HUB_LEASE_DRAINING);
        while ((result=hub_public_network_ready_quiesce(&ready))==1) poll(NULL,0,1);
        assert(!result && !ready.factory.started && !ready.configure.started && ready.transport.fd==-1 && ready.factory.transport.fd==-1);
        assert(fcntl(spec.namespace_fd,F_GETFD)>=0);close(spec.namespace_fd);
    }
    puts("PASS: readiness controller retires wrong-namespace/backward-clock candidates and joins pending factory without releasing caller namespace");
}

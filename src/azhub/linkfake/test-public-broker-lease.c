/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include "public-broker-lease.h"
struct fixture { uint64_t now;int pending;unsigned removed;struct in_addr expected; };
static uint64_t clock_now(void *opaque) { return ((struct fixture *)opaque)->now; }
static int remove_address(void *opaque,unsigned slot,const struct hub_public_network_spec *spec) {
    struct fixture *f=opaque;assert(slot==0 && spec->address.s_addr==f->expected.s_addr);
    assert(fcntl(spec->namespace_fd,F_GETFD)>=0);
    if (f->pending) return 1;
    ++f->removed;return 0;
}
static struct hub_public_dhcp_event offer(unsigned host) {
    char line[160];int n=snprintf(line,sizeof line,"1\tbound\tazpub0\t192.0.2.%u\t255.255.255.0\t60\t192.0.2.2\n",host);
    struct hub_public_dhcp_event event;assert(!hub_public_dhcp_event_parse(line,n,"azpub0",&event));return event;
}
static void activate(struct hub_public_network_lease *lease,uint64_t now) {
    struct hub_public_acd proof={.state=HUB_ACD_CLAIMING,.pending=HUB_ACD_CLAIM};
    memcpy(proof.address,&lease->current.address.s_addr,4);
    assert(!hub_public_lease_configured(lease,lease->generation,now,&proof));
    proof.state=HUB_ACD_ANNOUNCING;proof.pending=HUB_ACD_ANNOUNCE;proof.announcements=1;
    assert(!hub_public_lease_activate(lease,lease->generation,now,&proof));
}
int main(void) {
    struct fixture f={.now=300,.pending=1};struct hub_public_broker_leases owner={.now=clock_now,.remove=remove_address,.context=&f};
    struct hub_public_network_lease lease={0};struct hub_public_dhcp_event initial=offer(3),replacement=offer(4);
    assert(!hub_public_lease_offer(&lease,&initial,100,200));
    struct hub_public_network_spec spec={.namespace_fd=open("/proc/self/ns/net",O_RDONLY|O_CLOEXEC),
        .address=initial.address,.mask=initial.mask,.broadcast=initial.broadcast};assert(spec.namespace_fd>=0);
    assert(hub_public_broker_lease_prepare(&owner,0,&lease,&spec)<0);
    activate(&lease,300);assert(!hub_public_broker_lease_prepare(&owner,0,&lease,&spec));
    close(spec.namespace_fd);assert(hub_public_broker_lease_ready(&owner,0));
    assert(hub_public_broker_lease_prepare(&owner,0,&lease,&owner.slots[0].spec)<0 && errno==EBUSY);
    f.now=1000;assert(!hub_public_lease_offer(&lease,&replacement,900,1000));
    assert(!hub_public_broker_lease_ready(&owner,0) && lease.queued);
    f.expected=initial.address;assert(hub_public_broker_lease_release(&owner,0)==1);
    assert(lease.state==HUB_LEASE_REMOVING && lease.configured && lease.queued && owner.slots[0].prepared);
    f.pending=0;assert(!hub_public_broker_lease_release(&owner,0));
    assert(!owner.slots[0].prepared && owner.slots[0].spec.namespace_fd==-1 && f.removed==1);
    assert(lease.state==HUB_LEASE_CANDIDATE && lease.generation==2 && lease.current.address.s_addr==replacement.address.s_addr);
    activate(&lease,1000);spec.namespace_fd=open("/proc/self/ns/net",O_RDONLY|O_CLOEXEC);
    assert(hub_public_broker_lease_prepare(&owner,0,&lease,&spec)<0 && errno==EADDRNOTAVAIL);
    spec.address=replacement.address;assert(!hub_public_broker_lease_prepare(&owner,0,&lease,&spec));close(spec.namespace_fd);
    f.now=60900;assert(!hub_public_broker_lease_ready(&owner,0));
    f.expected=replacement.address;assert(!hub_public_broker_lease_release(&owner,0));
    assert(lease.state==HUB_LEASE_IDLE && !lease.configured && f.removed==2);
    puts("PASS: ready lease gates endpoint context; exact retained address, async removal, queued replacement and expiry retirement");
}

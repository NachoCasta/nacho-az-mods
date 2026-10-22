/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include "public-link-local.h"
#include "public-network-lease.h"
int main(void) {
    int pipes[2];assert(!pipe(pipes));uint16_t samples[]={65535,65024,0,65023};
    assert(write(pipes[1],samples,sizeof samples)==sizeof samples);close(pipes[1]);
    struct in_addr first,last;assert(!hub_public_link_local_sample(pipes[0],&first) && ntohl(first.s_addr)==0xa9fe0100);
    assert(!hub_public_link_local_sample(pipes[0],&last) && ntohl(last.s_addr)==0xa9fefeff);
    assert(hub_public_link_local_sample(pipes[0],&last)<0 && errno==EIO);close(pipes[0]);
    struct hub_public_network_lease lease={0};assert(!hub_public_lease_link_local(&lease,first,1000));
    assert(lease.origin==HUB_ADDRESS_LINK_LOCAL && lease.state==HUB_LEASE_CANDIDATE && !lease.seen_grant);
    assert(!lease.current.server.s_addr && !lease.current.lease_seconds && !lease.configured);
    assert(!hub_public_lease_poll(&lease,UINT64_C(3000000000)) && lease.state==HUB_LEASE_CANDIDATE);
    assert(!hub_public_lease_ready(&lease,UINT64_C(3000000000))); /* Sampling cannot publish. */
    assert(hub_public_lease_link_local(&lease,last,UINT64_C(3000000001))<0);
    hub_public_lease_retire(&lease);assert(!hub_public_lease_drained(&lease,1,UINT64_C(3000000001)));
    assert(!hub_public_lease_removed(&lease,1,UINT64_C(3000000002)) && lease.state==HUB_LEASE_IDLE);
    struct hub_public_network_lease invalid={0};
    assert(hub_public_lease_link_local(&invalid,(struct in_addr){htonl(0xa9fe00ff)},1)<0);
    assert(hub_public_lease_link_local(&invalid,(struct in_addr){htonl(0xa9feff00)},1)<0);
    struct hub_public_dhcp_event grant={.kind=HUB_DHCP_BOUND,.lease_seconds=60,
        .address={htonl(0xc0000203)},.mask={htonl(0xffffff00)},.broadcast={htonl(0xc00002ff)},.server={htonl(0xc0000202)}};
    assert(!hub_public_lease_offer(&invalid,&grant,1000,1000));
    assert(!hub_public_lease_poll(&invalid,61000) && invalid.state==HUB_LEASE_DRAINING);
    assert(!hub_public_lease_drained(&invalid,1,61000) && !hub_public_lease_removed(&invalid,1,61001));
    assert(!hub_public_lease_link_local(&invalid,first,61002) && invalid.generation==2 && !invalid.seen_grant);
    assert(!hub_public_lease_offer(&invalid,&grant,62000,62000) && invalid.state==HUB_LEASE_DRAINING && invalid.queued);
    assert(invalid.origin==HUB_ADDRESS_LINK_LOCAL); /* No live DHCP address replacement. */
    assert(!hub_public_lease_drained(&invalid,2,62001) && !hub_public_lease_removed(&invalid,2,62002));
    assert(invalid.origin==HUB_ADDRESS_DHCP && invalid.generation==3 && invalid.state==HUB_LEASE_CANDIDATE);
    puts("PASS: uniform allowed IPv4LL range, entropy failure, explicit non-DHCP ownership, no synthetic expiry and mandatory readiness/retirement");
}

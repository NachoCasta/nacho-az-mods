/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include "public-address-inventory.h"
int main(void) {
    struct hub_public_address_inventory inventory={0};
    assert(!hub_public_address_inventory_add(&inventory,(struct in_addr){0}) && !inventory.count);
    for (unsigned i=1;i<=32;++i) assert(!hub_public_address_inventory_add(&inventory,(struct in_addr){htonl(0x0a000000u+i)}));
    assert(inventory.count==32 && !hub_public_address_inventory_add(&inventory,inventory.addresses[0]));
    assert(hub_public_address_inventory_add(&inventory,(struct in_addr){htonl(0x0a000021u)})<0 && errno==E2BIG && inventory.count==32);
    assert(!hub_public_address_inventory_native(&inventory));
    assert(inventory.count && hub_public_address_inventory_contains(&inventory,(struct in_addr){htonl(INADDR_LOOPBACK)}));
    puts("PASS: bounded deduplicated native IPv4 snapshot, overflow refusal and no address mutation");
}

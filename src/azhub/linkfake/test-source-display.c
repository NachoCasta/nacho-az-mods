/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include "source-display.h"
int main(void) {
    const int players[]={7,7,8,8,7,8,7,8},slots[]={3,7,3,7,8,8,2,2};
    unsigned seen=0;
    for (int i=0;i<8;i++) {
        int n=hub_source_display_number(players[i],slots[i]);
        assert(n==i+1 && !(seen&(1u<<n))); seen|=1u<<n;
    }
    /* Removing earlier sources cannot alter the identities of survivors. */
    for (int i=7;i>=2;i--) assert(hub_source_display_number(players[i],slots[i])==i+1);
    assert(!hub_source_display_number(1,3));
    assert(!hub_source_display_number(9,3));
    assert(!hub_source_display_number(7,1));
    assert(!hub_source_display_number(8,0));
    /* First inserted stick is in port4; second inserted stick is in port1. */
    assert(hub_source_port_reserve(7,3,4));
    assert(hub_source_port_reserve(7,7,1));
    assert(hub_source_display_number(7,3)==4);
    assert(hub_source_display_number(7,7)==1);
    assert(!hub_source_port_reserve(8,3,4)); /* Never steal an existing label. */
    assert(!hub_source_port_reserve(7,3,3)); /* Never rename a cached identity. */
    assert(hub_source_port_reserve(7,3,4)); /* Same-port reconnect is idempotent. */
    assert(!hub_source_port_reserve(1,3,2));
    assert(!hub_source_port_reserve(8,3,9));
    return 0;
}

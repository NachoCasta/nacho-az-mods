/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include "link-udp-policy.h"
int main(void) {
    uint8_t p[1152]={0};memcpy(p,"Qspt1WmJOL",10);
    for (unsigned member=7;member<=8;member++) {
        p[10]=6;p[0x24]=member;p[0x2c]=127;p[0x2d]=77;p[0x2e]=0;p[0x2f]=member;
        assert(!probe_udp_reject(p,54,50000,member));
        assert(probe_udp_reject(p,54,50000,0));
        assert(probe_udp_reject(p,54,50000,15-member));
        p[0x24]=3;assert(probe_udp_reject(p,54,50000,0)); /* Embedded private IP alone. */
        p[0x2c]=192;p[0x2d]=0;p[0x2e]=2;p[0x2f]=2;
        assert(!probe_udp_reject(p,54,50000,0)); /* External discovery remains native. */
        p[0x24]=member;assert(probe_udp_reject(p,54,50000,0));
        p[10]=10;p[0x21]=member;p[0x24]=member;
        assert(!probe_udp_reject(p,1152,50002,member));
        assert(probe_udp_reject(p,1152,50002,0));
        p[0x21]=3;assert(probe_udp_reject(p,1152,50002,0));
        p[0x24]=3;assert(!probe_udp_reject(p,1152,50002,0));
        assert(probe_udp_reject(p,1152,50002,member));
        p[10]=6;p[0x21]=member;memset(p+0x24,0,4);p[0x27]=member;
        assert(!probe_udp_reject(p,192,50002,member));
        assert(probe_udp_reject(p,192,50002,0));
        p[0x21]=3;assert(probe_udp_reject(p,192,50002,0));
        p[0x27]=3;assert(!probe_udp_reject(p,192,50002,0));
        /* Every truncated extent is safe to inspect, including partial magic. */
        for (size_t n=0;n<sizeof p;n++) (void)probe_udp_reject(p,n,50002,member);
    }
    assert(!probe_udp_reject(NULL,1152,50002,0));
    p[10]=0x28;assert(!probe_udp_reject(p,60,50001,0)); /* Native beat packet. */
    assert(probe_udp_reject(p,60,50001,7));
    p[0]='!';assert(!probe_udp_reject(p,1152,50002,0));
    return 0;
}

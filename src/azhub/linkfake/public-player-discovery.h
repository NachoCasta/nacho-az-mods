/* SPDX-License-Identifier: GPL-3.0-only */
/* Complete discovery datagrams only; caller verifies interface/sender policy.
 * Reuse one parser for collision withdrawal and controller reservations. */
#ifndef HUB_PUBLIC_PLAYER_DISCOVERY_H
#define HUB_PUBLIC_PLAYER_DISCOVERY_H
#include <stddef.h>
#include <string.h>
#include "public-player-pool.h"
static int hub_public_player_claim(const unsigned char *bytes,size_t length,unsigned *number,const unsigned char **mac) {
    if (!bytes || !number || !mac || length<36 || memcmp(bytes,"Qspt1WmJOL",10) || bytes[35]!=length) return 0;
    unsigned n;const unsigned char *m;
    if (bytes[10]==2 && length==50) { n=bytes[46];m=bytes+40; }
    else if (bytes[10]==6 && length==54) { n=bytes[36];m=bytes+38; }
    else return 0;
    unsigned present=0;for (unsigned i=0;i<6;++i) present|=m[i];
    if (!n || !present || (m[0]&1)) return 0;
    *number=n;*mac=m;return 1;
}
static int hub_public_player_discovery_observe(struct hub_public_player_pool *pool,
        const unsigned char *bytes,size_t length,const unsigned char owned[][6],size_t count,uint64_t now) {
    if (!pool || count>9 || (count && !owned)) { errno=EINVAL;return -1; }
    unsigned number;const unsigned char *mac;
    if (!hub_public_player_claim(bytes,length,&number,&mac)) return 0;
    for (size_t i=0;i<count;++i) if (!memcmp(mac,owned[i],6)) return 0;
    return hub_public_player_observe(pool,number,now);
}
#endif

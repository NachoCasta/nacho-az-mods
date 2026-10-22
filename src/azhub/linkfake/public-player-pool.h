/* SPDX-License-Identifier: GPL-3.0-only */
/* Controller-owned reservations for the experimentally exercised computer
 * identities41..48. No CDJ interoperability is inferred from this range.
 * Feed validated foreign discovery only. Release a reservation only after
 * authenticated helper EMPTY; observations never move another active source. */
#ifndef HUB_PUBLIC_PLAYER_POOL_H
#define HUB_PUBLIC_PLAYER_POOL_H
#include <errno.h>
#include <stdint.h>
#define HUB_PUBLIC_PLAYER_FIRST 41
#define HUB_PUBLIC_PLAYER_COUNT 8
#define HUB_PUBLIC_PLAYER_HOLD_MS UINT64_C(60000)
struct hub_public_player_pool {
    uint64_t foreign[8],last;
    unsigned char seen[8],reserved[8]; /* reserved is indexed by source slot. */
};
static int hub_public_player_clock(struct hub_public_player_pool *pool,uint64_t now) {
    if (!pool || now<pool->last) { errno=ERANGE;return -1; }
    pool->last=now;return 0;
}
static int hub_public_player_observe(struct hub_public_player_pool *pool,unsigned number,uint64_t now) {
    if (hub_public_player_clock(pool,now)<0) return -1;
    if (number<HUB_PUBLIC_PLAYER_FIRST || number>=HUB_PUBLIC_PLAYER_FIRST+HUB_PUBLIC_PLAYER_COUNT) return 0;
    unsigned index=number-HUB_PUBLIC_PLAYER_FIRST;pool->foreign[index]=now;pool->seen[index]=1;return 1;
}
static int hub_public_player_foreign(const struct hub_public_player_pool *pool,unsigned index,uint64_t now) {
    return pool->seen[index] && now-pool->foreign[index]<HUB_PUBLIC_PLAYER_HOLD_MS;
}
/* Existing reservations are immutable, even after a competing claim. The caller
 * must retire/release the old generation before selecting a replacement. */
static int hub_public_player_reserve(struct hub_public_player_pool *pool,unsigned slot,uint64_t now) {
    if (!pool || slot>=8) { errno=EINVAL;return -1; }
    if (hub_public_player_clock(pool,now)<0) return -1;
    if (pool->reserved[slot]) {
        if (hub_public_player_foreign(pool,pool->reserved[slot]-HUB_PUBLIC_PLAYER_FIRST,now)) { errno=EEXIST;return -1; }
        return pool->reserved[slot];
    }
    for (unsigned step=0;step<8;++step) {
        unsigned index=(slot+step)%8,number=HUB_PUBLIC_PLAYER_FIRST+index;int used=0;
        if (hub_public_player_foreign(pool,index,now)) continue;
        for (unsigned i=0;i<8;++i) if (pool->reserved[i]==number) used=1;
        if (!used) { pool->reserved[slot]=(unsigned char)number;return (int)number; }
    }
    errno=ENOSPC;return -1;
}
static int hub_public_player_release(struct hub_public_player_pool *pool,unsigned slot,unsigned number) {
    if (!pool || slot>=8 || !number || pool->reserved[slot]!=number) { errno=ESTALE;return -1; }
    pool->reserved[slot]=0;return 0;
}
#endif

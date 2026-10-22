/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "public-player-pool.h"
int main(void) {
    struct hub_public_player_pool pool={0};
    for (unsigned i=0;i<8;++i) assert(hub_public_player_reserve(&pool,i,0)==41+(int)i);
    assert(hub_public_player_observe(&pool,41,10)==1);
    assert(hub_public_player_reserve(&pool,0,10)<0 && errno==EEXIST && pool.reserved[0]==41);
    for (unsigned i=1;i<8;++i) assert(hub_public_player_reserve(&pool,i,10)==41+(int)i);
    assert(hub_public_player_release(&pool,0,42)<0 && errno==ESTALE);
    assert(!hub_public_player_release(&pool,0,41));
    assert(hub_public_player_reserve(&pool,0,11)<0 && errno==ENOSPC);
    assert(!hub_public_player_release(&pool,1,42));
    assert(hub_public_player_reserve(&pool,0,12)==42); /* Free alternative, neighbor never stolen. */
    assert(hub_public_player_reserve(&pool,1,60009)<0 && errno==ENOSPC);
    assert(hub_public_player_reserve(&pool,1,60010)==41);
    assert(hub_public_player_observe(&pool,1,60010)==0); /* Never borrow native/private identities. */
    assert(hub_public_player_observe(&pool,41,60009)<0 && errno==ERANGE);
    assert(hub_public_player_reserve(&pool,8,60010)<0 && errno==EINVAL);
    struct hub_public_player_pool occupied={0};
    for (unsigned number=41;number<=48;++number) assert(hub_public_player_observe(&occupied,number,0)==1);
    assert(hub_public_player_reserve(&occupied,0,59999)<0 && errno==ENOSPC);
    assert(hub_public_player_reserve(&occupied,0,60000)==41);
    puts("PASS: stable eight-source reservations, foreign claims, post-release reselection, exhaustion, expiry and backward-clock refusal");
}

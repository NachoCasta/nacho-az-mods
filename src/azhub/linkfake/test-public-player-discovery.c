/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "public-player-discovery.h"
#include "public-provider-packets.h"
int main(void) {
    unsigned char ip[4]={192,0,2,3},mac[6]={2,0,0,0,0,41},packet[54];
    struct hub_public_provider_identity identity;assert(!hub_public_provider_identity_init(&identity,41,ip,mac,"Hub USB 1"));
    struct hub_public_player_pool pool={0};unsigned char owned[1][6];memcpy(owned[0],mac,6);
    for (unsigned kind=2;kind<=6;kind+=4) {
        memset(&pool,0,sizeof pool);
        size_t length=hub_public_provider_discovery(&identity,kind,6,packet);
        assert(!hub_public_player_discovery_observe(&pool,packet,length,owned,1,10));
        assert(!pool.seen[0]);
        assert(hub_public_player_discovery_observe(&pool,packet,length,NULL,0,10)==1 && pool.seen[0]);
        struct hub_public_player_pool before=pool;
        packet[35]--;assert(!hub_public_player_discovery_observe(&pool,packet,length,NULL,0,11));packet[35]++;
        assert(!hub_public_player_discovery_observe(&pool,packet,length-1,NULL,0,11));
        unsigned offset=kind==2 ? 40:38;packet[offset]|=1;
        assert(!hub_public_player_discovery_observe(&pool,packet,length,NULL,0,11));
        memset(packet+offset,0,6);assert(!hub_public_player_discovery_observe(&pool,packet,length,NULL,0,11));
        assert(!memcmp(&pool,&before,sizeof pool));
    }
    assert(hub_public_player_discovery_observe(&pool,packet,54,NULL,1,12)<0 && errno==EINVAL);
    puts("PASS: shared complete discovery parser, own-MAC exclusion, foreign claims and malformed/truncated claim refusal");
}

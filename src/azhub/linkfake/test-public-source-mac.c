/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "public-source-mac.h"
int main(void) {
    unsigned char parent[6]={0x52,0x54,0,0x12,0x34,0x56},all[8][6],repeat[6],other[6];
    for (unsigned slot=0;slot<8;++slot) {
        assert(!hub_public_source_mac(parent,slot,all[slot]));
        assert((all[slot][0]&3)==2 && (all[slot][5]&7)==slot && memcmp(all[slot],parent,6));
        assert(!hub_public_source_mac(parent,slot,repeat) && !memcmp(repeat,all[slot],6));
        for (unsigned prior=0;prior<slot;++prior) assert(memcmp(all[slot],all[prior],6));
    }
    const unsigned char vector[6]={0x42,0xd2,0xba,0xd9,0xaa,0x10};
    assert(!memcmp(all[0],vector,6));
    parent[5]^=1;assert(!hub_public_source_mac(parent,0,other) && memcmp(other,all[0],6));
    memset(repeat,0xa5,6);assert(hub_public_source_mac(parent,8,repeat)<0 && errno==EINVAL && repeat[0]==0xa5);
    memset(parent,0,6);assert(hub_public_source_mac(parent,0,repeat)<0);
    parent[0]=1;assert(hub_public_source_mac(parent,0,repeat)<0);
    assert(hub_public_source_mac(NULL,0,repeat)<0 && hub_public_source_mac(parent,0,NULL)<0);
    puts("PASS: stable eight-source LAA candidates, source separation, controller differentiation and invalid-input refusal");
}

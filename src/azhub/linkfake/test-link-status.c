/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include "link-status.h"
static void test_own_media(void) {
    const unsigned slots[]={2,3,7,8};
    const size_t offsets[]={0x73,0x6f,0x45e,0x45f};
    for(unsigned combination=0;combination<16;combination++) {
        uint8_t packet[1152],before[1152];unsigned mask=0;
        for(size_t i=0;i<sizeof packet;i++)packet[i]=(uint8_t)(i*17+combination);
        memcpy(packet,"Qspt1WmJOL",10);packet[10]=0x0a;
        memcpy(before,packet,sizeof packet);
        for(unsigned i=0;i<4;i++)if(combination&(1u<<i))mask|=1u<<slots[i];
        assert(hub_status_own_media(packet,sizeof packet,mask));
        for(unsigned i=0;i<4;i++)assert(packet[offsets[i]]==((combination&(1u<<i))?0:4));
        for(size_t i=0;i<sizeof packet;i++) {
            if((i>=0x6c&&i<=0x6e)||(i>=0x70&&i<=0x72)){assert(packet[i]==0);continue;}
            if(i==0x6f||i==0x73||i==0x45e||i==0x45f)continue;
            assert(packet[i]==before[i]);
        }
        memcpy(before,packet,sizeof packet);
        assert(!hub_status_own_media(packet,sizeof packet-1,mask));
        assert(!hub_status_own_media(packet,sizeof packet+1,mask));
        assert(!memcmp(packet,before,sizeof packet));
        packet[10]=6;memcpy(before,packet,sizeof packet);
        assert(!hub_status_own_media(packet,sizeof packet,mask));
        assert(!memcmp(packet,before,sizeof packet));
        packet[10]=0x0a;packet[0]='!';memcpy(before,packet,sizeof packet);
        assert(!hub_status_own_media(packet,sizeof packet,mask));
        assert(!memcmp(packet,before,sizeof packet));
    }
    assert(!hub_status_own_media(NULL,1152,0));
}
int main(void) {
    test_own_media();
    uint8_t packet[1152],before[sizeof packet];
    for (unsigned flags=0;flags<256;flags++) {
        memset(packet,0xa5,sizeof packet);
        memcpy(packet,"Qspt1WmJOL",10);packet[10]=0x0a;
        packet[0x89]=(uint8_t)flags;packet[0x9f]=3;
        memcpy(before,packet,sizeof packet);
        assert(hub_status_make_idle(packet,sizeof packet));
        assert(packet[0x89]==(flags&~0x7au));
        assert(packet[0x9f]==0xff);
        for (size_t i=0;i<sizeof packet;i++)
            if (i!=0x89 && i!=0x9f) assert(packet[i]==before[i]);
        memcpy(before,packet,sizeof packet);
        assert(hub_status_make_idle(packet,sizeof packet));
        assert(!memcmp(before,packet,sizeof packet));
    }
    for (size_t n=0;n<0xcc;n++) {
        memcpy(before,packet,sizeof packet);
        assert(!hub_status_make_idle(packet,n));
        assert(!memcmp(before,packet,sizeof packet));
    }
    assert(!hub_status_make_idle(NULL,sizeof packet));
    packet[10]=6;memcpy(before,packet,sizeof packet);
    assert(!hub_status_make_idle(packet,sizeof packet));
    assert(!memcmp(before,packet,sizeof packet));
    packet[10]=0x0a;packet[0]='!';memcpy(before,packet,sizeof packet);
    assert(!hub_status_make_idle(packet,sizeof packet));
    assert(!memcmp(before,packet,sizeof packet));
    memcpy(packet,"Qspt1WmJOL",10);packet[10]=0x0a;
    packet[0x28]=8;packet[0x29]=3;packet[0x2a]=1;
    memset(packet+0x2c,0x55,4);
    memcpy(before,packet,sizeof packet);
    assert(!hub_status_hide_source(packet,sizeof packet,7,3));
    assert(!hub_status_hide_source(packet,sizeof packet,8,7));
    assert(!hub_status_hide_source(packet,sizeof packet-1,8,3));
    assert(!memcmp(before,packet,sizeof packet));
    assert(hub_status_hide_source(packet,sizeof packet,8,3));
    for (size_t i=0;i<sizeof packet;i++) {
        if ((i>=0x28 && i<0x2b) || (i>=0x2c && i<0x30)) assert(packet[i]==0);
        else assert(packet[i]==before[i]);
    }
    assert(!hub_status_hide_source(packet,sizeof packet,8,3));
    /* Native current-source reporting must survive auxiliary private rows,
     * including a native location with the same content ID as a private one. */
    memset(packet,0xa5,sizeof packet);memcpy(packet,"Qspt1WmJOL",10);packet[10]=10;
    packet[0x21]=1;packet[0x24]=1;packet[0x28]=1;packet[0x29]=3;packet[0x2a]=1;
    packet[0x2c]=0;packet[0x2d]=0;packet[0x2e]=1;packet[0x2f]=239;
    for (unsigned i=0;i<11;i++) {
        size_t offset=i<10 ? 0x18c+0x40*i:0x428;
        packet[offset]=1;packet[offset+1]=2;packet[offset+2]=(i%2)?7:6;
        packet[offset+8]=0;packet[offset+9]=0;packet[offset+10]=1;packet[offset+11]=239;
    }
    /* One native and one external row remain visible despite equal IDs. */
    packet[0x1cc+1]=1;packet[0x1cc+2]=0;
    packet[0x20c+2]=2;
    memcpy(before,packet,sizeof packet);
    assert(hub_status_hide_private_locations(packet,sizeof packet)==9);
    for (size_t i=0;i<sizeof packet;i++) {
        int hidden=0;
        for (unsigned row=0;row<11;row++) {
            size_t offset=row<10 ? 0x18c+0x40*row:0x428;
            if (row!=1 && row!=2 && i>=offset && i<offset+36) hidden=1;
        }
        assert(packet[i]==(hidden ? 0:before[i]));
    }
    assert(!hub_status_hide_private_locations(packet,sizeof packet));
    memcpy(packet,before,sizeof packet);
    for (size_t n=0;n<sizeof packet;n++) {
        assert(!hub_status_hide_private_locations(packet,n));
        assert(!memcmp(packet,before,sizeof packet));
    }
    assert(!hub_status_hide_private_locations(NULL,sizeof packet));
    assert(!hub_status_hide_private_locations(packet,sizeof packet+1));
    packet[10]=6;memcpy(before,packet,sizeof packet);
    assert(!hub_status_hide_private_locations(packet,sizeof packet));
    assert(!memcmp(packet,before,sizeof packet));
    packet[10]=10;packet[0]='!';memcpy(before,packet,sizeof packet);
    assert(!hub_status_hide_private_locations(packet,sizeof packet));
    assert(!memcmp(packet,before,sizeof packet));
    /* Location matching never depends on content value or current deck. */
    memcpy(packet,"Qspt1WmJOL",10);packet[0x194]=0x12;packet[0x195]=0x34;
    packet[0x196]=0x56;packet[0x197]=0x78;
    assert(hub_status_hide_private_locations(packet,sizeof packet)==9);
    return 0;
}

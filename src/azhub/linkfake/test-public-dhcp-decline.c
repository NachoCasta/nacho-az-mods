/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include "public-dhcp-decline.h"
static unsigned check_sum(const unsigned char *p,size_t n,unsigned sum) {
    for (size_t i=0;i<n;++i) sum+=(unsigned)p[i]<<(i%2 ? 0:8);
    while (sum>65535) sum=(sum>>16)+(sum&65535);
    return sum;
}
int main(void) {
    const char line[]="1\tbound\tazpub0\t192.0.2.3\t255.255.255.0\t60\t192.0.2.2\n";
    struct hub_public_dhcp_event grant;
    assert(!hub_public_dhcp_event_parse(line,sizeof line-1,"azpub0",&grant));
    const unsigned char mac[6]={2,0,0,0,0x12,3},xid[4]={1,2,3,4};
    unsigned char packet[HUB_PUBLIC_DECLINE_SIZE];memset(packet,0xcc,sizeof packet);
    assert(hub_public_dhcp_decline_packet(packet,sizeof packet-1,&grant,mac,xid)<0);
    for (unsigned i=0;i<sizeof packet;++i) assert(packet[i]==0xcc);
    assert(hub_public_dhcp_decline_packet(packet,sizeof packet,&grant,mac,xid)==sizeof packet);
    assert(packet[0]==0x45 && packet[2]==1 && packet[3]==72 && packet[8]==64 && packet[9]==17);
    assert(!memcmp(packet+12,"\0\0\0\0\xff\xff\xff\xff",8));
    assert(check_sum(packet,20,0)==65535);
    assert(packet[20]==0 && packet[21]==68 && packet[22]==0 && packet[23]==67);
    assert(packet[24]==1 && packet[25]==52 && (packet[26] || packet[27]));
    unsigned pseudo=check_sum(packet+12,8,17+308);
    assert(check_sum(packet+20,308,pseudo)==65535);
    unsigned char *bootp=packet+28;
    assert(!memcmp(bootp,"\x01\x01\x06\0",4));assert(!memcmp(bootp+4,xid,4));
    assert(!memcmp(bootp+8,"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",20));
    assert(!memcmp(bootp+28,mac,6));
    assert(!memcmp(bootp+236,"\x63\x82\x53\x63",4));
    unsigned seen=0;size_t offset=240;
    while (bootp[offset]!=255) {
        assert(offset+2<300);unsigned code=bootp[offset++],length=bootp[offset++];
        assert(offset+length<300);
        if (code==53) { assert(length==1 && bootp[offset]==4);seen|=1; }
        else if (code==50) { assert(length==4 && !memcmp(bootp+offset,&grant.address.s_addr,4));seen|=2; }
        else if (code==54) { assert(length==4 && !memcmp(bootp+offset,&grant.server.s_addr,4));seen|=4; }
        else if (code==61) { assert(length==7 && bootp[offset]==1 && !memcmp(bootp+offset+1,mac,6));seen|=8; }
        else if (code==56) { assert(length==16 && !memcmp(bootp+offset,"Address conflict",16));seen|=16; }
        else assert(0);
        offset+=length;
    }
    assert(seen==31);
    while (++offset<300) assert(!bootp[offset]);
    packet[100]^=1;assert(check_sum(packet+20,308,pseudo)!=65535);
    unsigned char invalid_mac[6]={3,0,0,0,0x12,3},zero[4]={0};
    assert(hub_public_dhcp_decline_packet(packet,sizeof packet,&grant,invalid_mac,xid)<0);
    assert(hub_public_dhcp_decline_packet(packet,sizeof packet,&grant,mac,zero)<0);
    grant.address.s_addr=htonl(0xe0000001);
    assert(hub_public_dhcp_decline_packet(packet,sizeof packet,&grant,mac,xid)<0);
    struct hub_public_arp transport={.fd=-1};struct hub_public_acd arp={0};
    assert(hub_public_dhcp_decline_send(&transport,&arp,&grant,xid)<0 && errno==EINVAL);
    puts("PASS: DHCPDECLINE mandatory fields/options, MAC client identity, zero-source broadcast, IPv4/UDP checksums and conflict/capacity guards");
    return 0;
}

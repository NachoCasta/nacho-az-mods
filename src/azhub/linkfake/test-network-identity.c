/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include "network-identity.h"
static uint32_t ip(const char *s) {struct in_addr a;assert(inet_pton(AF_INET,s,&a)==1);return a.s_addr;}
int main(void) {
    const struct {const char *own,*mask,*a,*b;} cases[]={
      {"192.168.1.115","255.255.255.0","192.168.1.254","192.168.1.253"},
      {"10.0.2.15","255.255.255.0","10.0.2.254","10.0.2.253"},
      {"192.168.1.254","255.255.255.0","192.168.1.253","192.168.1.252"},
      {"172.16.7.2","255.255.0.0","172.16.255.254","172.16.255.253"},
      {"169.254.12.34","255.255.0.0","169.254.255.254","169.254.255.253"},
      {"10.1.0.9","255.255.255.248","10.1.0.14","10.1.0.13"}
    };uint32_t peers[2];
    for(unsigned i=0;i<sizeof cases/sizeof *cases;i++) {
      assert(hub_choose_peers(ip(cases[i].own),ip(cases[i].mask),peers));
      assert(peers[0]==ip(cases[i].a) && peers[1]==ip(cases[i].b));
      assert(peers[0]!=ip(cases[i].own) && peers[1]!=ip(cases[i].own));
    }
    const char *bad_masks[]={"255.0.255.0","255.255.255.252","255.255.255.254","255.255.255.255","0.0.0.0"};
    for(unsigned i=0;i<sizeof bad_masks/sizeof *bad_masks;i++) assert(!hub_choose_peers(ip("192.168.1.115"),ip(bad_masks[i]),peers));
    assert(!hub_choose_peers(ip("0.0.0.0"),ip("255.255.255.0"),peers));
    assert(!hub_choose_peers(ip("127.0.0.1"),ip("255.0.0.0"),peers));
    assert(!hub_choose_peers(ip("224.0.0.1"),ip("255.255.255.0"),peers));
    puts("network identity policy: 14 cases passed");
    if(hub_network_peers(0,peers)) {
      char a[32],b[32];inet_ntop(AF_INET,&peers[0],a,sizeof a);inet_ntop(AF_INET,&peers[1],b,sizeof b);
      printf("live interface selection: %s %s\n",a,b);
    }
}

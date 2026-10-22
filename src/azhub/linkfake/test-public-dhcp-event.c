/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "public-dhcp-event.h"
static const char good[]="1\tbound\tazpub0\t192.0.2.3\t255.255.255.0\t3600\t192.0.2.1\n";
static void rejected(const char *line) {
    struct hub_public_dhcp_event a,b;memset(&a,0x5a,sizeof a);b=a;
    assert(hub_public_dhcp_event_parse(line,strlen(line),"azpub0",&a)<0);
    assert(!memcmp(&a,&b,sizeof a));
}
int main(void) {
    struct hub_public_dhcp_event result;
    assert(!hub_public_dhcp_event_parse(good,sizeof good-1,"azpub0",&result));
    assert(result.kind==HUB_DHCP_BOUND && result.lease_seconds==3600);
    assert(ntohl(result.broadcast.s_addr)==0xc00002ff);
    for (size_t n=0;n<sizeof good-1;++n)
        assert(hub_public_dhcp_event_parse(good,n,"azpub0",&result)<0);
    assert(hub_public_dhcp_event_parse(good,sizeof good-1,"azpub1",&result)<0);
    const char *invalid[]={
        "1\tbound\tazpub0\t192.0.2.0\t255.255.255.0\t3600\t192.0.2.1\n",
        "1\tbound\tazpub0\t192.0.2.255\t255.255.255.0\t3600\t192.0.2.1\n",
        "1\tbound\tazpub0\t127.0.0.3\t255.255.255.0\t3600\t192.0.2.1\n",
        "1\tbound\tazpub0\t224.0.0.3\t255.255.255.0\t3600\t192.0.2.1\n",
        "1\tbound\tazpub0\t192.0.2.3\t255.0.255.0\t3600\t192.0.2.1\n",
        "1\tbound\tazpub0\t192.0.2.3\t255.255.255.255\t3600\t192.0.2.1\n",
        "1\tbound\tazpub0\t192.0.2.3\t255.255.255.0\t0\t192.0.2.1\n",
        "1\tbound\tazpub0\t192.0.2.3\t255.255.255.0\t4294967296\t192.0.2.1\n",
        "1\tbound\tazpub0\t192.0.2.3\t255.255.255.0\t-1\t192.0.2.1\n",
        "1\tbound\tazpub0\t192.0.2.3\t255.255.255.0\t3600\t0.0.0.0\n",
        "1\tdeconfig\tazpub0\t192.0.2.3\t-\t-\t-\n",
        "2\tbound\tazpub0\t192.0.2.3\t255.255.255.0\t3600\t192.0.2.1\n",
        "1\tbound\tazpub0\t192.0.2.3\t255.255.255.0\t3600\t192.0.2.1\tEXTRA\n"
    };
    for (unsigned i=0;i<sizeof invalid/sizeof *invalid;++i) rejected(invalid[i]);
    const char *events[]={"deconfig","nak","leasefail","invalid"};
    for (unsigned i=0;i<4;++i) {
        char line[128];int n=snprintf(line,sizeof line,"1\t%s\tazpub0\t-\t-\t-\t-\n",events[i]);
        assert(!hub_public_dhcp_event_parse(line,(size_t)n,"azpub0",&result));
        assert(result.kind==(enum hub_public_dhcp_kind)(HUB_DHCP_DECONFIG+i) && !result.address.s_addr);
    }
    const char *short_lease="1\trenew\tazpub0\t192.0.2.3\t255.255.255.0\t1\t192.0.2.1\n";
    assert(!hub_public_dhcp_event_parse(short_lease,strlen(short_lease),"azpub0",&result));
    assert(result.lease_seconds==1 && result.kind==HUB_DHCP_RENEW);
    const char *long_lease="1\tbound\tazpub0\t192.0.2.3\t255.255.255.0\t4294967295\t192.0.2.1\n";
    assert(!hub_public_dhcp_event_parse(long_lease,strlen(long_lease),"azpub0",&result));
    assert(result.lease_seconds==2147483);
    /* Every one-byte mutation is bounded; success may only produce known kinds. */
    for (size_t i=0;i<sizeof good-1;++i) for (unsigned byte=0;byte<256;++byte) {
        char line[sizeof good];memcpy(line,good,sizeof line);line[i]=(char)byte;
        int rc=hub_public_dhcp_event_parse(line,sizeof line-1,"azpub0",&result);
        assert(rc<0 || (unsigned)result.kind<6);
    }
    puts("PASS: bounded DHCP event parsing, exact interface, subnet/lease validation, retirement events and mutation controls");
    return 0;
}

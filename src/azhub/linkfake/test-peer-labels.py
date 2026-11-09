#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Test actual cached-label refresh, ingress rewrite and fallback transmit policy."""
import os,re,shlex,subprocess,tempfile
from pathlib import Path
root=Path(__file__).resolve().parent
source=(root/'linkfake.c').read_text()
def function(name,source=source):
    match=re.search(r'\b'+name+r'\([^;{}]*\)\s*\{',source);assert match,name
    start=source.rfind('static ',0,match.start());depth=1
    for end in range(match.end(),len(source)):
        depth+=(source[end]=='{')-(source[end]=='}')
        if not depth:return source[start:end+1]
    raise AssertionError(name)
harness=r'''
#define _GNU_SOURCE
#include <assert.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <net/if.h>
#include "link-udp-receive.h"
static uint32_t hub_observed_address,hub_peer_ips[2];
static int hub_local_transport_only,network_available,runtime_ready=1,finished;
static const char MAGIC[]="Qspt1WmJOL";
static int lf_runtime_ready(void) { return runtime_ready; }
static int hub_network_peers(uint32_t preferred,uint32_t peers[2]) {
    (void)preferred;
    if (!__atomic_load_n(&network_available,__ATOMIC_ACQUIRE)) return 0;
    peers[0]=htonl(0xc00002fe);peers[1]=htonl(0xc00002fd);return 1;
}
struct fake {int player;};static struct fake fake_members[2]={{7},{8}};
static struct fake *fake_by_port(uint16_t port) {
    return port==htons(10001) ? fake_members:port==htons(10002) ? fake_members+1:NULL;
}
'''
for name in ('hub_refresh_peer_labels','as_in','fix_source','foreign_link_packet'):
    harness+=function(name)+'\n'
harness+=function('probe_udp_loopback_ingress',(root/'link-udp-ingress.h').read_text())+'\n'
harness+=r'''
static void *writer(void *unused) {
    (void)unused;
    for (int i=0;i<10000;i++) {
        __atomic_store_n(&network_available,i&1,__ATOMIC_RELEASE);
        assert(hub_refresh_peer_labels());
    }
    __atomic_store_n(&finished,1,__ATOMIC_RELEASE);return NULL;
}
static void assert_member(unsigned member) {
    struct sockaddr_in address={.sin_family=AF_INET,.sin_port=htons(9994+member)};
    address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    fix_source((void *)&address,sizeof address);
    assert(address.sin_port==htons(9994+member));
    uint32_t label=ntohl(address.sin_addr.s_addr);
    assert(label==(0x7f4d0000u|member));
}
int main(void) {
    struct probe_udp_metadata metadata={0};
    struct sockaddr_in *source=(void *)&metadata.source;source->sin_family=AF_INET;
    source->sin_addr.s_addr=htonl(INADDR_LOOPBACK);metadata.source_length=sizeof *source;
    unsigned loopback=if_nametoindex("lo");assert(loopback);metadata.interface_index=loopback;
    assert(probe_udp_loopback_ingress(&metadata));
    metadata.interface_index=loopback+1;assert(!probe_udp_loopback_ingress(&metadata));
    metadata.interface_index=loopback;metadata.control_incomplete=1;assert(!probe_udp_loopback_ingress(&metadata));
    metadata.control_incomplete=0;source->sin_addr.s_addr=htonl(0xc0000202);assert(!probe_udp_loopback_ingress(&metadata));
    source->sin_addr.s_addr=htonl(INADDR_LOOPBACK);metadata.source_length=0;assert(!probe_udp_loopback_ingress(&metadata));
    assert(!hub_refresh_peer_labels() && !hub_peer_ips[0]);
    hub_local_transport_only=1;assert(hub_refresh_peer_labels());
    assert(ntohl(hub_peer_ips[0])==0x7f4d0007 && ntohl(hub_peer_ips[1])==0x7f4d0008);
    network_available=1;assert(hub_refresh_peer_labels());
    assert(ntohl(hub_peer_ips[0])==0x7f4d0007 && ntohl(hub_peer_ips[1])==0x7f4d0008);
    hub_local_transport_only=0;assert(hub_refresh_peer_labels());
    assert(ntohl(hub_peer_ips[0])==0xc00002fe && ntohl(hub_peer_ips[1])==0xc00002fd);
    network_available=0;assert(hub_refresh_peer_labels());assert(ntohl(hub_peer_ips[0])==0xc00002fe);
    struct sockaddr_in external={.sin_family=AF_INET};external.sin_addr.s_addr=htonl(0xc0000202);
    unsigned char link[54]={0};memcpy(link,MAGIC,10);
    assert(!foreign_link_packet(link,sizeof link,(void *)&external,sizeof external));
    hub_local_transport_only=1;
    assert(foreign_link_packet(link,sizeof link,(void *)&external,sizeof external));
    assert(!foreign_link_packet("ordinary internet",17,(void *)&external,sizeof external));
    assert(!foreign_link_packet(link,9,(void *)&external,sizeof external));
    struct sockaddr_in local=external;local.sin_addr.s_addr=htonl(0x7f4d0007);
    assert(!foreign_link_packet(link,sizeof link,(void *)&local,sizeof local));
    runtime_ready=0;assert(!foreign_link_packet(link,sizeof link,(void *)&external,sizeof external));runtime_ready=1;
    hub_local_transport_only=0;setenv("LINKFAKE_LOCAL_ONLY","1",1);
    assert(foreign_link_packet(link,sizeof link,(void *)&external,sizeof external));unsetenv("LINKFAKE_LOCAL_ONLY");
    local.sin_addr.s_addr=htonl(INADDR_LOOPBACK);local.sin_port=htons(9999);
    fix_source((void *)&local,sizeof local);assert(local.sin_addr.s_addr==htonl(INADDR_LOOPBACK));
    fix_source((void *)&external,sizeof external);assert(external.sin_addr.s_addr==htonl(0xc0000202));
    hub_local_transport_only=1;assert(hub_refresh_peer_labels());
    pthread_t thread;assert(!pthread_create(&thread,NULL,writer,NULL));
    unsigned reads=0;
    do {assert_member(7);assert_member(8);++reads;} while(!__atomic_load_n(&finished,__ATOMIC_ACQUIRE) || reads<10000);
    assert(!pthread_join(thread,NULL));
    puts("PASS: offline labels, native refresh, failed-observation retention, isolated cached ingress and fallback Link/Internet transmit policy;10000 concurrent refreshes");
}
'''
flags={'plain':['-O2'],'asan':['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=undefined'],'tsan':['-O1','-g','-fsanitize=thread']}
with tempfile.TemporaryDirectory(prefix='azhub-peer-labels-') as temp:
    p=Path(temp);(p/'test.c').write_text(harness)
    for mode in os.environ.get('SAN','plain').split():
        binary=p/mode
        subprocess.run(shlex.split(os.environ.get('TEST_CC','cc'))+flags[mode]+['-Wall','-Wextra','-Wno-unused-function','-I'+str(root),'-pthread',str(p/'test.c'),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)

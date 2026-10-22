/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include <poll.h>
#include "public-rpc-udp.h"
#include "public-file-catalog.h"
struct peer_policy { struct sockaddr_in client; int permit; unsigned calls; };
static int allow(void *opaque,const struct sockaddr_in *peer) {
    struct peer_policy *policy=opaque; ++policy->calls;
    return policy->permit && peer->sin_addr.s_addr==policy->client.sin_addr.s_addr &&
        peer->sin_port==policy->client.sin_port;
}
static int loopback_socket(struct sockaddr_in *address) {
    int fd=socket(AF_INET,SOCK_DGRAM|SOCK_CLOEXEC,0); assert(fd>=0);
    memset(address,0,sizeof *address); address->sin_family=AF_INET;
    address->sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    assert(!bind(fd,(struct sockaddr *)address,sizeof *address));
    socklen_t size=sizeof *address;
    assert(!getsockname(fd,(struct sockaddr *)address,&size)); return fd;
}
static void send_call(int fd,const struct sockaddr_in *server,const void *call,size_t length) {
    assert(sendto(fd,call,length,0,(const struct sockaddr *)server,sizeof *server)==(ssize_t)length);
}
static size_t receive_reply(int fd,unsigned char *out,size_t capacity) {
    struct pollfd ready={fd,POLLIN,0}; assert(poll(&ready,1,1000)==1);
    ssize_t bytes=recv(fd,out,capacity,0); assert(bytes>=0); return (size_t)bytes;
}
int main(void) {
    char directory[]="/tmp/azhub-rpc-udp.XXXXXX"; assert(mkdtemp(directory));
    char path[512]; snprintf(path,sizeof path,"%s/track",directory);
    int file=open(path,O_CREAT|O_EXCL|O_WRONLY,0600); assert(file>=0);
    assert(write(file,"abcdef",6)==6 && !close(file));
    struct hub_public_root root; assert(!hub_public_root_init(&root));
    assert(!hub_public_root_attach(&root,directory,1));
    struct hub_public_file_catalog *catalog=calloc(1,sizeof *catalog); assert(catalog);
    unsigned char nonce[16]={9},handle[32];
    assert(!hub_public_catalog_init(catalog,&root,1,nonce));
    assert(!hub_public_catalog_register(catalog,"track",5,handle));
    struct sockaddr_in server,client; int service=loopback_socket(&server),peer=loopback_socket(&client);
    struct peer_policy policy={client,1,0};
    unsigned char call[84]={0},reply[256];
    const uint32_t words[]={0x12345678,0,2,100003,2,6,0,0,0,0};
    for (unsigned i=0;i<10;++i) hub_public_rpc_store(call+i*4,words[i]);
    memcpy(call+40,handle,32); hub_public_rpc_store(call+72,1); hub_public_rpc_store(call+76,3);
    assert(hub_public_rpc_udp_once(service,&root,hub_public_catalog_resolve,catalog,allow,&policy)==HUB_UDP_IDLE);
    send_call(peer,&server,call,sizeof call);
    assert(hub_public_rpc_udp_once(service,&root,hub_public_catalog_resolve,catalog,allow,&policy)==HUB_UDP_REPLIED);
    size_t bytes=receive_reply(peer,reply,sizeof reply);
    assert(bytes==104 && hub_public_rpc_word(reply)==0x12345678);
    assert(!hub_public_rpc_word(reply+24) && hub_public_rpc_word(reply+96)==3);
    assert(!memcmp(reply+100,"bcd\0",4) && !root.active);
    /* Default-deny policy and explicit denied peer produce no response. */
    send_call(peer,&server,call,sizeof call);
    assert(hub_public_rpc_udp_once(service,&root,hub_public_catalog_resolve,catalog,NULL,NULL)==HUB_UDP_DROPPED);
    policy.permit=0; send_call(peer,&server,call,sizeof call);
    assert(hub_public_rpc_udp_once(service,&root,hub_public_catalog_resolve,catalog,allow,&policy)==HUB_UDP_DROPPED);
    policy.permit=1;
    unsigned char oversized[HUB_PUBLIC_RPC_UDP_REQUEST_MAX+1]={0};
    unsigned calls=policy.calls; send_call(peer,&server,oversized,sizeof oversized);
    assert(hub_public_rpc_udp_once(service,&root,hub_public_catalog_resolve,catalog,allow,&policy)==HUB_UDP_DROPPED);
    assert(policy.calls==calls);
    send_call(peer,&server,call,4);
    assert(hub_public_rpc_udp_once(service,&root,hub_public_catalog_resolve,catalog,allow,&policy)==HUB_UDP_DROPPED);
    assert(recv(peer,reply,sizeof reply,MSG_DONTWAIT)<0 && (errno==EAGAIN || errno==EWOULDBLOCK));
    hub_public_catalog_withdraw(catalog); assert(hub_public_root_drained(&root));
    send_call(peer,&server,call,sizeof call);
    assert(hub_public_rpc_udp_once(service,&root,hub_public_catalog_resolve,catalog,allow,&policy)==HUB_UDP_REPLIED);
    assert(receive_reply(peer,reply,sizeof reply)==28 && hub_public_rpc_word(reply+24)==70);
    close(peer); close(service); hub_public_catalog_destroy(catalog); free(catalog);
    hub_public_root_destroy(&root); assert(!unlink(path) && !rmdir(directory));
    puts("PASS: localhost UDP catalog READ, byte-exact response, default-deny peer policy, truncation and withdrawn STALE");
}

/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include "public-provider-udp.h"
static int peer_allowed(void *unused,const struct sockaddr_in *peer) {
    (void)unused; return peer->sin_addr.s_addr==htonl(0x7f000002);
}
static int endpoint(unsigned ip,unsigned port) {
    int fd=socket(AF_INET,SOCK_DGRAM|SOCK_NONBLOCK,0); assert(fd>=0);
    struct sockaddr_in address={.sin_family=AF_INET,.sin_addr={.s_addr=htonl(ip)},.sin_port=htons((uint16_t)port)};
    assert(!bind(fd,(struct sockaddr *)&address,sizeof address)); return fd;
}
static void send_request(int fd,unsigned port,const void *data,size_t length) {
    struct sockaddr_in address={.sin_family=AF_INET,.sin_addr={.s_addr=htonl(0x7f000001)},.sin_port=htons((uint16_t)port)};
    assert(sendto(fd,data,length,0,(struct sockaddr *)&address,sizeof address)==(ssize_t)length);
}
int main(void) {
    struct hub_public_provider_endpoint config={.discovery_fd=endpoint(0x7f000001,50000),
        .status_fd=endpoint(0x7f000001,50002),.broadcast=htonl(0x7f000002)};
    int discovery=endpoint(0x7f000002,50000),status=endpoint(0x7f000002,50002);
    int ephemeral=endpoint(0x7f000002,0);
    const unsigned char ip[]={127,0,0,1},mac[]={2,0,0,0,17,41};
    assert(!hub_public_provider_identity_init(&config.identity,41,ip,mac,"Hub USB1"));
    struct hub_public_provider_udp transport;
    hub_public_provider_udp_init(&transport,&config,0,peer_allowed,NULL);
    assert(!hub_public_provider_udp_step(&transport,100,0,1));
    unsigned char bytes[296]; assert(recv(discovery,bytes,sizeof bytes,0)==37 && bytes[10]==10);
    unsigned char request[48]={0}; memcpy(request,"Qspt1WmJOL",10); request[10]=0x46;
    send_request(ephemeral,50002,request,sizeof request);
    assert(!hub_public_provider_udp_step(&transport,100,1,1));
    assert(recv(status,bytes,sizeof bytes,0)==48 && bytes[10]==0x16);
    send_request(status,50002,request,sizeof request);
    assert(!hub_public_provider_udp_step(&transport,100,2,1));
    assert(recv(status,bytes,sizeof bytes,0)==72 && bytes[10]==0x47);
    send_request(status,50002,request,sizeof request);
    assert(!hub_public_provider_udp_step(&transport,100,3,0));
    assert(recv(status,bytes,sizeof bytes,0)<0 && (errno==EAGAIN || errno==EWOULDBLOCK));
    assert(!transport.state.active);
    hub_public_provider_udp_init(&transport,&config,100,peer_allowed,NULL);
    struct hub_public_provider_identity other=config.identity; other.mac[5]=42;
    size_t length=hub_public_provider_discovery(&other,2,6,bytes);
    send_request(discovery,50000,bytes,length);
    assert(!hub_public_provider_udp_step(&transport,100,100,1) && transport.conflict && !transport.state.active);
    assert(recv(discovery,bytes,sizeof bytes,0)<0 && (errno==EAGAIN || errno==EWOULDBLOCK));
    close(config.discovery_fd); close(config.status_fd); close(discovery); close(status); close(ephemeral);
    puts("PASS: loopback provider discovery, per-peer activation, withdrawal and identity-conflict suppression");
    return 0;
}

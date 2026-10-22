/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include "public-db-port.h"
static int allowed(void *context,const struct sockaddr_in *peer) {
    return *(int *)context && peer->sin_addr.s_addr==htonl(INADDR_LOOPBACK);
}
static int connect_peer(struct sockaddr_in *address) {
    int fd=socket(AF_INET,SOCK_STREAM,0); assert(fd>=0);
    assert(!connect(fd,(struct sockaddr *)address,sizeof *address)); return fd;
}
int main(void) {
    int listener=socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK,0); assert(listener>=0);
    struct sockaddr_in address={.sin_family=AF_INET,.sin_addr={.s_addr=htonl(INADDR_LOOPBACK)}};
    assert(!bind(listener,(struct sockaddr *)&address,sizeof address) && !listen(listener,16));
    socklen_t size=sizeof address; assert(!getsockname(listener,(struct sockaddr *)&address,&size));
    struct hub_db_port_service service; int permit=1;
    hub_db_port_init(&service,listener,1051,allowed,&permit);
    const unsigned char query[19]="\0\0\0\17RemoteDBServer";
    int stalled=connect_peer(&address),peer=connect_peer(&address);
    assert(!hub_db_port_step(&service,100,1)); assert(!hub_db_port_step(&service,101,1));
    for (unsigned i=0;i<sizeof query;++i) {
        assert(send(peer,query+i,1,0)==1); assert(!hub_db_port_step(&service,102+i,1));
    }
    unsigned char reply[2];
    assert(recv(peer,reply,2,MSG_WAITALL)==2 && reply[0]==4 && reply[1]==27);
    assert(recv(peer,reply,1,0)==0); close(peer);
    assert(!hub_db_port_step(&service,5100,1)); assert(recv(stalled,reply,1,0)==0); close(stalled);
    peer=connect_peer(&address); assert(send(peer,"bad",3,0)==3);
    assert(!hub_db_port_step(&service,5200,1)); assert(recv(peer,reply,1,0)==0); close(peer);
    int peers[9];
    for (unsigned i=0;i<9;++i) { peers[i]=connect_peer(&address); assert(!hub_db_port_step(&service,5300,1)); }
    assert(recv(peers[8],reply,1,0)==0);
    assert(!hub_db_port_step(&service,5301,0));
    for (unsigned i=0;i<9;++i) { assert(recv(peers[i],reply,1,0)==0); close(peers[i]); }
    permit=0; peer=connect_peer(&address); assert(!hub_db_port_step(&service,5400,1));
    assert(recv(peer,reply,1,0)==0); close(peer);
    hub_db_port_destroy(&service); hub_db_port_destroy(&service);
    assert(fcntl(listener,F_GETFD)>=0); close(listener);
    puts("PASS: fragmented DB port lookup, stalled-client deadline, malformed query, capacity, withdrawal and peer policy");
    return 0;
}

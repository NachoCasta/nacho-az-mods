/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include "link-socket-registry.h"
static struct link_socket_registry registry=LINK_SOCKET_REGISTRY_INIT;
static int fd_count(void) {
    DIR *d=opendir("/proc/self/fd");assert(d);int n=0;
    while (readdir(d)) n++;
    closedir(d);return n;
}
struct race { int client,server; };
static void *observe(void *arg) {
    struct race *r=arg;
    for (int i=0;i<1000;i++) {
        struct link_socket_proof p=link_socket_find(&registry,r->server);
        if (p.player) {assert(p.player==8);(void)link_socket_valid(&registry,&p);}
        link_socket_release(&p);
    }
    return NULL;
}
int main(void) {
    int initial=fd_count();
    int listener=socket(AF_INET,SOCK_STREAM,0);assert(listener>=0);
    struct sockaddr_in addr={.sin_family=AF_INET,.sin_addr.s_addr=htonl(INADDR_LOOPBACK)};
    assert(!bind(listener,(void *)&addr,sizeof addr));assert(!listen(listener,4));
    socklen_t size=sizeof addr;assert(!getsockname(listener,(void *)&addr,&size));
    int client=socket(AF_INET,SOCK_STREAM,0);assert(client>=0);
    struct link_socket_token first=link_socket_begin(&registry,client,8);assert(first.generation);
    assert(!connect(client,(void *)&addr,sizeof addr));
    int server=accept(listener,NULL,NULL);assert(server>=0);
    struct link_socket_proof p=link_socket_find(&registry,server);assert(!p.player);link_socket_release(&p);
    link_socket_finish(&registry,first,1);
    p=link_socket_find(&registry,server);assert(p.player==8 && link_socket_valid(&registry,&p));
    link_socket_forget(&registry,client);assert(!link_socket_valid(&registry,&p));
    struct link_socket_token second=link_socket_begin(&registry,client,8);assert(second.generation!=first.generation);
    link_socket_finish(&registry,first,1); /* Delayed completion must not activate the replacement. */
    struct link_socket_proof absent=link_socket_find(&registry,server);assert(!absent.player);link_socket_release(&absent);
    link_socket_finish(&registry,second,1);assert(!link_socket_valid(&registry,&p));link_socket_release(&p);
    p=link_socket_find(&registry,server);assert(link_socket_valid(&registry,&p));
    int other=socket(AF_INET,SOCK_STREAM,0);assert(other>=0);
    /* Simulate an unobserved dup2 overwrite of the server descriptor. */
    assert(dup2(other,server)==server);assert(!link_socket_valid(&registry,&p));link_socket_release(&p);
    close(server);close(other);link_socket_forget(&registry,client);close(client);

    client=socket(AF_INET,SOCK_STREAM,0);assert(client>=0);
    first=link_socket_begin(&registry,client,8);assert(!connect(client,(void *)&addr,sizeof addr));
    server=accept(listener,NULL,NULL);assert(server>=0);link_socket_finish(&registry,first,1);
    struct race args={client,server};pthread_t thread;assert(!pthread_create(&thread,NULL,observe,&args));
    for (int i=0;i<1000;i++) {
        link_socket_forget(&registry,client);
        second=link_socket_begin(&registry,client,8);link_socket_finish(&registry,second,1);
    }
    assert(!pthread_join(thread,NULL));
    p=link_socket_find(&registry,server);assert(link_socket_valid(&registry,&p));
    other=socket(AF_INET,SOCK_STREAM,0);assert(other>=0);
    assert(dup2(other,client)==client);assert(!link_socket_valid(&registry,&p));link_socket_release(&p);
    link_socket_forget(&registry,client);close(client);close(server);close(other);close(listener);
    /* Capacity exhaustion must fail closed without leaking pins. */
    int fds[LINK_SOCKET_SLOTS+1];
    for (unsigned i=0;i<LINK_SOCKET_SLOTS+1;i++) {
        fds[i]=socket(AF_INET,SOCK_STREAM,0);assert(fds[i]>=0);
        first=link_socket_begin(&registry,fds[i],7);
        assert((first.generation!=0)==(i<LINK_SOCKET_SLOTS));
    }
    for (unsigned i=0;i<LINK_SOCKET_SLOTS+1;i++) {link_socket_forget(&registry,fds[i]);close(fds[i]);}
    registry.serial=UINT64_MAX;
    client=socket(AF_INET,SOCK_STREAM,0);assert(client>=0);
    assert(!link_socket_begin(&registry,client,8).generation);close(client);
    assert(fd_count()==initial);
    puts("PASS: socket reuse, stale completion, concurrent lookup/invalidation, capacity, epoch exhaustion, no FD leak");
    return 0;
}

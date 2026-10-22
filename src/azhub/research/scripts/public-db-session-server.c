/* SPDX-License-Identifier: GPL-3.0-only */
/* Loopback-only handshake research harness; no library/provider advertisement. */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <netinet/in.h>
#include <poll.h>
#include "../../linkfake/public-db-session.h"
static int alive(void *context) { (void)context; return 1; }
static int begin(void *context,const struct hub_db_message *request) { (void)context; (void)request; return -1; }
static int next(void *context,struct hub_db_message *response) { (void)context; (void)response; return 0; }
static uint64_t milliseconds(void) {
    struct timespec now; assert(!clock_gettime(CLOCK_MONOTONIC,&now));
    return (uint64_t)now.tv_sec*1000+(uint64_t)now.tv_nsec/1000000;
}
int main(void) {
    int listener=socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0); assert(listener>=0);
    struct sockaddr_in address={.sin_family=AF_INET,.sin_addr={.s_addr=htonl(INADDR_LOOPBACK)}};
    assert(!bind(listener,(struct sockaddr *)&address,sizeof address) && !listen(listener,2));
    socklen_t size=sizeof address; assert(!getsockname(listener,(struct sockaddr *)&address,&size));
    printf("%u\n",ntohs(address.sin_port)); fflush(stdout);
    struct hub_db_session sessions[2]; unsigned accepted=0,completed=0; uint64_t started=milliseconds();
    struct hub_db_callbacks callbacks={begin,next,alive};
    while (milliseconds()-started<10000 && completed<2) {
        if (accepted<2) {
            int fd=accept4(listener,NULL,NULL,SOCK_NONBLOCK|SOCK_CLOEXEC);
            if (fd>=0) { assert(!hub_db_session_init(sessions+accepted,fd,41,milliseconds(),callbacks,NULL)); ++accepted; }
            else assert(errno==EAGAIN || errno==EWOULDBLOCK || errno==EINTR);
        }
        completed=0;
        for (unsigned i=0;i<accepted;++i) {
            hub_db_session_step(sessions+i,milliseconds());
            if (sessions[i].state==3) ++completed;
        }
        poll(NULL,0,1);
    }
    unsigned setups=0;
    for (unsigned i=0;i<accepted;++i) { setups+=sessions[i].requester==3; hub_db_session_destroy(sessions+i); }
    close(listener); assert(accepted==2 && setups==1 && completed==2);
    puts("PASS: idle peer and independent software-client handshake remain isolated");
}

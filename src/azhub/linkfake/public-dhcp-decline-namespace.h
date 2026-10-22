/* SPDX-License-Identifier: GPL-3.0-only */
/* Bound conflict notification from an owned source namespace. A queued send is
 * not server acknowledgement. Join before quiescing transport/namespace. */
#ifndef HUB_PUBLIC_DHCP_DECLINE_NAMESPACE_H
#define HUB_PUBLIC_DHCP_DECLINE_NAMESPACE_H
#include <pthread.h>
#include <sched.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <poll.h>
#include <time.h>
#include "public-dhcp-decline.h"
struct hub_public_decline_namespace {
    pthread_t thread;int started,done,error,namespace_fd;
    struct hub_public_arp transport;
    struct hub_public_acd conflict;
    struct hub_public_dhcp_event grant;
    unsigned char xid[4];
};
static uint64_t hub_public_decline_namespace_ms(void) {
    struct timespec now;if (clock_gettime(CLOCK_MONOTONIC,&now)<0) return 0;
    return (uint64_t)now.tv_sec*1000+now.tv_nsec/1000000;
}
static void *hub_public_decline_namespace_thread(void *opaque) {
    struct hub_public_decline_namespace *op=opaque;int error=0;
    if (setns(op->namespace_fd,CLONE_NEWNET)<0) error=errno;
    else {
        uint64_t deadline=hub_public_decline_namespace_ms()+1000;
        while (hub_public_dhcp_decline_send(&op->transport,&op->conflict,&op->grant,op->xid)<0) {
            if (errno!=EAGAIN && errno!=EWOULDBLOCK && errno!=EINTR) { error=errno;break; }
            if (hub_public_decline_namespace_ms()>=deadline) { error=ETIMEDOUT;break; }poll(NULL,0,1);
        }
    }
    close(op->namespace_fd);op->namespace_fd=-1;hub_public_arp_close(&op->transport);op->error=error;
    __atomic_store_n(&op->done,1,__ATOMIC_RELEASE);return NULL;
}
static int hub_public_decline_namespace_start(struct hub_public_decline_namespace *op,int namespace_fd,
        const struct hub_public_arp *transport,const struct hub_public_acd *conflict,const struct hub_public_dhcp_event *grant) {
    if (!op || op->started || !transport || transport->fd<0 || !conflict || !grant || conflict->state!=HUB_ACD_CONFLICT ||
            memcmp(transport->mac,conflict->mac,6) || memcmp(conflict->address,&grant->address.s_addr,4)) { errno=EINVAL;return -1; }
    struct stat current,target;if (stat("/proc/self/ns/net",&current)<0 || fstat(namespace_fd,&target)<0) return -1;
    if (current.st_dev==target.st_dev && current.st_ino==target.st_ino) { errno=EPERM;return -1; }
    unsigned char xid[4];int random=open("/dev/urandom",O_RDONLY|O_CLOEXEC);if (random<0) return -1;
    size_t used=0;while (used<4) {
        ssize_t got=read(random,xid+used,4-used);if (got<0 && errno==EINTR) continue;
        if (got<=0) { int error=got<0 ? errno:EIO;close(random);errno=error;return -1; }used+=(size_t)got;
    }
    close(random);if (!memcmp(xid,"\0\0\0\0",4)) { errno=EAGAIN;return -1; }
    int ns=fcntl(namespace_fd,F_DUPFD_CLOEXEC,0);if (ns<0) return -1;
    int arp=fcntl(transport->fd,F_DUPFD_CLOEXEC,0);if (arp<0) { int error=errno;close(ns);errno=error;return -1; }
    memset(op,0,sizeof *op);op->namespace_fd=ns;op->transport=*transport;op->transport.fd=arp;
    op->conflict=*conflict;op->grant=*grant;memcpy(op->xid,xid,4);
    int error=pthread_create(&op->thread,NULL,hub_public_decline_namespace_thread,op);
    if (error) { close(ns);close(arp);op->namespace_fd=op->transport.fd=-1;errno=error;return -1; }op->started=1;return 0;
}
static int hub_public_decline_namespace_poll(struct hub_public_decline_namespace *op) {
    if (!op || !op->started) { errno=EINVAL;return -1; }
    if (!__atomic_load_n(&op->done,__ATOMIC_ACQUIRE)) return 1;
    int error=pthread_join(op->thread,NULL);if (error) { errno=error;return -1; }op->started=0;
    if (op->error) { errno=op->error;return -1; }return 0;
}
#endif

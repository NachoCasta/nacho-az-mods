/* SPDX-License-Identifier: GPL-3.0-only */
/* Asynchronous namespace-bound ARP socket factory. Controller remains in its
 * namespace. Zero-initialize; poll/join and take or close transport before reuse. */
#ifndef HUB_PUBLIC_ADDRESS_ARP_NAMESPACE_H
#define HUB_PUBLIC_ADDRESS_ARP_NAMESPACE_H
#include <pthread.h>
#include <sched.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include "public-address-arp.h"
struct hub_public_arp_namespace {
    pthread_t thread;
    int namespace_fd,started,done,error;
    char interface[IFNAMSIZ];unsigned char mac[6];
    struct hub_public_arp transport;
};
static void *hub_public_arp_namespace_thread(void *opaque) {
    struct hub_public_arp_namespace *op=opaque;
    if (setns(op->namespace_fd,CLONE_NEWNET)<0 || hub_public_arp_open(&op->transport,op->interface,op->mac)<0)
        op->error=errno ? errno:EIO;
    close(op->namespace_fd);op->namespace_fd=-1;
    __atomic_store_n(&op->done,1,__ATOMIC_RELEASE);return NULL;
}
static int hub_public_arp_namespace_start(struct hub_public_arp_namespace *op,int namespace_fd,
        const char *interface,const unsigned char mac[6]) {
    if (!op || namespace_fd<0 || !interface || !mac || strncmp(interface,"azpub",5) ||
            strnlen(interface,IFNAMSIZ)==IFNAMSIZ || (mac[0]&3)!=2) { errno=EINVAL;return -1; }
    if (op->started || (op->done && op->transport.fd>=0)) { errno=EBUSY;return -1; }
    struct stat native,target;
    if (stat("/proc/self/ns/net",&native)<0 || fstat(namespace_fd,&target)<0) return -1;
    if (native.st_dev==target.st_dev && native.st_ino==target.st_ino) { errno=EPERM;return -1; }
    int fd=fcntl(namespace_fd,F_DUPFD_CLOEXEC,0);if (fd<0) return -1;
    memset(op,0,sizeof *op);op->namespace_fd=fd;op->transport.fd=-1;
    strcpy(op->interface,interface);memcpy(op->mac,mac,6);
    int error=pthread_create(&op->thread,NULL,hub_public_arp_namespace_thread,op);
    if (error) { close(fd);op->namespace_fd=-1;errno=error;return -1; }op->started=1;return 0;
}
/* 1 pending, 0 joined with owned transport, -1 error. A join failure retains
 * started/operation ownership and must not be treated as terminal cleanup. */
static int hub_public_arp_namespace_poll(struct hub_public_arp_namespace *op) {
    if (!op || !op->started) { errno=EINVAL;return -1; }
    if (!__atomic_load_n(&op->done,__ATOMIC_ACQUIRE)) return 1;
    int error=pthread_join(op->thread,NULL);if (error) { errno=error;return -1; }op->started=0;
    if (op->error) { errno=op->error;return -1; }return 0;
}
static int hub_public_arp_namespace_take(struct hub_public_arp_namespace *op,struct hub_public_arp *out) {
    if (!op || !out || op->started || !op->done || op->error || op->transport.fd<0 || out->fd>=0) { errno=EINVAL;return -1; }
    *out=op->transport;op->transport.fd=-1;return 0;
}
#endif

/* SPDX-License-Identifier: GPL-3.0-only */
/* FD-owned namespace creation, without bind mounts or controller-thread setns.
 * Zero-initialize; poll until joined. Close/take the owned FD before reuse.
 * Namespace release still requires all source endpoints/interfaces to retire. */
#ifndef HUB_PUBLIC_NETWORK_NAMESPACE_H
#define HUB_PUBLIC_NETWORK_NAMESPACE_H
#include <sched.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
struct hub_public_namespace_create { pthread_t thread;int started,done,error,fd; };
static void *hub_public_namespace_create_thread(void *opaque) {
    struct hub_public_namespace_create *op=opaque;
    if (unshare(CLONE_NEWNET)<0) op->error=errno;
    else {
        op->fd=open("/proc/thread-self/ns/net",O_RDONLY|O_CLOEXEC);
        if (op->fd<0) op->error=errno;
    }
    __atomic_store_n(&op->done,1,__ATOMIC_RELEASE);return NULL;
}
static int hub_public_namespace_create_start(struct hub_public_namespace_create *op) {
    if (!op) { errno=EINVAL;return -1; }
    if (op->started || (op->done && op->fd>=0)) { errno=EBUSY;return -1; }
    memset(op,0,sizeof *op);op->fd=-1;
    int error=pthread_create(&op->thread,NULL,hub_public_namespace_create_thread,op);
    if (error) { errno=error;return -1; }op->started=1;return 0;
}
/* 1 pending, 0 joined with an owned namespace FD, -1 error. A successful FD
 * survives creator-thread exit and is intentionally not installed in /run. */
static int hub_public_namespace_create_poll(struct hub_public_namespace_create *op) {
    if (!op || !op->started) { errno=EINVAL;return -1; }
    if (!__atomic_load_n(&op->done,__ATOMIC_ACQUIRE)) return 1;
    int error=pthread_join(op->thread,NULL);if (error) { errno=error;return -1; }
    op->started=0;
    if (op->error) { errno=op->error;return -1; }return 0;
}
static int hub_public_namespace_create_take(struct hub_public_namespace_create *op) {
    if (!op || op->started || !op->done || op->error || op->fd<0) { errno=EINVAL;return -1; }
    int fd=op->fd;op->fd=-1;return fd;
}
#endif

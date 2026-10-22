/* SPDX-License-Identifier: GPL-3.0-only */
/* Typed namespace validation for preallocated network namespaces. The calling
 * controller never enters the received namespace. A disposable thread performs
 * the kernel CLONE_NEWNET check and exits without configuring anything. */
#ifndef HUB_PUBLIC_BROKER_NAMESPACE_H
#define HUB_PUBLIC_BROKER_NAMESPACE_H
#include <sched.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
struct hub_public_broker_namespace_check { int fd,error; };
static void *hub_public_broker_namespace_thread(void *opaque) {
    struct hub_public_broker_namespace_check *check=opaque;
    if (setns(check->fd,CLONE_NEWNET)<0) check->error=errno;
    return NULL;
}
/* Caller keeps its FD valid until return. Success proves network namespace
 * kind/access only, not inventory ownership or permission to publish a source. */
static int hub_public_broker_namespace_validate(int fd) {
    struct stat received,current;struct statfs fs;
    if (fstat(fd,&received)<0 || fstatfs(fd,&fs)<0) return -1;
    if ((unsigned long)fs.f_type!=0x6e736673ul) { errno=ENOTTY;return -1; }
    int own=open("/proc/self/ns/net",O_RDONLY|O_CLOEXEC);
    if (own<0) return -1;
    int result=fstat(own,&current),error=errno;close(own);
    if (result<0) { errno=error;return -1; }
    /* Identity with the known network namespace proves kind without requiring
     * CAP_SYS_ADMIN merely to validate an already-current namespace. */
    if (received.st_dev==current.st_dev && received.st_ino==current.st_ino) return 0;
    struct hub_public_broker_namespace_check check={.fd=fd};pthread_t thread;
    error=pthread_create(&thread,NULL,hub_public_broker_namespace_thread,&check);
    if (error) { errno=error;return -1; }
    error=pthread_join(thread,NULL);
    if (error) { errno=error;return -1; }
    if (check.error) { errno=check.error;return -1; }
    return 0;
}
#endif

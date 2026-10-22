/* SPDX-License-Identifier: GPL-3.0-only */
/* One operation lease per bounded public file read, never per remote deck.
 * Callers still authorize library paths and select a discovered hub root.
 * No socket, announcement, mount mutation or native library slot is involved. */
#ifndef HUB_PUBLIC_FILE_ROOT_H
#define HUB_PUBLIC_FILE_ROOT_H
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define HUB_PUBLIC_PATH_MAX 4096
#define HUB_PUBLIC_READ_MAX 8192
#define HUB_PUBLIC_LEASES_MAX 16
struct hub_public_root {
    pthread_mutex_t mutex;
    int fd, withdrawing;
    uint64_t generation;
    unsigned active;
};
struct hub_public_file_lease { struct hub_public_root *owner; int fd; };
#define HUB_PUBLIC_FILE_LEASE_INIT { NULL, -1 }

static int hub_public_root_init(struct hub_public_root *root) {
    memset(root,0,sizeof *root);
    root->fd=-1; root->withdrawing=1;
    int error=pthread_mutex_init(&root->mutex,NULL);
    if (error) { errno=error; return -1; }
    return 0;
}
static void hub_public_root_close_if_drained(struct hub_public_root *root) {
    if (root->withdrawing && !root->active && root->fd>=0) {
        close(root->fd); root->fd=-1;
    }
}
/* Duplicate a caller-owned verified root. Never reopen its pathname. */
static int hub_public_root_attach_fd(struct hub_public_root *root,int directory,uint64_t generation) {
    pthread_mutex_lock(&root->mutex);
    if (root->fd>=0 || root->active) {
        pthread_mutex_unlock(&root->mutex); errno=EBUSY; return -1;
    }
    if (!generation || generation<=root->generation) {
        pthread_mutex_unlock(&root->mutex); errno=EINVAL; return -1;
    }
    struct stat state;
    int flags=fcntl(directory,F_GETFL);
    if (flags<0 || fstat(directory,&state)<0 || !S_ISDIR(state.st_mode) ||
            (flags&O_ACCMODE)!=O_RDONLY || (flags&O_PATH)) {
        pthread_mutex_unlock(&root->mutex); errno=EINVAL; return -1;
    }
    int fd=fcntl(directory,F_DUPFD_CLOEXEC,0);
    if (fd<0) { int error=errno; pthread_mutex_unlock(&root->mutex); errno=error; return -1; }
    root->fd=fd; root->generation=generation; root->withdrawing=0;
    pthread_mutex_unlock(&root->mutex);
    return 0;
}
static int hub_public_root_attach(struct hub_public_root *root,const char *path,uint64_t generation) {
    int fd=open(path,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
    if (fd<0) return -1;
    int result=hub_public_root_attach_fd(root,fd,generation),error=errno;
    close(fd);errno=error;return result;
}
static void hub_public_root_withdraw(struct hub_public_root *root) {
    pthread_mutex_lock(&root->mutex);
    root->withdrawing=1;
    hub_public_root_close_if_drained(root);
    pthread_mutex_unlock(&root->mutex);
}
static int hub_public_root_drained(struct hub_public_root *root) {
    pthread_mutex_lock(&root->mutex);
    int ready=root->withdrawing && root->fd<0 && !root->active;
    pthread_mutex_unlock(&root->mutex);
    return ready;
}
static void hub_public_file_finish(struct hub_public_file_lease *lease) {
    if (!lease->owner) return;
    struct hub_public_root *root=lease->owner;
    if (lease->fd>=0) close(lease->fd);
    lease->fd=-1; lease->owner=NULL;
    pthread_mutex_lock(&root->mutex);
    --root->active;
    hub_public_root_close_if_drained(root);
    pthread_mutex_unlock(&root->mutex);
}
static int hub_public_node_open(struct hub_public_root *root,uint64_t generation,
        const char *path,size_t length,int directory_node,struct hub_public_file_lease *lease) {
    if (!path || (!length && !directory_node) || length>=HUB_PUBLIC_PATH_MAX || memchr(path,0,length) ||
        (length && (path[0]=='/' || path[length-1]=='/')) || lease->owner || lease->fd!=-1) {
        errno=EINVAL; return -1;
    }
    char relative[HUB_PUBLIC_PATH_MAX];
    memcpy(relative,path,length); relative[length]=0;
    /* Reject ambiguous components before acquiring a descriptor. */
    for (size_t begin=0;begin<length;) {
        size_t end=begin;
        while (end<length && relative[end]!='/') ++end;
        size_t count=end-begin;
        if (!count || count>255 || (count==1 && relative[begin]=='.') ||
            (count==2 && relative[begin]=='.' && relative[begin+1]=='.')) {
            errno=EINVAL; return -1;
        }
        begin=end+1;
    }
    pthread_mutex_lock(&root->mutex);
    if (root->withdrawing || root->fd<0 || generation!=root->generation) {
        pthread_mutex_unlock(&root->mutex); errno=ESTALE; return -1;
    }
    if (root->active>=HUB_PUBLIC_LEASES_MAX) {
        pthread_mutex_unlock(&root->mutex); errno=EAGAIN; return -1;
    }
    ++root->active; lease->owner=root;
    int directory=fcntl(root->fd,F_DUPFD_CLOEXEC,0);
    pthread_mutex_unlock(&root->mutex);
    int error=errno, pinned=-1;
    if (directory<0) goto fail;
    /* The empty authorized directory path denotes only the attached root. */
    if (!length) { lease->fd=directory; return 0; }
    char *component=relative;
    for (;;) {
        char *slash=strchr(component,'/');
        if (!slash) break;
        *slash=0;
        int next=openat(directory,component,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
        error=errno; close(directory); directory=next;
        if (directory<0) goto fail;
        component=slash+1;
    }
    /* O_PATH pins the inode before any device/FIFO can be opened for reading.
     * Reopening our owned proc fd avoids a check/open path replacement race. */
    pinned=openat(directory,component,O_PATH|O_NOFOLLOW|O_CLOEXEC);
    error=errno; close(directory); directory=-1;
    if (pinned<0) goto fail;
    struct stat info;
    if (fstat(pinned,&info)<0) { error=errno; goto fail; }
    if (directory_node ? !S_ISDIR(info.st_mode) : !S_ISREG(info.st_mode)) { error=EACCES; goto fail; }
    char proc_path[64];
    snprintf(proc_path,sizeof proc_path,"/proc/self/fd/%d",pinned);
    lease->fd=open(proc_path,O_RDONLY|O_CLOEXEC|(directory_node ? O_DIRECTORY : 0));
    error=errno; close(pinned); pinned=-1;
    if (lease->fd<0) goto fail;
    return 0;
fail:
    if (directory>=0) close(directory);
    if (pinned>=0) close(pinned);
    hub_public_file_finish(lease);
    errno=error;
    return -1;
}
static int hub_public_file_open(struct hub_public_root *root,uint64_t generation,
        const char *path,size_t length,struct hub_public_file_lease *lease) {
    return hub_public_node_open(root,generation,path,length,0,lease);
}
static ssize_t hub_public_file_read(struct hub_public_file_lease *lease,void *buffer,
                                    size_t length,off_t offset) {
    if (!lease->owner || lease->fd<0 || length>HUB_PUBLIC_READ_MAX || offset<0) {
        errno=EINVAL; return -1;
    }
    return pread(lease->fd,buffer,length,offset);
}
/* Caller must first stop/join all users; withdrawal alone does not join threads. */
static int hub_public_root_destroy(struct hub_public_root *root) {
    if (!hub_public_root_drained(root)) { errno=EBUSY; return -1; }
    int error=pthread_mutex_destroy(&root->mutex);
    if (error) { errno=error; return -1; }
    return 0;
}
#endif

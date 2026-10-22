/* SPDX-License-Identifier: GPL-3.0-only */
/* Supervised one-shot DHCP acquisition. Separate exec'd helper enters namespace;
 * parent remains unchanged. A fresh pipe/process owns each attempt. Capture
 * CLOCK_MONOTONIC before begin and use that lower bound, not event receipt, as
 * the grant reference. Refresh means a fresh bounded acquisition with the same
 * MAC, not an unsupervised native renewal daemon. No address is applied here.
 * Zero-initialize each context; never reuse it until polling reaches terminal.
 * Requires exclusive child-reaping ownership and default SIGCHLD semantics. */
#ifndef HUB_PUBLIC_DHCP_ACQUIRE_H
#define HUB_PUBLIC_DHCP_ACQUIRE_H
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include "public-dhcp-event.h"
struct hub_public_dhcp_acquire {
    pid_t pid;
    int fd,reaped,status,eof,error,stopping,killed,complete,bound,records,deconfigured,lease_failed;
    uint64_t started,last,deadline,kill_at;
    char interface[16],line[192];size_t used;
    struct hub_public_dhcp_event proposal;
};
static void hub_public_dhcp_acquire_cancel(struct hub_public_dhcp_acquire *a,int error,uint64_t now) {
    if (!a || a->complete || a->stopping) return;
    a->error=error ? error : ECANCELED;a->stopping=1;
    a->kill_at=now>UINT64_MAX-2000 ? UINT64_MAX : now+2000;
    if (a->pid>0) kill(-a->pid,SIGTERM);
}
static int hub_public_dhcp_acquire_begin(struct hub_public_dhcp_acquire *a,const char *helper,
        int namespace_fd,const char *interface,const char *hook,const unsigned char mac[6],uint64_t now) {
    if (!a || !helper || helper[0]!='/' || namespace_fd<0 || !interface ||
            strnlen(interface,16)>=16 || strncmp(interface,"azpub",5) ||
            !hook || hook[0]!='/' || !mac || (mac[0]&1) || now>UINT64_MAX-15000) { errno=EINVAL;return -1; }
    if (a->pid>0 && !a->complete) { errno=EBUSY;return -1; }
    struct sigaction child_action;
    if (sigaction(SIGCHLD,NULL,&child_action)<0) return -1;
    if (child_action.sa_handler!=SIG_DFL || (child_action.sa_flags&SA_NOCLDWAIT)) { errno=ENOTSUP;return -1; }
    *a=(struct hub_public_dhcp_acquire){.fd=-1,.started=now,.last=now,.deadline=now+15000};
    strcpy(a->interface,interface);
    int descriptors[2],writer=-1,space=-1;
    if (pipe2(descriptors,O_CLOEXEC|O_NONBLOCK)<0) return -1;
    writer=fcntl(descriptors[1],F_DUPFD_CLOEXEC,10);
    if (writer>=0) space=fcntl(namespace_fd,F_DUPFD_CLOEXEC,10);
    if (writer<0 || space<0) goto failed;
    char mac_text[13],declared[64];
    for (unsigned i=0;i<6;++i) snprintf(mac_text+2*i,3,"%02x",mac[i]);
    snprintf(declared,sizeof declared,"AZHUB_DHCP_INTERFACE=%s",interface);
    char *argv[]={(char *)helper,"--dhcp-child",a->interface,(char *)hook,mac_text,NULL};
    char *env[]={"PATH=/usr/sbin:/usr/bin:/sbin:/bin","AZHUB_DHCP_EVENT_FD=3",declared,NULL};
    posix_spawn_file_actions_t actions;posix_spawnattr_t attributes;
    int error=posix_spawn_file_actions_init(&actions);
    if (error) { errno=error;goto failed; }
    error=posix_spawnattr_init(&attributes);
    if (error) { posix_spawn_file_actions_destroy(&actions);errno=error;goto failed; }
    sigset_t empty,defaults;sigemptyset(&empty);sigemptyset(&defaults);
    sigaddset(&defaults,SIGTERM);sigaddset(&defaults,SIGINT);sigaddset(&defaults,SIGPIPE);sigaddset(&defaults,SIGCHLD);
#define HUB_DHCP_SPAWN(call) do { if (!error) error=(call); } while (0)
    HUB_DHCP_SPAWN(posix_spawnattr_setsigmask(&attributes,&empty));
    HUB_DHCP_SPAWN(posix_spawnattr_setsigdefault(&attributes,&defaults));
    HUB_DHCP_SPAWN(posix_spawnattr_setpgroup(&attributes,0));
    HUB_DHCP_SPAWN(posix_spawnattr_setflags(&attributes,POSIX_SPAWN_SETSIGMASK|POSIX_SPAWN_SETSIGDEF|POSIX_SPAWN_SETPGROUP));
    HUB_DHCP_SPAWN(posix_spawn_file_actions_addclose(&actions,descriptors[0]));
    HUB_DHCP_SPAWN(posix_spawn_file_actions_addclose(&actions,descriptors[1]));
    HUB_DHCP_SPAWN(posix_spawn_file_actions_adddup2(&actions,writer,3));
    HUB_DHCP_SPAWN(posix_spawn_file_actions_adddup2(&actions,space,4));
    for (int fd=0;fd<3;++fd) HUB_DHCP_SPAWN(posix_spawn_file_actions_addopen(&actions,fd,"/dev/null",fd ? O_WRONLY : O_RDONLY,0));
    if (!error) error=posix_spawn(&a->pid,helper,&actions,&attributes,argv,env);
    posix_spawnattr_destroy(&attributes);posix_spawn_file_actions_destroy(&actions);
#undef HUB_DHCP_SPAWN
    if (error) { errno=error;goto failed; }
    close(writer);close(space);close(descriptors[1]);a->fd=descriptors[0];return 0;
failed: {
    int error=errno;if (writer>=0) close(writer);if (space>=0) close(space);
    close(descriptors[0]);close(descriptors[1]);errno=error;return -1;
    }
}
static void hub_public_dhcp_acquire_record(struct hub_public_dhcp_acquire *a,uint64_t now) {
    struct hub_public_dhcp_event event;
    if (++a->records>8 || hub_public_dhcp_event_parse(a->line,a->used,a->interface,&event)<0) {
        hub_public_dhcp_acquire_cancel(a,EPROTO,now);return;
    }
    if (event.kind==HUB_DHCP_DECONFIG && a->records==1 && !a->bound) { a->deconfigured=1;return; }
    if (event.kind==HUB_DHCP_LEASEFAIL && a->records==2 && a->deconfigured && !a->bound) { a->lease_failed=1;return; }
    if (a->lease_failed) { hub_public_dhcp_acquire_cancel(a,EPROTO,now);return; }
    if (event.kind!=HUB_DHCP_BOUND || a->bound) {
        hub_public_dhcp_acquire_cancel(a,event.kind==HUB_DHCP_NAK ? EADDRNOTAVAIL : EPROTO,now);return;
    }
    a->proposal=event;a->bound=1;
}
/*0 while running,1 on accepted complete grant,-1 on terminal failed attempt.
 * Cancellation still requires polling until terminal, to reap child/close IPC.
 * Source lease readiness must separately expire even while this child runs. */
static int hub_public_dhcp_acquire_poll(struct hub_public_dhcp_acquire *a,uint64_t now) {
    if (!a || a->fd<0) return a && a->complete && !a->error ? 1 : -1;
    if (now<a->last) { hub_public_dhcp_acquire_cancel(a,ERANGE,a->last);now=a->last; }
    a->last=now;
    if (now>=a->deadline) hub_public_dhcp_acquire_cancel(a,ETIMEDOUT,now);
    if (a->stopping && !a->killed && now>=a->kill_at) { kill(-a->pid,SIGKILL);a->killed=1; }
    for (unsigned reads=0;!a->eof && reads<16;++reads) {
        char bytes[256];ssize_t count=read(a->fd,bytes,sizeof bytes);
        if (!count) { a->eof=1;break; }
        if (count<0) {
            if (errno==EINTR) continue;
            if (errno!=EAGAIN && errno!=EWOULDBLOCK) hub_public_dhcp_acquire_cancel(a,errno,now);
            break;
        }
        if (a->stopping) continue;
        for (ssize_t i=0;i<count;++i) {
            if (a->used==sizeof a->line) { hub_public_dhcp_acquire_cancel(a,EMSGSIZE,now);break; }
            a->line[a->used++]=bytes[i];
            if (bytes[i]=='\n') { hub_public_dhcp_acquire_record(a,now);a->used=0; }
            if (a->stopping) break;
        }
    }
    if (!a->reaped) {
        pid_t got=waitpid(a->pid,&a->status,WNOHANG);
        if (got==a->pid) a->reaped=1;
        else if (got<0 && errno!=EINTR) { a->reaped=1;hub_public_dhcp_acquire_cancel(a,ECHILD,now); }
    }
    if (!a->reaped || !a->eof) return 0;
    /* Only the complete native no-offer sequence permits fallback. Exit1 alone
     * may be a setup/exec failure and must never authorize self-assignment. */
    if (!a->error && WIFEXITED(a->status) && WEXITSTATUS(a->status)==1 &&
            a->deconfigured && a->lease_failed && a->records==2 && !a->bound && !a->used) a->error=ENODATA;
    if (!a->error && (!WIFEXITED(a->status) || WEXITSTATUS(a->status) || !a->bound || a->used)) a->error=EPROTO;
    if (!a->error && now-a->started>=(uint64_t)a->proposal.lease_seconds*1000) a->error=ETIMEDOUT;
    close(a->fd);a->fd=-1;a->complete=1;return a->error ? -1 : 1;
}
static int hub_public_dhcp_acquire_no_offer(const struct hub_public_dhcp_acquire *a) {
    return a && a->complete && a->reaped && a->eof && a->fd==-1 && !a->stopping && a->error==ENODATA &&
        a->deconfigured && a->lease_failed && a->records==2 && !a->bound && !a->used &&
        WIFEXITED(a->status) && WEXITSTATUS(a->status)==1;
}
#endif

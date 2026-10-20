/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef LINK_SOCKET_REGISTRY_H
#define LINK_SOCKET_REGISTRY_H
/* Experimental socket ownership registry. No SO_COOKIE dependency: the vendor
 * kernel does not expose it. A duplicate pins each owned socket's inode until
 * invalidation. Call begin before connect, finish afterward, forget before
 * close; every intercepted reconnect must begin a new generation.
 * Proofs own one duplicate of the accepted socket and must be released.
 * All native callbacks/logging remain outside this mutex. Raw close avoids
 * recursively entering the mod's close interceptor for registry-owned pins.
 */
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#define LINK_SOCKET_SLOTS 64
struct link_socket_token { unsigned index; uint64_t generation; };
struct link_socket_entry { int fd,pin,player,ready; uint64_t generation; };
struct link_socket_proof {
    struct link_socket_token token;
    int server,server_pin,player;
};
struct link_socket_registry {
    pthread_mutex_t lock;
    uint64_t serial;
    struct link_socket_entry entries[LINK_SOCKET_SLOTS];
};
#define LINK_SOCKET_REGISTRY_INIT { .lock=PTHREAD_MUTEX_INITIALIZER }
static void link_socket_close_pin(int fd) { if (fd>=0) syscall(SYS_close,fd); }
static int link_socket_same(int a,int b) {
    struct stat sa,sb;
    return a>=0 && b>=0 && !fstat(a,&sa) && !fstat(b,&sb) &&
        S_ISSOCK(sa.st_mode) && S_ISSOCK(sb.st_mode) && sa.st_dev==sb.st_dev && sa.st_ino==sb.st_ino;
}
static void link_socket_forget_locked(struct link_socket_registry *r,int fd) {
    for (unsigned i=0;i<LINK_SOCKET_SLOTS;i++) if (r->entries[i].generation && r->entries[i].fd==fd) {
        link_socket_close_pin(r->entries[i].pin);
        memset(&r->entries[i],0,sizeof r->entries[i]);
    }
}
static void link_socket_forget(struct link_socket_registry *r,int fd) {
    pthread_mutex_lock(&r->lock);link_socket_forget_locked(r,fd);pthread_mutex_unlock(&r->lock);
}
static struct link_socket_token link_socket_begin(struct link_socket_registry *r,int fd,int player) {
    struct link_socket_token token={0};
    pthread_mutex_lock(&r->lock);
    link_socket_forget_locked(r,fd);
    if (player>0 && r->serial!=UINT64_MAX) for (unsigned i=0;i<LINK_SOCKET_SLOTS;i++)
        if (!r->entries[i].generation) {
            int pin=fcntl(fd,F_DUPFD_CLOEXEC,0);
            if (pin>=0 && link_socket_same(fd,pin)) {
                token=(struct link_socket_token){i,++r->serial};
                r->entries[i]=(struct link_socket_entry){fd,pin,player,0,token.generation};
            } else link_socket_close_pin(pin);
            break;
        }
    pthread_mutex_unlock(&r->lock);return token;
}
static void link_socket_finish(struct link_socket_registry *r,struct link_socket_token token,int connected) {
    pthread_mutex_lock(&r->lock);
    if (token.generation && token.index<LINK_SOCKET_SLOTS) {
        struct link_socket_entry *entry=&r->entries[token.index];
        if (entry->generation==token.generation) {
            if (connected && link_socket_same(entry->fd,entry->pin)) entry->ready=1;
            else link_socket_forget_locked(r,entry->fd);
        }
    }
    pthread_mutex_unlock(&r->lock);
}
static int link_socket_reverse(int server,int client) {
    struct sockaddr_in sl={0},sp={0},cl={0},cp={0};
    socklen_t n=sizeof sl;
    if (getsockname(server,(void *)&sl,&n) || n!=sizeof sl) return 0;
    n=sizeof sp;if (getpeername(server,(void *)&sp,&n) || n!=sizeof sp) return 0;
    n=sizeof cl;if (getsockname(client,(void *)&cl,&n) || n!=sizeof cl) return 0;
    n=sizeof cp;if (getpeername(client,(void *)&cp,&n) || n!=sizeof cp) return 0;
    return sl.sin_family==AF_INET && sp.sin_family==AF_INET && cl.sin_family==AF_INET && cp.sin_family==AF_INET &&
        sl.sin_addr.s_addr==htonl(INADDR_LOOPBACK) && sp.sin_addr.s_addr==htonl(INADDR_LOOPBACK) &&
        sl.sin_addr.s_addr==cp.sin_addr.s_addr && sl.sin_port==cp.sin_port &&
        sp.sin_addr.s_addr==cl.sin_addr.s_addr && sp.sin_port==cl.sin_port;
}
static struct link_socket_proof link_socket_find(struct link_socket_registry *r,int server) {
    struct link_socket_proof proof={.server=-1,.server_pin=-1};
    pthread_mutex_lock(&r->lock);
    int pin=fcntl(server,F_DUPFD_CLOEXEC,0);
    if (pin>=0) for (unsigned i=0;i<LINK_SOCKET_SLOTS;i++) {
        struct link_socket_entry *entry=&r->entries[i];
        if (entry->generation && entry->ready && link_socket_same(entry->fd,entry->pin) &&
            link_socket_reverse(pin,entry->pin)) {
            proof=(struct link_socket_proof){{i,entry->generation},server,pin,entry->player};
            pin=-1;break;
        }
    }
    link_socket_close_pin(pin);
    pthread_mutex_unlock(&r->lock);return proof;
}
static int link_socket_valid(struct link_socket_registry *r,const struct link_socket_proof *proof) {
    int valid=0;
    pthread_mutex_lock(&r->lock);
    if (proof->token.generation && proof->token.index<LINK_SOCKET_SLOTS) {
        const struct link_socket_entry *entry=&r->entries[proof->token.index];
        valid=entry->generation==proof->token.generation && entry->ready && entry->player==proof->player &&
            link_socket_same(entry->fd,entry->pin) && link_socket_same(proof->server,proof->server_pin);
    }
    pthread_mutex_unlock(&r->lock);return valid;
}
static void link_socket_release(struct link_socket_proof *proof) {
    link_socket_close_pin(proof->server_pin);
    *proof=(struct link_socket_proof){.server=-1,.server_pin=-1};
}
#endif

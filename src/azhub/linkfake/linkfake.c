/* SPDX-License-Identifier: GPL-3.0-only */
/*
 * linkfake.so: fake Pro DJ Link players INSIDE EP147, so extra USB sticks show up as
 * "N LINK" sources with no network at all. LD_PRELOAD it into EP147 (prototype).
 *
 * Each fake player is a line in /home/root/linkfake.conf (or $LINKFAKE_CONF):
 *     <player 3|4> <fake ip> [<slot 2|3|7|8>] <local dir> <volume name>
 * The fake ip only labels the member; no packet is ever sent to it.
 *
 * - an injector thread sends the player's keep-alive (0x06) to 127.0.0.1:50000 and its CDJ
 *   status (0x0a) to 127.0.0.1:50002, from one loopback socket per player
 * - recvfrom/recvmsg: a datagram from one of those sockets gets its source address rewritten
 *   to the player's fake ip, so EP147 sees a remote member
 * - sendto: datagrams EP147 sends to a fake ip are swallowed; its status (0x0a) becomes our
 *   template and a media slot query (0x05) is answered with a media slot response (0x06)
 * - connect: the community profile routes private library TCP through loopback
 * - msgsnd: EP147's "FuseFilsine <fake ip> 111 <mp> C 0" command to FuseMountDaemon is
 *   rewritten into a bind mount of the player's local dir on <mp>
 * The community profile enables only the internal Link cache; the persisted
 * user Link setting stays unchanged. Its logs use the SDK. Standalone research
 * runs retain /tmp/linkfake.log.
 */
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <dlfcn.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>
#ifndef LF_NEXT
#define LF_NEXT RTLD_NEXT
#endif
static int lf_runtime_ready(void) {
#ifdef LF_COMMUNITY
    return __atomic_load_n(&community_ready,__ATOMIC_ACQUIRE)==1;
#else
    return 1;
#endif
}

static const char MAGIC[] = "Qspt1WmJOL";
static const char *MODEL = "CDJ-3000"; /* $LINKFAKE_MODEL overrides */
#define MAX_FAKES 8

struct fake {
    int player;
    int slot; /* remote media slot: SD=2, USB=3, USB2=7, USB3=8 */
    struct in_addr ip;
    char dir[256];
    char name[128];
    char port_route[96]; /* Physical route, independent of flattened display rank. */
    unsigned char device_settings[8]; /* source DEVSETTING.DAT; zero means unspecified */
    int sock;          /* loopback socket the player's packets come from */
    uint16_t port;     /* its local port, network order */
    _Atomic int link_ready; /* native status received for this member transport */
    /* LINKFAKE_HUBSRV: this player's own library servers inside EP147 */
    uint64_t srv_music;   /* music_library::MusicLibraryServer */
    uint64_t srv_info;    /* track_info_repository::PersistentStorageServer */
    _Atomic uint64_t tag_cache; /* private TaggedTrackCacheService */
    char mp[64];          /* the LINK mount point, once EP147 asked for it */
    long mount_at;        /* epoch seconds when to mount the servers on mp, 0 = not yet */
    _Atomic int mounted;
    int metadata_probe_retired;
    _Atomic int present;
    int unmount_pending;
    long removed_at;
    int notify_pending;
    long announced_at, queried_at;
#ifdef LF_SAFE_EJECT
    _Atomic int eject_reserved, eject_withdraw, eject_done, eject_dispatching;
    int eject_phase; /* Injector-owned; the worker communicates through atomics. */
    _Atomic(void *) eject_music_listener, eject_info_listener;
#endif
};

static struct fake fakes[MAX_FAKES];
static _Atomic int nfakes;
static uint8_t tmpl[2048];
static int tmpl_len;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static FILE *logf;
static void patch_slot(int fd, uint8_t *p, size_t n);
static void dump(const char *dir, int fd, const void *p, ssize_t n);
#include "link-socket-registry.h"
#include "link-private-requester.h"
static struct link_socket_registry probe_sockets=LINK_SOCKET_REGISTRY_INIT;
static unsigned char traced[4096];
static unsigned char setup_wait[4096]; /* setup sent, server number not yet read */
static int target_of_fd[4096];
static void fix_setup_reply(int fd, uint8_t *p, ssize_t r);
static void note_setup(int fd, const uint8_t *p, size_t n);
static void note_srv_read(int fd, ssize_t r, void *lr);
static int hub_slot_code(int player, int slot);
static int probe_cached_status_private(const uint8_t *,size_t);
static int probe_message_allowed(uint64_t message,int player);
static uint64_t probe_dispatch(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
static void hub_mount_poll(void);
static void probe_mode_reset_sources(time_t *,int *);

static ssize_t (*real_sendto)(int, const void *, size_t, int, const struct sockaddr *, socklen_t);
static ssize_t (*real_recvfrom)(int, void *, size_t, int, struct sockaddr *, socklen_t *);
static ssize_t (*real_recvmsg)(int, struct msghdr *, int);
static int (*real_connect)(int, const struct sockaddr *, socklen_t);
static int (*real_msgsnd)(int, const void *, size_t, int);
static ssize_t (*real_read)(int, void *, size_t);
static ssize_t (*real_write)(int, const void *, size_t);
static ssize_t (*real_send)(int, const void *, size_t, int);
static ssize_t (*real_recv)(int, void *, size_t, int);
/* traced[]: TCP fds connected to a fake player (LINKFAKE_TCP_LOG), declared above */

static void lf_log(const char *fmt, ...) {
#ifdef LF_COMMUNITY
    if (!lf_core || !lf_core->log) return;
    char message[512]; va_list args;
    va_start(args,fmt);vsnprintf(message,sizeof message,fmt,args);va_end(args);
    lf_core->log("usb-hub-support","%s",message);
#else
    if (!logf) return;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    va_list ap;
    va_start(ap, fmt);
    fprintf(logf, "%5ld.%03ld ", (long)ts.tv_sec, ts.tv_nsec / 1000000);
    vfprintf(logf, fmt, ap);
    fputc('\n', logf);
    fflush(logf);
    va_end(ap);
#endif
}

#include "network-identity.h"
#include "source-name.h"
#include "source-display.h"
#include "source-metadata.h"
#include "source-settings.h"
static unsigned char hub_default_settings[8]={1,1,1,1,1,1,1,1};
static _Atomic uint32_t hub_observed_address;
static uint32_t hub_peer_ips[2];

static struct fake *fake_by_ip(struct in_addr a) {
    for (int i = 0; i < nfakes; i++)
        if (fakes[i].ip.s_addr == a.s_addr) return &fakes[i];
    return NULL;
}

static struct fake *fake_by_source(int player, int slot) {
    for (int i=0;i<nfakes;i++)
        if (fakes[i].player==player && fakes[i].slot==slot) return &fakes[i];
    return NULL;
}
static int first_for_player(int index) {
    for (int i=0;i<index;i++) if (fakes[i].player==fakes[index].player) return 0;
    return 1;
}
static int media_sdid(int slot) {
    return slot==2 ? 3 : slot==7 ? 0x22 : slot==8 ? 0x42 : 2;
}
static const char *media_leaf(int slot) {
    return slot==2 ? "sd" : slot==7 ? "usb2" : slot==8 ? "usb3" : "usb";
}
static int mount_slot(const char *mp) {
    const char *leaf=strrchr(mp,'/');
    if (!leaf) return 0;
    for (int slot=2;slot<=8;slot++)
        if ((slot==2 || slot==3 || slot==7 || slot==8) && !strcmp(leaf+1,media_leaf(slot))) return slot;
    return 0;
}
static struct fake *fake_by_port(uint16_t port) {
    for (int i = 0; i < nfakes; i++)
        if (fakes[i].port == port) return &fakes[i];
    return NULL;
}

static const struct sockaddr_in *as_in(const struct sockaddr *sa, socklen_t len) {
    return (sa && len >= sizeof(struct sockaddr_in) && sa->sa_family == AF_INET) ? (const void *)sa : NULL;
}

static void to_loopback(struct fake *f, int port, const void *p, size_t n) {
    /* Several media slots belong to one member and must use its one transport. */
    struct fake *member=fake_by_ip(f->ip);
    if (member) f=member;
    struct sockaddr_in a = { .sin_family = AF_INET, .sin_port = htons(port) };
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    real_sendto(f->sock, p, n, 0, (void *)&a, sizeof a);
}

static void send_keepalive(struct fake *f) {
    uint8_t ka[54] = { 0 };
    memcpy(ka, MAGIC, 10);
    ka[0x0a] = 0x06;
    memcpy(ka + 0x0c, MODEL, strlen(MODEL));
    ka[0x20] = 0x01; ka[0x21] = 0x03; ka[0x22] = 0x00; ka[0x23] = 0x36;
    ka[0x24] = f->player;
    ka[0x25] = 0x01;
    uint8_t mac[6] = { 0x02, 0x0a, 0x2d, 0x00, 0x00, (uint8_t)f->player };
    memcpy(ka + 0x26, mac, 6);
    memcpy(ka + 0x2c, &f->ip, 4);
    ka[0x30] = 0x01;
    ka[0x34] = 0x01;
    to_loopback(f, 50000, ka, sizeof ka);
}

/* The deck's own status as template, turned into "player N, idle, USB loaded". */
#include "link-status.h"
static void send_status(struct fake *f) {
    uint8_t st[2048];
    pthread_mutex_lock(&lock);
    int n = tmpl_len;
    memcpy(st, tmpl, n);
    unsigned char defaults[8];memcpy(defaults,hub_default_settings,8);
    pthread_mutex_unlock(&lock);
    if (!hub_status_make_idle(st,(size_t)n)) return;

    memset(st + 0x0b, 0, 0x14);
    memcpy(st + 0x0b, MODEL, strlen(MODEL));
    st[0x21] = f->player;
    st[0x24] = f->player;
    st[0x27] = 0x00;
    memset(st + 0x28, 0, 3);
    st[0x6a] = 0x04; st[0x6f] = 0x00; st[0x73] = 0x04; st[0x75] = 0x01; st[0xb7] = 0x01;
    memcpy(st + 0x7c, "9.99", 4);
    unsigned own_slots=0;
    for(int i=0;i<nfakes;i++)
        if(fakes[i].player==f->player && fakes[i].present)
            own_slots|=1u<<fakes[i].slot;
    if(!hub_status_own_media(st,(size_t)n,own_slots))return;
    /* A local deck template is transport framing, not this peer's preferences.
     * Each media slot owns its exported settings; never rebroadcast other decks. */
    hub_settings_clear_foreign(st,(size_t)n,defaults);
    for(int i=0;i<nfakes;i++)if(fakes[i].player==f->player) {
        unsigned char row[8];hub_settings_merge(row,defaults,fakes[i].device_settings);
        hub_settings_write(st,(size_t)n,fakes[i].slot,row);
    }
    to_loopback(f, 50002, st, n);
}

/* Media slot response 0x06 (dysentery media page): D at 0x24..0x27, S at 0x28..0x2b. */
static uint64_t hub_media_fields(struct fake *f,char *name,size_t size,unsigned char *color) {
    struct hub_source_properties properties;
#ifdef LF_SAFE_EJECT
    pthread_mutex_lock(&safe_source_mutex);
#endif
    uint64_t revision=hub_metadata_read((int)(f-fakes),f->dir,f->port_route,&properties);
    hub_metadata_name(&properties,f->name,hub_source_display_number(f->player,f->slot),name,size);
    *color=properties.color_valid ? properties.color:0;
#ifdef LF_SAFE_EJECT
    pthread_mutex_unlock(&safe_source_mutex);
#endif
    return revision;
}
static void send_media(struct fake *f, int slot) {
    uint8_t r[0x24 + 0x9c] = { 0 };
    memcpy(r, MAGIC, 10);
    r[0x0a] = 0x06;
    memcpy(r + 0x0b, MODEL, strlen(MODEL));
    r[0x1f] = 0x01; r[0x21] = f->player; r[0x22] = 0x00; r[0x23] = 0x9c;
    r[0x27] = f->player;
    r[0x2b] = slot;
    char name[128];unsigned char color=0;
    hub_media_fields(f,name,sizeof name,&color);
    hub_name_utf16be(name,r+0x2c,32);
    r[0xa8]=color;
    const char *dt = "2026-10-16";
    for (int j = 0; dt[j]; j++) r[0x6c + 2 * j + 1] = dt[j];
    if (!getenv("LINKFAKE_NORB")) {
        r[0xa7] = 100;  /* tracks */
        r[0xaa] = 0x01; /* rekordbox */
        r[0xaf] = 5;    /* playlists */
    } /* LINKFAKE_NORB: advertise plain media with no rekordbox library */
    uint64_t tot = 128ULL * 1000000000ULL, fr = 64ULL * 1000000000ULL;
    for (int j = 0; j < 8; j++) { r[0xb0 + j] = tot >> (56 - 8 * j); r[0xb8 + j] = fr >> (56 - 8 * j); }
    to_loopback(f, 50002, r, sizeof r);
}

static void hub_media_refresh(struct fake *f,uint64_t *sent) {
    struct fake *member=fake_by_ip(f->ip);
    if (!f->present || !__atomic_load_n(&f->announced_at,__ATOMIC_ACQUIRE) || !member ||
        !__atomic_load_n(&member->link_ready,__ATOMIC_ACQUIRE)) return;
    char name[128];unsigned char color;
    uint64_t revision=hub_media_fields(f,name,sizeof name,&color);
    if (revision && revision!=*sent) {
        send_media(f,f->slot);
        *sent=revision;
    }
}

static void mem_scan(void);
static const char *jstr(uint64_t p);
/* The media registry the track-info servers use (PersistentStorageServer::onMediaMounted task):
 * FUN_00e69560(&provider, 1), then FUN_00e5e7d8(provider, &LocalMediaID) -> media info or NULL */
static uint64_t media_provider;
static void media_lookup(unsigned id) {
    uint64_t provider = media_provider;
    uint16_t lid = id;
    char ok = provider != 0;
    uint64_t info = ok && provider ? ((uint64_t (*)(uint64_t, uint16_t *))0xe5e7d8)(provider, &lid) : 0;
    lf_log("media %#06x: provider %#lx info %#lx", id, (unsigned long)provider, (unsigned long)info);
    if (info) {
        /* MountMediaInfoReference is only 0x20 bytes. Resolve its path through
         * the native backend instead of reading beyond the allocation. */
        uint64_t saved=*(uint64_t *)info;
        for (int pass=0;pass<2;pass++) {
            if (pass) {
                unsigned player=id>>8;
                if (player<7 || player>8) break;
                *(uint64_t *)info=((uint64_t)(0x100|(id&255))<<32)|((player-1)<<8)|2;
            }
            uint64_t fields[3]={0};
            ((void (*)(void *))0x247c9b0)(&fields[1]);
            ((void (*)(void *))0x247c9b0)(&fields[2]);
            int status=((int (*)(uint64_t,void *))0xe5fb00)(info,fields);
            lf_log("  reference=%#lx status=%d path=%.160s alternate=%.80s",
                   *(uint64_t *)info,status,jstr((uint64_t)&fields[1]),jstr((uint64_t)&fields[2]));
            ((void (*)(void *))0x2469aa0)(&fields[2]);
            ((void (*)(void *))0x2469aa0)(&fields[1]);
        }
        *(uint64_t *)info=saved;
        ((void (*)(uint64_t))0xe5f920)(info);
        free((void *)info);
    }
}
static int linkfake_load_config(void);
static void *injector(void *arg) {
    (void)arg;
    uint64_t metadata_sent[8]={0};
    for (unsigned tick = 0;; tick++) {
#ifdef LF_COMMUNITY
        int state=__atomic_load_n(&community_ready,__ATOMIC_ACQUIRE);
        if (state<0) return NULL;
        if (!state) { usleep(100000); continue; }
        if (tick%20==0) community_discovery_poll();
#endif
        if (tick % 10 == 0 && access("/tmp/lf.scan", F_OK) == 0) {
            rename("/tmp/lf.scan", "/tmp/lf.scan.req");
            mem_scan();
        }
        FILE *mq = tick % 10 == 0 ? fopen("/tmp/lf.media", "re") : NULL;
        if (mq) { /* hex LocalMediaIDs (sdid | member << 8): ask EP147's media registry */
            unsigned id;
            while (fscanf(mq, "%x", &id) == 1) media_lookup(id);
            fclose(mq);
            unlink("/tmp/lf.media");
        }
        FILE *dq = tick % 10 == 0 ? fopen("/tmp/lf.dump", "re") : NULL;
        if (dq) { /* "<hex addr> <qwords>" per line: log memory (research) */
            unsigned long at;
            int cnt;
            while (fscanf(dq, "%lx %d", &at, &cnt) == 2)
                for (int i = 0; i < cnt; i += 4) {
                    const unsigned long *q = (const unsigned long *)at + i;
                    lf_log("dump %#lx: %#lx %#lx %#lx %#lx", (unsigned long)q, q[0], q[1], q[2], q[3]);
                }
            fclose(dq);
            unlink("/tmp/lf.dump");
        }
        if(tick%20==0)for(int i=0;i<nfakes;i++) {
            unsigned char settings[8]={0};
            if(fakes[i].present)hub_settings_read(fakes[i].dir,settings);
            memcpy(fakes[i].device_settings,settings,8);
        }
        for (int i = 0; i < nfakes; i++) {
            if (tick % 15 == 0 && first_for_player(i)) send_keepalive(&fakes[i]);
            if (tick % 2 == 0 && first_for_player(i)) send_status(&fakes[i]);
            hub_media_refresh(&fakes[i],&metadata_sent[i]);
        }
#ifdef LF_PUBLIC_SHARING_EMULATOR
        public_mod_poll();
#endif
#ifdef LF_PUBLIC_SDK_PROBE
        public_sdk_probe_poll();
#endif
#ifdef LF_PUBLIC_SHARING_RUNTIME
        public_runtime_poll();
#endif
        if (tick % 10 == 0 && getenv("LINKFAKE_HUBSRV")) hub_mount_poll();
        usleep(100000);
    }
    return NULL;
}

/* Local-only mode isolates native Link datagrams while retaining loopback.
 * Check packet signature as well as address: ordinary Internet traffic passes. */
static int foreign_link_packet(const void *data,size_t size,const struct sockaddr *peer,socklen_t length) {
    const struct sockaddr_in *a=as_in(peer,length);
    return lf_runtime_ready() && getenv("LINKFAKE_LOCAL_ONLY") && a && size>=10 &&
           !memcmp(data,MAGIC,10) && (ntohl(a->sin_addr.s_addr)>>24)!=127;
}
ssize_t sendto(int fd, const void *buf, size_t n, int flags, const struct sockaddr *to, socklen_t tl) {
    if (!lf_runtime_ready()) return real_sendto(fd,buf,n,flags,to,tl);
    const struct sockaddr_in *a = as_in(to, tl);
    if (a && n==54 && !memcmp(buf,MAGIC,10) && ((const uint8_t *)buf)[10]==6 && ntohs(a->sin_port)==50000) {
        uint32_t own;memcpy(&own,(const uint8_t *)buf+0x2c,4);
        if (own) __atomic_store_n(&hub_observed_address,own,__ATOMIC_RELAXED);
    }
    struct fake *f = a ? fake_by_ip(a->sin_addr) : NULL;
    if (!f && foreign_link_packet(buf,n,to,tl)) {
        static unsigned dropped;
        if (!dropped++) lf_log("local-only: blocked physical Link transmit");
        return n;
    }
    if (!f && !to && fd >= 0 && fd < 4096 && traced[fd] && n <= 8192) {
        uint8_t tmp[8192]; /* send() on a connected traced TCP socket arrives here */
        memcpy(tmp, buf, n);
        patch_slot(fd, tmp, n);
        dump(">", fd, tmp, n);
        return real_sendto(fd, tmp, n, flags, to, tl);
    }
    if (!f && a && (ntohl(a->sin_addr.s_addr)>>24)!=127 &&
        ntohs(a->sin_port)==50002 && n==1152) {
        uint8_t external_status[1152];
        memcpy(external_status,buf,n);
        int private_locations=hub_status_hide_private_locations(external_status,n);
        if (probe_cached_status_private(external_status,n) &&
            hub_status_hide_source(external_status,n,external_status[0x28],external_status[0x29]))
            return real_sendto(fd,external_status,n,flags,to,tl);
        for (int source=0;source<nfakes;source++)
            if (hub_status_hide_source(external_status,n,fakes[source].player,fakes[source].slot)) {
                return real_sendto(fd,external_status,n,flags,to,tl);
            }
        if(private_locations) return real_sendto(fd,external_status,n,flags,to,tl);
    }
    if (!f) return real_sendto(fd, buf, n, flags, to, tl);
    const uint8_t *d = buf;
    static uint32_t seen[64];
    uint32_t key = (n > 10 ? d[0x0a] : 0xff) << 24 | ntohs(a->sin_port) << 8 | f->player;
    int k;
    for (k = 0; k < 64 && seen[k] && seen[k] != key; k++) {}
    if (k < 64 && !seen[k]) {
        seen[k] = key;
        lf_log("deck -> player %d :%d type %#x len %zu (first)", f->player, ntohs(a->sin_port),
               n > 10 ? d[0x0a] : 0xff, n);
    }
    if (n > 0x24 && !memcmp(d, MAGIC, 10)) {
        if (d[0x0a] == 0x0a && n <= sizeof tmpl) {
            pthread_mutex_lock(&lock);
            if (!tmpl_len) lf_log("status template from deck: %zu B", n);
            hub_settings_observe(hub_default_settings,d,n);
            memcpy(tmpl, d, n);
            tmpl_len = n;
            pthread_mutex_unlock(&lock);
            if (ntohs(a->sin_port)==50002 && !__atomic_exchange_n(&f->link_ready,1,__ATOMIC_RELEASE))
                lf_log("hub member ready player %d",f->player);
        } else if (d[0x0a] == 0x05 && n >= 0x30 && ntohs(a->sin_port) == 50002) {
            struct fake *source=fake_by_source(f->player,d[n-1]);
            lf_log("media query player %d slot %d -> %s",f->player,d[n-1],source ? "answered" : "unconfigured");
            if (source && source->present) {
                send_media(source,d[n-1]);
                if (__atomic_load_n(&source->announced_at,__ATOMIC_ACQUIRE)) {
                    __atomic_store_n(&source->queried_at,time(NULL),__ATOMIC_RELEASE);
                    __atomic_store_n(&source->notify_pending,0,__ATOMIC_RELEASE);
                    if (!source->mounted && !source->mount_at) source->mount_at=time(NULL)+3;
                }
            }
        }
    }
    return n; /* swallowed: nothing for a fake ip reaches the wire */
}

static void fix_source(struct sockaddr *sa, socklen_t len) {
    struct sockaddr_in *a = (struct sockaddr_in *)as_in(sa, len);
    if (!a || a->sin_addr.s_addr != htonl(INADDR_LOOPBACK)) return;
    struct fake *f = fake_by_port(a->sin_port);
    if (f) a->sin_addr.s_addr = hub_peer_ips[f->player-7];
}

#include "link-udp-ingress.h"

int connect(int fd, const struct sockaddr *to, socklen_t tl) {
    if (!lf_runtime_ready()) return real_connect(fd,to,tl);
    link_socket_forget(&probe_sockets,fd);
    if (fd >= 0 && fd < 4096) traced[fd] = 0; /* a reused fd number is a new connection */
    const struct sockaddr_in *a = as_in(to, tl);
    struct fake *f = a ? fake_by_ip(a->sin_addr) : NULL;
    int type = 0;
    socklen_t sl = sizeof type;
    if (f && getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &sl) == 0 && type == SOCK_STREAM) {
        if (!getenv("LINKFAKE_TCP_LOOP")) {
            lf_log("refused TCP connect to player %d port %d", f->player, ntohs(a->sin_port));
            errno = ECONNREFUSED;
            return -1;
        }
        /* experiment: send it to EP147's own server on the same port */
        struct sockaddr_in lo = *a;
        lo.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        struct link_socket_token ownership=link_socket_begin(&probe_sockets,fd,f->player);
        int r = real_connect(fd, (void *)&lo, sizeof lo);
        int connect_errno=errno;
        link_socket_finish(&probe_sockets,ownership,r==0 || connect_errno==EINPROGRESS);
        errno=connect_errno;
        if (fd >= 0 && fd < 4096) { traced[fd] = 1; target_of_fd[fd] = f->player; }
        lf_log("TCP connect for player %d port %d -> loopback: %d (errno %d)", f->player, ntohs(a->sin_port), r, r ? errno : 0);
        return r;
    }
    return real_connect(fd, to, tl);
}

static void backtrace_log(const char *what);
static void dump(const char *dir, int fd, const void *p, ssize_t n) {
    if (n <= 0) return;
    if (dir[0] == '>') note_setup(fd, p, n);
    if (!getenv("LINKFAKE_TCP_LOG")) return;
    const uint8_t *packet=p;
    if (dir[0]=='>' && n==20 && packet[10]==0x10 && packet[11]==0 && packet[12]==1)
        backtrace_log("client command 0001");
    char hex[3 * 96 + 1];
    int m = n < 96 ? n : 96;
    for (int i = 0; i < m; i++) sprintf(hex + 3 * i, "%02x ", ((const uint8_t *)p)[i]);
    hex[3 * m] = 0;
    lf_log("tcp fd %d %s %zd: %s", fd, dir, n, hex);
}

#ifdef LF_EJECT_UI
static void eject_input_read_note(int fd,const void *buffer,ssize_t returned);
static void eject_input_closed(int fd);
#endif
ssize_t read(int fd, void *b, size_t n) {
    if (!real_read) real_read = dlsym(LF_NEXT, "read");
    ssize_t r = real_read(fd, b, n);
    if (!lf_runtime_ready()) return r;
#ifdef LF_EJECT_UI
    eject_input_read_note(fd,b,r);
#endif
    note_srv_read(fd, r, __builtin_return_address(0));
    if (fd >= 0 && fd < 4096 && traced[fd]) { fix_setup_reply(fd, b, r); dump("<", fd, b, r); }
    return r;
}
/* LINKFAKE_SLOT=<s>: experiment, rewrite the slot byte of dbserver DMST args (11 DD MM 03 TT)
 * from 3 (USB) to <s>, so EP147's own dbserver serves one of its local slots instead. */
static void note_setup(int fd, const uint8_t *p, size_t n) {
    if (fd >= 0 && fd < 4096 && n >= 13 && p[0] == 0x11 && p[5] == 0x11 && p[6] == 0xff && p[9] == 0xfe &&
        p[10] == 0x10 && p[11] == 0 && p[12] == 0)
        setup_wait[fd] = 1;
}
/* LINKFAKE_SETUP_AS_TARGET: the server answers setup with its own number; show the fake
 * player's number to the client instead */
static void fix_setup_reply(int fd, uint8_t *p, ssize_t r) {
    if (r == 5 && fd >= 0 && fd < 4096 && setup_wait[fd] && getenv("LINKFAKE_SETUP_AS_TARGET") &&
        p[0] == 0x11 && p[1] == 0 && p[2] == 0 && p[3] == 0 && p[4] >= 1 && p[4] <= 4) {
        p[4] = target_of_fd[fd];
        setup_wait[fd] = 0;
    }
}
static void patch_slot(int fd, uint8_t *p, size_t n) {
    const char *e = getenv("LINKFAKE_SLOT"), *rq = getenv("LINKFAKE_REQ");
    /* Multiple virtual peers loop into one native DataStream. Its server-side
     * communicators are keyed by requester number, so each peer needs one. */
    int requester=fd>=0 && fd<4096 ? probe_requester_for_player(target_of_fd[fd]):0;
    /* LINKFAKE_REQ=<n>: also present the requester as player <n>, in the setup message
     * (10 00 00 ... 11 00 00 00 01) and in the DMST D byte */
    if (rq && n >= 32 && p[10] == 0x10 && p[11] == 0x00 && p[12] == 0x00 && p[22] == 0x11 && p[26] == 0x01)
        p[26] = requester;
    for (size_t i = 0; i + 5 <= n; i++)
        if (p[i] == 0x11 && p[i + 1] == 0x01 && p[i + 2] >= 1 && p[i + 2] <= 0x10 && p[i + 3] >= 1 && p[i + 3] <= 0x20 &&
            p[i + 4] <= 0x10) {
            if (e) p[i + 3] = atoi(e);
            else if (fd >= 0 && fd < 4096 && hub_slot_code(target_of_fd[fd],p[i+3]))
                p[i + 3] = hub_slot_code(target_of_fd[fd],p[i+3]); /* claimed by that player's own servers */
            if (rq) p[i + 1] = requester;
        }
}

ssize_t write(int fd, const void *b, size_t n) {
    if (!real_write) real_write = dlsym(LF_NEXT, "write");
    if (!lf_runtime_ready()) return real_write(fd,b,n);
    if (fd >= 0 && fd < 4096 && traced[fd]) {
        uint8_t tmp[4096];
        if (n <= sizeof tmp) { memcpy(tmp, b, n); patch_slot(fd, tmp, n); b = tmp; }
        dump(">", fd, b, n);
        return real_write(fd, b, n);
    }
    return real_write(fd, b, n);
}
ssize_t recv(int fd, void *b, size_t n, int fl) {
    if (!real_recv) real_recv = dlsym(LF_NEXT, "recv");
    ssize_t r = real_recv(fd, b, n, fl);
    if (!lf_runtime_ready()) return r;
    note_srv_read(fd, r, __builtin_return_address(0));
    if (fd >= 0 && fd < 4096 && traced[fd]) { fix_setup_reply(fd, b, r); dump("<", fd, b, r); }
    return r;
}
ssize_t send(int fd, const void *b, size_t n, int fl) {
    if (!real_send) real_send = dlsym(LF_NEXT, "send");
    if (!lf_runtime_ready()) return real_send(fd,b,n,fl);
    if (fd >= 0 && fd < 4096 && traced[fd]) {
        uint8_t tmp[4096];
        if (n <= sizeof tmp) { memcpy(tmp, b, n); patch_slot(fd, tmp, n); b = tmp; }
        dump(">", fd, b, n);
        return real_send(fd, b, n, fl);
    }
    return real_send(fd, b, n, fl);
}
#include <sys/uio.h>
/* writev/sendmsg on a traced fd: flatten, patch and send as one write */
static ssize_t flat_write(int fd, const struct iovec *iov, int cnt) {
    uint8_t tmp[8192];
    size_t n = 0;
    for (int i = 0; i < cnt; i++) {
        if (n + iov[i].iov_len > sizeof tmp) return -2;
        memcpy(tmp + n, iov[i].iov_base, iov[i].iov_len);
        n += iov[i].iov_len;
    }
    patch_slot(fd, tmp, n);
    dump(">", fd, tmp, n);
    if (!real_write) real_write = dlsym(LF_NEXT, "write");
    return real_write(fd, tmp, n);
}
ssize_t writev(int fd, const struct iovec *iov, int cnt) {
    static ssize_t (*real_writev)(int, const struct iovec *, int);
    if (!real_writev) real_writev = dlsym(LF_NEXT, "writev");
    if (fd >= 0 && fd < 4096 && traced[fd]) {
        ssize_t r = flat_write(fd, iov, cnt);
        if (r != -2) return r;
    }
    return real_writev(fd, iov, cnt);
}
ssize_t sendmsg(int fd, const struct msghdr *m, int fl) {
    static ssize_t (*real_sendmsg)(int, const struct msghdr *, int);
    if (!real_sendmsg) real_sendmsg = dlsym(LF_NEXT, "sendmsg");
    if (fd >= 0 && fd < 4096 && traced[fd] && m && !m->msg_name) {
        ssize_t r = flat_write(fd, m->msg_iov, m->msg_iovlen);
        if (r != -2) return r;
    }
    return real_sendmsg(fd, m, fl);
}
/* LINKFAKE_PROBE: who accepts dbserver connections, and who reads them */
static unsigned char srv_fd[4096];
static void thread_name(char *b, size_t n) {
    char path[64];
    snprintf(path, sizeof path, "/proc/self/task/%ld/comm", (long)syscall(SYS_gettid));
    FILE *f = fopen(path, "re");
    b[0] = 0;
    if (f) { if (fgets(b, n, f)) b[strcspn(b, "\n")] = 0; fclose(f); }
}
static void note_accept(int lfd, int fd, void *lr) {
    if (!logf || fd < 0 || fd >= 4096 || !getenv("LINKFAKE_PROBE")) return;
    struct sockaddr_in a;
    socklen_t al = sizeof a;
    if (getsockname(lfd, (void *)&a, &al) || a.sin_family != AF_INET) return;
    char tn[32];
    thread_name(tn, sizeof tn);
    lf_log("accept on port %d -> fd %d, caller %p thread %s", ntohs(a.sin_port), fd, lr, tn);
    srv_fd[fd] = 1;
}
int accept(int lfd, struct sockaddr *sa, socklen_t *sl) {
    static int (*real_accept)(int, struct sockaddr *, socklen_t *);
    if (!real_accept) real_accept = dlsym(LF_NEXT, "accept");
    int fd = real_accept(lfd, sa, sl);
    note_accept(lfd, fd, __builtin_return_address(0));
    return fd;
}
int accept4(int lfd, struct sockaddr *sa, socklen_t *sl, int fl) {
    static int (*real_accept4)(int, struct sockaddr *, socklen_t *, int);
    if (!real_accept4) real_accept4 = dlsym(LF_NEXT, "accept4");
    int fd = real_accept4(lfd, sa, sl, fl);
    note_accept(lfd, fd, __builtin_return_address(0));
    return fd;
}
/* frame-pointer backtrace of EP147 code (addresses below 0x4000000) */
static void backtrace_log(const char *what) {
    char line[256];
    int n = snprintf(line, sizeof line, "%s bt:", what);
    uintptr_t *fp = __builtin_frame_address(0);
    for (int i = 0; i < 12 && fp && n < (int)sizeof line - 20; i++) {
        uintptr_t ret = fp[1];
        if (ret && ret < 0x4000000) n += snprintf(line + n, sizeof line - n, " %#lx", (unsigned long)ret);
        uintptr_t *next = (uintptr_t *)fp[0];
        if (next <= fp) break;
        fp = next;
    }
    lf_log("%s", line);
}
static void note_srv_read(int fd, ssize_t r, void *lr) {
    if (fd < 0 || fd >= 4096 || !srv_fd[fd] || r <= 0) return;
    static int count;
    if (count++ < 40) {
        char tn[32], what[96];
        thread_name(tn, sizeof tn);
        snprintf(what, sizeof what, "server fd %d read %zd by %p thread %s", fd, r, lr, tn);
        backtrace_log(what);
    }
}

int close(int fd) {
#ifdef LF_EJECT_UI
    eject_input_closed(fd);
#endif
    static int (*real_close)(int);
    if (!real_close) real_close = dlsym(LF_NEXT, "close");
    link_socket_forget(&probe_sockets,fd);
    probe_udp_forget(fd);
    if (fd >= 0 && fd < 4096) srv_fd[fd] = 0;
    if (fd >= 0 && fd < 4096 && traced[fd]) { traced[fd] = 0; lf_log("tcp fd %d closed", fd); }
    return real_close(fd);
}

/* FuseMountDaemon message: long mtype; 4 bytes; command text. */
int msgsnd(int q, const void *msgp, size_t sz, int flg) {
    if (!lf_runtime_ready()) return real_msgsnd(q,msgp,sz,flg);
    const char *cmd = (const char *)msgp + sizeof(long) + 4;
    const char *key = "/home/root/pdj/FuseFilsine ";
    if (sz > 4 + strlen(key) && !strncmp(cmd, key, strlen(key))) {
        char ip[32] = "", mp[256] = "";
        struct in_addr a;
        if (sscanf(cmd + strlen(key), "%31s %*s %255s", ip, mp) == 2 && inet_pton(AF_INET, ip, &a) == 1) {
            struct fake *f = fake_by_ip(a);
            if (f) {
                f=fake_by_source(f->player,mount_slot(mp));
                if (f && !f->present) f=NULL;
                if (f && getenv("LINKFAKE_HUBSRV") && !f->announced_at)
                    __atomic_store_n(&f->notify_pending,1,__ATOMIC_RELEASE);
                char *copy = malloc(sizeof(long) + sz);
                memcpy(copy, msgp, sizeof(long) + sz);
                char *c = copy + sizeof(long) + 4;
                size_t room = sz - 4;
                memset(c, 0, room);
                /* FuseFilsine announces its mount on /proc/udev_nfs ("mount %s nfs"); that
                 * one-message mailbox is drained about once a second, so players take turns. */
                if (!f) {
                    snprintf(c,room,":"); /* unconfigured slot: never alias another stick */
                    int r=real_msgsnd(q,copy,sz,flg);
                    free(copy);
                    return r;
                }
                if (getenv("LINKFAKE_HUBSRV"))
                    snprintf(c,room,"mkdir -p %s && { mountpoint -q %s || mount -o bind,ro '%s' %s; }",mp,mp,f->dir,mp);
                else
                    snprintf(c,room,"mkdir -p %s && { mountpoint -q %s || mount -o bind,ro '%s' %s; } && "
                             "(sleep %d; printf 'mount %s nfs' > /proc/udev_nfs) &",
                             mp,mp,f->dir,mp,1+3*(int)(f-fakes),mp);
                lf_log("mount for player %d: %s", f->player, c);
                int r = real_msgsnd(q, copy, sz, flg);
                free(copy);
                return r;
            }
        }
    }
    return real_msgsnd(q, msgp, sz, flg);
}

/* ---- LINKFAKE_PROBE: log calls into EP147's library code (research) ----
 * A minimal inline hook, installed from the constructor before EP147's main runs: the first
 * 4 instructions (verified position-independent for these targets) move to a trampoline and
 * are replaced by LDR X17,#8 / BR X17 / .quad hook. */
#include <sys/mman.h>
typedef uint64_t (*fn8)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
static void *make_hook(uintptr_t target, void *hook) {
#ifdef LF_COMMUNITY
    void *original=NULL;
    if (lf_core->claim_hook("usb-hub-support",target,hook,&original)) {
        lf_hook_failures++;
        return (void *)target;
    }
    return original;
#else
    uint32_t *t = (uint32_t *)target;
    uint32_t *tr = mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (tr == MAP_FAILED) return NULL;
    memcpy(tr, t, 16);
    tr[4] = 0x58000051; /* ldr x17, #8 */
    tr[5] = 0xd61f0220; /* br x17 */
    *(uint64_t *)&tr[6] = target + 16;
    __builtin___clear_cache((char *)tr, (char *)(tr + 8));
    uintptr_t pg = target & ~0xfffUL;
    if (mprotect((void *)pg, 8192, PROT_READ | PROT_WRITE | PROT_EXEC)) return NULL;
    t[0] = 0x58000051;
    t[1] = 0xd61f0220;
    *(uint64_t *)&t[2] = (uint64_t)hook;
    __builtin___clear_cache((char *)t, (char *)(t + 4));
    mprotect((void *)pg, 8192, PROT_READ | PROT_EXEC);
    return tr;
#endif
}
static const char *jstr(uint64_t p) { /* juce::String / File: first word is the UTF-8 text */
    if (p < 0x10000) return "?";
    const char *c = *(const char **)p;
    return (uintptr_t)c > 0x10000 ? c : "?";
}
static fn8 o_mounted, o_libopen;
static uint64_t srv_seen[4]; /* library servers seen in onMediaMounted, for mem_scan */
/* /tmp/lf.scan: log every writable word equal to one of the hex values listed in the file
 * (empty file: the servers seen in onMediaMounted), plus the words around it */
static void mem_scan(void) {
    uint64_t want[4] = { 0 };
    FILE *w = fopen("/tmp/lf.scan.req", "re");
    if (w) {
        for (int i = 0; i < 4 && fscanf(w, "%lx", (unsigned long *)&want[i]) == 1; i++) {}
        fclose(w);
        unlink("/tmp/lf.scan.req");
    }
    if (!want[0]) memcpy(want, srv_seen, sizeof want);
    FILE *m = fopen("/proc/self/maps", "re");
    if (!m) return;
    char line[512];
    int hits = 0;
    while (fgets(line, sizeof line, m) && hits < 200) {
        unsigned long lo, hi;
        char perm[8];
        if (sscanf(line, "%lx-%lx %7s", &lo, &hi, perm) != 3 || perm[0] != 'r' || perm[1] != 'w') continue;
        if (strstr(line, "/dev/") || hi - lo > (512UL << 20)) continue;
        for (uint64_t *p = (uint64_t *)lo; p < (uint64_t *)hi && hits < 200; p++)
            for (int s = 0; s < 4; s++)
                if (want[s] && *p == want[s] && (uint64_t)p != (uint64_t)&want[s] &&
                    (uint64_t)p != (uint64_t)&srv_seen[s]) {
                    hits++;
                    lf_log("scan: %p = srv%d  [-2..+3] %#lx %#lx | %#lx %#lx %#lx %#lx", (void *)p, s,
                           (unsigned long)p[-2], (unsigned long)p[-1], (unsigned long)p[0], (unsigned long)p[1],
                           (unsigned long)p[2], (unsigned long)p[3]);
                }
    }
    fclose(m);
    lf_log("scan done, %d hits", hits);
}
static uint64_t h_mounted(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f, uint64_t g, uint64_t h) {
    for (int s = 0; s < 4; s++)
        if (!srv_seen[s] || srv_seen[s] == a) { srv_seen[s] = a; break; }
    lf_log("onMediaMounted this=%#lx id=%#x b=%#lx c=%#lx path=%s d=%#lx", (unsigned long)a,
           b > 0x10000 ? *(uint16_t *)b : 0, (unsigned long)b, (unsigned long)c, jstr(c), (unsigned long)d);
    const uint64_t *o = (const uint64_t *)a;
    lf_log("  this words: %#lx %#lx %#lx %#lx %#lx %#lx  caller=%p", (unsigned long)o[0], (unsigned long)o[1],
           (unsigned long)o[2], (unsigned long)o[3], (unsigned long)o[4], (unsigned long)o[5], __builtin_return_address(0));
    static int scanned;
    if (!scanned++) /* how many per-slot objects sit around this one (same vtable, 0x200 apart)? */
        for (long off = -0x1000; off <= 0x1000; off += 0x200) {
            const uint64_t *q = (const uint64_t *)(a + off);
            lf_log("  scan %+ld: %#lx %#lx", off, (unsigned long)q[0], (unsigned long)q[1]);
        }
    return o_mounted(a, b, c, d, e, f, g, h);
}
static uint64_t h_libopen(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f, uint64_t g, uint64_t h) {
    lf_log("libopen this=%#lx id=%#x c=%#lx(%s) d=%#lx(%s) e=%#lx", (unsigned long)a, b > 0x10000 ? *(uint16_t *)b : 0,
           (unsigned long)c, jstr(c), (unsigned long)d, jstr(d), (unsigned long)e);
    return o_libopen(a, b, c, d, e, f, g, h);
}
/* FUN_01116bf0(this, u8 link slot) -> sdid (3->0x02, 7->0x22, 8->0x42, ...) */
static fn8 o_slot2sdid;
static uint64_t h_slot2sdid(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f, uint64_t g, uint64_t h) {
    uint64_t r = o_slot2sdid(a, b, c, d, e, f, g, h);
    static void *seen[64];
    void *lr = __builtin_return_address(0);
    int k;
    for (k = 0; k < 64 && seen[k] && seen[k] != lr; k++) {}
    if (k < 64 && !seen[k]) {
        seen[k] = lr;
        char tn[32];
        thread_name(tn, sizeof tn);
        lf_log("slot2sdid this=%#lx slot %d -> %#x caller %p thread %s", (unsigned long)a, (int)(b & 0xff), (int)r, lr, tn);
    }
    return r;
}
/* FUN_00ef05c8(conn): reads one dbserver message, returns a 0x40 B message object that
 * FUN_00ef8258 hands to conn[2]->vtable[+0x10](msg, conn[1]) */
static fn8 o_dbparse;
static uint64_t probe_parse(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
static uint64_t h_dbparse(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f, uint64_t g, uint64_t h) {
    uint64_t r = o_dbparse(a, b, c, d, e, f, g, h);
    static int count;
    if (r && count++ < 30) {
        const uint64_t *conn = (const uint64_t *)a, *m = (const uint64_t *)r;
        uint64_t disp = conn[2], vt = disp ? *(uint64_t *)disp : 0;
        lf_log("dbmsg conn=%#lx id=%ld disp=%#lx vt=%#lx handler=%#lx", (unsigned long)a, (long)conn[1],
               (unsigned long)disp, (unsigned long)vt, vt ? (unsigned long)((uint64_t *)vt)[2] : 0);
        lf_log("  msg %#lx %#lx %#lx %#lx %#lx %#lx %#lx %#lx", (unsigned long)m[0], (unsigned long)m[1],
               (unsigned long)m[2], (unsigned long)m[3], (unsigned long)m[4], (unsigned long)m[5],
               (unsigned long)m[6], (unsigned long)m[7]);
    }
    return r;
}
/* FUN_0236da70(fmt, ...): EP147's WARN printf. Backtrace the ones naming a dbserver menu. */
static fn8 o_warn;
static uint64_t h_warn(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f, uint64_t g, uint64_t h) {
    static int count;
    if (d > 0x10000 && count < 20 && strstr((const char *)a, "%s(%d) - %s") && strstr((const char *)d, "unsupported command=CMD_") &&
        !strstr((const char *)d, "FILTER")) {
        count++;
        char what[160];
        snprintf(what, sizeof what, "WARN %s:", (const char *)d);
        backtrace_log(what);
    }
    return o_warn(a, b, c, d, e, f, g, h);
}
/* Per-player slot code for the dbserver requests (patch_slot). Not implemented: 0 = unchanged. */
#include "hubservers.h"
/* FUN_007d6e28(task): ServerMessageDispenser::receiveMessage lambda. task[0] = message,
 * task[1] = owner, whose list at +0x2c0 holds the servers tried in turn (FUN_00e6b1c8). */
static fn8 o_dispense;
static uint64_t h_dispense(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f, uint64_t g, uint64_t h) {
    if (!lf_runtime_ready()) return o_dispense(a,b,c,d,e,f,g,h);
    const uint64_t *t = (const uint64_t *)a;
    if (getenv("LINKFAKE_HUBSRV")) hub_register(t[1]);
    static uint64_t seen[16];
    int k;
    for (k = 0; k < 16 && seen[k] && seen[k] != t[1]; k++) {}
    if (k < 16 && !seen[k]) {
        seen[k] = t[1];
        uint64_t head = t[1] + 0x2c0;
        lf_log("dispenser owner %#lx vt %#lx msg type %#x", (unsigned long)t[1], *(unsigned long *)t[1],
               *(uint16_t *)t[0]);
        int n = 0;
        for (uint64_t node = *(uint64_t *)head; node != head && n < 40; node = *(uint64_t *)node, n++) {
            uint64_t s = ((uint64_t *)node)[2];
            lf_log("  [%d] %#lx vt %#lx sdid %#x/%#x", n, (unsigned long)s, *(unsigned long *)s,
                   *(uint8_t *)(s + 0xd0), *(uint8_t *)(s + 0xd1));
        }
    }
    return probe_dispatch(a, b, c, d, e, f, g, h);
}
/* FUN_00e5e7d8(provider, LocalMediaID*): media registry lookup */
static fn8 o_mlookup;
static uint64_t h_mlookup(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f, uint64_t g, uint64_t h) {
    if (!lf_runtime_ready()) return o_mlookup(a,b,c,d,e,f,g,h);
    media_provider = a;
    uint64_t r = o_mlookup(a, b, c, d, e, f, g, h);
    if (r && getenv("LINKFAKE_HUBSRV")) {
        char name[32]={0};
        pthread_getname_np(pthread_self(),name,sizeof name);
        for (int i=0;i<nfakes;i++) {
            char music[24],info[24];
            snprintf(music,sizeof music,"HubLib%d",i);
            snprintf(info,sizeof info,"HubInfo%d",i);
            if (strcmp(name,music) && strcmp(name,info) &&
                !(hub_info_library && hub_info_library==fakes[i].srv_music)) continue;
            uint16_t lid=*(uint16_t *)b;
            if (lid!=0x102) continue;
            /* LocalMediaID lookup constructs an AllInOne member. These owned
             * servers represent SinglePlayer media; preserve backend pointers
             * but give it the actual remote member and media identity. */
            *(uint64_t *)r=((uint64_t)(0x100|media_sdid(fakes[i].slot))<<32)
                         |((fakes[i].player-1)<<8)|2;
#ifdef LF_HUB_REQUEST_TRACE
            static unsigned identity_traces;
            if (__atomic_fetch_add(&identity_traces,1,__ATOMIC_RELAXED)<32) {
                uint64_t fields[3]={0};
                ((void (*)(void *))0x247c9b0)(&fields[1]);
                ((void (*)(void *))0x247c9b0)(&fields[2]);
                int status=((int (*)(uint64_t,void *))0xe5fb00)(r,fields);
                lf_log("hub identity trace source=%d thread=%s id=%#x media=%#lx status=%d path=%.160s",
                    i,name,lid,*(uint64_t *)r,status,jstr((uint64_t)&fields[1]));
                ((void (*)(void *))0x2469aa0)(&fields[2]);
                ((void (*)(void *))0x2469aa0)(&fields[1]);
            }
#endif
            break;
        }
    }
    static uint32_t seen[64];
    uint32_t key = (uint32_t)*(uint16_t *)b << 1 | (r != 0);
    int k;
    for (k = 0; k < 64 && seen[k] && seen[k] != key + 1; k++) {}
    if (k < 64 && !seen[k]) {
        seen[k] = key + 1;
        char what[96];
        snprintf(what, sizeof what, "mlookup id %#06x -> %#lx (provider %#lx)", *(uint16_t *)b, (unsigned long)r, (unsigned long)a);
        backtrace_log(what);
    }
    return r;
}
#include "browser-guard.h"
static fn8 o_request_owner_destroy;
static uint64_t h_request_owner_destroy(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    if (*(uint64_t *)(a+0x18) && *(uint32_t *)(a+0x30) && !*(uint8_t *)(a+0x34)) {
        char label[160];
        snprintf(label,sizeof label,"request owner destroy owner=%#lx request=%#lx/%#lx",a,*(uint64_t *)(a+0x28),*(uint64_t *)(a+0x30));
        backtrace_log(label);
    }
    return o_request_owner_destroy(a,b,c,d,e,f,g,h);
}
static fn8 o_cancel_request;
static uint64_t h_cancel_request(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    char label[120];
    snprintf(label,sizeof label,"cancel origin client=%#lx request=%#lx/%#lx",a,*(uint64_t *)b,*(uint64_t *)(b+8));
    backtrace_log(label);
    return o_cancel_request(a,b,c,d,e,f,g,h);
}
static fn8 o_list_cancel;
static uint64_t h_list_cancel(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    char label[160];
    snprintf(label,sizeof label,"list cancel ticket=%#lx vt=%#lx media=%#lx txn=%u state=%u",a,*(uint64_t *)a,*(uint64_t *)(a+8),*(uint32_t *)(a+0x20),*(uint8_t *)(a+0x41));
    backtrace_log(label);
    return o_list_cancel(a,b,c,d,e,f,g,h);
}
static fn8 o_track_publish;
static uint64_t h_track_publish(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    uint64_t begin=*(uint64_t *)(a+0xc0),end=*(uint64_t *)(a+0xc8);
    uint64_t listener=*(uint64_t *)(a+0x58);
    lf_log("track publish ticket=%#lx txn=%u media=%#lx flags=%#x/%#x countbytes=%lu total=%u listener=%#lx callback=%#lx",a,*(uint32_t *)(a+0x20),*(uint64_t *)(a+8),*(uint8_t *)(a+0x61),*(uint8_t *)(a+0x62),end>=begin ? end-begin : 0,*(uint32_t *)(a+0x78),listener,listener ? (*(uint64_t **)listener)[2] : 0);
    return o_track_publish(a,b,c,d,e,f,g,h);
}
static fn8 o_client_receive;
static uint64_t h_client_receive(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    uint16_t command=b ? *(uint16_t *)b : 0;
    uint32_t txn=b ? *(uint32_t *)(b+8) : 0;
    uint64_t result=o_client_receive(a,b,c,d,e,f,g,h);
    if (command==0x4001 || command==0x4201) {
        lf_log("client receive object=%#lx vt=%#lx peer=%u cmd=%#x txn=%u result=%lu",a,*(uint64_t *)a,(unsigned)c,command,txn,result);
        pthread_mutex_t *mutex=(pthread_mutex_t *)(a+0x128);
        pthread_mutex_lock(mutex);
        uint64_t *begin=*(uint64_t **)(a+0x110),*end=*(uint64_t **)(a+0x118);
        for (int n=0;begin && begin<end && n<12;begin++,n++) {
            uint64_t t=*begin;
            if (t) lf_log("  ticket=%#lx vt=%#lx member=%#x txn=%u expect=%#x sent=%#x state=%u",t,*(uint64_t *)t,*(uint32_t *)(t+8),*(uint32_t *)(t+0x20),*(uint16_t *)(t+0x1a),*(uint16_t *)(t+0x1c),*(uint8_t *)(t+0x41));
        }
        pthread_mutex_unlock(mutex);
    }
    return result;
}
#include "tag-cache.h"
#include "db-cache-probe.h"
static void probe_install(void) {
    if (getenv("LINKFAKE_HUBSRV")) hub_install();
    if (getenv("LINKFAKE_PRIVATE_TAG_CACHE")) hub_tag_cache_install();
    if (getenv("LINKFAKE_TRACE_DB_CACHE")) o_db_cache_query=make_hook(0x12552e0,h_db_cache_query);
#ifdef LF_COMMUNITY
    /* One native trampoline serves logging and private message ownership. */
    o_dbparse=make_hook(0xef05c8,probe_parse);
    o_mlookup=make_hook(0xe5e7d8,h_mlookup);
    o_dispense=make_hook(0x7d6e28,h_dispense);
    if (getenv("LINKFAKE_BROWSER_GUARD")) {
        o_browser_update=make_hook(0x201f5d8,h_browser_update);
        o_browser_reply=make_hook(0x1fc9ae0,h_browser_reply);
        o_browser_event_destroy=make_hook(0x200f0d0,h_browser_event_destroy);
    }
    if (getenv("LINKFAKE_TRACE_CLIENT")) {
        o_client_receive=make_hook(0xe67678,h_client_receive);
        o_track_publish=make_hook(0x114dff8,h_track_publish);
        if (getenv("LINKFAKE_TRACE_CANCEL")) {
            o_list_cancel=make_hook(0x1142460,h_list_cancel);
            o_cancel_request=make_hook(0xe61ff0,h_cancel_request);
            o_request_owner_destroy=make_hook(0x1fd2d08,h_request_owner_destroy);
        }
    }
#else
    if (getenv("LINKFAKE_TRACE_CLIENT")) {
        o_client_receive=make_hook(0xe67678,h_client_receive);
        o_track_publish=make_hook(0x114dff8,h_track_publish);
        o_list_cancel=make_hook(0x1142460,h_list_cancel);
        o_cancel_request=make_hook(0xe61ff0,h_cancel_request);
        if (getenv("LINKFAKE_TRACE_CANCEL")) o_request_owner_destroy=make_hook(0x1fd2d08,h_request_owner_destroy);
    }
    o_mlookup = make_hook(0xe5e7d8, h_mlookup);
    o_warn = make_hook(0x236da70, h_warn);
    o_dispense = make_hook(0x7d6e28, h_dispense);
    if (getenv("LINKFAKE_BROWSER_GUARD")) {
        o_browser_update=make_hook(0x201f5d8,h_browser_update);
        o_browser_reply=make_hook(0x1fc9ae0,h_browser_reply);
        o_browser_event_destroy=make_hook(0x200f0d0,h_browser_event_destroy);
    }
    o_dbparse = make_hook(0xef05c8, h_dbparse);
    o_mounted = make_hook(0x1373ca8, h_mounted);
    o_libopen = make_hook(0x1352ee8, h_libopen);
    o_slot2sdid = make_hook(0x1116bf0, h_slot2sdid);
    lf_log("probe hooks: mounted=%p libopen=%p", (void *)o_mounted, (void *)o_libopen);
#endif
}

static int is_ep147(void) {
    char exe[256];
    ssize_t n = readlink("/proc/self/exe", exe, sizeof exe - 1);
    if (n <= 0) return 0;
    exe[n] = 0;
    const char *b = strrchr(exe, '/');
    return b && !strcmp(b + 1, "EP147");
}

#include "source-display.h"
static int linkfake_load_config(void) {
    const char *conf=getenv("LINKFAKE_CONF") ? getenv("LINKFAKE_CONF") : "/home/root/linkfake.conf";
    FILE *c=fopen(conf,"re");
    if (!c) { lf_log("no %s, inactive",conf); return 0; }
    int parsed=nfakes;
    int automatic=getenv("LINKFAKE_AUTODISCOVER")!=NULL;
    char line[512],next_route[96]={0};unsigned next_port=0;
    while (fgets(line,sizeof line,c)) {
        if (sscanf(line,"#hub-port %u %95s",&next_port,next_route)==2) continue;
        unsigned physical_port=next_port;next_port=0;
        struct fake candidate={0};
        struct fake *f=&candidate;
        strcpy(f->port_route,next_route);next_route[0]=0;
        char ip[32]; int used=0,explicit_slot=0;
        f->slot=3;
        if (line[0]=='#') continue;
        if (sscanf(line,"%d %31s %d %255s %n",&f->player,ip,&explicit_slot,f->dir,&used)==4)
            f->slot=explicit_slot;
        else if (sscanf(line,"%d %31s %255s %n",&f->player,ip,f->dir,&used)<3) continue;
        int valid=f->dir[0]=='/';
        for (const unsigned char *ch=(const unsigned char *)f->dir;*ch;ch++)
            if (!isalnum(*ch) && !strchr("/._-",*ch)) valid=0;
        if (!valid) { lf_log("invalid source path"); continue; }
#ifdef LF_SAFE_EJECT
        if (safe_eject_blocked_path(f->dir)) continue;
#endif
        snprintf(f->name,sizeof f->name,"%s",line+used);
        f->name[strcspn(f->name,"\r\n")]=0;
        if (automatic) {
            if (physical_port<1 || physical_port>8) continue;
            int known=0,reuse=-1;
            for (int i=0;i<parsed;i++) {
                if (!strcmp(fakes[i].dir,f->dir) &&
                    (fakes[i].unmount_pending ||
                     !strcmp(fakes[i].port_route,f->port_route))) known=1;
                if (!fakes[i].present && !fakes[i].unmount_pending && reuse<0
                    && hub_source_display_number(fakes[i].player,fakes[i].slot)==(int)physical_port
#ifdef LF_SAFE_EJECT
                    && !fakes[i].eject_reserved
#endif
                ) reuse=i;
            }
            if (known) continue;
            if (reuse>=0) {
                /* Presence reconciliation reserves all still-visible sources
                 * before this pass. Native unmount and alias release finish
                 * before a dormant pair can serve another mount path. Keep
                 * its member, socket, private request code and native objects. */
                struct fake *old=&fakes[reuse];
                hub_metadata_invalidate(reuse);
                strcpy(old->dir,f->dir);
                strcpy(old->name,f->name);
                strcpy(old->port_route,f->port_route);
                old->mount_at=old->announced_at=old->queried_at=0;
                old->metadata_probe_retired=0;
                old->mounted=0; old->notify_pending=1;
                old->present=1; /* Publish replacement only after fields agree. */
                lf_log("hub source reused player %d slot %d path=%s port=%u route=%s",old->player,old->slot,old->dir,physical_port,old->port_route);
                continue;
            }
            if (parsed>=MAX_FAKES) continue;
            static const int players[]={7,7,8,8,7,8,7,8};
            static const int slots[]={3,7,3,7,8,8,2,2};
            f->player=players[parsed]; f->slot=slots[parsed];
            if (!hub_source_port_available(f->player,f->slot,physical_port)) continue;
            if (!hub_peer_ips[0] && !hub_network_peers(__atomic_load_n(&hub_observed_address,__ATOMIC_RELAXED),hub_peer_ips)) {
                /* Interface discovery can finish after mod init. Retry on the
                 * normal poll without publishing unusable fixed-subnet peers. */
                continue;
            }
            struct in_addr peer={.s_addr=htonl(0x7f4d0000u | (unsigned)f->player)};
            if (!inet_ntop(AF_INET,&peer,ip,sizeof ip)) continue;
        }
        if (parsed>=MAX_FAKES) continue;
        if (f->player<7 || f->player>8 || (f->slot!=2 && f->slot!=3 && f->slot!=7 && f->slot!=8)) {
            lf_log("invalid player/slot in config"); continue;
        }
        if (inet_pton(AF_INET,ip,&f->ip)!=1) continue;
        for (int i=0;i<parsed;i++)
            if ((fakes[i].player==f->player && fakes[i].slot==f->slot) ||
                ((fakes[i].player==f->player)!=(fakes[i].ip.s_addr==f->ip.s_addr))) valid=0;
        if (!valid) { lf_log("duplicate source or inconsistent player IP"); continue; }
        snprintf(f->mp,sizeof f->mp,"/media/player%02d/%s",f->player,media_leaf(f->slot));
        f->present=1; f->notify_pending=1;
        f->sock=socket(AF_INET,SOCK_DGRAM|SOCK_CLOEXEC,0);
        struct sockaddr_in address={.sin_family=AF_INET};
        address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
        socklen_t length=sizeof address;
        if (f->sock<0) continue;
        if (bind(f->sock,(void *)&address,sizeof address) || getsockname(f->sock,(void *)&address,&length)) {
            close(f->sock); continue;
        }
        f->port=address.sin_port;
        probe_udp_sender_add(f->sock,f->player);
        if (automatic && !hub_source_port_reserve(f->player,f->slot,physical_port)) {
            close(f->sock);continue;
        }
        /* Native initialization may already have prepared this dormant slot.
         * Keep its independent servers when publishing a newly discovered USB. */
        f->srv_music=fakes[parsed].srv_music;
        f->srv_info=fakes[parsed].srv_info;
        memcpy(&fakes[parsed],f,sizeof *f);
        lf_log("player %d slot %d at %s serves %s port=%u route=%s",f->player,f->slot,ip,f->dir,physical_port,f->port_route);
        parsed++;
    }
    fclose(c);
    nfakes=parsed; /* Publish the initialized batch to hook threads at once. */
    return parsed;
}

#ifndef LF_COMMUNITY
__attribute__((constructor))
#endif
static void linkfake_init(void) {
    real_sendto = dlsym(LF_NEXT, "sendto");
    real_recvfrom = dlsym(LF_NEXT, "recvfrom");
    real_recvmsg = dlsym(LF_NEXT, "recvmsg");
    real_connect = dlsym(LF_NEXT, "connect");
    real_msgsnd = dlsym(LF_NEXT, "msgsnd");
    real_read = dlsym(LF_NEXT, "read");
    real_write = dlsym(LF_NEXT, "write");
    real_send = dlsym(LF_NEXT, "send");
    real_recv = dlsym(LF_NEXT, "recv");
    if (!is_ep147()) return; /* children (sh, FuseFilsine...) inherit LD_PRELOAD */
#ifndef LF_COMMUNITY
    logf = fopen("/tmp/linkfake.log", "ae");
#endif
    if (getenv("LINKFAKE_MODEL")) MODEL = getenv("LINKFAKE_MODEL");
#ifndef LF_COMMUNITY
    if (getenv("LINKFAKE_PROBE")) probe_install();
#endif
#ifdef LF_COMMUNITY
    if (lf_hook_failures) return; /* Never publish sources/start injection with partial hooks. */
#endif
    linkfake_load_config();
#ifdef LF_COMMUNITY
    if (lf_hook_failures) return;
#endif
    if (nfakes || getenv("LINKFAKE_AUTODISCOVER")) {
#ifndef LF_COMMUNITY
        pthread_t t;
        if (!pthread_create(&t, NULL, injector, NULL)) pthread_detach(t);
#endif
    }
}

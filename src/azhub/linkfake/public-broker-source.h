/* SPDX-License-Identifier: GPL-3.0-only */
/* Versioned SOURCE payload. Parsing supplies no filesystem/namespace authority:
 * dispatch must compare the live mount identity and authorize slot/generation.
 * Address selection belongs to the helper's lease controller, not this request. */
#ifndef HUB_PUBLIC_BROKER_SOURCE_H
#define HUB_PUBLIC_BROKER_SOURCE_H
#include "public-broker-wire.h"
#define HUB_BROKER_SOURCE_HEADER 48
struct hub_public_broker_source {
    uint64_t identity,device,inode;
    unsigned player;
    unsigned char mac[6];
    char directory[3072],interface[16],label[256];
};
static int hub_public_broker_source_valid(const struct hub_public_broker_source *s) {
    if (!s || !s->identity || !s->device || !s->inode || !s->player || s->player>255 ||
            (s->mac[0]&3)!=2) return 0;
    size_t root=strnlen(s->directory,sizeof s->directory),
        interface=strnlen(s->interface,sizeof s->interface),label=strnlen(s->label,sizeof s->label);
    if (root<2 || root>=sizeof s->directory || s->directory[0]!='/' ||
            s->directory[root-1]=='/' || !interface || interface>=sizeof s->interface ||
            !label || label>=sizeof s->label) return 0;
    for (size_t i=0;i<interface;++i) {
        unsigned char c=s->interface[i];
        if (!((c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') || c=='_' || c=='-')) return 0;
    }
    for (const char *p=s->directory+1;*p;) {
        const char *end=strchr(p,'/');size_t n=end ? (size_t)(end-p):strlen(p);
        if (!n || (n==1 && p[0]=='.') || (n==2 && p[0]=='.' && p[1]=='.')) return 0;
        for (size_t i=0;i<n;++i) if ((unsigned char)p[i]<32 || (unsigned char)p[i]==127) return 0;
        if (!end) break;
        p=end+1;
    }
    for (size_t i=0;i<label;++i) if ((unsigned char)s->label[i]<32 || (unsigned char)s->label[i]==127) return 0;
    return 1;
}
static int hub_public_broker_source_encode(const struct hub_public_broker_source *s,
        unsigned char *out,size_t capacity,size_t *length) {
    if (!out || !length || !hub_public_broker_source_valid(s)) { errno=EINVAL;return -1; }
    size_t root=strlen(s->directory),interface=strlen(s->interface),label=strlen(s->label);
    size_t total=HUB_BROKER_SOURCE_HEADER+root+interface+label;
    if (total>capacity || total>HUB_BROKER_PAYLOAD) { errno=EMSGSIZE;return -1; }
    memset(out,0,HUB_BROKER_SOURCE_HEADER);out[0]=1;
    hub_public_broker_put(out+4,s->identity,8);hub_public_broker_put(out+12,s->device,8);
    hub_public_broker_put(out+20,s->inode,8);out[28]=(unsigned char)s->player;memcpy(out+29,s->mac,6);
    hub_public_broker_put(out+36,root,2);hub_public_broker_put(out+38,interface,2);
    hub_public_broker_put(out+40,label,2);
    memcpy(out+48,s->directory,root);memcpy(out+48+root,s->interface,interface);
    memcpy(out+48+root+interface,s->label,label);*length=total;return 0;
}
static int hub_public_broker_source_decode(const unsigned char *in,size_t length,
        struct hub_public_broker_source *out) {
    if (!in || !out || length<HUB_BROKER_SOURCE_HEADER || length>HUB_BROKER_PAYLOAD) goto bad;
    if (in[0]!=1 || in[1] || in[2] || in[3] || in[35]) goto bad;
    for (unsigned i=42;i<48;++i) if (in[i]) goto bad;
    size_t root=hub_public_broker_get(in+36,2),interface=hub_public_broker_get(in+38,2),
        label=hub_public_broker_get(in+40,2);
    struct hub_public_broker_source parsed={0};
    if (!root || root>=sizeof parsed.directory || !interface || interface>=sizeof parsed.interface ||
            !label || label>=sizeof parsed.label || 48+root+interface+label!=length ||
            memchr(in+48,0,root+interface+label)) goto bad;
    parsed.identity=hub_public_broker_get(in+4,8);parsed.device=hub_public_broker_get(in+12,8);
    parsed.inode=hub_public_broker_get(in+20,8);parsed.player=in[28];memcpy(parsed.mac,in+29,6);
    memcpy(parsed.directory,in+48,root);memcpy(parsed.interface,in+48+root,interface);
    memcpy(parsed.label,in+48+root+interface,label);
    if (!hub_public_broker_source_valid(&parsed)) goto bad;
    *out=parsed;return 0;
bad:
    errno=EPROTO;return -1;
}
/* Return an owned pinned directory, not a pathname checked then reopened.
 * Every component is opened without following symlinks. This checks object
 * identity, not that it is an authorized mount: dispatch must separately verify
 * its source inventory, generation and namespace before using the descriptor. */
static int hub_public_broker_source_open(const struct hub_public_broker_source *s) {
    if (!hub_public_broker_source_valid(s)) { errno=EINVAL;return -1; }
    char path[sizeof s->directory];strcpy(path,s->directory);
    int fd=open("/",O_RDONLY|O_DIRECTORY|O_CLOEXEC);
    if (fd<0) return -1;
    for (char *p=path+1;*p;) {
        char *end=strchr(p,'/');if (end) *end=0;
        int next=openat(fd,p,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
        int error=errno;close(fd);
        if (next<0) { errno=error;return -1; }
        fd=next;if (!end) break;p=end+1;
    }
    struct stat state;
    if (fstat(fd,&state)<0) { int error=errno;close(fd);errno=error;return -1; }
    if ((uint64_t)state.st_dev!=s->device || (uint64_t)state.st_ino!=s->inode) {
        close(fd);errno=ESTALE;return -1;
    }
    return fd;
}
/* ALLOCATE carries a root capability, distinct from SOURCE's namespace FD. */
static int hub_public_broker_source_pin(const struct hub_public_broker_source *s,int fd) {
    if (!hub_public_broker_source_valid(s)) { errno=EINVAL;return -1; }
    struct stat state;int flags=fcntl(fd,F_GETFL);
    if (flags<0 || fstat(fd,&state)<0) return -1;
    if (!S_ISDIR(state.st_mode) || (flags&O_ACCMODE)!=O_RDONLY || (flags&O_PATH)) { errno=EINVAL;return -1; }
    if ((uint64_t)state.st_dev!=s->device || (uint64_t)state.st_ino!=s->inode) { errno=ESTALE;return -1; }
    return fcntl(fd,F_DUPFD_CLOEXEC,0);
}
#endif

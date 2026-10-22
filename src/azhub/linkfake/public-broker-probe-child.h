/* SPDX-License-Identifier: GPL-3.0-only */
/* Explicit IPC-only helper probe: authenticates its parent, acknowledges a
 * namespace FD and exits on STOP. Never enters/configures a namespace or starts
 * a library. Production broker operation dispatch is intentionally absent. */
#ifndef HUB_PUBLIC_BROKER_PROBE_CHILD_H
#define HUB_PUBLIC_BROKER_PROBE_CHILD_H
#include <sys/un.h>
#include <poll.h>
#include <time.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/vfs.h>
#include "public-broker-wire.h"
static uint64_t hub_public_broker_ms(void) {
    struct timespec now;if (clock_gettime(CLOCK_MONOTONIC,&now)<0) return 0;
    return (uint64_t)now.tv_sec*1000+(uint64_t)now.tv_nsec/1000000;
}
static int hub_public_broker_token_parse(const char *hex,unsigned char out[16]) {
    if (!hex || strlen(hex)!=32) return -1;
    for (unsigned i=0;i<16;++i) {
        unsigned value=0;
        for (unsigned j=0;j<2;++j) {
            unsigned char c=hex[2*i+j];unsigned digit;
            if (c>='0' && c<='9') digit=c-'0';
            else if (c>='a' && c<='f') digit=c-'a'+10;
            else if (c>='A' && c<='F') digit=c-'A'+10;
            else return -1;
            value=value*16+digit;
        }
        out[i]=(unsigned char)value;
    }
    return memcmp(out,"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",16) ? 0:-1;
}
static int hub_public_broker_await(int fd,const unsigned char token[16],struct hub_public_broker_frame *frame) {
    uint64_t deadline=hub_public_broker_ms()+5000;
    do {
        int result=hub_public_broker_receive_token(fd,token,frame);
        if (result) return result;
        struct pollfd wait={.fd=fd,.events=POLLIN};
        if (poll(&wait,1,50)<0 && errno!=EINTR) return -1;
    } while (hub_public_broker_ms()<deadline);
    errno=ETIMEDOUT;return -1;
}
static void hub_public_broker_probe_error(int fd,const unsigned char token[16],unsigned char phase,int error) {
    struct hub_public_broker_frame report={.op=HUB_BROKER_STATUS,.generation=1,.length=1,.error=error ? (unsigned)error:EPROTO};
    memcpy(report.token,token,16);report.payload[0]=phase;hub_public_broker_send(fd,&report);
}
static int hub_public_broker_connect(int argc,char **argv,const char *mode,unsigned char token[16]) {
    const char *preload=getenv("LD_PRELOAD");if (preload && *preload) return -1;
    if (argc!=5 || strcmp(argv[1],mode) || argv[2][0]!='/' ||
            strlen(argv[2])>=sizeof ((struct sockaddr_un *)0)->sun_path) return -1;
    if (hub_public_broker_token_parse(argv[3],token)) return -1;
    unsigned long parent=0;
    if (!argv[4][0]) return -1;
    for (const char *p=argv[4];*p;++p) {
        if (*p<'0' || *p>'9' || parent>((unsigned long)INT_MAX-(unsigned)(*p-'0'))/10) return -1;
        parent=parent*10+(unsigned)(*p-'0');
    }
    if (!parent) return -1;
    char directory[108];strcpy(directory,argv[2]);char *slash=strrchr(directory,'/');
    if (!slash || slash==directory) return -1;
    *slash=0;
    struct stat dir,socket_state;
    if (lstat(directory,&dir) || !S_ISDIR(dir.st_mode) || dir.st_uid!=geteuid() || (dir.st_mode&0777)!=0700 ||
            lstat(argv[2],&socket_state) || !S_ISSOCK(socket_state.st_mode) ||
            socket_state.st_uid!=geteuid() || (socket_state.st_mode&0777)!=0600) return -1;
    int fd=socket(AF_UNIX,SOCK_SEQPACKET|SOCK_NONBLOCK|SOCK_CLOEXEC,0);if (fd<0) return -1;
    struct sockaddr_un address={.sun_family=AF_UNIX};strcpy(address.sun_path,argv[2]);
    int error=0;
    if (connect(fd,(struct sockaddr *)&address,offsetof(struct sockaddr_un,sun_path)+strlen(address.sun_path)+1)<0) {
        if (errno!=EINPROGRESS) goto failed;
        struct pollfd wait={.fd=fd,.events=POLLOUT};socklen_t size=sizeof error;
        if (poll(&wait,1,2000)!=1 || getsockopt(fd,SOL_SOCKET,SO_ERROR,&error,&size)<0 || error) goto failed;
    }
    if (hub_public_broker_peer(fd,(pid_t)parent,geteuid(),-1)) goto failed;
    return fd;
failed:
    close(fd);return -1;
}
static int hub_public_broker_probe_child(int argc,char **argv) {
    unsigned char token[16];int fd=hub_public_broker_connect(argc,argv,"--broker-probe",token),result=126;
    if (fd<0) return 126;
    struct hub_public_broker_frame message={.op=HUB_BROKER_HELLO};memcpy(message.token,token,16);
    if (hub_public_broker_send(fd,&message)) goto done;
    if (hub_public_broker_await(fd,token,&message)!=1) { hub_public_broker_probe_error(fd,token,'R',errno);goto done; }
    if (message.op!=HUB_BROKER_SOURCE || message.length!=1 || message.payload[0]!='N') {
        hub_public_broker_dispose(&message);hub_public_broker_probe_error(fd,token,'V',EPROTO);goto done;
    }
    /* A bind-mounted namespace FD may resolve to /run/netns/NAME rather than
     * net:[inode]. Validate its kernel filesystem, not that display string.
     * NSFS_MAGIC is present in the supported vendor4.4 namespace filesystem. */
    struct stat namespace;struct statfs filesystem;
    int valid=!fstat(message.descriptors[0],&namespace) && !fstatfs(message.descriptors[0],&filesystem) &&
        (unsigned long)filesystem.f_type==0x6e736673ul;
    hub_public_broker_dispose(&message);if (!valid) { hub_public_broker_probe_error(fd,token,'N',ENOTTY);goto done; }
    message.op=HUB_BROKER_STATUS;message.length=8;hub_public_broker_put(message.payload,namespace.st_ino,8);
    if (hub_public_broker_send(fd,&message)) goto done;
    if (hub_public_broker_await(fd,token,&message)!=1) goto done;
    if (message.op!=HUB_BROKER_STOP) { hub_public_broker_dispose(&message);goto done; }
    message.op=HUB_BROKER_STATUS;
    if (!hub_public_broker_send(fd,&message)) result=0;
done:
    close(fd);return result;
}
#endif

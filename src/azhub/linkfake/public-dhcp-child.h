/* SPDX-License-Identifier: GPL-3.0-only */
/* Run only in an independently exec'd helper, NEVER after fork in EP147.
 * fd3 is the nonblocking event writer; fd4 is the caller-owned namespace.
 * The launcher supplies a clean environment without native LD_PRELOAD. */
#ifndef HUB_PUBLIC_DHCP_CHILD_H
#define HUB_PUBLIC_DHCP_CHILD_H
#include <dirent.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
static int hub_public_dhcp_child(int argc,char **argv) {
    if (argc!=5 || strcmp(argv[1],"--dhcp-child") || strnlen(argv[2],IFNAMSIZ)>=IFNAMSIZ ||
            strncmp(argv[2],"azpub",5) || argv[3][0]!='/' || strlen(argv[4])!=12) return 126;
    unsigned char mac[6];
    for (unsigned i=0;i<6;++i) {
        char pair[3]={argv[4][2*i],argv[4][2*i+1],0},*end;
        unsigned long n=strtoul(pair,&end,16);if (*end || n>255) return 126;mac[i]=(unsigned char)n;
    }
    struct stat pipe_state,current,target;
    int flags=fcntl(3,F_GETFL);
    if (flags<0 || (flags&O_ACCMODE)!=O_WRONLY || !(flags&O_NONBLOCK) ||
            fstat(3,&pipe_state) || !S_ISFIFO(pipe_state.st_mode) ||
            stat("/proc/self/ns/net",&current) || fstat(4,&target) ||
            (current.st_dev==target.st_dev && current.st_ino==target.st_ino) ||
            setns(4,CLONE_NEWNET)) return 126;
    close(4);
    int fd=socket(AF_INET,SOCK_DGRAM|SOCK_CLOEXEC,0);if (fd<0) return 126;
    struct ifreq req={0};strcpy(req.ifr_name,argv[2]);
    int valid=!ioctl(fd,SIOCGIFFLAGS,&req) && (req.ifr_flags&IFF_UP) && !(req.ifr_flags&IFF_LOOPBACK);
    if (valid) valid=!ioctl(fd,SIOCGIFHWADDR,&req) && req.ifr_hwaddr.sa_family==ARPHRD_ETHER &&
        !memcmp(req.ifr_hwaddr.sa_data,mac,6);
    close(fd);if (!valid) return 126;
    /* All remaining inherited native descriptors must be closed before the
     * native client or shell hook can run. Only stdio and event fd3 survive. */
    DIR *directory=opendir("/proc/self/fd");if (!directory) return 126;
    struct dirent *entry;int own=dirfd(directory);
    while ((entry=readdir(directory))) {
        char *end;long value=strtol(entry->d_name,&end,10);
        if (!*end && value>=4 && value!=own) close((int)value);
    }
    closedir(directory);
    char client_id[32];snprintf(client_id,sizeof client_id,"0x3d:01%s",argv[4]);
    char *client[]={"/usr/sbin/udhcpc","-B","-f","-n","-q","-t","3","-T","2",
        "-i",argv[2],"-s",argv[3],"-x",client_id,NULL};
    execv(client[0],client);return 127;
}
#endif

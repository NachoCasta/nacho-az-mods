/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HUB_PUBLIC_HELPER_LOGS_H
#define HUB_PUBLIC_HELPER_LOGS_H
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Keep the newest completed session before creating the next random-named log.
 * The SDK caps/rotates each session's log once. Preserve names so diagnostics
 * can still distinguish sessions. Unknown names, links and directories belong
 * to the user, not this retention policy. Never recurse or follow a link. */
static int hub_public_helper_log_name(const char *name,char base[52]) {
    size_t length=strlen(name);
    if ((length!=51 && length!=53) || strncmp(name,"public-sharing-",15) ||
        memcmp(name+47,".log",4) || (length==53 && strcmp(name+51,".1"))) return 0;
    for(unsigned i=15;i<47;++i)
        if (!((name[i]>='0' && name[i]<='9') || (name[i]>='a' && name[i]<='f'))) return 0;
    memcpy(base,name,51);base[51]=0;return 1;
}
static int hub_public_helper_logs_prepare(const char *data) {
    int fd=open(data,O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
    if(fd<0) return -1;
    DIR *directory=fdopendir(fd);
    if(!directory) { int error=errno;close(fd);errno=error;return -1; }
    char keep[52]={0};struct timespec newest={0};
    for(unsigned pass=0;pass<2;++pass) {
        if(pass) rewinddir(directory);
        for(;;) {
            errno=0;struct dirent *entry=readdir(directory);
            if(!entry) { if(errno) goto failed;break; }
            char base[52];struct stat state;
            if(!hub_public_helper_log_name(entry->d_name,base)) continue;
            if(fstatat(fd,entry->d_name,&state,AT_SYMLINK_NOFOLLOW)) {
                if(errno==ENOENT) continue;
                goto failed;
            }
            if(!S_ISREG(state.st_mode) || state.st_nlink!=1) continue;
            if(!pass) {
                if(!*keep || state.st_mtim.tv_sec>newest.tv_sec ||
                    (state.st_mtim.tv_sec==newest.tv_sec &&
                     (state.st_mtim.tv_nsec>newest.tv_nsec ||
                      (state.st_mtim.tv_nsec==newest.tv_nsec && strcmp(base,keep)>0)))) {
                    memcpy(keep,base,sizeof keep);newest=state.st_mtim;
                }
            } else if(strcmp(base,keep) && unlinkat(fd,entry->d_name,0) && errno!=ENOENT) goto failed;
        }
    }
    return closedir(directory);
failed: {
    int error=errno;closedir(directory);errno=error;return -1;
}
}
#endif

/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
static int deny_unlink;
static int controlled_unlinkat(int fd,const char *name,int flags) {
    if(deny_unlink) { errno=EIO;return -1; }
    return unlinkat(fd,name,flags);
}
#define unlinkat controlled_unlinkat
#include "public-helper-logs.h"
#undef unlinkat
static void name_for(char name[64],unsigned session,int rotated) {
    snprintf(name,64,"public-sharing-%032x.log%s",session,rotated?".1":"");
}
static void create_log(int fd,unsigned session,int rotated) {
    char name[64];name_for(name,session,rotated);
    int file=openat(fd,name,O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,0600);assert(file>=0);
    assert(write(file,"log",3)==3);assert(!close(file));
    struct timespec times[2]={{session,0},{session,0}};
    assert(!utimensat(fd,name,times,0));
}
static int present(int fd,unsigned session,int rotated) {
    char name[64];name_for(name,session,rotated);struct stat state;
    return !fstatat(fd,name,&state,AT_SYMLINK_NOFOLLOW);
}
int main(void) {
    char base[52];assert(!hub_public_helper_log_name("public-sharing-x.log",base));
    assert(!hub_public_helper_log_name("public-sharing-0000000000000000000000000000000Z.log",base));
    assert(!hub_public_helper_log_name("public-sharing-00000000000000000000000000000001.log.2",base));
    char root[]="/tmp/azhub-log-retention-XXXXXX";assert(mkdtemp(root));
    int fd=open(root,O_RDONLY|O_DIRECTORY|O_CLOEXEC);assert(fd>=0);
    assert(!hub_public_helper_logs_prepare(root));
    /* Each launch retains the previous pair, then SDK creates the next pair. */
    for(unsigned session=1;session<=40;++session) {
        assert(!hub_public_helper_logs_prepare(root));
        for(unsigned old=1;old+1<session;++old) assert(!present(fd,old,0) && !present(fd,old,1));
        if(session>1) assert(present(fd,session-1,0) && present(fd,session-1,1));
        create_log(fd,session,0);create_log(fd,session,1);
    }
    assert(!hub_public_helper_logs_prepare(root));
    char link_name[64],directory_name[64],hard_name[64];
    name_for(link_name,100,0);name_for(directory_name,101,0);name_for(hard_name,102,0);
    assert(!symlinkat("outside-unrelated-log",fd,link_name));
    assert(!mkdirat(fd,directory_name,0700));
    int unrelated=openat(fd,"unrelated.txt",O_WRONLY|O_CREAT|O_EXCL,0600);assert(unrelated>=0);close(unrelated);
    assert(!linkat(fd,"unrelated.txt",fd,hard_name,0));
    create_log(fd,41,0);deny_unlink=1;
    assert(hub_public_helper_logs_prepare(root)<0 && errno==EIO);
    assert(present(fd,40,0) && present(fd,41,0));
    deny_unlink=0;assert(!hub_public_helper_logs_prepare(root));
    assert(!present(fd,40,0) && !present(fd,40,1) && present(fd,41,0));
    struct stat state;assert(!fstatat(fd,link_name,&state,AT_SYMLINK_NOFOLLOW) && S_ISLNK(state.st_mode));
    assert(!fstatat(fd,directory_name,&state,AT_SYMLINK_NOFOLLOW) && S_ISDIR(state.st_mode));
    assert(!fstatat(fd,hard_name,&state,AT_SYMLINK_NOFOLLOW) && state.st_nlink==2);
    char alias[256];snprintf(alias,sizeof alias,"%s-link",root);assert(!symlink(root,alias));
    assert(hub_public_helper_logs_prepare(alias)<0);assert(!unlink(alias));
    /* Remove only this test's explicitly created entries. */
    char last[64];name_for(last,41,0);assert(!unlinkat(fd,last,0));
    assert(!unlinkat(fd,link_name,0));assert(!unlinkat(fd,hard_name,0));assert(!unlinkat(fd,"unrelated.txt",0));
    assert(!unlinkat(fd,directory_name,AT_REMOVEDIR));assert(!close(fd));assert(!rmdir(root));
    puts("PASS: forty bounded sessions, paired rotation retention, strict names, link/directory preservation, pinned directory and fail-closed cleanup");
}

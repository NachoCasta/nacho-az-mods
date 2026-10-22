/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <unistd.h>
static const char *release="4.4.194",*executable="/usb/mod/assets/helper";
static int registered,loads,closed,open_error,stat_error,regular=1,load_error,race_registered,hide_success;
static int fake_uname(struct utsname *state) { memset(state,0,sizeof *state);strcpy(state->release,release);return 0; }
static int fake_access(const char *path,int mode) {
    assert(!strcmp(path,"/sys/module/usb_hub_macvlan") && mode==F_OK);
    if (registered) return 0;errno=ENOENT;return -1;
}
static ssize_t fake_readlink(const char *path,char *out,size_t size) {
    assert(!strcmp(path,"/proc/self/exe"));size_t length=strlen(executable);
    if (length>size) length=size;memcpy(out,executable,length);return (ssize_t)length;
}
static int fake_open(const char *path,int flags,...) {
    assert(!strcmp(path,"/usb/mod/assets"));
    assert((flags&(O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW))==(O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW));
    return 10;
}
static int fake_openat(int directory,const char *path,int flags,...) {
    assert(directory==10 && !strcmp(path,"usb_hub_macvlan.ko"));
    assert((flags&(O_CLOEXEC|O_NOFOLLOW))==(O_CLOEXEC|O_NOFOLLOW));
    if (open_error) { errno=open_error;return -1; }return 11;
}
static int fake_close(int fd) { assert(fd==10 || fd==11);closed|=1<<(fd-10);return 0; }
static int fake_fstat(int fd,struct stat *state) {
    assert(fd==11);if (stat_error) { errno=stat_error;return -1; }
    memset(state,0,sizeof *state);state->st_mode=regular ? S_IFREG:S_IFDIR;return 0;
}
static long fake_syscall(long call,...) {
    assert(call==SYS_finit_module);va_list ap;va_start(ap,call);
    assert(va_arg(ap,int)==11);assert(!strcmp(va_arg(ap,const char *),""));assert(va_arg(ap,int)==0);va_end(ap);
    ++loads;
    if (load_error) { registered=race_registered;errno=load_error;return -1; }
    registered=!hide_success;return 0;
}
#define uname fake_uname
#define access fake_access
#define readlink fake_readlink
#define open fake_open
#define openat fake_openat
#define close fake_close
#define fstat fake_fstat
#define syscall fake_syscall
#include "public-macvlan-asset.h"
int main(void) {
    release="6.6.138-dirty";assert(hub_public_macvlan_asset()<0 && errno==ENOTSUP && !loads);
    release="4.4.195";assert(hub_public_macvlan_asset()<0 && !loads);
    release="4.4.194";registered=1;assert(!hub_public_macvlan_asset() && !loads && !closed);
    registered=0;executable="/usb/mod/data/helper";
    assert(hub_public_macvlan_asset()<0 && errno==EACCES && !loads);
    executable="/helper";assert(hub_public_macvlan_asset()<0 && errno==EINVAL && !loads);
    executable="/usb/mod/assets/helper";open_error=ENOENT;
    assert(hub_public_macvlan_asset()<0 && errno==ENOENT && !loads && closed==1);
    open_error=ELOOP;assert(hub_public_macvlan_asset()<0 && errno==ELOOP && !loads);
    open_error=0;stat_error=EIO;
    assert(hub_public_macvlan_asset()<0 && errno==EIO && !loads && closed==3);
    stat_error=0;regular=0;assert(hub_public_macvlan_asset()<0 && errno==EINVAL && !loads);
    regular=1;load_error=ENOEXEC;
    assert(hub_public_macvlan_asset()<0 && errno==ENOEXEC && loads==1 && closed==3);
    load_error=EEXIST;assert(hub_public_macvlan_asset()<0 && errno==EEXIST && loads==2);
    race_registered=1;assert(!hub_public_macvlan_asset() && loads==3);
    registered=0;load_error=0;hide_success=1;
    assert(hub_public_macvlan_asset()<0 && errno==EIO && loads==4);
    hide_success=0;assert(!hub_public_macvlan_asset() && loads==5 && closed==3);
    puts("PASS: exact target kernel, pinned assets directory, no-follow/regular module, no force, verified registration and descriptor cleanup (mocked syscalls)");
}

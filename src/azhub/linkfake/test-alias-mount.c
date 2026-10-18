/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <mntent.h>
static int mounted,wrong_source,busy,observe_error,readonly_error,parent_link;
static int binds,remounts,unmounts;
static const char *alias="/media/player03/usb";
static FILE *test_setmntent(const char *path,const char *mode) {
    (void)path;(void)mode;
    if(observe_error) return NULL;
    static char contents[128];
    snprintf(contents,sizeof contents,"%s",mounted ? "/dev/sdb1 /media/player03/usb vfat rw 0 0\n":"");
    return fmemopen(contents,strlen(contents),"r");
}
static int test_stat(const char *path,struct stat *st) {
    memset(st,0,sizeof *st); st->st_mode=S_IFDIR|0755;
    st->st_dev=11;st->st_ino=22;
    if(!strcmp(path,alias) && mounted && wrong_source) {st->st_dev=33;st->st_ino=44;}
    return 0;
}
static int test_lstat(const char *path,struct stat *st) {
    test_stat(path,st);
    if(parent_link && !strcmp(path,"/media/player03")) st->st_mode=S_IFLNK|0777;
    return 0;
}
static int test_mkdir(const char *path,mode_t mode) {(void)path;(void)mode;return 0;}
static int test_mount(const char *source,const char *target,const char *type,unsigned long flags,const void *data) {
    (void)type;(void)data;assert(!strcmp(target,alias));
    if(flags&MS_REMOUNT) {
        assert(!source && flags==(MS_BIND|MS_REMOUNT|MS_RDONLY));remounts++;
        return readonly_error ? -1:0;
    }
    assert(source && !strcmp(source,"/media/usb/sdb1") && flags==MS_BIND);
    binds++;mounted=1;wrong_source=0;return 0;
}
static int test_umount(const char *path) {
    assert(!strcmp(path,alias));unmounts++;
    if(busy) {errno=EBUSY;return -1;}
    mounted=0;return 0;
}
#define setmntent test_setmntent
#define stat(path,st) test_stat(path,st)
#define lstat test_lstat
#define mkdir test_mkdir
#define mount test_mount
#define umount test_umount
#include "alias-mount.h"
int main(void) {
    assert(!hub_alias_valid("/media/usb/sda1"));
    assert(!hub_alias_valid("/media/player03/usb/../usb2"));
    assert(!hub_alias_valid("/media/player01/usb"));
    assert(hub_alias_valid("/media/player04/usb3"));
    observe_error=1;assert(hub_alias_prepare("/media/usb/sdb1",alias)<0 && !binds);observe_error=0;
    mounted=wrong_source=busy=1;
    assert(hub_alias_prepare("/media/usb/sdb1",alias)<0 && !binds && mounted);
    busy=0;
    assert(!hub_alias_prepare("/media/usb/sdb1",alias) && binds==1 && remounts==1 && !wrong_source);
    assert(!hub_alias_prepare("/media/usb/sdb1",alias) && binds==1);
    assert(!hub_alias_release(alias) && !mounted);
    readonly_error=1;
    assert(hub_alias_prepare("/media/usb/sdb1",alias)<0 && !mounted);
    readonly_error=0;parent_link=1;
    int previous=binds;
    assert(hub_alias_prepare("/media/usb/sdb1",alias)<0 && binds==previous);
    assert(hub_alias_release("/media/usb/sda1")<0);
    puts("alias mounts: owned paths, failed observation, busy old source, source replacement, read-only bind, rollback and symlink refusal passed");
}

/* SPDX-License-Identifier: GPL-3.0-only */
/* Worker-only library aliases. Never fork the EP147 process to manage mounts. */
#ifndef HUB_ALIAS_MOUNT_H
#define HUB_ALIAS_MOUNT_H
#include <sys/mount.h>
#include <sys/stat.h>
#include <mntent.h>
#include <errno.h>
#include <string.h>
static int hub_alias_valid(const char *path) {
    if (!path || (strncmp(path,"/media/player03/",16) && strncmp(path,"/media/player04/",16))) return 0;
    const char *leaf=path+16;
    return !strcmp(leaf,"sd") || !strcmp(leaf,"usb") || !strcmp(leaf,"usb2") || !strcmp(leaf,"usb3");
}
static int hub_alias_mounted(const char *path) {
    FILE *table=setmntent("/proc/mounts","re");
    if (!table) return -1;
    struct mntent entry,*m; char buffer[8192]; int found=0;
    while ((m=getmntent_r(table,&entry,buffer,sizeof buffer)))
        if (!strcmp(m->mnt_dir,path)) { found=1; break; }
    if (ferror(table)) found=-1;
    endmntent(table); return found;
}
static int hub_alias_release(const char *path) {
    if (!hub_alias_valid(path)) return -1;
    int mounted=hub_alias_mounted(path);
    if (mounted<0 || (mounted && umount(path))) return -1;
    return hub_alias_mounted(path)==0 ? 0:-1;
}
static int hub_alias_directory(const char *path) {
    if (mkdir(path,0755) && errno!=EEXIST) return -1;
    struct stat st;
    return !lstat(path,&st) && S_ISDIR(st.st_mode) ? 0:-1;
}
static int hub_alias_prepare(const char *source,const char *path) {
    if (!hub_alias_valid(path)) return -1;
    struct stat backing,alias;
    if (stat(source,&backing) || !S_ISDIR(backing.st_mode)) return -1;
    int mounted=hub_alias_mounted(path);
    if (mounted<0) return -1;
    if (mounted) {
        if (!stat(path,&alias) && backing.st_dev==alias.st_dev && backing.st_ino==alias.st_ino) return 0;
        if (hub_alias_release(path)) return -1;
    }
    char parent[32]; memcpy(parent,path,15); parent[15]=0;
    if (hub_alias_directory(parent) || hub_alias_directory(path)) return -1;
    if (mount(source,path,NULL,MS_BIND,NULL)) return -1;
    /* A read-only bind requires a remount on Linux; never remount the backing USB. */
    if (mount(NULL,path,NULL,MS_BIND|MS_REMOUNT|MS_RDONLY,NULL)) {
        umount(path); return -1;
    }
    if (stat(path,&alias) || backing.st_dev!=alias.st_dev || backing.st_ino!=alias.st_ino) {
        umount(path); return -1;
    }
    return 0;
}
#endif

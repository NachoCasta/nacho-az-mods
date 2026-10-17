/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef AZHUB_MOUNT_WORKER_H
#define AZHUB_MOUNT_WORKER_H
#include <dirent.h>
#include <spawn.h>
#include <sys/mount.h>
#include <sys/wait.h>
/* Stock udev handles sda..sdf. Restrict fallback mounting to names it misses. */
static int hub_fallback_device(const char *name) {
    if (strncmp(name,"sd",2)) return 0;
    const char *p=name+2,*letters=p;
    while (*p>='a' && *p<='z') p++;
    if (p==letters || (p-letters==1 && *letters<='f')) return 0;
    while (*p>='0' && *p<='9') p++;
    return !*p;
}
static int hub_device_mounted(const char *device) {
    FILE *table=setmntent("/proc/mounts","r");
    if (!table) return 1; /* Fail closed on observation errors. */
    struct mntent *m,entry; char buffer[8192]; int found=0;
    while ((m=getmntent_r(table,&entry,buffer,sizeof buffer))) if (!strcmp(m->mnt_fsname,device)) { found=1; break; }
    endmntent(table); return found;
}
static int hub_open_device(dev_t device) {
    DIR *fds=opendir("/proc/self/fd");
    if (!fds) return 1;
    struct dirent *entry; int busy=0;
    while ((entry=readdir(fds))) {
        char path[128]; struct stat st;
        snprintf(path,sizeof path,"/proc/self/fd/%s",entry->d_name);
        if (!stat(path,&st) && st.st_dev==device) { busy=1; break; }
    }
    closedir(fds); return busy;
}
static void hub_cleanup_missing_mounts(void) {
    struct {char device[64],mount[256];} stale[32]; unsigned count=0;
    FILE *table=setmntent("/proc/mounts","r"); if (!table) return;
    struct mntent *m,entry; char buffer[8192];
    while ((m=getmntent_r(table,&entry,buffer,sizeof buffer)) && count<32) {
        if (strncmp(m->mnt_fsname,"/dev/",5) || !hub_fallback_device(m->mnt_fsname+5)) continue;
        char expected[256],node[128];
        snprintf(expected,sizeof expected,"/media/usb/%s",m->mnt_fsname+5);
        snprintf(node,sizeof node,"/sys/class/block/%s",m->mnt_fsname+5);
        if (strcmp(expected,m->mnt_dir) || !access(node,F_OK)) continue;
        snprintf(stale[count].device,sizeof stale[count].device,"%s",m->mnt_fsname);
        snprintf(stale[count++].mount,256,"%s",expected);
    }
    endmntent(table);
    for (unsigned i=0;i<count;i++) {
        struct stat st; if (stat(stale[i].mount,&st) || hub_open_device(st.st_dev)) continue;
        /* Native aliases must disappear before the backing mount is released. */
        int aliases=0; table=setmntent("/proc/mounts","r"); if (!table) continue;
        while ((m=getmntent_r(table,&entry,buffer,sizeof buffer))) if (!strcmp(m->mnt_fsname,stale[i].device) && strcmp(m->mnt_dir,stale[i].mount)) aliases=1;
        endmntent(table);
        if (!aliases && !umount(stale[i].mount)) {
            rmdir(stale[i].mount);
            lf_log("fallback hub mount released %s",stale[i].mount);
        }
    }
}
static void *community_mount_worker(void *unused) {
    (void)unused;
    for (;;) {
        hub_cleanup_missing_mounts();
        const char *assets=lf_core->assets_dir("hub-libraries");
        char helper[1024];
        if (assets && snprintf(helper,sizeof helper,"%s/mount-hub.sh",assets)<(int)sizeof helper && !access(helper,R_OK)) {
            DIR *blocks=opendir("/sys/class/block"); struct dirent *entry;
            if (blocks) {
                while ((entry=readdir(blocks))) {
                    if (!hub_fallback_device(entry->d_name)) continue;
                    char node[PATH_MAX],resolved[PATH_MAX],device[128];
                    snprintf(node,sizeof node,"/sys/class/block/%s",entry->d_name);
                    if (!realpath(node,resolved) || !azhub_nested_usb_path(resolved)) continue;
                    snprintf(device,sizeof device,"/dev/%s",entry->d_name);
                    if (hub_device_mounted(device)) continue;
                    /* A small explicit environment keeps the preload out of the helper. */
                    char *args[]={"sh",helper,entry->d_name,NULL};
                    char *environment[]={"PATH=/usr/sbin:/usr/bin:/sbin:/bin",NULL};
                    pid_t child; int status;
                    if (!posix_spawn(&child,"/bin/sh",NULL,NULL,args,environment))
                        while (waitpid(child,&status,0)<0 && errno==EINTR) {}
                }
                closedir(blocks);
            }
        }
        sleep(3);
    }
    return NULL;
}
#endif

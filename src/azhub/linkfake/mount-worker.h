/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef AZHUB_MOUNT_WORKER_H
#define AZHUB_MOUNT_WORKER_H
#include <dirent.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <stdatomic.h>
/* A timed-out external mount can outlive its stop request. Keep eject blocked
 * across application reloads; only an OS restart clears this /run marker. */
#ifndef HUB_MOUNT_FAULT_FILE
#define HUB_MOUNT_FAULT_FILE "/run/azhub-mount-fault"
#endif
static _Atomic int hub_mount_fault;
static int hub_mount_faulted(void) { return atomic_load_explicit(&hub_mount_fault,memory_order_acquire); }
static void hub_mount_restore_fault(void) {
    struct stat st;
    if (!lstat(HUB_MOUNT_FAULT_FILE,&st) || errno!=ENOENT)
        atomic_store_explicit(&hub_mount_fault,1,memory_order_release);
}
static void hub_mount_latch_fault(void) {
    atomic_store_explicit(&hub_mount_fault,1,memory_order_release);
    int fd=open(HUB_MOUNT_FAULT_FILE,O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);
    if (fd>=0) close(fd);
    else if (errno!=EEXIST) lf_log("Could not preserve mount timeout across reload; leave hub connected and restart AZ");
}
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
    FILE *table=setmntent("/proc/mounts","re");
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
        char path[sizeof entry->d_name+16]; struct stat st;
        snprintf(path,sizeof path,"/proc/self/fd/%s",entry->d_name);
        if (!stat(path,&st) && st.st_dev==device) { busy=1; break; }
    }
    closedir(fds); return busy;
}
static void hub_cleanup_missing_mounts(void) {
    struct {char device[64],mount[256];} stale[32]; unsigned count=0;
    FILE *table=setmntent("/proc/mounts","re"); if (!table) return;
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
        int aliases=0; table=setmntent("/proc/mounts","re"); if (!table) continue;
        while ((m=getmntent_r(table,&entry,buffer,sizeof buffer))) if (!strcmp(m->mnt_fsname,stale[i].device) && strcmp(m->mnt_dir,stale[i].mount)) aliases=1;
        endmntent(table);
        if (!aliases && !umount(stale[i].mount)) {
            rmdir(stale[i].mount);
            lf_log("fallback hub mount released %s",stale[i].mount);
        }
    }
}
/* SDK argv contains only arguments: launch() supplies the executable itself. */
static void hub_run_mount_helper(const char *helper,const char *device) {
    if (hub_mount_faulted() || !lf_core || !lf_core->launch || !lf_core->launch_alive || !lf_core->launch_stop) return;
    const char *args[]={device,NULL};
    int child=lf_core->launch("usb-hub-support",helper,args,NULL);
    if (child<0) return;
    unsigned waits=0; int alive;
    while ((alive=lf_core->launch_alive("usb-hub-support",child))>0 && waits++<100)
        usleep(100000);
    if (alive<0 || waits>100) {
        hub_mount_latch_fault();
        lf_core->launch_stop("usb-hub-support",child);
        lf_log("fallback hub helper %s for %s",alive<0 ? "state unavailable":"timed out",device);
    }
}
static void *community_mount_worker(void *unused) {
    (void)unused;
    for (;;) {
#ifdef LF_COMMUNITY
        int state=__atomic_load_n(&community_ready,__ATOMIC_ACQUIRE);
        if (state<0) return NULL;
        if (!state) { sleep(3); continue; }
#endif
        if (hub_mount_faulted()) { sleep(3); continue; }
        hub_cleanup_missing_mounts();
        const char *assets=lf_core->assets_dir("usb-hub-support");
        char helper[1024];
        if (assets && snprintf(helper,sizeof helper,"%s/mount-hub.sh",assets)<(int)sizeof helper && !access(helper,R_OK)) {
            DIR *blocks=opendir("/sys/class/block"); struct dirent *entry;
            if (blocks) {
                while ((entry=readdir(blocks))) {
                    if (!hub_fallback_device(entry->d_name)) continue;
#ifdef LF_SAFE_EJECT
                    pthread_mutex_lock(&safe_mount_mutex);
                    if (safe_eject_blocked_device(entry->d_name)) { pthread_mutex_unlock(&safe_mount_mutex); continue; }
#endif
                    char node[PATH_MAX],resolved[PATH_MAX],device[sizeof entry->d_name+6];
                    snprintf(node,sizeof node,"/sys/class/block/%s",entry->d_name);
                    if (!realpath(node,resolved) || !azhub_nested_usb_path(resolved)) {
#ifdef LF_SAFE_EJECT
                        pthread_mutex_unlock(&safe_mount_mutex);
#endif
                        continue;
                    }
                    snprintf(device,sizeof device,"/dev/%s",entry->d_name);
                    if (hub_device_mounted(device)) {
#ifdef LF_SAFE_EJECT
                        pthread_mutex_unlock(&safe_mount_mutex);
#endif
                        continue;
                    }
                    /* The SDK launches the helper outside EP147 and strips its preload.
                     * A timeout latches an eject fault before releasing mount admission. */
                    hub_run_mount_helper(helper,entry->d_name);
#ifdef LF_SAFE_EJECT
                    pthread_mutex_unlock(&safe_mount_mutex);
#endif
                }
                closedir(blocks);
            }
        }
        sleep(3);
    }
    return NULL;
}
#endif

/* SPDX-License-Identifier: GPL-3.0-only */
/* Independent helper-side root admission. Paths come from trusted helper policy,
 * never IPC. MAC/player authorization remains a separate admission gate. */
#ifndef HUB_PUBLIC_BROKER_ROOT_POLICY_H
#define HUB_PUBLIC_BROKER_ROOT_POLICY_H
#include "public-broker-source.h"
#include "discovery.h"
static int hub_public_broker_root_policy(const struct hub_public_broker_source *source,int root,
        const char *usb_root,const char *mounts,const char *sysblock) {
    if (!source || !usb_root || !mounts || !sysblock || !hub_public_broker_source_valid(source)) { errno=EINVAL;return -1; }
    size_t prefix=strlen(usb_root);
    if (!prefix || strncmp(source->directory,usb_root,prefix) || source->directory[prefix]!='/') { errno=EPERM;return -1; }
    const char *name=source->directory+prefix+1;
    if (strncmp(name,"sd",2)) { errno=EPERM;return -1; }
    const char *p=name+2;while (*p>='a' && *p<='z') ++p;
    if (p==name+2) { errno=EPERM;return -1; }
    if (*p=='0' || !*p) { errno=EPERM;return -1; }
    while (*p>='0' && *p<='9') ++p;
    if (*p || strlen(name)>=64) { errno=EPERM;return -1; }
    int current=hub_public_broker_source_open(source);if (current<0) return -1;
    struct stat pinned,live;
    int same=!fstat(root,&pinned) && !fstat(current,&live) && pinned.st_dev==live.st_dev && pinned.st_ino==live.st_ino;
    close(current);if (!same) { errno=ESTALE;return -1; }
    FILE *table=setmntent(mounts,"re");if (!table) return -1;
    struct mntent entry,*m;char buffer[8192],device[80];unsigned matches=0;int valid=0;
    snprintf(device,sizeof device,"/dev/%s",name);
    while ((m=getmntent_r(table,&entry,buffer,sizeof buffer))) {
        if (strcmp(m->mnt_dir,source->directory)) continue;
        ++matches;valid=!strcmp(m->mnt_fsname,device);
    }
    int failed=ferror(table) || !feof(table);endmntent(table);
    if (failed || matches!=1 || !valid) { errno=EPERM;return -1; }
    char node[PATH_MAX],topology[PATH_MAX];
    if (snprintf(node,sizeof node,"%s/%s",sysblock,name)>=(int)sizeof node ||
            !realpath(node,topology) || !azhub_nested_usb_path(topology)) { errno=EPERM;return -1; }
    /* Reopen after mount/topology inspection to reject replacement during that
     * window. The retained capability, not the path, owns subsequent service IO. */
    current=hub_public_broker_source_open(source);if (current<0) return -1;
    same=!fstat(current,&live) && pinned.st_dev==live.st_dev && pinned.st_ino==live.st_ino;
    close(current);if (!same) { errno=ESTALE;return -1; }
    return 0;
}
#endif

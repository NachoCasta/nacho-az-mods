/* SPDX-License-Identifier: GPL-3.0-only */
/* Match the native library layout without renaming anything on the USB. */
#ifndef AZHUB_LIBRARY_PATH_H
#define AZHUB_LIBRARY_PATH_H
#include <errno.h>
#include <stdio.h>
#include <sys/vfs.h>

static const char *hub_library_directory(long filesystem) {
    /* Linux HFS+ superblock magic. Native EP147 uses the hidden directory on Mac media. */
    return filesystem==0x482b ? ".PIONEER" : "PIONEER";
}
static int hub_library_is_mac(const char *root) {
    struct statfs filesystem;
    return root && !statfs(root,&filesystem) && filesystem.f_type==0x482b;
}
static int hub_library_path(const char *root,const char *relative,char *out,size_t size) {
    struct statfs filesystem;
    if (!root || !relative || !out || !size) { errno=EINVAL; return -1; }
    if (statfs(root,&filesystem)) return -1;
    int n=snprintf(out,size,"%s/%s/%s",root,hub_library_directory(filesystem.f_type),relative);
    if (n<0 || (size_t)n>=size) { errno=ENAMETOOLONG; return -1; }
    return 0;
}
#endif

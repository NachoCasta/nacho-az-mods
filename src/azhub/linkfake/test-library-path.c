/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/vfs.h>
static long fixture_filesystem;
static int fixture_statfs(const char *root,struct statfs *out) {
    if (!strcmp(root,"/gone")) { errno=ENOENT; return -1; }
    memset(out,0,sizeof *out);out->f_type=fixture_filesystem;return 0;
}
#define statfs(root,out) fixture_statfs(root,out)
#include "library-path.h"
#undef statfs
int main(void) {
    char path[512];
    const long filesystems[]={0x4d44,0x2011bab0,0xef53,0x482b};
    for (unsigned i=0;i<sizeof filesystems/sizeof *filesystems;i++) {
        fixture_filesystem=filesystems[i];
        assert(!hub_library_path("/media/usb/sdb1","rekordbox/exportLibrary.db",path,sizeof path));
        assert(!strcmp(path,fixture_filesystem==0x482b ?
            "/media/usb/sdb1/.PIONEER/rekordbox/exportLibrary.db":
            "/media/usb/sdb1/PIONEER/rekordbox/exportLibrary.db"));
        assert(!hub_library_path("/media/usb/sdb1","DEVSETTING.DAT",path,sizeof path));
        assert(strstr(path,fixture_filesystem==0x482b ? "/.PIONEER/":"/PIONEER/"));
    }
    assert(hub_library_path("/gone","DEVSETTING.DAT",path,sizeof path)<0 && errno==ENOENT);
    assert(hub_library_path("/media/usb/sdb1","DEVSETTING.DAT",path,8)<0 && errno==ENAMETOOLONG);
    assert(hub_library_path(NULL,"DEVSETTING.DAT",path,sizeof path)<0 && errno==EINVAL);
    puts("library paths: HFS+ hidden layout, FAT/exFAT unchanged, missing roots and bounded output passed");
    return 0;
}

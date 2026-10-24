/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef AZHUB_USB_PRESENCE_H
#define AZHUB_USB_PRESENCE_H
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
/* assets_dir resolves the matching owner's package on an actually present USB,
 * rather than the persistent executable stage. Require its expected files even
 * on the emulator before any initialization that can install hooks or workers.
 * This is an initialization gate; removing a running library does not unload it. */
static int community_require_usb(const az_core_v1 *core) {
    if (!AZ_HAS(core,assets_dir)) return -40;
    const char *assets=core->assets_dir("usb-hub-support");
    if (!assets || !*assets) return -40;
    const char *required[]={"mount-hub.sh","recover-hubs.sh","azhub.ko"};
    for (unsigned i=0;i<sizeof required/sizeof required[0];i++) {
        char path[1024];
        int length=snprintf(path,sizeof path,"%s/%s",assets,required[i]);
        if (length<0 || (size_t)length>=sizeof path) return -40;
        int fd=open(path,O_RDONLY|O_CLOEXEC);
        if (fd<0) return -40;
        struct stat st;
        int valid=!fstat(fd,&st) && S_ISREG(st.st_mode) && st.st_size>0;
        close(fd);
        if (!valid) return -40;
    }
    return 0;
}
#endif

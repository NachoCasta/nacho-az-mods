/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef AZHUB_KERNEL_ASSET_H
#define AZHUB_KERNEL_ASSET_H
#include <fcntl.h>
#include <sys/utsname.h>
/* Only the known AZ module/kernel pair is attempted. The module itself also
 * verifies the exact instructions before applying its reversible patch. */
static int community_prepare_kernel(const az_core_v1 *core) {
    struct utsname kernel;
    if (uname(&kernel)) return -20;
    if (!strcmp(kernel.release,"6.6.138-dirty")) {
        core->log("usb-hub-support","emulator kernel: no hub compatibility module needed");
        return 0;
    }
    if (strcmp(kernel.release,"4.4.194")) {
        core->log("usb-hub-support","unsupported hub kernel %s",kernel.release);
        return -21;
    }
    if (!access("/sys/module/azhub",F_OK)) return 0;
    if (!AZ_HAS(core,assets_dir)) return -22;
    const char *assets=core->assets_dir("usb-hub-support");
    if (!assets) {
        core->log("usb-hub-support","azhub.ko asset is unavailable on the mod USB");
        return -23;
    }
    char module[1024];
    if (snprintf(module,sizeof module,"%s/azhub.ko",assets)>=(int)sizeof module) return -24;
    int fd=open(module,O_RDONLY|O_CLOEXEC);
    if (fd<0) return -25;
    int result=(int)syscall(SYS_finit_module,fd,"",0);
    int error=errno;
    close(fd);
    if (result && !(error==EEXIST && !access("/sys/module/azhub",F_OK))) {
        core->log("usb-hub-support","azhub.ko refused by kernel: %s",strerror(error));
        return -26;
    }
    core->log("usb-hub-support","hub compatibility module loaded; startup worker will retry unbound external hubs");
    return 0;
}
#endif

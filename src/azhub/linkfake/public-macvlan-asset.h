/* SPDX-License-Identifier: GPL-3.0-only */
/* Called only by the authenticated target helper, never by EP147. The owned
 * module is an asset beside the actual executable; no modprobe, persistent
 * installation, force flags or compatibility fallback is permitted. */
#ifndef HUB_PUBLIC_MACVLAN_ASSET_H
#define HUB_PUBLIC_MACVLAN_ASSET_H
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <unistd.h>
static int hub_public_macvlan_asset(void) {
    struct utsname kernel;
    if (uname(&kernel)<0) return -1;
    if (strcmp(kernel.release,"4.4.194")) { errno=ENOTSUP;return -1; }
    const char *registered="/sys/module/usb_hub_macvlan";
    if (!access(registered,F_OK)) return 0;
    char executable[1024];
    ssize_t length=readlink("/proc/self/exe",executable,sizeof executable-1);
    if (length<=0 || length>=(ssize_t)sizeof executable-1) { errno=ENAMETOOLONG;return -1; }
    executable[length]=0;
    char *leaf=strrchr(executable,'/');
    if (!leaf || leaf==executable) { errno=EINVAL;return -1; }
    *leaf=0;leaf=strrchr(executable,'/');
    if (!leaf || strcmp(leaf+1,"assets")) { errno=EACCES;return -1; }
    int assets=open(executable,O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
    if (assets<0) return -1;
    int module=openat(assets,"usb_hub_macvlan.ko",O_RDONLY|O_CLOEXEC|O_NOFOLLOW),error=errno;
    close(assets);
    if (module<0) { errno=error;return -1; }
    struct stat state;
    if (fstat(module,&state)<0) { error=errno;close(module);errno=error;return -1; }
    if (!S_ISREG(state.st_mode)) { close(module);errno=EINVAL;return -1; }
    int result=(int)syscall(SYS_finit_module,module,"",0);error=errno;
    close(module);
    if (result && !(error==EEXIST && !access(registered,F_OK))) { errno=error;return -1; }
    if (access(registered,F_OK)) { errno=EIO;return -1; }
    return 0;
}
#endif

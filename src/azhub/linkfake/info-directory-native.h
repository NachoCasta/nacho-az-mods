/* SPDX-License-Identifier: GPL-3.0-only */
/* Private repositories must not resolve analysis roots through native USB1. */
#ifndef HUB_INFO_DIRECTORY_NATIVE_H
#define HUB_INFO_DIRECTORY_NATIVE_H
#include "library-path.h"
#ifndef HUB_INFO_DIRECTORY_FILE
#define HUB_INFO_DIRECTORY_FILE(out,path) ((void (*)(void *,const char *))0x24a1180)((out),(path))
#endif
static void *hub_info_directory_original __attribute__((used));
__attribute__((used,noinline)) static int hub_info_directory_c(uintptr_t server,uintptr_t reference,uintptr_t output) {
    int source=-1;
    for (int i=0;server && i<MAX_FAKES;i++)
        if (fakes[i].srv_info==server) {source=i;break;}
    if (source<0) return 0;
    char path[512]={0};
    if (lf_runtime_ready() && reference && *(const uintptr_t *)reference &&
        source<nfakes && fakes[source].present && safe_server_begin(source)) {
        /* Metadata tickets copy the root under their own mutex. Taking the
         * discovery mutex here would deadlock a synchronous native mount callback. */
        struct hub_metadata_ticket ticket=hub_metadata_capture(source);
        struct statfs filesystem;
        if (*ticket.root && !statfs(ticket.root,&filesystem))
            snprintf(path,sizeof path,"%s/%s",ticket.root,hub_library_directory(filesystem.f_type));
        safe_server_end(source);
    }
    /* File and String each contain one native reference-counted word here, as
     * in the existing library mount adapter. Only initialize that one word. */
    HUB_INFO_DIRECTORY_FILE((void *)output,path);
    return 1;
}
#if defined(__aarch64__)
/* Native File returns through x8 despite occupying only eight bytes. Preserve
 * x0/x1/x8 for untouched native repositories, and provide unwind information
 * for exceptions from the native constructor called by our C helper. */
__asm__(".text\n.align 2\n.global hub_info_directory_entry\n.hidden hub_info_directory_entry\n.type hub_info_directory_entry,%function\n"
        "hub_info_directory_entry:\n.cfi_startproc\n"
        "stp x29,x30,[sp,#-64]!\n.cfi_def_cfa_offset 64\n"
        ".cfi_offset 29,-64\n.cfi_offset 30,-56\nmov x29,sp\n"
        "stp x0,x1,[sp,#16]\nstr x8,[sp,#32]\nmov x2,x8\n"
        "bl hub_info_directory_c\ncbz w0,1f\nldr x0,[sp,#32]\n"
        ".cfi_remember_state\nldp x29,x30,[sp],#64\n"
        ".cfi_def_cfa_offset 0\n.cfi_restore 29\n.cfi_restore 30\nret\n"
        ".cfi_restore_state\n1:\nldp x0,x1,[sp,#16]\nldr x8,[sp,#32]\n"
        "adrp x16,hub_info_directory_original\n"
        "ldr x16,[x16,:lo12:hub_info_directory_original]\n"
        "ldp x29,x30,[sp],#64\n.cfi_def_cfa_offset 0\n"
        ".cfi_restore 29\n.cfi_restore 30\nbr x16\n.cfi_endproc\n");
extern void hub_info_directory_entry(void);
#endif
#ifndef HUB_INFO_DIRECTORY_TEST
static int hub_info_directory_prepare(void) {
    static const uint32_t expected[]={0xa9b97bfd,0xd2892a02,0xf2a9c9e2,0x910003fd};
    return !memcmp((const void *)0x1978970,expected,sizeof expected);
}
static int hub_info_directory_install(void) {
    hub_info_directory_original=make_hook(0x1978970,hub_info_directory_entry);
    return hub_info_directory_original ? 0:-95;
}
#endif
#endif

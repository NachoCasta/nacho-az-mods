/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/vfs.h>
#include "source-metadata.h"
#define MAX_FAKES 8
static struct {uintptr_t srv_info;int present;} fakes[MAX_FAKES];
static int nfakes=2,ready=1,allowed=1,leases;
static int lf_runtime_ready(void) {return ready;}
static int safe_server_begin(int source) {assert(source==0 || source==1);if(allowed)leases++;return allowed;}
static void safe_server_end(int source) {(void)source;assert(leases==1);leases--;}
static int fixture_statfs(const char *root,struct statfs *out) {
    memset(out,0,sizeof *out);out->f_type=!strcmp(root,"/media/usb/mac") ? 0x482b:0x4d44;return 0;
}
#ifdef HUB_INFO_DIRECTORY_BRIDGE_TEST
extern void info_directory_throw_if_requested(void);
#endif
static void fixture_file(void *output,const char *path) {
    assert(!leases);
#ifdef HUB_INFO_DIRECTORY_BRIDGE_TEST
    info_directory_throw_if_requested();
#endif
    uintptr_t value=!strcmp(path,"/media/usb/mac/.PIONEER") ? 11:
        !strcmp(path,"/media/usb/fat/PIONEER") ? 22:0;
    assert(value || !*path);memcpy(output,&value,sizeof value);
}
#define HUB_INFO_DIRECTORY_TEST 1
#define statfs(root,out) fixture_statfs(root,out)
#define HUB_INFO_DIRECTORY_FILE(out,path) fixture_file(out,path)
#include "info-directory-native.h"
#undef statfs
void info_directory_setup(int enabled) {
    ready=1;allowed=enabled;fakes[0].srv_info=101;fakes[1].srv_info=202;
    fakes[0].present=fakes[1].present=1;
    hub_metadata_prepare(0,"/media/usb/mac","3-2.1");
    hub_metadata_prepare(1,"/media/usb/fat","3-2.2");
}
#ifdef HUB_INFO_DIRECTORY_BRIDGE_TEST
void info_directory_set_original(void *function) {hub_info_directory_original=function;}
#else
int main(void) {
    uintptr_t reference=1;
    struct {uintptr_t before,value,after;} result={77,99,88};
    info_directory_setup(1);
    for(int i=0;i<1000;i++) {
        assert(hub_info_directory_c(101,(uintptr_t)&reference,(uintptr_t)&result.value));assert(result.value==11);
        assert(hub_info_directory_c(202,(uintptr_t)&reference,(uintptr_t)&result.value));assert(result.value==22);
    }
    assert(!hub_info_directory_c(303,(uintptr_t)&reference,(uintptr_t)&result.value) && result.value==22);
    allowed=0;assert(hub_info_directory_c(101,(uintptr_t)&reference,(uintptr_t)&result.value) && !result.value);
    allowed=1;hub_metadata_invalidate(0);
    assert(hub_info_directory_c(101,(uintptr_t)&reference,(uintptr_t)&result.value) && !result.value);
    info_directory_setup(1);fakes[0].present=0;
    assert(hub_info_directory_c(101,(uintptr_t)&reference,(uintptr_t)&result.value) && !result.value);
    assert(result.before==77 && result.after==88 && !leases);
    puts("info directory: independent Mac/FAT roots, untouched native server, retired/reserved roots and one-word result passed");
    return 0;
}
#endif

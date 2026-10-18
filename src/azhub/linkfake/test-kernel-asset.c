/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include "azmod-mod.h"
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <unistd.h>
static const char *release_name="6.6.138-dirty";
static int loaded,have_assets=1,refuse,loads,closes;
static int test_uname(struct utsname *u) { memset(u,0,sizeof *u);strcpy(u->release,release_name);return 0; }
static int test_access(const char *p,int mode) { (void)p;(void)mode;return loaded ? 0 : -1; }
static int test_open(const char *p,int flags,...) { (void)flags;return strstr(p,"/azhub.ko") ? 9 : -1; }
static int test_close(int fd) { if(fd!=9)return -1;closes++;return 0; }
static long test_syscall(long number,...) { if(number!=SYS_finit_module)return -1;loads++;errno=ENOEXEC;return refuse ? -1 : 0; }
static void test_log(const char *id,const char *fmt,...) { (void)id;(void)fmt; }
static const char *test_assets(const char *id) { (void)id;return have_assets ? "/test/assets" : NULL; }
#define uname test_uname
#define access test_access
#define open test_open
#define close test_close
#define syscall test_syscall
#include "kernel-asset.h"
#define CHECK(condition) do { if(!(condition)) { fprintf(stderr,"check failed at line %d\n",__LINE__);return 1; } } while(0)
int main(void) {
    az_core_v1 core={.abi=AZ_MOD_ABI,.size=sizeof core,.log=test_log,.assets_dir=test_assets};
    CHECK(community_prepare_kernel(&core)==0 && !loads);
    release_name="4.4.195";
    CHECK(community_prepare_kernel(&core)==-21 && !loads);
    release_name="4.4.194"; loaded=1;
    CHECK(community_prepare_kernel(&core)==0 && !loads);
    loaded=0; have_assets=0;
    CHECK(community_prepare_kernel(&core)==-23 && !loads);
    have_assets=1; refuse=1;
    CHECK(community_prepare_kernel(&core)==-26 && loads==1 && closes==1);
    refuse=0;
    CHECK(community_prepare_kernel(&core)==0 && loads==2 && closes==2);
    puts("kernel asset policy passed (6 cases; mocked syscalls)");return 0;
}

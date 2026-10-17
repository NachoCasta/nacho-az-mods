/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "discovery.h"
static void lf_log(const char *format,...) {(void)format;}
struct test_core {const char *(*assets_dir)(const char *);};
static struct test_core *lf_core;
#include "mount-worker.h"
int main(void) {
 const char *stock[]={"sda","sda1","sdf","sdf8"};
 const char *fallback[]={"sdg","sdg1","sdh12","sdz1","sdaa1","sdab"};
 const char *invalid[]={"sd","sd1","sdg/../sda","sdg;id","sdg1x","mmcblk0","sdG1","sdg-1"};
 for(unsigned i=0;i<sizeof stock/sizeof *stock;i++) assert(!hub_fallback_device(stock[i]));
 for(unsigned i=0;i<sizeof fallback/sizeof *fallback;i++) assert(hub_fallback_device(fallback[i]));
 for(unsigned i=0;i<sizeof invalid/sizeof *invalid;i++) assert(!hub_fallback_device(invalid[i]));
 puts("mount worker device policy: 18 checks passed");
 return 0;
}

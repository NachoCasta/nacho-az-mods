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
struct test_core {
 const char *(*assets_dir)(const char *);
 int (*launch)(const char *,const char *,const char *const [],const void *);
 int (*launch_alive)(const char *,int);
 int (*launch_stop)(const char *,int);
};
static struct test_core *lf_core;
static char fault_file[PATH_MAX];
#define HUB_MOUNT_FAULT_FILE fault_file
static int launch_calls,alive_calls,stop_calls,sleep_calls,launch_result=42,alive_steps;
static char recovery_helper[PATH_MAX];
static int recovery_mode;
static int test_launch(const char *id,const char *path,const char *const args[],const void *opts) {
 assert(!strcmp(id,"usb-hub-support") && !strcmp(path,recovery_mode ? recovery_helper:"/fixture/mount-hub.sh"));
 assert(args && !strcmp(args[0],recovery_mode ? "startup":"sdg") && !args[1] && !opts);
 launch_calls++;return launch_result;
}
static int test_alive(const char *id,int pid) {
 assert(!strcmp(id,"usb-hub-support") && pid==42);alive_calls++;
 return alive_steps==-2 ? -1 : alive_steps<0 || alive_calls<=alive_steps;
}
static int test_stop(const char *id,int pid) {
 assert(!strcmp(id,"usb-hub-support") && pid==42);stop_calls++;return 0;
}
static int test_sleep(useconds_t delay) {assert(delay==100000);sleep_calls++;return 0;}
#define usleep test_sleep
#include "mount-worker.h"
#undef usleep
/* Reset emulates a new process; the marker independently survives it. */
static void reset_helper(void) {launch_calls=alive_calls=stop_calls=sleep_calls=0;atomic_store_explicit(&hub_mount_fault,0,memory_order_release);}

int main(void) {
 char directory[]="/tmp/azhub-mount-fault-XXXXXX";assert(mkdtemp(directory));
 assert(snprintf(fault_file,sizeof fault_file,"%s/fault",directory)>0);
 hub_mount_restore_fault();assert(!hub_mount_faulted());
 const char *stock[]={"sda","sda1","sdf","sdf8"};
 const char *fallback[]={"sdg","sdg1","sdh12","sdz1","sdaa1","sdab"};
 const char *invalid[]={"sd","sd1","sdg/../sda","sdg;id","sdg1x","mmcblk0","sdG1","sdg-1"};
 for(unsigned i=0;i<sizeof stock/sizeof *stock;i++) assert(!hub_fallback_device(stock[i]));
 for(unsigned i=0;i<sizeof fallback/sizeof *fallback;i++) assert(hub_fallback_device(fallback[i]));
 for(unsigned i=0;i<sizeof invalid/sizeof *invalid;i++) assert(!hub_fallback_device(invalid[i]));
 struct test_core core={.launch=test_launch,.launch_alive=test_alive,.launch_stop=test_stop};
 lf_core=&core;alive_steps=2;
 hub_run_mount_helper("/fixture/mount-hub.sh","sdg");
 assert(launch_calls==1 && alive_calls==3 && sleep_calls==2 && !stop_calls);
 reset_helper();launch_result=-1;
 hub_run_mount_helper("/fixture/mount-hub.sh","sdg");
 assert(launch_calls==1 && !alive_calls && !sleep_calls && !stop_calls);
 reset_helper();launch_result=42;alive_steps=-1;
 hub_run_mount_helper("/fixture/mount-hub.sh","sdg");
 assert(launch_calls==1 && alive_calls==101 && sleep_calls==100 && stop_calls==1);
 assert(hub_mount_faulted() && !access(fault_file,F_OK));
 hub_run_mount_helper("/fixture/mount-hub.sh","sdg");assert(launch_calls==1);
 reset_helper();hub_mount_restore_fault();assert(hub_mount_faulted());
 hub_run_mount_helper("/fixture/mount-hub.sh","sdg");assert(!launch_calls);
 assert(!unlink(fault_file));
 reset_helper();alive_steps=-2;
 hub_run_mount_helper("/fixture/mount-hub.sh","sdg");
 assert(hub_mount_faulted() && alive_calls==1 && !sleep_calls && stop_calls==1);
 assert(!unlink(fault_file));
 reset_helper();core.launch_stop=NULL;
 hub_run_mount_helper("/fixture/mount-hub.sh","sdg");assert(!launch_calls);
 lf_core=NULL;hub_run_mount_helper("/fixture/mount-hub.sh","sdg");assert(!launch_calls);
 lf_core=&core;core.launch_stop=test_stop;reset_helper();recovery_mode=1;
 unsigned passes=0;
 hub_recover_startup(&passes,NULL);hub_recover_startup(&passes,directory);assert(!passes && !launch_calls);
 assert(snprintf(recovery_helper,sizeof recovery_helper,"%s/recover-hubs.sh",directory)>0);
 FILE *helper=fopen(recovery_helper,"w");assert(helper);assert(fputs("fixture",helper)>=0);assert(!fclose(helper));
 alive_steps=0;
 for(int i=0;i<20;i++) hub_recover_startup(&passes,directory);
 assert(passes==3 && launch_calls==3 && !stop_calls);
 reset_helper();passes=0;alive_steps=-1;
 hub_recover_startup(&passes,directory);assert(passes==1 && hub_mount_faulted() && stop_calls==1);
 hub_recover_startup(&passes,directory);assert(passes==1 && launch_calls==1);
 assert(!unlink(fault_file));assert(!unlink(recovery_helper));
 assert(!rmdir(directory));
 puts("mount worker: device policy, SDK arguments/refusal, timeout inhibition across reload and missing services passed");
 return 0;
}

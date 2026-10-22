/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include "source-metadata.h"
typedef uint64_t (*fn8)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
static pthread_mutex_t safe_source_mutex=PTHREAD_MUTEX_INITIALIZER;
static struct {int present;uintptr_t srv_music;char dir[256],port_route[96];int unmount_pending;} fakes[2];
static int nfakes=2,ready=1,calls,reads,retire_during_mount;
static int lf_runtime_ready(void) {return ready;}
static const char *jstr(uintptr_t p) {return *(const char **)p;}
#define lf_log(...) ((void)0)
struct hub_metadata_blob;
static struct hub_metadata_blob native_get(uintptr_t);
#define HUB_METADATA_GET native_get
static uint64_t original(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    assert(a && b==11 && c==12 && d==13 && e==14 && f==15 && g==16 && h==17);
    calls++;
    if (retire_during_mount) hub_metadata_invalidate(0);
    return 0xcafe;
}
static fn8 make_hook(uintptr_t,fn8);
#include "source-metadata-native.h"
static unsigned char point[0x40];
static struct hub_metadata_blob native_get(uintptr_t p) {
    assert(p==(uintptr_t)point);reads++;
    struct hub_metadata_blob b={{0}};const char *name="Nacho Chips";
    for (size_t i=0;name[i];i++) b.bytes[i*2]=(unsigned char)name[i];
    b.bytes[0x7c]=6;return b;
}
static fn8 make_hook(uintptr_t address,fn8 fn) {
    assert(address==0x1376c98 && fn==hub_metadata_mount);return original;
}
static void word(void *p,size_t offset,uintptr_t value) {memcpy((char *)p+offset,&value,sizeof value);}
static void mount(unsigned char *task) {
    assert(hub_metadata_mount((uintptr_t)task,11,12,13,14,15,16,17)==0xcafe);
}
int main(void) {
    unsigned char server[0x1e0]={0},commands[0x20]={0},info[16]={0},task[40]={0};
    const char *root="/media/usb/first",*route="3-2.4";
    fakes[0].present=1;fakes[0].srv_music=(uintptr_t)server;
    strcpy(fakes[0].dir,root);strcpy(fakes[0].port_route,route);
    word(server,0x1d0,(uintptr_t)commands);word(commands,0x10,(uintptr_t)info);
    word(info,0,(uintptr_t)point);word(point,0,0x2baf7e8);word(point,0x10,(uintptr_t)root);
    word(task,0,(uintptr_t)server);word(task,0x10,(uintptr_t)root);
    hub_metadata_prepare(0,root,route);assert(!hub_metadata_install());mount(task);
    struct hub_source_properties p;
    assert(calls==1 && reads==1 && hub_metadata_read(0,root,route,&p));
    assert(p.name_valid && !strcmp(p.name,"Nacho Chips") && p.color==6);
    hub_metadata_prepare(0,root,route);retire_during_mount=1;mount(task);
    assert(calls==2 && reads==2 && !hub_metadata_read(0,root,route,&p));
    retire_during_mount=0;hub_metadata_prepare(0,root,route);
    word(point,0,0x1234);mount(task);assert(reads==2); /* Unknown mount type. */
    word(point,0,0x2baf7e8);word(point,0x10,(uintptr_t)"/media/usb/other");
    mount(task);assert(reads==2); /* Cannot borrow another root. */
    word(point,0x10,(uintptr_t)root);word(commands,0x10,0);mount(task);assert(reads==2);
    ready=0;mount(task);assert(reads==2 && calls==6);
    return 0;
}

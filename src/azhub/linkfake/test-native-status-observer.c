/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>
typedef uint64_t (*fn8)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
static unsigned called,logs,hooks;
static int lf_hook_failures,decline_hook;
static uint64_t expected[8];
static const char *kernel_release="6.6.138-dirty";
static uint64_t original(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    uint64_t actual[]={a,b,c,d,e,f,g,h};assert(!memcmp(actual,expected,sizeof actual));++called;return UINT64_C(0x123456789abcdef0);
}
static fn8 make_hook(uintptr_t address,fn8 replacement) {
    assert((address==0x10d324c || address==0x10cf350) && replacement);++hooks;
    if (decline_hook) { ++lf_hook_failures;return (fn8)address; }
    return original;
}
static int fake_uname(struct utsname *state) { memset(state,0,sizeof *state);strcpy(state->release,kernel_release);return 0; }
#define uname fake_uname
#define lf_log(...) (++logs)
#include "native-status-observer.h"
static void observe(unsigned char key[4],unsigned char fields[16]) {
    expected[0]=1;expected[1]=(uintptr_t)key;expected[2]=(uintptr_t)fields;
    for(unsigned i=3;i<8;++i) expected[i]=10+i;
    assert(native_status_store_observe(expected[0],expected[1],expected[2],expected[3],expected[4],expected[5],expected[6],expected[7])==UINT64_C(0x123456789abcdef0));
}
int main(void) {
    kernel_release="4.4.194";assert(native_status_observer_install()<0 && !hooks && !logs);
    kernel_release="6.6.138-dirty";decline_hook=1;
    assert(native_status_observer_install()<0 && hooks==1 && !logs && !native_status_store_original);
    decline_hook=0;assert(!native_status_observer_install() && hooks==3 && logs==1);
    unsigned char key[4]={2,1,3,0},fields[16]={3,1,0,0,1,3,1,0};
    uint32_t track=495;memcpy(fields+8,&track,sizeof track);
    unsigned char saved[16];memcpy(saved,fields,sizeof saved);
    observe(key,fields);assert(called==1 && logs==2 && !memcmp(fields,saved,sizeof fields));
    for(unsigned i=0;i<100;++i) observe(key,fields);
    assert(called==101 && logs==2);
    for(unsigned i=0;i<100;++i) { fields[0]=(unsigned char)i;observe(key,fields); }
    assert(native_status_observer_count[2]==16 && logs==17 && called==201);
    key[0]=41;observe(key,fields);assert(called==202 && logs==17);
    uint32_t member=0x0202;
    expected[0]=1;expected[1]=(uintptr_t)fields;expected[2]=3;expected[3]=4;
    expected[4]=(uintptr_t)&member;expected[5]=6;expected[6]=7;expected[7]=8;
    memcpy(saved,fields,sizeof saved);
    assert(native_status_receiver_observe(expected[0],expected[1],expected[2],expected[3],expected[4],expected[5],expected[6],expected[7])==UINT64_C(0x123456789abcdef0));
    assert(called==203 && logs==18 && !memcmp(fields,saved,sizeof fields) && member==0x0202);
    for(unsigned i=0;i<200;++i) native_status_receiver_observe(expected[0],expected[1],expected[2],expected[3],expected[4],expected[5],expected[6],expected[7]);
    assert(native_status_receiver_count[0]==128 && logs==145 && called==403);
    member=0x0302; /* A second member retains its own budget. */
    native_status_receiver_observe(expected[0],expected[1],expected[2],expected[3],expected[4],expected[5],expected[6],expected[7]);
    assert(native_status_receiver_count[1]==1 && logs==146 && called==404);
    puts("PASS: emulator-only observer, exact eight-argument/return forwarding, unchanged input, duplicate suppression and bounded per-member logs");
}

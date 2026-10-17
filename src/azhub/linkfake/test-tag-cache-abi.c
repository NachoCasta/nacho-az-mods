/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
static void *hub_tag_cache_originals[8] __attribute__((used));
static uint64_t method_seen,caller_seen,receiver_seen;
__attribute__((used,noinline)) static uint64_t hub_tag_cache_this(uint64_t receiver,uint64_t method,uint64_t caller) {
    receiver_seen=receiver;method_seen=method;caller_seen=caller;return receiver+0x100;
}
#include "tag-cache-abi.h"
#define ARGS uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h,uint64_t i,uint64_t j
#define VALUES 7,11,13,17,19,23,29,31,37,41
static uint64_t scalar(ARGS) { return a+2*b+3*c+4*d+5*e+6*f+7*g+8*h+9*i+10*j; }
struct aggregate { uint64_t first,second,third; };
static struct aggregate indirect(ARGS) { return (struct aggregate){scalar(a,b,c,d,e,f,g,h,i,j),a^j,b^i^0xa55a}; }
int main(void) {
    void *entries[]={hub_tag_0,hub_tag_1,hub_tag_2,hub_tag_3,hub_tag_4,hub_tag_5,hub_tag_6,hub_tag_7};
    for(unsigned n=0;n<8;n++) {
        hub_tag_cache_originals[n]=scalar;
        uint64_t result=((uint64_t (*)(ARGS))entries[n])(VALUES);
        if(result!=scalar(7+0x100,11,13,17,19,23,29,31,37,41) || method_seen!=n || receiver_seen!=7 || !caller_seen) return 1;
        hub_tag_cache_originals[n]=indirect;
        struct aggregate got=((struct aggregate (*)(ARGS))entries[n])(VALUES);
        struct aggregate want=indirect(7+0x100,11,13,17,19,23,29,31,37,41);
        if(got.first!=want.first || got.second!=want.second || got.third!=want.third || method_seen!=n || receiver_seen!=7 || !caller_seen) return 2;
    }
    puts("tag cache ABI passed (8 adapters: scalar + x8 aggregate + stack args)");return 0;
}

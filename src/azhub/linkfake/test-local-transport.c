/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
typedef uint64_t (*fn8)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
static int runtime_ready=1,tmpl_len,changes,forwards,logs;
static int hub_local_transport_only;
static uintptr_t call_site;
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static int lf_runtime_ready(void) { return runtime_ready; }
#define lf_log(...) ((void)++logs)
static uint64_t original(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    ++forwards;memcpy((void *)a,(void *)b,c ? 16:4);return 0x1234;
}
static fn8 make_hook(uintptr_t site,fn8 callback) {
    assert(site==0x2479b90 && callback);return original;
}
static void network_change(uintptr_t owner);
#define HUB_LOCAL_ADDRESS_CALL_SITE() call_site
#define HUB_LOCAL_NETWORK_CHANGE(owner) network_change(owner)
#include "local-transport-native.h"
static void network_change(uintptr_t owner) {
    ++changes;
    unsigned char output[16]={0};uint32_t address=0;
    call_site=0x1406464;
    assert(hub_local_address((uintptr_t)output,(uintptr_t)&address,0,0,0,0,0,0)==0x1234);
    const unsigned char loopback[4]={127,0,0,1};assert(!memcmp(output,loopback,4));
    /* A synchronous native re-entry must not start a second operation. */
    hub_local_transport_kick(owner);
}
int main(void) {
    hub_local_transport_install();
    unsigned char output[16]={0},zero[16]={0},real[16]={192,0,2,9};
    call_site=0x1406460; /* An adjacent, unrelated call must pass unchanged. */
    assert(hub_local_address((uintptr_t)output,(uintptr_t)zero,0,0,0,0,0,0)==0x1234);
    assert(!memcmp(output,zero,4) && !hub_local_transport_only);
    call_site=0x1406464;runtime_ready=0;
    hub_local_address((uintptr_t)output,(uintptr_t)zero,0,0,0,0,0,0);
    assert(!memcmp(output,zero,4) && !hub_local_transport_only);
    runtime_ready=1;
    hub_local_address((uintptr_t)output,(uintptr_t)zero,1,0,0,0,0,0);
    assert(!memcmp(output,zero,16) && !hub_local_transport_only); /* IPv6 untouched. */
    hub_local_address((uintptr_t)output,(uintptr_t)zero,0,0,0,0,0,0);
    assert(output[0]==127 && output[3]==1 && hub_local_transport_only && !memcmp(zero,(unsigned char[16]){0},16));
    int before=logs;
    hub_local_address((uintptr_t)output,(uintptr_t)zero,0,0,0,0,0,0);
    assert(logs==before); /* An unchanged state does not repeatedly log. */
    hub_local_address((uintptr_t)output,(uintptr_t)real,0,0,0,0,0,0);
    assert(!memcmp(output,real,4) && !hub_local_transport_only);
    uint64_t owner[0xa0]={0};uintptr_t address=(uintptr_t)owner;
    hub_local_transport_kick(0);assert(!changes);
    owner[0x358/8]=1;hub_local_transport_kick(address);assert(!changes);
    owner[0x358/8]=0;tmpl_len=1152;hub_local_transport_kick(address);assert(!changes);
    tmpl_len=0;runtime_ready=0;hub_local_transport_kick(address);assert(!changes);
    runtime_ready=1;hub_local_transport_kick(address);
    assert(changes==1 && hub_local_transport_only && !hub_local_transport_starting);
    assert(!owner[0x358/8]); /* Observed native property was never written. */
    tmpl_len=1152;hub_local_transport_kick(address);assert(changes==1);
    assert(forwards>=6);
    puts("Scoped local-transport address, inactive/IPv6 forwarding, native property retention and re-entry controls passed");
    return 0;
}

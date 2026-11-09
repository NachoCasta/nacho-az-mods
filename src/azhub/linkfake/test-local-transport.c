/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
typedef uint64_t (*fn8)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
static int runtime_ready=1,tmpl_len,changes,forwards,logs,ethernet_state=1;
#define HUB_LOCAL_ETHERNET_STATE() ethernet_state
static int hub_local_transport_only;
static uintptr_t call_site,config_call_site;
#define HUB_LOCAL_CONFIG_CALL_SITE() config_call_site
static unsigned char copied_mask[17];
static uintptr_t forwarded_mask;
static uint64_t original_config(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    (void)a;(void)b;(void)d;(void)e;(void)f;(void)g;(void)h;
    forwarded_mask=c;memcpy(copied_mask,(void *)c,17);return 0x5678;
}
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static int lf_runtime_ready(void) { return runtime_ready; }
#define lf_log(...) ((void)++logs)
static uint64_t original(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    ++forwards;memcpy((void *)a,(void *)b,c ? 16:4);return 0x1234;
}
static fn8 make_hook(uintptr_t site,fn8 callback) {
    assert(callback);
    if (site==0x13dec28) return original_config;
    assert(site==0x2479b90);return original;
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
    /* A nonzero Wi-Fi IPv4 must not suppress a missing native bootstrap. */
    tmpl_len=0;owner[0x358/8]=0x7301a8c0;ethernet_state=0;
    hub_local_transport_kick(address);assert(changes==2 && hub_local_transport_only);
    assert(owner[0x358/8]==0x7301a8c0);
    call_site=0x1406464;
    hub_local_address((uintptr_t)output,(uintptr_t)real,0,0,0,0,0,0);
    assert(output[0]==127 && output[3]==1 && hub_local_transport_only);
    /* Observation failure cannot promote an existing fallback to external Link. */
    ethernet_state=-1;
    hub_local_address((uintptr_t)output,(uintptr_t)real,0,0,0,0,0,0);
    assert(output[0]==127 && hub_local_transport_only);
    /* A usable wired interface retires fallback and preserves native address. */
    ethernet_state=1;
    hub_local_address((uintptr_t)output,(uintptr_t)real,0,0,0,0,0,0);
    assert(!memcmp(output,real,4) && !hub_local_transport_only);
    hub_local_transport_kick(address);assert(changes==2);
    ethernet_state=-1;hub_local_transport_kick(address);assert(changes==2);
    unsigned char native_ip[17]={127,0,0,1},native_mask[17]={255,255,255,0};
    config_call_site=0x140693c;hub_local_transport_only=1;
    assert(hub_local_config(1,(uintptr_t)native_ip,(uintptr_t)native_mask,0,0,0,0,0)==0x5678);
    assert(forwarded_mask!=(uintptr_t)native_mask && copied_mask[0]==255 && copied_mask[1]==0 && copied_mask[16]==0);
    assert(native_mask[1]==255 && native_mask[2]==255); /* Input stays untouched. */
    config_call_site=0x1406940;hub_local_config(1,(uintptr_t)native_ip,(uintptr_t)native_mask,0,0,0,0,0);
    assert(forwarded_mask==(uintptr_t)native_mask);
    config_call_site=0x140693c;native_ip[0]=192;
    hub_local_config(1,(uintptr_t)native_ip,(uintptr_t)native_mask,0,0,0,0,0);assert(forwarded_mask==(uintptr_t)native_mask);
    native_ip[0]=127;native_mask[16]=1;
    hub_local_config(1,(uintptr_t)native_ip,(uintptr_t)native_mask,0,0,0,0,0);assert(forwarded_mask==(uintptr_t)native_mask);
    native_mask[16]=0;hub_local_transport_only=0;
    hub_local_config(1,(uintptr_t)native_ip,(uintptr_t)native_mask,0,0,0,0,0);assert(forwarded_mask==(uintptr_t)native_mask);
    hub_local_transport_only=1;runtime_ready=0;
    hub_local_config(1,(uintptr_t)native_ip,(uintptr_t)native_mask,0,0,0,0,0);assert(forwarded_mask==(uintptr_t)native_mask);
    assert(forwards>=6);
    puts("Scoped local-transport address, inactive/IPv6 forwarding, native property retention and re-entry controls passed");
    return 0;
}

/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HUB_LOCAL_TRANSPORT_NATIVE_H
#define HUB_LOCAL_TRANSPORT_NATIVE_H
#include "local-transport-policy.h"
#ifndef HUB_LOCAL_ETHERNET_STATE
#define HUB_LOCAL_ETHERNET_STATE() hub_local_ethernet_state()
#endif
/* Exact EP147 network-change stack: construct only its temporary IPv4 value.
 * The manager's observed address, interfaces and saved settings stay native.
 * A missing native event can leave Link stopped even when Wi-Fi has IPv4. */
#ifndef HUB_LOCAL_ADDRESS_CALL_SITE
#define HUB_LOCAL_ADDRESS_CALL_SITE() ((uintptr_t)__builtin_return_address(0))
#endif
#ifndef HUB_LOCAL_NETWORK_CHANGE
#define HUB_LOCAL_NETWORK_CHANGE(owner) ((void (*)(uintptr_t))0x1406428)(owner)
#endif
#ifndef HUB_LOCAL_CONFIG_CALL_SITE
#define HUB_LOCAL_CONFIG_CALL_SITE() ((uintptr_t)__builtin_return_address(0))
#endif
static fn8 hub_local_address_original,hub_local_config_original;
static __thread int hub_local_transport_starting;
static int hub_local_transport_required(uint32_t observed_address) {
    if (!observed_address) return 1;
    int ethernet=HUB_LOCAL_ETHERNET_STATE();
    if (ethernet<0) return __atomic_load_n(&hub_local_transport_only,__ATOMIC_ACQUIRE);
    return !ethernet;
}
static uint64_t hub_local_address(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    static const unsigned char loopback[4]={127,0,0,1};
    if (lf_runtime_ready() && HUB_LOCAL_ADDRESS_CALL_SITE()==0x1406464 && b && !c) {
        uint32_t address;memcpy(&address,(const void *)b,sizeof address);
        int local=hub_local_transport_required(address);
        int previous=__atomic_exchange_n(&hub_local_transport_only,local,__ATOMIC_ACQ_REL);
        if (local) b=(uintptr_t)loopback;
        if (local!=previous)
            lf_log("private local transport %s; native interface and saved settings unchanged",
                local ? "enabled":"retired");
    }
    return hub_local_address_original(a,b,c,d,e,f,g,h);
}
/* The native operation copies its observed Wi-Fi mask separately from IPv4.
 * Give only this call's by-value configuration a matching loopback subnet;
 * never edit the observed NetworkManager property or any OS interface. The
 * native API copies this temporary value before returning or queuing work. */
static uint64_t hub_local_config(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    static const unsigned char loopback[4]={127,0,0,1};
    unsigned char mask[17] __attribute__((aligned(8)))={255,0,0,0};
    if (lf_runtime_ready() && HUB_LOCAL_CONFIG_CALL_SITE()==0x140693c && b && c &&
        __atomic_load_n(&hub_local_transport_only,__ATOMIC_ACQUIRE) &&
        !((const unsigned char *)b)[16] && !((const unsigned char *)c)[16] &&
        !memcmp((const void *)b,loopback,sizeof loopback)) c=(uintptr_t)mask;
    return hub_local_config_original(a,b,c,d,e,f,g,h);
}
static void hub_local_transport_leave(int *scope) {
    (void)scope;hub_local_transport_starting=0;
}
static void hub_local_transport_kick(uintptr_t owner) {
    if (!lf_runtime_ready() || !owner || hub_local_transport_starting) return;
    /* Native code reads this observed property atomically as eight bytes. */
    uint32_t observed=(uint32_t)__atomic_load_n((uint64_t *)(owner+0x358),__ATOMIC_ACQUIRE);
    if (!hub_local_transport_required(observed)) return;
    pthread_mutex_lock(&lock);
    int ready=tmpl_len!=0;
    pthread_mutex_unlock(&lock);
    if (ready) return;
    /* Caller is the completed native mode task, with no mod mutex held. Do
     * not move this native operation to the discovery/injector worker. */
    int scope __attribute__((cleanup(hub_local_transport_leave)))=0;
    (void)scope;hub_local_transport_starting=1;
    HUB_LOCAL_NETWORK_CHANGE(owner);
}
static void hub_local_transport_install(void) {
    hub_local_address_original=make_hook(0x2479b90,hub_local_address);
    hub_local_config_original=make_hook(0x13dec28,hub_local_config);
}
#endif

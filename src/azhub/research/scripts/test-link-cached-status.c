/* SPDX-License-Identifier: GPL-3.0-only */
/* Portable test of the actual experimental adapter with a simulated native
 * interface and load ledger. This does not validate firmware callback ordering. */
#include <assert.h>
#include <pthread.h>
#include <string.h>
#include "../../linkfake/eject-detach.h"
#define LF_SAFE_EJECT 1
#define EJECT_MAX 16
#include "../../linkfake/eject-ui-model.h"
#define PROBE_COMMITTED_STATUS_ORIGIN 1
static pthread_mutex_t safe_lock=PTHREAD_MUTEX_INITIALIZER;
static uintptr_t native_owner=17;
static uintptr_t safe_native_interface(uintptr_t *a,uintptr_t *b) {
    (void)a;(void)b;return native_owner;
}
static struct safe_load {
    uintptr_t controller,request;
    int source,loading,deck,failed;
    uint32_t content_id;
    uint64_t epoch,media;
    int committed_valid,committed_source;
    uint32_t committed_content_id;
    uint64_t committed_media,committed_epoch;
    struct eject_item committed_origin;
} safe_loads[16];
static struct hub_detach_pool safe_detach_pool;
static struct { uintptr_t controller; uint64_t epoch; uint32_t content_id; int valid,ticket; } safe_cached_decks[4];
#include "link-cached-status-probe.h"
int main(void) {
    uint8_t packet[1152]={0};
    memcpy(packet,"Qspt1WmJOL",10);packet[10]=10;packet[0x21]=2;
    packet[0x28]=1;packet[0x29]=3;packet[0x2c]=0;packet[0x2e]=1;packet[0x2f]=239;
    safe_loads[0]=(struct safe_load){.controller=23,.deck=1,.epoch=9,.media=123,.content_id=495};
    safe_cached_decks[1].controller=23;safe_cached_decks[1].epoch=9;
    safe_cached_decks[1].content_id=495;
    safe_cached_decks[1].valid=1;safe_cached_decks[1].ticket=0;
    safe_detach_pool.slots[0]=(struct hub_detach_ticket){.owner=17,.media=123,.connection=4,
        .references=1,.phase=HUB_DETACH_PENDING,.submitted=1};
    assert(probe_cached_status_private(packet,sizeof packet));
    safe_detach_pool.slots[0].phase=HUB_DETACH_DONE;
    assert(probe_cached_status_private(packet,sizeof packet));
    /* Admission mutates the ledger before success. The old cached track stays
     * private through a pending/failed native same-ID replacement. */
    safe_loads[0].epoch++;
    safe_loads[0].media++;safe_loads[0].loading=1;
    assert(probe_cached_status_private(packet,sizeof packet));
    safe_loads[0].loading=0;safe_loads[0].failed=1;
    assert(probe_cached_status_private(packet,sizeof packet));
    /* A failed different-ID attempt likewise cannot overwrite cached origin. */
    safe_loads[0].content_id=800;
    assert(probe_cached_status_private(packet,sizeof packet));
    packet[0x2e]=3;packet[0x2f]=32;
    assert(!probe_cached_status_private(packet,sizeof packet));
    packet[0x2e]=1;packet[0x2f]=239;
    /* Existing success/unload callback retires the retained proof. Native USB
     * and external replacement status with the same content ID stays visible. */
    safe_cached_decks[1].valid=0;
    assert(!probe_cached_status_private(packet,sizeof packet));
    packet[0x28]=3;assert(!probe_cached_status_private(packet,sizeof packet));packet[0x28]=1;
    safe_cached_decks[1].valid=1;
    safe_loads[0].deck=0;assert(!probe_cached_status_private(packet,sizeof packet));safe_loads[0].deck=1;
    safe_loads[0].controller++;assert(!probe_cached_status_private(packet,sizeof packet));safe_loads[0].controller--;
    packet[0x2f]--;assert(!probe_cached_status_private(packet,sizeof packet));packet[0x2f]++;
    native_owner++;assert(!probe_cached_status_private(packet,sizeof packet));native_owner--;
    native_owner=0;assert(!probe_cached_status_private(packet,sizeof packet));native_owner=17;
    safe_detach_pool.slots[0].connection=0;assert(!probe_cached_status_private(packet,sizeof packet));
    safe_detach_pool.slots[0].connection=4;
    safe_detach_pool.slots[0].references=0;assert(!probe_cached_status_private(packet,sizeof packet));
    safe_detach_pool.slots[0].references=1;
    safe_detach_pool.slots[0].phase=0;assert(!probe_cached_status_private(packet,sizeof packet));
    safe_detach_pool.slots[0].phase=HUB_DETACH_DONE;
    safe_cached_decks[1].epoch=0;assert(!probe_cached_status_private(packet,sizeof packet));safe_cached_decks[1].epoch=9;
    safe_detach_pool.slots[0].submitted=0;assert(!probe_cached_status_private(packet,sizeof packet));
    safe_detach_pool.slots[0].submitted=1;
    safe_cached_decks[1].ticket=HUB_DETACH_SLOTS;assert(!probe_cached_status_private(packet,sizeof packet));
    safe_cached_decks[1].ticket=-1;assert(!probe_cached_status_private(packet,sizeof packet));
    safe_cached_decks[1].ticket=0;
    /* Abrupt physical disconnect creates no detach ticket. The last completed
     * private load retains its exact physical origin independently of pending
     * requests, including same-content native requests and failed replacements. */
    safe_cached_decks[1].valid=0;
    safe_loads[0].committed_valid=1;safe_loads[0].committed_source=0;
    safe_loads[0].committed_content_id=495;safe_loads[0].committed_media=123;
    safe_loads[0].committed_epoch=9;
    safe_loads[0].committed_origin=(struct eject_item){.generation=40,.devnum=7,.usb="/fixture/usb-A"};
    assert(probe_cached_status_private(packet,sizeof packet));
    /* Before USB STOP has captured the native disconnect controller, committed
     * load origin remains valid. Only the detach-ticket fallback needs owner. */
    native_owner=0;
    assert(probe_cached_status_private(packet,sizeof packet));
    native_owner=17;
    safe_loads[0].loading=1;safe_loads[0].epoch=10;safe_loads[0].media=999;
    assert(probe_cached_status_private(packet,sizeof packet));
    safe_loads[0].loading=0;safe_loads[0].failed=1;safe_loads[0].content_id=800;
    assert(probe_cached_status_private(packet,sizeof packet));
    safe_loads[0].committed_origin.generation=0;assert(!probe_cached_status_private(packet,sizeof packet));
    safe_loads[0].committed_origin.generation=40;
    safe_loads[0].committed_origin.devnum=0;assert(!probe_cached_status_private(packet,sizeof packet));
    safe_loads[0].committed_origin.devnum=7;
    safe_loads[0].committed_origin.usb[0]=0;assert(!probe_cached_status_private(packet,sizeof packet));
    strcpy(safe_loads[0].committed_origin.usb,"/fixture/usb-A");
    safe_loads[0].committed_epoch=0;assert(!probe_cached_status_private(packet,sizeof packet));safe_loads[0].committed_epoch=9;
    safe_loads[0].committed_media=0;assert(!probe_cached_status_private(packet,sizeof packet));safe_loads[0].committed_media=123;
    safe_loads[0].committed_source=-1;assert(!probe_cached_status_private(packet,sizeof packet));
    safe_loads[0].committed_source=8;assert(!probe_cached_status_private(packet,sizeof packet));safe_loads[0].committed_source=0;
    safe_loads[0].committed_content_id=800;assert(!probe_cached_status_private(packet,sizeof packet));safe_loads[0].committed_content_id=495;
    safe_loads[0].controller=0;assert(!probe_cached_status_private(packet,sizeof packet));safe_loads[0].controller=23;
    safe_loads[0].deck=0;assert(!probe_cached_status_private(packet,sizeof packet));safe_loads[0].deck=1;
    /* Matched successful native replacement or real unload retires provenance. */
    safe_loads[0].committed_valid=0;
    assert(!probe_cached_status_private(packet,sizeof packet));
    for (size_t n=0;n<sizeof packet;n++) assert(!probe_cached_status_private(packet,n));
    assert(!probe_cached_status_private(NULL,sizeof packet));
    packet[0x21]=0;assert(!probe_cached_status_private(packet,sizeof packet));
    packet[0x21]=5;assert(!probe_cached_status_private(packet,sizeof packet));
    packet[0x21]=2;packet[10]=6;assert(!probe_cached_status_private(packet,sizeof packet));
    packet[10]=10;packet[0]='!';assert(!probe_cached_status_private(packet,sizeof packet));
    return 0;
}

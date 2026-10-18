/* SPDX-License-Identifier: GPL-3.0-only */
/* SDK harness adapter for the actual production surface callbacks. Native
 * filesystem/server teardown is a separate emulator gate, not simulated here. */
#define _GNU_SOURCE
#define LF_SAFE_EJECT 1
#include "azmod-mod.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#define EJECT_ID "usb-hub-support"
#define EJECT_MAX 8
#include "eject-ui-model.h"
static const az_core_v1 *lf_core;
static pthread_mutex_t eject_lock=PTHREAD_MUTEX_INITIALIZER;
static struct eject_inventory eject_live,eject_shown;
static int eject_active,eject_port,eject_selected,eject_closing,eject_focus_owned;
static _Atomic unsigned eject_keys_down,eject_click_edges,eject_encoder_turn_calls;
static _Atomic int eject_capture,eject_pin_guard;
static uint64_t eject_close_deadline;
static unsigned eject_click_origin,eject_ui_turns;
static az_surface *eject_surface;
static _Atomic unsigned fixture_requests;
static _Atomic int fixture_worker_started;
static uint64_t eject_clock_ms(void) {
    struct timespec now;clock_gettime(CLOCK_MONOTONIC,&now);
    return (uint64_t)now.tv_sec*1000+(uint64_t)now.tv_nsec/1000000;
}
static void safe_eject_submit(const struct eject_item *item,int port) {
    (void)item;(void)port;
    atomic_fetch_add(&fixture_requests,1);
}
#include "eject-view.h"
static void *fixture_worker(void *unused) {
    (void)unused;
    for (unsigned n=0;;n++) {
        pthread_mutex_lock(&eject_lock);
        eject_shown.items[n%8].state=n%4;
        pthread_mutex_unlock(&eject_lock);
        usleep(1000);
    }
    return NULL;
}
static int fixture_init(const az_core_v1 *core) {
    lf_core=core;
    pthread_mutex_lock(&eject_lock);
    eject_active=1;eject_port=1;eject_selected=0;
    eject_live.count=eject_shown.count=8;
    for (int n=0;n<8;n++) {
        struct eject_item *item=&eject_shown.items[n];
        snprintf(item->label,sizeof item->label,"Storage %d - long label for clipping and Unicode: USB",n+1);
        snprintf(item->location,sizeof item->location,"%d",n+1);
        snprintf(item->topology,sizeof item->topology,"fixture-%d",n);
        item->port=1;eject_live.items[n]=*item;
    }
    pthread_mutex_unlock(&eject_lock);
    az_surface_v1 surface={.size=sizeof surface,.layer=AZ_LAYER_TOP,.views=0x7f,
        .on_paint=eject_paint,.on_input=eject_input};
    eject_surface=core->surface_create(EJECT_ID,&surface);
    if (!eject_surface) return -1;
    core->surface_place(eject_surface,(az_rect){280,160,720,500});
    core->surface_show(eject_surface,1);
    if (!atomic_exchange(&fixture_worker_started,1) &&
        core->thread_start(EJECT_ID,"UIFixture",fixture_worker,NULL)) return -2;
    return 0;
}
static void fixture_tick(void) {
    static unsigned ticks;
    if (++ticks%30) return;
    pthread_mutex_lock(&eject_lock);eject_active=1;pthread_mutex_unlock(&eject_lock);
    lf_core->surface_show(eject_surface,1);
    lf_core->surface_place(eject_surface,(az_rect){280,160,720,500});
    lf_core->surface_redraw(eject_surface);
}
static const char *const fixture_tags[]={"any",NULL};
static const az_mod_v1 fixture={.abi=AZ_MOD_ABI,.size=sizeof fixture,.id=EJECT_ID,.fw_tags=fixture_tags,
    .init=fixture_init,.tick=fixture_tick};
AZ_MOD_EXPORT const az_mod_v1 *az_mod_v1_entry(void) {return &fixture;}

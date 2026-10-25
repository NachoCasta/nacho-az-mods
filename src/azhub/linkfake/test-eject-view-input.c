/* SPDX-License-Identifier: GPL-3.0-only */
/* Real surface callbacks: stale events, physical click proof and held release. */
#define _GNU_SOURCE
#define LF_SAFE_EJECT 1
#include "azmod-mod.h"
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
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
static unsigned submissions,hide_calls,focus_releases,collapse_calls;
static char submitted[64];
static uint64_t eject_clock_ms(void) {return 1000;}
static void safe_eject_submit(const struct eject_item *item,int port) {
    assert(item && port==-1);submissions++;
    snprintf(submitted,sizeof submitted,"%s",item->disk);
}
#include "eject-view.h"
static void show(az_surface *s,int on) {(void)s;assert(!on);hide_calls++;}
static void place(az_surface *s,az_rect r) {(void)s;assert(r.w==1 && r.h==1);collapse_calls++;}
static void redraw(az_surface *s) {(void)s;}
static int focus(az_surface *s,int on) {(void)s;assert(!on);focus_releases++;return 1;}
static void log_message(const char *id,const char *format,...) {(void)id;(void)format;}
static void banner(const char *id,const char *text,int ms) {(void)id;(void)text;(void)ms;}
static void reset(void) {
    memset(&eject_shown,0,sizeof eject_shown);eject_shown.count=2;
    for(int n=0;n<2;n++) {
        struct eject_item *i=&eject_shown.items[n];
        snprintf(i->disk,sizeof i->disk,"sd%c",'b'+n);
        snprintf(i->usb,sizeof i->usb,"2-1.%d",n+1);
        snprintf(i->topology,sizeof i->topology,"fixture/%d",n+1);
        i->generation=n+1;i->devnum=n+1;i->port=1;
    }
    eject_live=eject_shown;eject_selected=0;eject_port=1;eject_active=1;
    eject_closing=0;eject_capture=1;eject_focus_owned=1;eject_pin_guard=0;
    eject_keys_down=0;eject_click_edges=0;eject_click_origin=0;
    submissions=hide_calls=focus_releases=collapse_calls=0;*submitted=0;
}
int main(void) {
    static az_core_v1 api={.surface_show=show,.surface_place=place,
        .surface_redraw=redraw,.surface_focus=focus,.log=log_message,.banner=banner};
    lf_core=&api;
    reset();
    az_event event={.kind=AZ_EV_CLICK};
    assert(eject_input(&event,NULL)==1 && !submissions && eject_active);
    event=(az_event){.kind=AZ_EV_TAP,.x=850,.y=300};
    assert(eject_input(&event,NULL)==1 && eject_selected==1);
    event=(az_event){.kind=AZ_EV_TAP,.x=800,.y=620};
    assert(eject_input(&event,NULL)==1 && submissions==1 && !strcmp(submitted,"sdc"));
    assert(hide_calls==1 && focus_releases==1 && !eject_active && !eject_capture);
    assert(eject_input(&event,NULL)==1 && submissions==1); /* queued after hide */
    reset();eject_click_edges=1;eject_keys_down=1;event=(az_event){.kind=AZ_EV_CLICK};
    assert(eject_input(&event,NULL)==1 && submissions==1 && eject_closing && eject_capture);
    assert(!focus_releases && !hide_calls && collapse_calls==1);
    assert(eject_input(&event,NULL)==1 && submissions==1);
    eject_keys_down=0;eject_close();
    assert(focus_releases==1 && hide_calls==1 && !eject_capture && !eject_closing);
    assert(eject_input(&event,NULL)==1 && submissions==1);
    reset();eject_live.items[0].generation++;
    event=(az_event){.kind=AZ_EV_TAP,.x=800,.y=620};
    assert(eject_input(&event,NULL)==1 && !submissions && hide_calls==1);
    reset();event=(az_event){.kind=AZ_EV_TAP,.x=400,.y=620};
    assert(eject_input(&event,NULL)==1 && !submissions && hide_calls==1 && focus_releases==1);
    puts("eject view input: physical click proof, exact touch target, held close, late-event refusal, replacement-source refusal and cancel passed");
}

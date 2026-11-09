/* SPDX-License-Identifier: GPL-3.0-only */
/* Persistent eject feedback. Publication is thread-safe; all SDK work stays on UI. */
#ifndef HUB_EJECT_NOTICE_H
#define HUB_EJECT_NOTICE_H
#include <pthread.h>
#include <stdio.h>
#include <string.h>
enum eject_notice_phase { EJECT_NOTICE_HIDDEN, EJECT_NOTICE_WORKING, EJECT_NOTICE_SAFE, EJECT_NOTICE_FAILED };
struct eject_notice_state { enum eject_notice_phase phase; int hub; char target[128],reason[256]; unsigned revision; };
static pthread_mutex_t eject_notice_lock=PTHREAD_MUTEX_INITIALIZER;
static struct eject_notice_state eject_notice;
static az_surface *eject_notice_surface;
static unsigned eject_notice_painted;
static int eject_notice_visible;
static struct eject_notice_state eject_notice_snapshot(void) {
    pthread_mutex_lock(&eject_notice_lock);
    struct eject_notice_state state=eject_notice;
    pthread_mutex_unlock(&eject_notice_lock);return state;
}
static void eject_notice_begin(const struct eject_item *item,int port) {
    pthread_mutex_lock(&eject_notice_lock);
    eject_notice.phase=EJECT_NOTICE_WORKING;eject_notice.hub=port>=0;
    if (port>=0) snprintf(eject_notice.target,sizeof eject_notice.target,"Hub on USB%d",port+1);
    else snprintf(eject_notice.target,sizeof eject_notice.target,"%s",item && *item->label ? item->label:"USB drive");
    *eject_notice.reason=0;eject_notice.revision++;
    pthread_mutex_unlock(&eject_notice_lock);
}
static void eject_notice_finish(int success,const char *reason) {
    pthread_mutex_lock(&eject_notice_lock);
    eject_notice.phase=success ? EJECT_NOTICE_SAFE:EJECT_NOTICE_FAILED;
    snprintf(eject_notice.reason,sizeof eject_notice.reason,"%s",reason ? reason:"");
    eject_notice.revision++;
    pthread_mutex_unlock(&eject_notice_lock);
}
/* A later mount-owner change must not leave an old green confirmation visible. */
static void eject_notice_invalidate(const char *reason) {
    pthread_mutex_lock(&eject_notice_lock);
    if (eject_notice.phase==EJECT_NOTICE_SAFE) {
        eject_notice.phase=EJECT_NOTICE_FAILED;
        snprintf(eject_notice.reason,sizeof eject_notice.reason,"%s",reason);
        eject_notice.revision++;
    }
    pthread_mutex_unlock(&eject_notice_lock);
}
static int eject_notice_dismiss(void) {
    pthread_mutex_lock(&eject_notice_lock);
    int working=eject_notice.phase==EJECT_NOTICE_WORKING;
    if (!working && eject_notice.phase!=EJECT_NOTICE_HIDDEN) {
        eject_notice.phase=EJECT_NOTICE_HIDDEN;eject_notice.revision++;
    }
    pthread_mutex_unlock(&eject_notice_lock);return !working;
}
static void eject_notice_tick(void) {
    if (!eject_notice_surface) return;
    struct eject_notice_state state=eject_notice_snapshot();
    int visible=state.phase!=EJECT_NOTICE_HIDDEN;
    if (visible!=eject_notice_visible) {
        lf_core->surface_show(eject_notice_surface,visible);eject_notice_visible=visible;
    }
    if (state.revision!=eject_notice_painted) {
        eject_notice_painted=state.revision;
        if (visible) lf_core->surface_redraw(eject_notice_surface);
    }
}
static const char *eject_notice_title(struct eject_notice_state state) {
    if (state.phase==EJECT_NOTICE_WORKING) return state.hub ? "Ejecting hub...":"Ejecting USB...";
    if (state.phase==EJECT_NOTICE_SAFE) return "Safe to remove";
    return state.hub ? "Couldn't eject hub":"Couldn't eject USB";
}
static void eject_notice_paint(az_draw *draw,az_rect area,void *unused) {
    (void)unused;struct eject_notice_state state=eject_notice_snapshot();
    if (state.phase==EJECT_NOTICE_HIDDEN || area.x<0 || area.y<0 || area.w<=0 || area.h<=0 ||
        area.x>1280 || area.y>800 || area.w>1280-area.x || area.h>800-area.y) return;
    uint32_t accent=state.phase==EJECT_NOTICE_SAFE ? 0x75d6a0:state.phase==EJECT_NOTICE_FAILED ? 0xffbc73:0x83b9ef;
    eject_box(draw,area,area.x,area.y,area.w,area.h,0x151719,0);
    eject_box(draw,area,area.x,area.y,area.w,area.h,0x4b5055,1);
    eject_box(draw,area,area.x,area.y,4,area.h,accent,0);
    eject_text_fit(draw,area,area.x+26,area.y+18,eject_notice_title(state),26,accent);
    eject_text_fit(draw,area,area.x+26,area.y+66,state.target,21,0xf1f2f3);
    const char *instruction=state.phase==EJECT_NOTICE_WORKING ? "Keep the USB connected. Please wait.":
        state.phase==EJECT_NOTICE_SAFE ? (state.hub ? "You can now unplug the hub.":"You can now unplug this USB."):
        "Keep the USB connected. Eject it again when ready.";
    eject_text_fit(draw,area,area.x+26,area.y+106,instruction,18,0xf1f2f3);
    if (state.phase==EJECT_NOTICE_FAILED)
        eject_text_fit(draw,area,area.x+26,area.y+141,state.reason,15,0xa6acb1);
    if (state.phase!=EJECT_NOTICE_WORKING) {
        eject_box(draw,area,area.x+area.w-134,area.y+188,108,36,0x30363c,0);
        eject_box(draw,area,area.x+area.w-134,area.y+188,108,36,0x4b5055,1);
        eject_text_fit(draw,area,area.x+area.w-107,area.y+192,"Done",18,0xf1f2f3);
    }
}
static int eject_notice_input(const az_event *event,void *unused) {
    (void)unused;
    /* This panel never takes knob/BACK focus and cannot cancel an active eject. */
    if (event->kind!=AZ_EV_TAP) return 0;
    if (event->x>=826 && event->x<934 && event->y>=348 && event->y<384) {
        eject_notice_dismiss();eject_notice_tick();
    }
    return 1;
}
static int eject_notice_init(void) {
    az_surface_v1 surface={.size=sizeof surface,.layer=AZ_LAYER_TOP,.views=0x7f,
        .on_paint=eject_notice_paint,.on_input=eject_notice_input};
    eject_notice_surface=lf_core->surface_create(EJECT_ID,&surface);
    if (!eject_notice_surface) return -80;
    lf_core->surface_place(eject_notice_surface,(az_rect){320,160,640,244});
    return 0;
}
#endif

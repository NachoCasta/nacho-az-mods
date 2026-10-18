/* SPDX-License-Identifier: GPL-3.0-only */
/* Shared production surface callbacks; native I/O stays in the coordinator. */
#ifndef HUB_EJECT_VIEW_H
#define HUB_EJECT_VIEW_H
#include <stdatomic.h>
/* Bound all text to the supplied surface, including unusually long UTF-8
 * labels and the small areas used by the SDK harness. No heap allocation. */
static void eject_text_fit(az_draw *draw,az_rect area,int x,int y,const char *text,int size,uint32_t color) {
    int right=area.x+area.w,bottom=area.y+area.h;
    if (x<area.x || x>=right || y<area.y || y+(size*3+1)/2>bottom) return;
    int available=right-x-4;
    if (available<=0) return;
    int width=lf_core->text_width(text,size);
    if (width<0) return;
    if (width<=available) { lf_core->draw_text(draw,x,y,text,size,color); return; }
    if (lf_core->text_width("...",size)>available) return;
    char fit[256]; size_t length=strnlen(text,sizeof fit-4),low=0,high=length,best=0;
    while (low<high) {
        size_t middle=low+(high-low+1)/2,cut=middle;
        while (cut && ((unsigned char)text[cut]&0xc0)==0x80) cut--;
        memcpy(fit,text,cut);memcpy(fit+cut,"...",4);
        if (lf_core->text_width(fit,size)<=available) {best=cut;low=middle;}
        else high=middle-1;
    }
    memcpy(fit,text,best);memcpy(fit+best,"...",4);
    if (lf_core->text_width(fit,size)<=available) lf_core->draw_text(draw,x,y,fit,size,color);
}
static void eject_close(void) {
    pthread_mutex_lock(&eject_lock); eject_active=0; pthread_mutex_unlock(&eject_lock);
    /* Keep ownership through the initiating key's release. Otherwise SOURCE can
     * see the held click after focus returns and activate the core's menu row. */
    if (eject_keys_down) {
        if (!eject_closing) eject_close_deadline=eject_clock_ms()+1500;
        eject_closing=1;
        lf_core->surface_place(eject_surface,(az_rect){0,0,1,1});
        lf_core->surface_redraw(eject_surface);
        return;
    }
    if (eject_focus_owned) {
        eject_pin_guard=1;
        lf_core->surface_focus(eject_surface,0);
        eject_focus_owned=0;
    }
    lf_core->surface_show(eject_surface,0);
    eject_closing=0;
    eject_capture=0;
}
static void eject_request_one(void) {
    struct eject_item item; int valid=0;
    pthread_mutex_lock(&eject_lock);
    if (eject_selected>=0 && eject_selected<eject_shown.count) {
        item=eject_shown.items[eject_selected];
        for (int i=0;i<eject_live.count;i++)
            if (eject_live.items[i].port==item.port && !strcmp(eject_live.items[i].topology,item.topology)) valid=1;
    }
    pthread_mutex_unlock(&eject_lock);
    eject_close();
    if (!valid) { lf_core->banner(EJECT_ID,"USB changed. Open the selector again.",2500); return; }
#ifdef LF_SAFE_EJECT
    safe_eject_submit(&item,-1);
#else
    lf_core->log(EJECT_ID,"eject-ui request-one port=%d disk=%s label=%s (no unmount)",item.port+1,item.disk,item.label);
    lf_core->banner(EJECT_ID,"Eject request recorded. Prototype: USB remains mounted.",3000);
#endif
}
static void eject_paint(az_draw *draw,az_rect area,void *unused) {
    (void)unused;
    struct eject_inventory shown; int selected,port;
    pthread_mutex_lock(&eject_lock); shown=eject_shown; selected=eject_selected; port=eject_port; int active=eject_active; pthread_mutex_unlock(&eject_lock);
    if (!active) return;
    if (area.x<0 || area.y<0 || area.w<=0 || area.h<=0 || area.x>1280 || area.y>800 ||
        area.w>1280-area.x || area.h>800-area.y) return;
    uint32_t bg=lf_core->color(AZ_COL_BG),text=lf_core->color(AZ_COL_TEXT);
    lf_core->fill_rect(draw,area.x,area.y,area.w,area.h,bg);
    lf_core->draw_rect(draw,area.x,area.y,area.w,area.h,2,lf_core->color(AZ_COL_HI));
    char title[80]; snprintf(title,sizeof title,"USB %d hub - select a USB",port+1);
    eject_text_fit(draw,area,area.x+24,area.y+12,title,24,text);
    eject_text_fit(draw,area,area.x+24,area.y+52,
#ifdef LF_SAFE_EJECT
        "Select a USB to safely remove. Other USBs stay available.",
#else
        "Interface preview - no devices will be unmounted",
#endif
        16,lf_core->color(AZ_COL_TEXT_DIM));
    for (int i=0;i<shown.count;i++) {
        int y=area.y+88+i*42;
        if (i==selected && area.w>32 && y+40<=area.y+area.h)
            lf_core->fill_rect(draw,area.x+16,y,area.w-32,40,lf_core->color(AZ_COL_HI));
        char label[220]; snprintf(label,sizeof label,"%s - hub port %s",shown.items[i].label,shown.items[i].location);
#ifdef LF_SAFE_EJECT
        if (shown.items[i].state==2) snprintf(label,sizeof label,"%.120s - port %.24s - safe to remove",shown.items[i].label,shown.items[i].location);
        if (shown.items[i].state==3) snprintf(label,sizeof label,"%.120s - port %.24s - eject failed; retry",shown.items[i].label,shown.items[i].location);
#endif
        eject_text_fit(draw,area,area.x+28,y+5,label,18,lf_core->color(i==selected ? AZ_COL_HI_TEXT:AZ_COL_TEXT));
    }
    if (!shown.count) eject_text_fit(draw,area,area.x+24,area.y+94,"No USB sticks found on this hub",18,text);
    eject_text_fit(draw,area,area.x+30,area.y+450,"Cancel / BACK",19,text);
    eject_text_fit(draw,area,area.x+420,area.y+450,
#ifdef LF_SAFE_EJECT
        "Safely remove",
#else
        "Request eject",
#endif
        19,text);
}
static int eject_input(const az_event *event,void *unused) {
    (void)unused;
    if (eject_closing) return 1;
    if (event->kind==AZ_EV_BACK) { eject_close(); lf_core->log(EJECT_ID,"eject-ui cancel"); return 1; }
    if (event->kind==AZ_EV_KNOB) {
        if (!(eject_ui_turns++%25))
            lf_core->log(EJECT_ID,"eject-ui input-path turns=%u encoder-calls=%u view=%d",
                eject_ui_turns,atomic_load_explicit(&eject_encoder_turn_calls,memory_order_relaxed),lf_core->view());
        pthread_mutex_lock(&eject_lock);
        if (eject_shown.count) eject_selected=(eject_selected+(event->dir>0 ? 1:eject_shown.count-1))%eject_shown.count;
        pthread_mutex_unlock(&eject_lock); lf_core->surface_redraw(eject_surface); return 1;
    }
    if (event->kind==AZ_EV_CLICK) {
        unsigned edge=atomic_load_explicit(&eject_click_edges,memory_order_relaxed);
        if (edge!=eject_click_origin) { eject_click_origin=edge; eject_request_one(); }
        return 1;
    }
    if (event->kind==AZ_EV_TAP) {
        if (event->y>=600) {
            if (event->x<640) eject_close(); else eject_request_one();
            return 1;
        }
        int row=(event->y-248)/42;
        pthread_mutex_lock(&eject_lock);
        if (event->y>=248 && row>=0 && row<eject_shown.count) eject_selected=row;
        pthread_mutex_unlock(&eject_lock); lf_core->surface_redraw(eject_surface); return 1;
    }
    return 1;
}
#endif

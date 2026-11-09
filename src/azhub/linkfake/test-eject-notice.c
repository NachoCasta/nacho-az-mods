/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise real publication, rendering and input; no native eject is simulated. */
#define main existing_view_input_main
#include "test-eject-view-input.c"
#undef main
#include "eject-notice.h"
static int visible,shows,hides,repaints,finished;
static char rendered[2048];
static az_surface_v1 registered;
static az_surface *create_notice(const char *id,const az_surface_v1 *s) {
    (void)id;registered=*s;return (az_surface *)(uintptr_t)3;
}
static void place_notice(az_surface *s,az_rect r) {(void)s;assert(r.x==320 && r.y==160 && r.w==640 && r.h==244);}
static void show_notice(az_surface *s,int on) {(void)s;visible=on;if(on)shows++;else hides++;}
static void redraw_notice(az_surface *s) {(void)s;repaints++;}
static int text_width(const char *text,int size) {return (int)strlen(text)*size/2;}
static void text(az_draw *d,int x,int y,const char *t,int size,uint32_t color) {
    (void)d;(void)color;assert(x>=320 && y>=160 && x+text_width(t,size)<=960 && y+(size*3+1)/2<=404);
    strncat(rendered,t,sizeof rendered-strlen(rendered)-2);strcat(rendered,"|");
}
static void fill(az_draw *d,int x,int y,int w,int h,uint32_t c) {(void)d;(void)c;assert(x>=320 && y>=160 && x+w<=960 && y+h<=404);}
static void border(az_draw *d,int x,int y,int w,int h,int width,uint32_t c) {(void)width;fill(d,x,y,w,h,c);}
static void paint(void) {*rendered=0;eject_notice_paint(NULL,(az_rect){320,160,640,244},NULL);}
static void *complete_worker(void *unused) {
    (void)unused;
    for (unsigned i=0;i<10000;i++) {eject_notice_finish(i&1,"Please retry");eject_notice_invalidate("Mount changed");}
    __atomic_store_n(&finished,1,__ATOMIC_RELEASE);return NULL;
}
int main(void) {
    static const az_core_v1 api={.surface_create=create_notice,.surface_place=place_notice,.surface_show=show_notice,
        .surface_redraw=redraw_notice,.log=log_message,.draw_text=text,.text_width=text_width,.fill_rect=fill,.draw_rect=border};
    lf_core=&api;assert(!eject_notice_init());assert(registered.layer==AZ_LAYER_TOP && registered.views==0x7f);
    struct eject_item item={0};snprintf(item.label,sizeof item.label,"CAMI");
    eject_notice_begin(&item,-1);eject_notice_tick();paint();
    assert(visible && shows==1 && hides==0 && strstr(rendered,"Ejecting USB...") && strstr(rendered,"CAMI|") && strstr(rendered,"Keep the USB connected"));
    az_event done={.kind=AZ_EV_TAP,.x=880,.y=365};
    assert(!eject_notice_dismiss());eject_notice_input(&done,NULL);assert(visible && !hides);
    /* No duration exists: arbitrarily many delayed UI ticks never hide progress. */
    for (unsigned i=0;i<100000;i++) eject_notice_tick();
    assert(visible && shows==1 && hides==0 && repaints==1);
    eject_notice_finish(1,NULL);eject_notice_tick();paint();
    assert(visible && !hides && shows==1 && strstr(rendered,"Safe to remove|") && strstr(rendered,"You can now unplug this USB."));
    eject_notice_invalidate("Mounts changed");eject_notice_tick();paint();
    assert(visible && !hides && strstr(rendered,"Couldn't eject USB") && strstr(rendered,"Keep the USB connected") && !strstr(rendered,"Safe to remove"));
    eject_notice_input(&done,NULL);assert(!visible && hides==1);
    memset(item.label,'W',sizeof item.label-1);item.label[sizeof item.label-1]=0;
    eject_notice_begin(&item,-1);eject_notice_tick();paint();
    assert(strstr(rendered,"...") && strstr(rendered,"Keep the USB connected"));
    eject_notice_begin(NULL,1);eject_notice_tick();paint();
    assert(strstr(rendered,"Ejecting hub") && strstr(rendered,"Hub on USB2"));
    eject_notice_finish(1,NULL);eject_notice_tick();paint();assert(strstr(rendered,"You can now unplug the hub."));
    az_event back={.kind=AZ_EV_BACK};assert(!eject_notice_input(&back,NULL));
    pthread_t worker;assert(!pthread_create(&worker,NULL,complete_worker,NULL));
    while (!__atomic_load_n(&finished,__ATOMIC_ACQUIRE)) {eject_notice_tick();paint();}
    pthread_join(worker,NULL);
    eject_notice_finish(0,"The drive is still busy.");eject_notice_tick();paint();
    assert(strstr(rendered,"Couldn't eject hub") && strstr(rendered,"The drive is still busy."));
    assert(eject_notice_dismiss());eject_notice_tick();assert(!visible);
    puts("Persistent notice: no progress timeout/hide gap, safe/failure in-place transitions, mount invalidation, exact target, hub scope, guarded Done and concurrent worker publication passed");
    return 0;
}

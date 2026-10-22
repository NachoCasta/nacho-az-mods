/* SPDX-License-Identifier: GPL-3.0-only */
/* Native resources remain on device; only detached private images are edited. */
#ifndef HUB_SOURCE_BADGE_NATIVE_COMPOSE_H
#define HUB_SOURCE_BADGE_NATIVE_COMPOSE_H
#include <stdio.h>
#include "source-badge-native-image.h"
#ifndef HUB_NATIVE_COMPOSE_DECODE
#if defined(__aarch64__)
__asm__(".text\n.align 2\n.hidden hub_native_compose_decode_sret\n"
        ".type hub_native_compose_decode_sret,%function\n"
        "hub_native_compose_decode_sret:\nmov x8,x0\nmov x0,x1\nmov x1,x2\n"
        "movz x16,#0xac60\nmovk x16,#0x255,lsl #16\nbr x16\n"
        ".size hub_native_compose_decode_sret,.-hub_native_compose_decode_sret\n");
#endif
extern void hub_native_compose_decode_sret(uintptr_t *,const unsigned char *,int);
#define HUB_NATIVE_COMPOSE_DECODE hub_native_compose_decode_sret
#endif
#ifndef HUB_NATIVE_COMPOSE_DRAW_CROP
#define HUB_NATIVE_COMPOSE_DRAW_CROP(g,image,dx,dy,w,h,sx,sy) \
    ((void (*)(void *,const uintptr_t *,int,int,int,int,int,int,int,int,int))0x254b730) \
        ((g),(image),(dx),(dy),(w),(h),(sx),(sy),(w),(h),0)
#endif
#ifndef HUB_NATIVE_COMPOSE_LOG
#define HUB_NATIVE_COMPOSE_LOG(...) ((void)0)
#endif
/* Native file dimensions, independent of the owning Component's dimensions. */
static int hub_native_compose_geometry(int family,const struct hub_native_pixels *pixels) {
    static const int widths[]={90,95,170},heights[]={90,70,41};
    return pixels && family>=0 && family<3 && pixels->format==2 &&
        pixels->width==widths[family] && pixels->height==heights[family];
}
static int hub_native_compose_fill(uintptr_t *image,const uintptr_t *stock,
                                  struct hub_native_edit_rect rect,uint32_t argb) {
    struct hub_native_pixels dst,original;
    if (!image || !stock || !*image || *image==*stock ||
        !source_badge_native_image_pixels(image,&dst) ||
        !source_badge_native_image_pixels(stock,&original) || dst.data==original.data ||
        dst.format!=2 || rect.x<0 || rect.y<0 || rect.x>=dst.width || rect.y>=dst.height ||
        rect.width<=0 || rect.height<=0 || rect.width>dst.width-rect.x || rect.height>dst.height-rect.y) return 0;
    for (int y=rect.y;y<rect.y+rect.height;y++) for (int x=rect.x;x<rect.x+rect.width;x++) {
        unsigned char *pixel=(unsigned char *)source_badge_native_pixel_at(&dst,x,y);
        if (!pixel) return 0;
        for (unsigned channel=0;channel<4;channel++) pixel[channel]=(unsigned char)(argb>>(8*channel));
    }
    return 1;
}
/* The temporary canvas is itself a detached native image. Draw once with the
 * device's native font, trim its alpha bounds, then use native Graphics blending
 * to transfer the unscaled glyph. USB artwork is never rendered into this canvas. */
static int hub_native_compose_digit(int family,int number,const uintptr_t *stock,
                                    uintptr_t *canvas,struct hub_native_edit_rect *ink) {
    static const char *faces[]={"PDJ-TsukuGoCustom Pro","PDJ-TsukuGoCustom Pro","PioneerDJ-FZLanTingHei-DB-BIG5"};
    static const float heights[]={18.625f,13.125f,24.625f},scales[]={0.975f,0.975f,0.95f};
    struct hub_native_pixels pixels;struct hub_native_graphics graphics={{0}};
    if (!source_badge_native_image_clone(canvas,stock) ||
        !source_badge_native_image_pixels(canvas,&pixels) ||
        !hub_native_compose_fill(canvas,stock,(struct hub_native_edit_rect){0,0,pixels.width,pixels.height},0) ||
        !source_badge_native_graphics_begin(&graphics,canvas)) return 0;
    char digit[2]={(char)('0'+number),0};
    int ok=source_badge_native_text_options(&graphics,4,0,pixels.width-8,pixels.height,digit,
        heights[family],0xffffffff,1,faces[family],0,scales[family]);
    source_badge_native_graphics_end(&graphics);
    if (!ok) return 0;
    int left=pixels.width,top=pixels.height,right=-1,bottom=-1;
    for (int y=0;y<pixels.height;y++) for (int x=0;x<pixels.width;x++) {
        const unsigned char *pixel=source_badge_native_pixel_at(&pixels,x,y);
        if (pixel && pixel[3]) {
            if (x<left) left=x;
            if (x>right) right=x;
            if (y<top) top=y;
            if (y>bottom) bottom=y;
        }
    }
    if (right<left || bottom<top || left==0 || top==0 || right==pixels.width-1 || bottom==pixels.height-1) return 0;
    *ink=(struct hub_native_edit_rect){left,top,right-left+1,bottom-top+1};
    return 1;
}
/* Compose one cache entry before hook installation. Caller retains the returned
 * native Image reference and releases it with source_badge_native_image_release.
 * Native USB1/2 icon, caption, digit and spacing stay literal. Only digits3..8
 * use the user-approved nearest native face. Source adds one pixel vertically
 * to align its solid glyph rows with the untouched native USB caption. */
static int source_badge_native_compose(int family,int number,uintptr_t *out) {
    static const char *names[]={"Source","StatusArea","Waveform"};
    static const struct hub_native_edit_rect digit_regions[]={
        {62,49,18,23},{61,40,14,18},{81,7,20,27}};
    static const int anchors[]={63,62,82},baselines[]={69,53,28};
    uintptr_t stock=0,private_image=0,canvas=0,frame=0,marker=0;
    struct hub_native_graphics graphics={{0}},marker_graphics={{0}};
    struct hub_native_pixels pixels;
    struct hub_native_edit_rect edits[2],ink={0};size_t edit_count=1;
    int ok=0;
    if (!out || *out || family<0 || family>2 || number<1 || number>8) return 0;
    char path[160];
    snprintf(path,sizeof path,"/home/root/pdj/DynamicLoad/DeviceIcon/img_%s_USB%d.png",names[family],number==1 ? 1:2);
    if (!source_badge_native_image_load(&stock,path) ||
        !source_badge_native_image_pixels(&stock,&pixels) || !hub_native_compose_geometry(family,&pixels) ||
        !source_badge_native_image_clone(&private_image,&stock)) goto cleanup;
    int header=family==0 ? 22:16;
    edits[0]=(struct hub_native_edit_rect){0,0,family==2 ? 32:pixels.width,family==2 ? pixels.height:header};
    if (family==2 && !hub_native_compose_fill(&private_image,&stock,edits[0],0)) goto cleanup;
    int digit_y=0;
    if (number>=3) {
        edits[1]=digit_regions[family];edit_count=2;
        if (!hub_native_compose_digit(family,number,&stock,&canvas,&ink)) goto cleanup;
        digit_y=baselines[family]-ink.height;
        HUB_NATIVE_COMPOSE_LOG("native-digit-bounds family=%d number=%d crop=%d,%d,%d,%d destination=%d,%d",
            family,number,ink.x,ink.y,ink.width,ink.height,anchors[family],digit_y);
        if (anchors[family]<edits[1].x || digit_y<edits[1].y ||
            ink.width>edits[1].x+edits[1].width-anchors[family] ||
            ink.height>edits[1].y+edits[1].height-digit_y ||
            !hub_native_compose_fill(&private_image,&stock,edits[1],family==1 ? 0xff232323:0)) goto cleanup;
    }
    if (!source_badge_native_graphics_begin(&graphics,&private_image)) goto cleanup;
    if (family<2) {
        if (!source_badge_native_fill_rect(&graphics,0,0,(float)pixels.width,(float)header,0xffe1e1e1) ||
            !source_badge_native_text(&graphics,0,0,pixels.width,header,"HUB",family==0 ? 20.0f:13.0f,0xff202020,1,NULL)) goto cleanup;
    } else {
        HUB_NATIVE_COMPOSE_DECODE(&frame,(const unsigned char *)0x2e37d40,0xcc);
        int width=0,height=0;
        if (!source_badge_native_image_geometry(&frame,&width,&height) || width!=22 || height!=22 ||
            !source_badge_native_image_clone(&marker,&frame) ||
            !source_badge_native_graphics_begin(&marker_graphics,&marker) ||
            !source_badge_native_fill_rect(&marker_graphics,1,1,20,20,0xff000000) ||
            !source_badge_native_text(&marker_graphics,1,1,20,20,"H",18.0f,0xffe1e1e1,1,NULL)) goto cleanup;
        source_badge_native_graphics_end(&marker_graphics);
        size_t checked=0,different=0;struct hub_native_edit_rect interior={1,1,20,20};
        if (!source_badge_native_preserved(&frame,&marker,&interior,1,&checked,&different) || different ||
            !source_badge_native_fill_rect(&graphics,6,9,25,25,0xff232323) ||
            !source_badge_native_image_draw(&graphics,&marker,10,9)) goto cleanup;
    }
    if (number>=3) HUB_NATIVE_COMPOSE_DRAW_CROP(&graphics,&canvas,anchors[family],digit_y,ink.width,ink.height,ink.x,ink.y);
    source_badge_native_graphics_end(&graphics);
    size_t checked=0,different=0;
    if (!source_badge_native_preserved(&stock,&private_image,edits,edit_count,&checked,&different)) goto cleanup;
    HUB_NATIVE_COMPOSE_LOG("native-body-preserved family=%d number=%d checked=%zu different=%zu",
        family,number,checked,different);
    if (different) goto cleanup;
    *out=private_image;private_image=0;ok=1;
cleanup:
    source_badge_native_graphics_end(&marker_graphics);
    source_badge_native_graphics_end(&graphics);
    source_badge_native_image_release(&marker);source_badge_native_image_release(&frame);
    source_badge_native_image_release(&canvas);source_badge_native_image_release(&private_image);
    source_badge_native_image_release(&stock);
    return ok;
}
#endif

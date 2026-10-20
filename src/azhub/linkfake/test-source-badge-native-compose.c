/* SPDX-License-Identifier: GPL-3.0-only */
/* Synthetic native resources and drawing adapters; no proprietary pixels. */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void image_file(uintptr_t *,const void *),string_new(void *,const char *),string_delete(void *);
static void image_copy(uintptr_t *,const uintptr_t *),image_detach(uintptr_t *),image_delete(uintptr_t *);
static void graphics_new(void *,const uintptr_t *),graphics_delete(void *),graphics_draw(void *,const uintptr_t *,int,int);
static void graphics_save(void *),graphics_restore(void *),graphics_color(void *,const uint32_t *);
static void graphics_fill(void *,float,float,float,float),graphics_font(void *,const void *);
static void graphics_text(void *,const void *,const int *,const int *,int,float);
static void font_factory(void *,float),font_family(void *,const void *,int,float),font_delete(void *),font_scale(void *,float);
static void frame_decode(uintptr_t *,const unsigned char *,int);
static void draw_crop(void *,const uintptr_t *,int,int,int,int,int,int);
#define HUB_NATIVE_IMAGE_FILE image_file
#define HUB_NATIVE_STRING_NEW string_new
#define HUB_NATIVE_STRING_DELETE string_delete
#define HUB_NATIVE_IMAGE_COPY image_copy
#define HUB_NATIVE_IMAGE_DETACH image_detach
#define HUB_NATIVE_IMAGE_DELETE image_delete
#define HUB_NATIVE_GRAPHICS_NEW_IMAGE graphics_new
#define HUB_NATIVE_GRAPHICS_DELETE_IMAGE graphics_delete
#define HUB_NATIVE_GRAPHICS_IMAGE_AT graphics_draw
#define HUB_NATIVE_GRAPHICS_SAVE graphics_save
#define HUB_NATIVE_GRAPHICS_RESTORE graphics_restore
#define HUB_NATIVE_GRAPHICS_COLOR graphics_color
#define HUB_NATIVE_GRAPHICS_FILL_FLOAT graphics_fill
#define HUB_NATIVE_GRAPHICS_FONT graphics_font
#define HUB_NATIVE_GRAPHICS_TEXT graphics_text
#define HUB_NATIVE_FONT_FACTORY font_factory
#define HUB_NATIVE_FONT_NEW_FAMILY font_family
#define HUB_NATIVE_FONT_DELETE font_delete
#define HUB_NATIVE_FONT_HORIZONTAL_SCALE font_scale
#define HUB_NATIVE_COMPOSE_DECODE frame_decode
#define HUB_NATIVE_COMPOSE_DRAW_CROP draw_crop
#include "source-badge-native-compose.h"
static int live_images,live_strings,live_fonts,live_graphics,family,loaded_number,font_draws,digit_draws,crop_draws;
static int operation,fail_at,corrupt_outside,alias_clone,empty_ink,bad_geometry,oversized_ink;
static uintptr_t stock_handles[3][2],frame_handle;
struct context { void *graphics; uintptr_t image; uint32_t color,saved; int saving; };
static struct context contexts[4];
static int fail(void) { return ++operation==fail_at; }
static void put_int(uintptr_t image,int offset,int value) { memcpy((void *)(image+offset),&value,4); }
static int get_int(uintptr_t image,int offset) { int value;memcpy(&value,(void *)(image+offset),4);return value; }
static unsigned char *data(uintptr_t image) { return *(unsigned char **)(image+0x48); }
static uintptr_t make_image(int width,int height) {
    unsigned char *object=calloc(1,0x58),*pixels=calloc((size_t)width*height,4);assert(object&&pixels);
    uintptr_t image=(uintptr_t)object;*(uintptr_t *)image=0x34f6228;
    put_int(image,8,1);put_int(image,0xc,2);put_int(image,0x10,width);put_int(image,0x14,height);
    put_int(image,0x50,4);put_int(image,0x54,width*4);memcpy(object+0x48,&pixels,sizeof pixels);live_images++;return image;
}
static void image_copy(uintptr_t *out,const uintptr_t *source) { *out=*source;if(*out)put_int(*out,8,get_int(*out,8)+1); }
static void image_delete(uintptr_t *image) {
    if(!*image) return;
    int refs=get_int(*image,8)-1;assert(refs>=0);put_int(*image,8,refs);
    if(!refs){free(data(*image));free((void *)*image);live_images--;}
}
static void image_detach(uintptr_t *image) {
    if(alias_clone)return;
    uintptr_t original=*image;int width=get_int(original,0x10),height=get_int(original,0x14);
    image_delete(image);if(fail()){*image=0;return;}
    *image=make_image(width,height);memcpy(data(*image),data(original),(size_t)width*height*4);
}
static void string_new(void *out,const char *text) { *(char **)out=strdup(text);assert(*(char **)out);live_strings++; }
static void string_delete(void *text) { free(*(char **)text);*(char **)text=NULL;live_strings--; }
static void image_file(uintptr_t *out,const void *file) {
    const char *path=*(const char *const *)file;
    const char *name=strstr(path,"img_");assert(name);
    family=strstr(name,"Source") ? 0:strstr(name,"StatusArea") ? 1:2;
    loaded_number=strstr(name,"USB1.png") ? 1:2;
    assert(strstr(name,loaded_number==1 ? "USB1.png":"USB2.png"));
    if(fail()){*out=0;return;}
    if(bad_geometry) { *out=make_image(89,90);return; }
    image_copy(out,&stock_handles[family][loaded_number-1]);
}
static struct context *context(void *graphics) {
    for(unsigned i=0;i<4;i++) if(contexts[i].graphics==graphics) return &contexts[i];
    assert(0);return NULL;
}
static void graphics_new(void *graphics,const uintptr_t *image) {
    if(fail())return;
    for(unsigned i=0;i<4;i++)if(!contexts[i].graphics){
        contexts[i]=(struct context){.graphics=graphics,.image=*image};
        ((uintptr_t *)graphics)[0]=*image;((uintptr_t *)graphics)[1]=1;live_graphics++;return;
    }assert(0);
}
static void graphics_delete(void *graphics) {
    if(!((uintptr_t *)graphics)[1])return;
    struct context *ctx=context(graphics);assert(!ctx->saving);memset(ctx,0,sizeof *ctx);live_graphics--;
    ((uintptr_t *)graphics)[0]=((uintptr_t *)graphics)[1]=0;
}
static void graphics_save(void *graphics) {struct context *ctx=context(graphics);assert(!ctx->saving);ctx->saving=1;ctx->saved=ctx->color;}
static void graphics_restore(void *graphics) {struct context *ctx=context(graphics);assert(ctx->saving);ctx->saving=0;ctx->color=ctx->saved;}
static void graphics_color(void *graphics,const uint32_t *color) {context(graphics)->color=*color;}
static void set_pixel(uintptr_t image,int x,int y,uint32_t argb) {
    int width=get_int(image,0x10),height=get_int(image,0x14);assert(x>=0&&y>=0&&x<width&&y<height);
    unsigned char *p=data(image)+(y*width+x)*4;for(unsigned c=0;c<4;c++)p[c]=(unsigned char)(argb>>(8*c));
}
static void graphics_fill(void *graphics,float x,float y,float width,float height) {
    struct context *ctx=context(graphics);for(int py=(int)y;py<(int)(y+height);py++)for(int px=(int)x;px<(int)(x+width);px++)set_pixel(ctx->image,px,py,ctx->color);
}
static void font_factory(void *font,float height) {
    assert(height==(family==0 ? 20.0f:family==1 ? 13.0f:18.0f));
    if(fail()){*(uintptr_t *)font=0;return;}*(uintptr_t *)font=1;live_fonts++;
}
static void font_family(void *font,const void *face,int style,float height) {
    const char *name=*(const char *const *)face;
    assert(style==0&&!strcmp(name,family==2 ? "PioneerDJ-FZLanTingHei-DB-BIG5":"PDJ-TsukuGoCustom Pro"));
    assert(height==(family==0 ? 18.625f:family==1 ? 13.125f:24.625f));
    if(fail()){*(uintptr_t *)font=0;return;}*(uintptr_t *)font=2;live_fonts++;
}
static void font_scale(void *font,float scale) {assert(*(uintptr_t *)font==2&&scale==(family==2 ? 0.95f:0.975f));}
static void font_delete(void *font) {if(*(uintptr_t *)font)live_fonts--;*(uintptr_t *)font=0;}
static void graphics_font(void *graphics,const void *font) {(void)graphics;assert(*(const uintptr_t *)font);}
static void graphics_text(void *graphics,const void *string,const int *rect,const int *justification,int lines,float scale) {
    struct context *ctx=context(graphics);const char *text=*(const char *const *)string;
    assert(*justification==36&&lines==1&&scale==1.0f);font_draws++;
    if(text[0]>='3'&&text[0]<='8'&&!text[1]) {
        digit_draws++;if(empty_ink)return;
        int width=family==0 ? 10:family==1 ? 7:11,height=family==0 ? 15:family==1 ? 10:16;
        if(oversized_ink) width=height=30;
        int x=rect[0]+rect[2]/2-width/2,y=rect[1]+rect[3]/2-height/2;
        for(int py=y;py<y+height;py++)for(int px=x;px<x+width;px++)set_pixel(ctx->image,px,py,0xffffffff);
    } else {
        assert(!strcmp(text,family==2 ? "H":"HUB"));
        for(int y=rect[1]+2;y<rect[1]+rect[3]-2;y++)set_pixel(ctx->image,rect[0]+3,y,ctx->color);
    }
}
static void blend_crop(void *graphics,const uintptr_t *image,int dx,int dy,int width,int height,int sx,int sy) {
    struct context *ctx=context(graphics);int src_width=get_int(*image,0x10),dst_width=get_int(ctx->image,0x10);
    for(int y=0;y<height;y++)for(int x=0;x<width;x++) {
        unsigned char *s=data(*image)+((sy+y)*src_width+sx+x)*4,*d=data(ctx->image)+((dy+y)*dst_width+dx+x)*4;
        for(unsigned c=0;c<4;c++)d[c]=(unsigned char)(s[c]+(d[c]*(255-s[3])+127)/255);
    }
}
static void graphics_draw(void *graphics,const uintptr_t *image,int x,int y) {blend_crop(graphics,image,x,y,get_int(*image,0x10),get_int(*image,0x14),0,0);}
static void draw_crop(void *graphics,const uintptr_t *image,int dx,int dy,int width,int height,int sx,int sy) {
    static const int anchors[]={63,62,82},baselines[]={68,53,28};
    assert(dx==anchors[family]&&dy+height==baselines[family]);crop_draws++;
    blend_crop(graphics,image,dx,dy,width,height,sx,sy);
    if(corrupt_outside)set_pixel(context(graphics)->image,40,35,0x12345678);
}
static void frame_decode(uintptr_t *out,const unsigned char *bytes,int size) {
    assert((uintptr_t)bytes==0x2e37d40&&size==0xcc);
    if(fail()){*out=0;return;}image_copy(out,&frame_handle);
}
static void reset_faults(void) {operation=0;fail_at=-1;alias_clone=corrupt_outside=empty_ink=bad_geometry=oversized_ink=0;}
static void assert_balanced(void) {assert(live_images==7&&!live_strings&&!live_fonts&&!live_graphics);}
int main(void) {
    static const int widths[]={90,95,170},heights[]={90,70,41};
    for(int f=0;f<3;f++)for(int n=0;n<2;n++) {
        uintptr_t image=stock_handles[f][n]=make_image(widths[f],heights[f]);
        for(int y=0;y<heights[f];y++)for(int x=0;x<widths[f];x++) {
            uint32_t pixel=0xff000000u|((unsigned)(x+y+n+1)*0x010101u&0xffffff);
            if(f==0 && y<45 && !(x>=32 && x<56 && y>=20)) pixel=0;
            set_pixel(image,x,y,pixel);
        }
    }
    frame_handle=make_image(22,22);for(int y=0;y<22;y++)for(int x=0;x<22;x++)set_pixel(frame_handle,x,y,0xffe1e1e1);
    unsigned char *copies[3][2];
    for(int f=0;f<3;f++)for(int n=0;n<2;n++){size_t bytes=(size_t)widths[f]*heights[f]*4;copies[f][n]=malloc(bytes);assert(copies[f][n]);memcpy(copies[f][n],data(stock_handles[f][n]),bytes);}
    for(int f=0;f<3;f++)for(int n=1;n<=8;n++) {
        reset_faults();uintptr_t output=0;int before=digit_draws,crops=crop_draws;
        assert(source_badge_native_compose(f,n,&output)&&output);
        assert(loaded_number==(n==1 ? 1:2)&&digit_draws==before+(n>=3)&&crop_draws==crops+(n>=3));
        struct hub_native_edit_rect edits[2]={{0,0,f==2?32:widths[f],f==2?heights[f]:f==0?22:16},
            f==0 ? (struct hub_native_edit_rect){62,49,18,23}:f==1 ? (struct hub_native_edit_rect){61,40,14,18}:(struct hub_native_edit_rect){81,7,20,27}};
        size_t checked,different;assert(source_badge_native_preserved(&stock_handles[f][loaded_number-1],&output,edits,n>=3?2:1,&checked,&different)&&checked&&!different);
        if(f==0) {
            /* Match the native LINK banner: its 22-row band overlaps two icon rows. */
            for(int y=20;y<22;y++)for(int x=32;x<56;x++) {
                unsigned off=(unsigned)(y*90+x)*4;
                assert(data(output)[off]==0xe1 && data(output)[off+1]==0xe1 &&
                    data(output)[off+2]==0xe1 && data(output)[off+3]==255);
            }
            for(int y=22;y<46;y++)for(int x=32;x<56;x++) {
                unsigned off=(unsigned)(y*90+x)*4;
                assert(!memcmp(data(output)+off,copies[0][loaded_number-1]+off,4));
            }
        }
        source_badge_native_image_release(&output);assert_balanced();
        for(int a=0;a<3;a++)for(int b=0;b<2;b++)assert(!memcmp(copies[a][b],data(stock_handles[a][b]),(size_t)widths[a]*heights[a]*4));
    }
    /* Every fallible allocation/constructor stage releases all partial handles. */
    for(int f=0;f<3;f++) {
        reset_faults();uintptr_t output=0;assert(source_badge_native_compose(f,8,&output));int steps=operation;source_badge_native_image_release(&output);
        for(int failure=1;failure<=steps;failure++) {reset_faults();fail_at=failure;assert(!source_badge_native_compose(f,8,&output)&&!output);assert_balanced();}
        reset_faults();alias_clone=1;assert(!source_badge_native_compose(f,3,&output));assert_balanced();
        reset_faults();empty_ink=1;assert(!source_badge_native_compose(f,3,&output));assert_balanced();
        reset_faults();oversized_ink=1;assert(!source_badge_native_compose(f,3,&output));assert_balanced();
        reset_faults();bad_geometry=1;assert(!source_badge_native_compose(f,3,&output));assert_balanced();
        reset_faults();corrupt_outside=1;assert(!source_badge_native_compose(f,3,&output));assert_balanced();
    }
    reset_faults();uintptr_t output=0;
    assert(!source_badge_native_compose(-1,1,&output)&&!source_badge_native_compose(3,1,&output));
    assert(!source_badge_native_compose(0,0,&output)&&!source_badge_native_compose(0,9,&output));
    assert(!source_badge_native_compose(0,1,NULL));output=1;assert(!source_badge_native_compose(0,1,&output)&&output==1);
    for(int f=0;f<3;f++)for(int n=0;n<2;n++){free(copies[f][n]);source_badge_native_image_release(&stock_handles[f][n]);}
    source_badge_native_image_release(&frame_handle);assert(!live_images&&!live_strings&&!live_fonts&&!live_graphics);
    puts("native compose: all24 paths, literal USB1/2, digit-only edits, stock preservation and failure cleanup pass");return 0;
}

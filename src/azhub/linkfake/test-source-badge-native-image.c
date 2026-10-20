/* SPDX-License-Identifier: GPL-3.0-only */
/* Synthetic adapter tests; these do not establish the native firmware ABI. */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void image_file(uintptr_t *,const void *);
static void string_new(void *,const char *);
static void string_delete(void *);
static void image_copy(uintptr_t *,const uintptr_t *);
static void image_detach(uintptr_t *);
static void image_delete(uintptr_t *);
static void graphics_new(void *,const uintptr_t *);
static void graphics_delete(void *);
static void graphics_draw(void *,const uintptr_t *,int,int);
static void graphics_save(void *);
static void graphics_color(void *,const uint32_t *);
static void graphics_fill(void *,float,float,float,float);
static void graphics_restore(void *);
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
#define HUB_NATIVE_GRAPHICS_COLOR graphics_color
#define HUB_NATIVE_GRAPHICS_FILL_FLOAT graphics_fill
#define HUB_NATIVE_GRAPHICS_RESTORE graphics_restore
#include "source-badge-native-image.h"

static int allocations,strings,new_strings,deleted_strings,file_calls;
static int detach_mode,graphics_fail,graphics_news,graphics_deletes,draw_calls,step;
static uintptr_t file_image;
static uint32_t ink;
static void put_int(uintptr_t p,size_t offset,int value) { memcpy((void *)(p+offset),&value,4); }
static int get_int(uintptr_t p,size_t offset) { int value;memcpy(&value,(void *)(p+offset),4);return value; }
static uintptr_t make_image(int format,int width,int height,int stride) {
    unsigned char *object=calloc(1,0x58),*pixels=malloc((size_t)stride*(size_t)height);
    assert(object && pixels);
    uintptr_t image=(uintptr_t)object;
    *(uintptr_t *)object=0x34f6228;
    put_int(image,8,1);put_int(image,0xc,format);put_int(image,0x10,width);put_int(image,0x14,height);
    put_int(image,0x50,format==1 ? 3:4);put_int(image,0x54,stride);
    memcpy(object+0x48,&pixels,sizeof pixels);
    memset(pixels,0x35,(size_t)stride*(size_t)height);
    allocations++;
    return image;
}
static unsigned char *mutable_pixels(uintptr_t image) {
    return *(unsigned char **)(image+0x48);
}
static void image_copy(uintptr_t *out,const uintptr_t *source) {
    *out=*source;
    if (*out) put_int(*out,8,get_int(*out,8)+1);
}
static void image_delete(uintptr_t *image) {
    if (!*image) return;
    int references=get_int(*image,8)-1;
    assert(references>=0);
    put_int(*image,8,references);
    if (!references) {
        free(mutable_pixels(*image));free((void *)*image);allocations--;
    }
    /* Match the native destructor: it does not zero the handle. */
}
static void image_detach(uintptr_t *image) {
    if (detach_mode==1) return; /* Deliberately broken clone still aliases. */
    uintptr_t original=*image;
    image_delete(image);
    if (detach_mode==2) { *image=0;return; }
    *image=make_image(get_int(original,0xc),get_int(original,0x10),
                     get_int(original,0x14),get_int(original,0x54));
    memcpy(mutable_pixels(*image),mutable_pixels(original),
           (size_t)get_int(original,0x14)*(size_t)get_int(original,0x54));
    if (detach_mode==3) *(uintptr_t *)*image=0; /* Invalid returned class. */
}
static void string_new(void *out,const char *text) {
    *(char **)out=strdup(text);assert(*(char **)out);
    strings++;new_strings++;
}
static void string_delete(void *string) {
    assert(*(char **)string);free(*(char **)string);
    strings--;deleted_strings++;
}
static void image_file(uintptr_t *out,const void *file) {
    const char *path=*(const char *const *)file;
    assert(strings==1 && path[0]=='/');
    assert(!strcmp(path,"/native/USB1.png"));
    file_calls++;image_copy(out,&file_image);
}
static void graphics_new(void *opaque,const uintptr_t *image) {
    struct hub_native_graphics *graphics=opaque;
    assert(!graphics->words[0] && !graphics->words[1] && !graphics->words[2]);
    graphics_news++;
    if (!graphics_fail) graphics->words[0]=graphics->words[1]=*image;
}
static void graphics_delete(void *opaque) {
    struct hub_native_graphics *graphics=opaque;
    assert(!graphics->words[0] || graphics->words[0]==graphics->words[1]);
    graphics_deletes++;
}
static void graphics_draw(void *opaque,const uintptr_t *image,int x,int y) {
    struct hub_native_graphics *graphics=opaque;
    assert(graphics->words[0] && *image && x==2 && y==3);draw_calls++;
}
static void graphics_save(void *graphics) { assert(graphics && step==0);step=1; }
static void graphics_color(void *graphics,const uint32_t *color) {
    assert(graphics && step==1);ink=*color;step=2;
}
static void graphics_fill(void *opaque,float x,float y,float w,float h) {
    struct hub_native_graphics *graphics=opaque;
    assert(step==2 && x==1 && y==1 && w==1 && h==1);step=3;
    uintptr_t image=graphics->words[0];
    memcpy(mutable_pixels(image)+get_int(image,0x54)+4,&ink,4);
}
static void graphics_restore(void *graphics) { assert(graphics && step==3);step=0; }

static void test_pixels(uintptr_t image) {
    struct hub_native_pixels view;
    int width=0,height=0;
    assert(source_badge_native_image_pixels(&image,&view));
    assert(view.format==2 && view.pixel_stride==4 && view.line_stride==20 && view.extent==56);
    assert(source_badge_native_image_geometry(&image,&width,&height) && width==4 && height==3);
    uint32_t value=0x80aabbcc,result=0;
    memcpy(mutable_pixels(image),&value,4);
    assert(source_badge_native_pixel_argb(&view,0,0,&result) && result==value);
    assert(source_badge_native_pixel_at(&view,3,2)==view.data+52);
    assert(!source_badge_native_pixel_at(&view,-1,0));
    assert(!source_badge_native_pixel_at(&view,4,0));
    assert(!source_badge_native_pixel_at(&view,0,3));
    assert(!source_badge_native_pixel_argb(&view,4,0,&result));
    assert(!source_badge_native_pixel_argb(&view,0,0,NULL));
    assert(!source_badge_native_pixel_at(NULL,0,0));
    assert(!source_badge_native_image_pixels(NULL,&view));
    assert(!source_badge_native_image_pixels(&image,NULL));
    assert(!source_badge_native_image_geometry(&image,NULL,&height));
    uintptr_t empty=0;
    assert(!source_badge_native_image_pixels(&empty,&view));
    uintptr_t old_vt=*(uintptr_t *)image;
    *(uintptr_t *)image=0x1234;
    assert(!source_badge_native_image_pixels(&image,&view));
    *(uintptr_t *)image=old_vt;
    const int invalid[][2]={{0xc,0},{0xc,3},{0x10,0},{0x10,4097},{0x14,0},
                            {0x14,4097},{0x50,3},{0x54,15},{0x54,1048577},{0x54,-1}};
    for (unsigned i=0;i<sizeof invalid/sizeof invalid[0];i++) {
        int old=get_int(image,(size_t)invalid[i][0]);
        put_int(image,(size_t)invalid[i][0],invalid[i][1]);
        assert(!source_badge_native_image_pixels(&image,&view));
        put_int(image,(size_t)invalid[i][0],old);
    }
    unsigned char *old_pixels=mutable_pixels(image),*bad=NULL;
    memcpy((void *)(image+0x48),&bad,sizeof bad);
    assert(!source_badge_native_image_pixels(&image,&view));
    bad=(unsigned char *)(UINTPTR_MAX-5);
    memcpy((void *)(image+0x48),&bad,sizeof bad);
    assert(!source_badge_native_image_pixels(&image,&view));
    memcpy((void *)(image+0x48),&old_pixels,sizeof old_pixels);
    uintptr_t rgb=make_image(1,2,2,8);
    assert(source_badge_native_image_pixels(&rgb,&view) && view.pixel_stride==3 && view.extent==14);
    assert(source_badge_native_pixel_at(&view,1,1)==view.data+11);
    assert(!source_badge_native_pixel_argb(&view,0,0,&result));
    source_badge_native_image_release(&rgb);assert(!rgb);
}
int main(void) {
    uintptr_t stock=make_image(2,4,3,20),out=0;
    test_pixels(stock);
    unsigned char original[60];memcpy(original,mutable_pixels(stock),sizeof original);
    file_image=stock;
    assert(!source_badge_native_image_load(&out,"relative.png") && !file_calls && !strings);
    assert(!source_badge_native_image_load(&out,"/") && !file_calls);
    assert(!source_badge_native_image_load(NULL,"/native/USB1.png"));
    assert(source_badge_native_image_load(&out,"/native/USB1.png") && out==stock);
    assert(file_calls==1 && new_strings==1 && deleted_strings==1 && !strings);
    assert(!source_badge_native_image_load(&out,"/native/USB1.png") && file_calls==1);
    source_badge_native_image_release(&out);assert(!out && get_int(stock,8)==1);
    file_image=0;
    assert(!source_badge_native_image_load(&out,"/native/USB1.png") && !strings && !out);
    assert(new_strings==2 && deleted_strings==2);
    for (detach_mode=1;detach_mode<=3;detach_mode++) {
        assert(!source_badge_native_image_clone(&out,&stock) && !out);
        assert(get_int(stock,8)==1 && allocations==1);
    }
    detach_mode=0;
    assert(!source_badge_native_image_clone(NULL,&stock));
    assert(!source_badge_native_image_clone(&out,NULL));
    assert(source_badge_native_image_clone(&out,&stock) && out && out!=stock);
    assert(mutable_pixels(out)!=mutable_pixels(stock) && get_int(stock,8)==1);
    assert(!source_badge_native_image_clone(&out,&stock));
    size_t checked=123,different=123;
    assert(source_badge_native_preserved(&stock,&out,NULL,0,&checked,&different));
    assert(checked==12 && different==0);
    mutable_pixels(out)[4]^=1;
    assert(source_badge_native_preserved(&stock,&out,NULL,0,&checked,&different));
    assert(checked==12 && different==1);
    struct hub_native_edit_rect edit={1,0,1,1};
    assert(source_badge_native_preserved(&stock,&out,&edit,1,&checked,&different));
    assert(checked==11 && different==0);
    /* An unexpected alpha change outside the permitted region is detected. */
    mutable_pixels(out)[23]^=1;
    assert(source_badge_native_preserved(&stock,&out,&edit,1,&checked,&different));
    assert(checked==11 && different==1);
    mutable_pixels(out)[4]^=1;mutable_pixels(out)[23]^=1;
    const struct hub_native_edit_rect bad_edits[]={{-1,0,1,1},{0,0,0,1},
        {4,0,1,1},{1,0,INT_MAX,1},{0,2,1,2},{0,0,4,3}};
    for (size_t i=0;i<sizeof bad_edits/sizeof bad_edits[0];i++) {
        assert(!source_badge_native_preserved(&stock,&out,bad_edits+i,1,&checked,&different));
        assert(!checked && !different);
    }
    assert(!source_badge_native_preserved(&stock,&out,NULL,1,&checked,&different));
    assert(!source_badge_native_preserved(&stock,&out,&edit,9,&checked,&different));
    uintptr_t ink=make_image(2,2,2,12);
    unsigned char *ip=mutable_pixels(ink);
    ip[0]=20;ip[1]=80;ip[2]=120;ip[3]=130;
    unsigned char ink_original[24];memcpy(ink_original,ip,sizeof ink_original);
    struct hub_native_edit_rect crop={0,0,1,1};
    assert(source_badge_native_copy_ink(&out,&stock,&ink,crop,1,0,1));
    assert(mutable_pixels(out)[4]==120 && mutable_pixels(out)[5]==120 &&
           mutable_pixels(out)[6]==120 && mutable_pixels(out)[7]==130);
    assert(!memcmp(ink_original,ip,sizeof ink_original));
    assert(!memcmp(original,mutable_pixels(stock),sizeof original));
    assert(source_badge_native_copy_ink(&out,&stock,&ink,crop,1,0,0));
    assert(!memcmp(mutable_pixels(out)+4,ip,4));
    assert(!source_badge_native_copy_ink(&stock,&stock,&ink,crop,0,0,1));
    assert(!source_badge_native_copy_ink(&out,&stock,&out,crop,0,0,1));
    assert(!source_badge_native_copy_ink(&out,&stock,&ink,crop,4,0,1));
    assert(!source_badge_native_copy_ink(&out,&stock,&ink,crop,0,3,1));
    assert(!source_badge_native_copy_ink(&out,&stock,&ink,crop,0,0,2));
    crop.width=INT_MAX;
    assert(!source_badge_native_copy_ink(&out,&stock,&ink,crop,0,0,1));
    source_badge_native_image_release(&ink);
    memcpy(mutable_pixels(out),original,sizeof original);
    struct hub_native_graphics graphics={{0}};
    assert(!source_badge_native_graphics_begin(NULL,&out));
    graphics_fail=1;
    assert(!source_badge_native_graphics_begin(&graphics,&out));
    assert(!graphics.words[0] && !graphics.words[1] && !graphics.words[2]);
    assert(graphics_news==1 && graphics_deletes==1);
    graphics_fail=0;
    assert(source_badge_native_graphics_begin(&graphics,&out));
    assert(source_badge_native_image_draw(&graphics,&stock,2,3) && draw_calls==1);
    assert(!source_badge_native_image_draw(NULL,&stock,2,3));
    assert(!source_badge_native_fill_rect(&graphics,1,1,0,1,0xff102030));
    assert(!source_badge_native_fill_rect(&graphics,1,1,1,-1,0xff102030));
    assert(!source_badge_native_fill_rect(&graphics,70000,1,1,1,0xff102030));
    assert(source_badge_native_fill_rect(&graphics,1,1,1,1,0xff102030) && step==0);
    assert(!memcmp(original,mutable_pixels(stock),sizeof original));
    assert(memcmp(original,mutable_pixels(out),sizeof original));
    source_badge_native_graphics_end(&graphics);
    assert(!graphics.words[0] && !graphics.words[1] && !graphics.words[2]);
    assert(graphics_deletes==2);
    source_badge_native_graphics_end(&graphics);source_badge_native_graphics_end(NULL);
    assert(graphics_deletes==2);
    assert(!source_badge_native_image_draw(&graphics,&stock,2,3));
    source_badge_native_image_release(&out);source_badge_native_image_release(&out);
    source_badge_native_image_release(&stock);source_badge_native_image_release(NULL);
    assert(!allocations && !strings && !stock && !out);
    puts("PASS native-image synthetic adapter: validation, ownership, cleanup and read-only resources");
    return 0;
}

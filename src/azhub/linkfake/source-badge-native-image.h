/* SPDX-License-Identifier: GPL-3.0-only */
/* Runtime-only native resources and private image composition for exact EP147. */
#ifndef HUB_SOURCE_BADGE_NATIVE_IMAGE_H
#define HUB_SOURCE_BADGE_NATIVE_IMAGE_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "source-badge-native-text.h"

#ifndef HUB_NATIVE_IMAGE_FILE
#if defined(__aarch64__)
__asm__(".text\n"
        ".align 2\n"
        ".hidden hub_native_image_file_sret\n"
        ".type hub_native_image_file_sret,%function\n"
        "hub_native_image_file_sret:\n"
        "mov x8,x0\n"
        "mov x0,x1\n"
        "movz x16,#0xb740\n"
        "movk x16,#0x255,lsl #16\n"
        "br x16\n"
        ".size hub_native_image_file_sret,.-hub_native_image_file_sret\n");
#endif
extern void hub_native_image_file_sret(uintptr_t *,const struct hub_native_string *);
#define HUB_NATIVE_IMAGE_FILE hub_native_image_file_sret
#endif
#ifndef HUB_NATIVE_IMAGE_COPY
#define HUB_NATIVE_IMAGE_COPY(dst,src) \
    ((void (*)(uintptr_t *,const uintptr_t *))0x251e8b0)((dst),(src))
#endif
#ifndef HUB_NATIVE_IMAGE_DETACH
#define HUB_NATIVE_IMAGE_DETACH(image) ((void (*)(uintptr_t *))0x251eae0)(image)
#endif
#ifndef HUB_NATIVE_IMAGE_DELETE
#define HUB_NATIVE_IMAGE_DELETE(image) ((void (*)(uintptr_t *))0x2511a90)(image)
#endif
#ifndef HUB_NATIVE_GRAPHICS_NEW_IMAGE
#define HUB_NATIVE_GRAPHICS_NEW_IMAGE(graphics,image) \
    ((void (*)(void *,const uintptr_t *))0x251c070)((graphics),(image))
#endif
#ifndef HUB_NATIVE_GRAPHICS_DELETE_IMAGE
#define HUB_NATIVE_GRAPHICS_DELETE_IMAGE(graphics) ((void (*)(void *))0x251c0c0)(graphics)
#endif
#ifndef HUB_NATIVE_GRAPHICS_IMAGE_AT
#define HUB_NATIVE_GRAPHICS_IMAGE_AT(graphics,image,x,y) \
    ((void (*)(void *,const uintptr_t *,int,int,int))0x251da80)((graphics),(image),(x),(y),0)
#endif
#ifndef HUB_NATIVE_GRAPHICS_FILL_FLOAT
#define HUB_NATIVE_GRAPHICS_FILL_FLOAT(graphics,x,y,w,h) \
    ((void (*)(void *,float,float,float,float))0x251cb60)((graphics),(x),(y),(w),(h))
#endif

struct hub_native_graphics { uintptr_t words[3]; };
_Static_assert(sizeof(struct hub_native_graphics)==24,"Exact EP147 Graphics size");
_Static_assert(_Alignof(struct hub_native_graphics)>=8,"Native Graphics alignment");
struct hub_native_pixels {
    const unsigned char *data;
    int width,height,format,pixel_stride,line_stride;
    size_t extent;
};

static void source_badge_native_image_release(uintptr_t *image) {
    if (!image) return;
    if (*image) HUB_NATIVE_IMAGE_DELETE(image);
    *image=0;
}
/* Empty output required. Absolute paths already have File's canonical layout:
 * one owned native String handle. No resource bytes leave the native process. */
static int source_badge_native_image_load(uintptr_t *out,const char *path) {
    if (!out || *out || !path || path[0]!='/' || !path[1]) return 0;
    struct hub_native_string file;
    HUB_NATIVE_STRING_NEW(&file,path);
    HUB_NATIVE_IMAGE_FILE(out,&file);
    HUB_NATIVE_STRING_DELETE(&file);
    return *out!=0;
}
/* Geometry is read only from the verified SoftwarePixelData layout. Its clone
 * routine 0x255cf10 and BitmapData mapper 0x255d8f0 establish these offsets. */
static int source_badge_native_image_pixels(const uintptr_t *image,struct hub_native_pixels *out) {
    if (!image || !*image || !out) return 0;
    uintptr_t data=*image;
    if (*(const uintptr_t *)data!=0x34f6228) return 0;
    int format=*(const int *)(data+0xc),width=*(const int *)(data+0x10);
    int height=*(const int *)(data+0x14),pixel_stride=*(const int *)(data+0x50);
    int line_stride=*(const int *)(data+0x54);
    const unsigned char *pixels=*(const unsigned char *const *)(data+0x48);
    if (!pixels || width<1 || width>4096 || height<1 || height>4096 ||
        (format!=1 && format!=2) || pixel_stride!=(format==1 ? 3:4) ||
        line_stride<width*pixel_stride || line_stride>1048576) return 0;
    size_t extent=(size_t)(height-1)*(size_t)line_stride+(size_t)width*(size_t)pixel_stride;
    if ((uintptr_t)pixels>UINTPTR_MAX-extent) return 0;
    *out=(struct hub_native_pixels){pixels,width,height,format,pixel_stride,line_stride,extent};
    return 1;
}
static int source_badge_native_image_geometry(const uintptr_t *image,int *width,int *height) {
    struct hub_native_pixels pixels;
    if (!width || !height || !source_badge_native_image_pixels(image,&pixels)) return 0;
    *width=pixels.width;*height=pixels.height;
    return 1;
}
/* Raw native premultiplied bytes, const for both RGB and ARGB. No write API is
 * provided: original resource handles must remain read-only. */
static const unsigned char *source_badge_native_pixel_at(const struct hub_native_pixels *pixels,int x,int y) {
    if (!pixels || !pixels->data || x<0 || y<0 || x>=pixels->width || y>=pixels->height) return NULL;
    size_t offset=(size_t)y*(size_t)pixels->line_stride+(size_t)x*(size_t)pixels->pixel_stride;
    if (offset>pixels->extent || (size_t)pixels->pixel_stride>pixels->extent-offset) return NULL;
    return pixels->data+offset;
}
/* ARGB software pixels are little-endian BGRA bytes on the supported AArch64
 * firmware. Return the unchanged premultiplied 0xAARRGGBB word for comparison. */
static int source_badge_native_pixel_argb(const struct hub_native_pixels *pixels,int x,int y,uint32_t *argb) {
    if (!pixels || pixels->format!=2 || !argb) return 0;
    const unsigned char *pixel=source_badge_native_pixel_at(pixels,x,y);
    if (!pixel) return 0;
    *argb=(uint32_t)pixel[0]|((uint32_t)pixel[1]<<8)|((uint32_t)pixel[2]<<16)|((uint32_t)pixel[3]<<24);
    return 1;
}
struct hub_native_edit_rect { int x,y,width,height; };
/* Transfer literal native ink into a detached image. Native orange numerals
 * share the white caption's red/alpha bytes; selecting red preserves their
 * exact premultiplied coverage. Never infer alpha from opaque player images.
 * The original body is mandatory so aliasing cannot silently alter its cache. */
static int source_badge_native_copy_ink(uintptr_t *destination,const uintptr_t *original,
                                       const uintptr_t *ink,struct hub_native_edit_rect crop,
                                       int dx,int dy,int gray_from_red) {
    struct hub_native_pixels dst,stock,src;
    if (!destination || !original || !ink || !*destination || *destination==*original ||
        *destination==*ink || !source_badge_native_image_pixels(destination,&dst) ||
        !source_badge_native_image_pixels(original,&stock) ||
        !source_badge_native_image_pixels(ink,&src) || dst.format!=2 || src.format!=2 ||
        dst.data==stock.data || dst.data==src.data || dst.width!=stock.width ||
        dst.height!=stock.height || crop.x<0 || crop.y<0 || crop.x>=src.width ||
        crop.y>=src.height || crop.width<1 || crop.height<1 ||
        crop.width>src.width-crop.x || crop.height>src.height-crop.y ||
        dx<0 || dy<0 || dx>=dst.width || dy>=dst.height ||
        crop.width>dst.width-dx || crop.height>dst.height-dy ||
        (gray_from_red!=0 && gray_from_red!=1)) return 0;
    for (int y=0;y<crop.height;y++) for (int x=0;x<crop.width;x++) {
        const unsigned char *p=source_badge_native_pixel_at(&src,crop.x+x,crop.y+y);
        unsigned char *q=(unsigned char *)source_badge_native_pixel_at(&dst,dx+x,dy+y);
        if (!p || !q) return 0;
        q[0]=gray_from_red ? p[2]:p[0];
        q[1]=gray_from_red ? p[2]:p[1];q[2]=p[2];q[3]=p[3];
    }
    return 1;
}
/* Verify the untouched icon/caption bytes, including alpha. Rectangles describe
 * only intentional edits; malformed exclusions fail rather than hiding changes.
 * This reads both resources and never exports their pixel data. */
static int source_badge_native_preserved(const uintptr_t *stock,const uintptr_t *edited,
                                         const struct hub_native_edit_rect *rects,size_t count,
                                         size_t *checked,size_t *different) {
    struct hub_native_pixels a,b;
    if (checked) *checked=0;
    if (different) *different=0;
    if (!checked || !different || count>8 || (count && !rects) ||
        !source_badge_native_image_pixels(stock,&a) ||
        !source_badge_native_image_pixels(edited,&b) ||
        a.width!=b.width || a.height!=b.height || a.format!=b.format) return 0;
    for (size_t i=0;i<count;i++) {
        const struct hub_native_edit_rect *r=&rects[i];
        if (r->x<0 || r->y<0 || r->x>=a.width || r->y>=a.height ||
            r->width<1 || r->height<1 || r->width>a.width-r->x ||
            r->height>a.height-r->y) return 0;
    }
    for (int y=0;y<a.height;y++) for (int x=0;x<a.width;x++) {
        int excluded=0;
        for (size_t i=0;i<count;i++) {
            const struct hub_native_edit_rect *r=&rects[i];
            if (x>=r->x && x-r->x<r->width && y>=r->y && y-r->y<r->height) {
                excluded=1;break;
            }
        }
        if (excluded) continue;
        const unsigned char *pa=source_badge_native_pixel_at(&a,x,y);
        const unsigned char *pb=source_badge_native_pixel_at(&b,x,y);
        if (!pa || !pb) return 0;
        (*checked)++;
        if (memcmp(pa,pb,(size_t)a.pixel_stride)) (*different)++;
    }
    return *checked>0;
}
/* Copy first to force sharing, then let native duplicateIfShared clone pixels.
 * Reject failure or aliasing before the caller can construct a drawing context. */
static int source_badge_native_image_clone(uintptr_t *out,const uintptr_t *stock) {
    struct hub_native_pixels pixels;
    if (!out || *out || !source_badge_native_image_pixels(stock,&pixels)) return 0;
    HUB_NATIVE_IMAGE_COPY(out,stock);
    HUB_NATIVE_IMAGE_DETACH(out);
    if (!*out || *out==*stock || !source_badge_native_image_pixels(out,&pixels)) {
        source_badge_native_image_release(out);
        return 0;
    }
    return 1;
}
/* Only pass a successfully detached private image here. The native constructor
 * creates a pixel-data graphics context; this helper cannot infer ownership. */
static int source_badge_native_graphics_begin(struct hub_native_graphics *graphics,const uintptr_t *private_image) {
    struct hub_native_pixels pixels;
    if (!graphics || !source_badge_native_image_pixels(private_image,&pixels)) return 0;
    memset(graphics,0,sizeof *graphics);
    HUB_NATIVE_GRAPHICS_NEW_IMAGE(graphics,private_image);
    if (!graphics->words[0]) {
        HUB_NATIVE_GRAPHICS_DELETE_IMAGE(graphics);
        memset(graphics,0,sizeof *graphics);
        return 0;
    }
    return 1;
}
static void source_badge_native_graphics_end(struct hub_native_graphics *graphics) {
    if (!graphics) return;
    if (graphics->words[1]) HUB_NATIVE_GRAPHICS_DELETE_IMAGE(graphics);
    memset(graphics,0,sizeof *graphics);
}
static int source_badge_native_image_draw(struct hub_native_graphics *graphics,const uintptr_t *image,int x,int y) {
    struct hub_native_pixels pixels;
    if (!graphics || !graphics->words[0] || !source_badge_native_image_pixels(image,&pixels)) return 0;
    HUB_NATIVE_GRAPHICS_IMAGE_AT(graphics,image,x,y);
    return 1;
}
static int source_badge_native_fill_rect(struct hub_native_graphics *graphics,
                                        float x,float y,float width,float height,uint32_t argb) {
    if (!graphics || !graphics->words[0] || !(x>=-65536.0f && x<=65536.0f) ||
        !(y>=-65536.0f && y<=65536.0f) || !(width>0.0f && width<=65536.0f) ||
        !(height>0.0f && height<=65536.0f)) return 0;
    HUB_NATIVE_GRAPHICS_SAVE(graphics);
    HUB_NATIVE_GRAPHICS_COLOR(graphics,&argb); /* Flush the deferred native save. */
    HUB_NATIVE_GRAPHICS_FILL_FLOAT(graphics,x,y,width,height);
    HUB_NATIVE_GRAPHICS_RESTORE(graphics);
    return 1;
}
#endif

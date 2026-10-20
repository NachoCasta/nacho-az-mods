/* SPDX-License-Identifier: GPL-3.0-only */
/* Exact EP147 native text ABI; no bundled font or reconstructed artwork. */
#ifndef HUB_SOURCE_BADGE_NATIVE_TEXT_H
#define HUB_SOURCE_BADGE_NATIVE_TEXT_H
#include <stdint.h>

/* Both are nontrivial eight-byte native objects. A zero Font can be destroyed;
 * a String must first be constructed, even when its requested text is empty. */
struct hub_native_font { uintptr_t handle; };
struct hub_native_string { uintptr_t handle; };
_Static_assert(sizeof(struct hub_native_font)==8,"Exact EP147 Font size");
_Static_assert(sizeof(struct hub_native_string)==8,"Exact EP147 String size");

#ifndef HUB_NATIVE_FONT_FACTORY
#if defined(__aarch64__)
__asm__(".text\n"
        ".align 2\n"
        ".hidden hub_native_font_factory_sret\n"
        ".type hub_native_font_factory_sret,%function\n"
        "hub_native_font_factory_sret:\n"
        "mov x8,x0\n"
        "movz x16,#0x6928\n"
        "movk x16,#0x20b,lsl #16\n"
        "br x16\n"
        ".size hub_native_font_factory_sret,.-hub_native_font_factory_sret\n");
#endif
/* C output pointer x0 becomes native hidden return x8; float height stays s0.
 * A host can preprocess this header, but execution needs explicit test stubs. */
extern void hub_native_font_factory_sret(struct hub_native_font *,float);
#define HUB_NATIVE_FONT_FACTORY hub_native_font_factory_sret
#endif
#ifndef HUB_NATIVE_STRING_NEW
#define HUB_NATIVE_STRING_NEW(dst,text) \
    ((void (*)(struct hub_native_string *,const char *))0x24a1180)((dst),(text))
#endif
#ifndef HUB_NATIVE_STRING_DELETE
#define HUB_NATIVE_STRING_DELETE(text) \
    ((void (*)(struct hub_native_string *))0x2469aa0)(text)
#endif
#ifndef HUB_NATIVE_FONT_NEW_FAMILY
#define HUB_NATIVE_FONT_NEW_FAMILY(dst,family,style,height) \
    ((void (*)(struct hub_native_font *,const struct hub_native_string *,int,float))0x2550d50) \
        ((dst),(family),(style),(height))
#endif
#ifndef HUB_NATIVE_FONT_HORIZONTAL_SCALE
#define HUB_NATIVE_FONT_HORIZONTAL_SCALE(font,scale) \
    ((void (*)(struct hub_native_font *,float))0x2547b30)((font),(scale))
#endif
#ifndef HUB_NATIVE_FONT_DELETE
#define HUB_NATIVE_FONT_DELETE(font) \
    ((void (*)(struct hub_native_font *))0x2546a90)(font)
#endif
#ifndef HUB_NATIVE_GRAPHICS_SAVE
#define HUB_NATIVE_GRAPHICS_SAVE(graphics) ((void (*)(void *))0x251c390)(graphics)
#endif
#ifndef HUB_NATIVE_GRAPHICS_RESTORE
#define HUB_NATIVE_GRAPHICS_RESTORE(graphics) ((void (*)(void *))0x251c3e0)(graphics)
#endif
#ifndef HUB_NATIVE_GRAPHICS_FONT
#define HUB_NATIVE_GRAPHICS_FONT(graphics,font) \
    ((void (*)(void *,const struct hub_native_font *))0x251ca30)((graphics),(font))
#endif
#ifndef HUB_NATIVE_GRAPHICS_COLOR
#define HUB_NATIVE_GRAPHICS_COLOR(graphics,argb) \
    ((void (*)(void *,const uint32_t *))0x251c5e0)((graphics),(argb))
#endif
#ifndef HUB_NATIVE_GRAPHICS_TEXT
#define HUB_NATIVE_GRAPHICS_TEXT(graphics,text,rect,justification,lines,scale) \
    ((void (*)(void *,const struct hub_native_string *,const int *,const int *,int,float))0x25586a0) \
        ((graphics),(text),(rect),(justification),(lines),(scale))
#endif

/* family==NULL selects the native Tsuku UI factory including its own tracking.
 * A nonempty family selects the native Font constructor with style flags 0..3, without
 * added tracking. These choices expose native rendering; matching a bitmap's
 * original design face/size still requires a visual or pixel comparison.
 * Caller must already be in a native paint callback with a valid Graphics.
 * Coordinates are local to that Graphics, not framebuffer/screen coordinates.
 * Return 1 after dispatch, 0 for invalid input or an invalid native Font. */
static int source_badge_native_text_options(void *graphics,int x,int y,int width,int height,
                                    const char *text,float font_height,uint32_t argb,
                                    int centered,const char *family,int style,float horizontal_scale) {
    if (!graphics || !text || !*text || width<=0 || height<=0 ||
        !(font_height>0.0f && font_height<=512.0f) || (family && !*family) ||
        style<0 || style>3 || (!family && style!=0) ||
        !(horizontal_scale>=0.5f && horizontal_scale<=2.0f)) return 0;
    struct hub_native_font font={0};
    if (family) {
        struct hub_native_string face;
        HUB_NATIVE_STRING_NEW(&face,family);
        HUB_NATIVE_FONT_NEW_FAMILY(&font,&face,style,font_height);
        HUB_NATIVE_STRING_DELETE(&face);
    } else {
        HUB_NATIVE_FONT_FACTORY(&font,font_height);
    }
    if (!font.handle) {
        HUB_NATIVE_FONT_DELETE(&font);
        return 0;
    }
    if (horizontal_scale!=1.0f) HUB_NATIVE_FONT_HORIZONTAL_SCALE(&font,horizontal_scale);
    struct hub_native_string string;
    HUB_NATIVE_STRING_NEW(&string,text);
    const int rect[4]={x,y,width,height};
    const int justification=centered ? 36:33; /* Horizontal + vertical center. */
    HUB_NATIVE_GRAPHICS_SAVE(graphics);
    HUB_NATIVE_GRAPHICS_FONT(graphics,&font);
    HUB_NATIVE_GRAPHICS_COLOR(graphics,&argb);
    HUB_NATIVE_GRAPHICS_TEXT(graphics,&string,rect,&justification,1,1.0f);
    HUB_NATIVE_GRAPHICS_RESTORE(graphics);
    HUB_NATIVE_STRING_DELETE(&string);
    HUB_NATIVE_FONT_DELETE(&font);
    return 1;
}
/* Preserve the original factory/plain behavior without an extra scale setter.
 * Nonzero styles need an explicit family because the Tsuku factory has no
 * style argument; unsupported combinations are refused before construction. */
static int source_badge_native_text(void *graphics,int x,int y,int width,int height,
                                    const char *text,float font_height,uint32_t argb,
                                    int centered,const char *family) {
    return source_badge_native_text_options(graphics,x,y,width,height,text,font_height,argb,
        centered,family,0,1.0f);
}
#endif

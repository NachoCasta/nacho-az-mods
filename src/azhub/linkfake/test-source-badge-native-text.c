/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise the production native-text wrapper without calling firmware addresses. */
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
struct hub_native_font;
struct hub_native_string;
static void font_factory(struct hub_native_font *,float);
static void string_new(struct hub_native_string *,const char *);
static void string_delete(struct hub_native_string *);
static void font_family(struct hub_native_font *,const struct hub_native_string *,int,float);
static void font_delete(struct hub_native_font *);
static void font_horizontal_scale(struct hub_native_font *,float);
static void graphics_save(void *);
static void graphics_restore(void *);
static void graphics_font(void *,const struct hub_native_font *);
static void graphics_color(void *,const uint32_t *);
static void graphics_text(void *,const struct hub_native_string *,const int *,const int *,int,float);
#define HUB_NATIVE_FONT_FACTORY font_factory
#define HUB_NATIVE_STRING_NEW string_new
#define HUB_NATIVE_STRING_DELETE string_delete
#define HUB_NATIVE_FONT_NEW_FAMILY font_family
#define HUB_NATIVE_FONT_DELETE font_delete
#define HUB_NATIVE_FONT_HORIZONTAL_SCALE font_horizontal_scale
#define HUB_NATIVE_GRAPHICS_SAVE graphics_save
#define HUB_NATIVE_GRAPHICS_RESTORE graphics_restore
#define HUB_NATIVE_GRAPHICS_FONT graphics_font
#define HUB_NATIVE_GRAPHICS_COLOR graphics_color
#define HUB_NATIVE_GRAPHICS_TEXT graphics_text
#include "source-badge-native-text.h"
struct graphics_state { uintptr_t font,saved_font; uint32_t color,saved_color; int saved; };
static struct graphics_state graphics;
static int fail_font,live_strings,live_fonts;
static unsigned next_string;
static struct { struct hub_native_string *object; const char *text; } strings[2];
static char events[64];static unsigned event_count;
static const char *expected_text,*expected_family;
static float expected_height,expected_scale;
static int expected_style;
static uint32_t expected_color;
static int expected_rect[4],expected_justification;
static void event(char code) { assert(event_count+1<sizeof events);events[event_count++]=code;events[event_count]=0; }
static const char *string_value(const struct hub_native_string *object) {
    for (unsigned n=0;n<2;n++) if (strings[n].object==object) return strings[n].text;
    assert(!"Unconstructed native String used or destroyed");return NULL;
}
static void string_new(struct hub_native_string *object,const char *text) {
    assert(text);
    for (unsigned n=0;n<2;n++) if (!strings[n].object) {
        strings[n].object=object;strings[n].text=text;object->handle=++next_string;
        live_strings++;event(expected_family && !strcmp(text,expected_family) ? 'A':'S');return;
    }
    assert(!"Native String leak");
}
static void string_delete(struct hub_native_string *object) {
    const char *text=string_value(object);
    event(expected_family && !strcmp(text,expected_family) ? 'a':'s');
    for (unsigned n=0;n<2;n++) if (strings[n].object==object) {
        strings[n].object=NULL;strings[n].text=NULL;live_strings--;object->handle=0;return;
    }
    assert(0);
}
static void font_factory(struct hub_native_font *font,float height) {
    assert(!expected_family && height==expected_height && !font->handle);
    event('F');font->handle=fail_font ? 0:123;live_fonts+=!fail_font;
}
static void font_family(struct hub_native_font *font,const struct hub_native_string *family,int style,float height) {
    assert(expected_family && !strcmp(string_value(family),expected_family));
    assert(style==expected_style && height==expected_height && !font->handle);
    event('N');font->handle=fail_font ? 0:123;live_fonts+=!fail_font;
}
static void font_horizontal_scale(struct hub_native_font *font,float scale) {
    assert(font->handle==123 && live_fonts==1 && !graphics.saved);
    assert(scale==expected_scale && scale!=1.0f);event('W');
}
static void font_delete(struct hub_native_font *font) {
    assert(!graphics.saved && graphics.font==77);
    assert(font->handle==(uintptr_t)(fail_font ? 0:123));
    if (font->handle) live_fonts--;
    font->handle=0;event('f');
}
static void graphics_save(void *context) {
    assert(context==&graphics && !graphics.saved);event('V');graphics.saved=1;
    graphics.saved_font=graphics.font;graphics.saved_color=graphics.color;
}
static void graphics_restore(void *context) {
    assert(context==&graphics && graphics.saved);event('R');graphics.saved=0;
    graphics.font=graphics.saved_font;graphics.color=graphics.saved_color;
}
static void graphics_font(void *context,const struct hub_native_font *font) {
    assert(context==&graphics && graphics.saved && font->handle==123 && live_fonts==1);
    graphics.font=font->handle;event('T');
}
static void graphics_color(void *context,const uint32_t *argb) {
    assert(context==&graphics && graphics.saved && *argb==expected_color);
    graphics.color=*argb;event('C');
}
static void graphics_text(void *context,const struct hub_native_string *text,const int *rect,const int *justification,int lines,float scale) {
    assert(context==&graphics && graphics.saved && graphics.font==123 && graphics.color==expected_color);
    assert(!strcmp(string_value(text),expected_text) && !memcmp(rect,expected_rect,sizeof expected_rect));
    assert(*justification==expected_justification && lines==1 && scale==1.0f);event('D');
}
static void reset(const char *family,int failure) {
    assert(!live_strings && !live_fonts && !graphics.saved);
    memset(strings,0,sizeof strings);event_count=0;events[0]=0;
    graphics=(struct graphics_state){.font=77,.color=0xaabbccddu};
    expected_family=family;fail_font=failure;expected_scale=1.0f;expected_style=0;
    expected_text="USB 8";expected_height=21.75f;expected_color=0x80fefdfcu;
    expected_rect[0]=-7;expected_rect[1]=13;expected_rect[2]=170;expected_rect[3]=41;
}
static void assert_restored(void) {
    assert(!live_strings && !live_fonts && !graphics.saved);
    assert(graphics.font==77 && graphics.color==0xaabbccddu);
}
int main(void) {
    /* Factory and explicit-family paths preserve caller Graphics state and ABI arguments. */
    for (int family=0;family<2;family++) for (int centered=0;centered<2;centered++) {
        reset(family ? "Native Device Face":NULL,0);expected_justification=centered ? 36:33;
        assert(source_badge_native_text(&graphics,-7,13,170,41,expected_text,expected_height,expected_color,centered,expected_family)==1);
        assert(!strcmp(events,family ? "ANaSVTCDRsf":"FSVTCDRsf"));assert_restored();
    }
    /* Explicit native style flags and fractional width are forwarded once. */
    const float scales[]={0.5f,0.875f,1.0f,1.25f,2.0f};
    for (int style=0;style<=3;style++) for (unsigned n=0;n<sizeof scales/sizeof scales[0];n++) {
        reset("Native Device Face",0);expected_style=style;expected_scale=scales[n];expected_justification=36;
        assert(source_badge_native_text_options(&graphics,-7,13,170,41,expected_text,expected_height,expected_color,
            1,expected_family,style,scales[n]));
        assert(!strcmp(events,scales[n]==1.0f ? "ANaSVTCDRsf":"ANaWSVTCDRsf"));assert_restored();
    }
    reset(NULL,0);expected_scale=0.75f;expected_justification=33;
    assert(source_badge_native_text_options(&graphics,-7,13,170,41,expected_text,expected_height,expected_color,0,NULL,0,0.75f));
    assert(!strcmp(events,"FWSVTCDRsf"));assert_restored();
    reset("Native Device Face",1);expected_style=3;expected_scale=1.5f;
    assert(!source_badge_native_text_options(&graphics,0,0,90,90,"USB 1",expected_height,0,0,expected_family,3,1.5f));
    assert(!strcmp(events,"ANaf"));assert_restored(); /* No setter on an invalid Font. */
    reset("Native Device Face",0);
    const float invalid_scales[]={0,0.499f,2.001f,NAN,INFINITY,-INFINITY};
    for (unsigned n=0;n<sizeof invalid_scales/sizeof invalid_scales[0];n++)
        assert(!source_badge_native_text_options(&graphics,0,0,90,90,"USB 1",12,0,0,expected_family,0,invalid_scales[n]));
    assert(!source_badge_native_text_options(&graphics,0,0,90,90,"USB 1",12,0,0,expected_family,-1,1));
    assert(!source_badge_native_text_options(&graphics,0,0,90,90,"USB 1",12,0,0,expected_family,4,1));
    assert(!source_badge_native_text_options(&graphics,0,0,90,90,"USB 1",12,0,0,NULL,1,1));
    assert(!events[0]);assert_restored();
    /* Invalid native Font still gets its safe destructor; no text String exists yet. */
    for (int family=0;family<2;family++) {
        reset(family ? "Native Device Face":NULL,1);
        assert(!source_badge_native_text(&graphics,0,0,90,90,"USB 1",expected_height,expected_color,0,expected_family));
        assert(!strcmp(events,family ? "ANaf":"Ff"));assert_restored();
    }
    reset(NULL,0);
    const float invalid_heights[]={0,-1,NAN,INFINITY,-INFINITY,512.01f};
    for (unsigned n=0;n<sizeof invalid_heights/sizeof invalid_heights[0];n++)
        assert(!source_badge_native_text(&graphics,0,0,90,90,"USB 1",invalid_heights[n],0,0,NULL));
    assert(!source_badge_native_text(NULL,0,0,90,90,"USB 1",12,0,0,NULL));
    assert(!source_badge_native_text(&graphics,0,0,90,90,NULL,12,0,0,NULL));
    assert(!source_badge_native_text(&graphics,0,0,90,90,"",12,0,0,NULL));
    assert(!source_badge_native_text(&graphics,0,0,0,90,"USB 1",12,0,0,NULL));
    assert(!source_badge_native_text(&graphics,0,0,90,-1,"USB 1",12,0,0,NULL));
    assert(!source_badge_native_text(&graphics,0,0,90,90,"USB 1",12,0,0,""));
    assert(!events[0]);assert_restored();
    reset(NULL,0);expected_height=512.0f;expected_justification=33;
    assert(source_badge_native_text(&graphics,-7,13,170,41,expected_text,expected_height,expected_color,0,NULL));
    assert_restored();
    puts("native text wrapper: construction, cleanup, Graphics state, float/rectangle ABI and invalid inputs pass");
    return 0;
}

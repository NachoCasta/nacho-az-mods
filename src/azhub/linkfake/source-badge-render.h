/* SPDX-License-Identifier: GPL-3.0-only */
/* Native presentation adapter for the exact supported EP147 firmware. */
#ifndef HUB_SOURCE_BADGE_RENDER_H
#define HUB_SOURCE_BADGE_RENDER_H
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include "source-display.h"
enum hub_source_badge_family { HUB_BADGE_SOURCE, HUB_BADGE_BROWSE, HUB_BADGE_WAVEFORM };

#ifndef HUB_BADGE_ASSIGN
#define HUB_BADGE_ASSIGN(dst,src) ((void (*)(uintptr_t *,const uintptr_t *))0x251e8e0)((dst),(src))
#endif
#ifndef HUB_BADGE_DESTROY
#define HUB_BADGE_DESTROY(image) ((void (*)(uintptr_t *))0x2511a90)(image)
#endif

/* Private composites remain alive for the mod's process lifetime. Native
 * resources stay on the device; copy assignment retains a widget reference. */
static uintptr_t source_badge_images[3][8];
static int source_badge_prepared;
static fn8 source_badge_callback_original,source_badge_base_original,source_badge_root_original;
struct source_badge_scope { uintptr_t controller; int number; };
static __thread struct source_badge_scope source_badge_current;

/* Only use before hook installation, or on preparation failure. The native
 * destructor does not clear its handle, so explicitly clear each released word. */
static void source_badge_discard_images(void) {
    for (int family=0;family<3;family++) for (int number=0;number<8;number++) {
        HUB_BADGE_DESTROY(&source_badge_images[family][number]);
        source_badge_images[family][number]=0;
    }
    source_badge_prepared=0;
}
#ifndef HUB_BADGE_COMPOSE
#define HUB_NATIVE_COMPOSE_LOG(...) lf_core->log(EJECT_ID,__VA_ARGS__)
#include "source-badge-native-compose.h"
#define HUB_BADGE_COMPOSE source_badge_native_compose
#endif
static int source_badge_prepare(void) {
    if (source_badge_prepared) return 0;
    for (int family=0;family<3;family++) for (int number=1;number<=8;number++) {
        if (!HUB_BADGE_COMPOSE(family,number,&source_badge_images[family][number-1]) ||
            !source_badge_images[family][number-1]) goto failed;
    }
    source_badge_prepared=1;
    return 0;
failed:
    source_badge_discard_images();
    return -90;
}
static int source_badge_widget_family(uintptr_t widget) {
    if (!widget) return -1;
    switch (*(uintptr_t *)widget) {
    case 0x2e34e68: return HUB_BADGE_SOURCE;
    case 0x2e32658: return HUB_BADGE_BROWSE;
    case 0x2e36388: return HUB_BADGE_WAVEFORM;
    default: return -1;
    }
}
static int source_badge_owned_widget(uintptr_t controller,uintptr_t *widget) {
    if (!lf_runtime_ready() || !source_badge_prepared || !controller ||
        source_badge_current.controller!=controller || source_badge_current.number<1 ||
        source_badge_current.number>8) return -1;
    *widget=*(uintptr_t *)(controller+0x28);
    return source_badge_widget_family(*widget);
}
static uint64_t source_badge_callback(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
                                    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    struct source_badge_scope previous=source_badge_current;
    source_badge_current=(struct source_badge_scope){0};
    if (lf_runtime_ready() && source_badge_prepared && a) {
        /* Function storage -> heap closure {controller, incoming MediaID}.
         * The controller's cached MediaID is still old during updateImage. */
        uintptr_t closure=*(uintptr_t *)(uintptr_t)a;
        if (closure) {
            int source=safe_source_identity(*(uint64_t *)(closure+8));
            if (source>=0) {
                int number=hub_source_display_number(fakes[source].player,fakes[source].slot);
                if (number) source_badge_current=(struct source_badge_scope){
                    *(uintptr_t *)closure,number};
            }
        }
    }
    uint64_t result=source_badge_callback_original(a,b,c,d,e,f,g,h);
    source_badge_current=previous;
    return result;
}
static uint64_t source_badge_base(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
                                uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    uintptr_t widget=0;
    int family=source_badge_owned_widget((uintptr_t)a,&widget);
    if (family<0 || !b) return source_badge_base_original(a,b,c,d,e,f,g,h);
    unsigned char description[16];
    memcpy(description,(const void *)(uintptr_t)b,sizeof description);
    description[5]=0; /* Hide the native LINK and numeric-player children. */
    memset(description+8,0,8);
    uint64_t result=source_badge_base_original(a,(uintptr_t)description,c,d,e,f,g,h);
    HUB_BADGE_ASSIGN((uintptr_t *)(widget+0xd0),
                     &source_badge_images[family][source_badge_current.number-1]);
    return result;
}
static uint64_t source_badge_root(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
                                uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    uintptr_t widget=0;
    if (source_badge_owned_widget((uintptr_t)a,&widget)<0 || !b)
        return source_badge_root_original(a,b,c,d,e,f,g,h);
    unsigned char description[16];
    memcpy(description,(const void *)(uintptr_t)b,sizeof description);
    description[5]=0;
    memset(description+8,0,8); /* Hide the native USB/SD root-marker child. */
    return source_badge_root_original(a,(uintptr_t)description,c,d,e,f,g,h);
}
static int source_badge_install(void) {
    if (!source_badge_prepared) return -90;
    source_badge_base_original=make_hook(0x1c88820,source_badge_base);
    source_badge_root_original=make_hook(0x1c88900,source_badge_root);
    source_badge_callback_original=make_hook(0x1c88618,source_badge_callback);
    return source_badge_base_original && source_badge_root_original &&
           source_badge_callback_original ? 0:-91;
}
#endif

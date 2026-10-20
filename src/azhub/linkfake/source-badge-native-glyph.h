/* SPDX-License-Identifier: GPL-3.0-only */
/* Read-only native digit-mask comparison. Initialization only; no pixels exported. */
#ifndef HUB_SOURCE_BADGE_NATIVE_GLYPH_H
#define HUB_SOURCE_BADGE_NATIVE_GLYPH_H
#include <limits.h>
#include "source-badge-native-image.h"
/* polarity 0 measures light ink; 1 measures dark ink against alpha coverage.
 * A crop borrows the original view; its image must outlive every comparison. */
struct hub_native_glyph_crop {
    struct hub_native_pixels pixels;
    int x,y,width,height,polarity;
    uint32_t background_argb; /* Representative pixel at the modal light level. */
    unsigned background_luminance;
    int matted_background; /* Raw coverage comparison intentionally stays unnormalized. */
};
struct hub_native_glyph_score {
    int same_dimensions,exact;
    unsigned compared_pixels,different_pixels,max_error;
    uint64_t absolute_error;
    unsigned mean_error_ppm; /* Absolute coverage error / (255 * pixel count). */
};
static int hub_native_glyph_view_valid(const struct hub_native_pixels *pixels) {
    if (!pixels || !pixels->data || pixels->width<1 || pixels->height<1 ||
        pixels->width>256 || pixels->height>256 || pixels->format!=2 ||
        pixels->pixel_stride!=4 || pixels->line_stride<pixels->width*4 ||
        pixels->line_stride>1048576) return 0;
    size_t needed=(size_t)(pixels->height-1)*(size_t)pixels->line_stride+(size_t)pixels->width*4;
    return pixels->extent>=needed && (uintptr_t)pixels->data<=UINTPTR_MAX-needed;
}
static unsigned hub_native_glyph_coverage(const struct hub_native_pixels *pixels,int x,int y,int polarity) {
    const unsigned char *p=source_badge_native_pixel_at(pixels,x,y);
    if (!p) return 0;
    unsigned alpha=p[3];
    unsigned light=((unsigned)p[2]*54+(unsigned)p[1]*183+(unsigned)p[0]*19+128)>>8;
    if (light>alpha) light=alpha; /* Transparent RGB cannot create visible ink. */
    return polarity ? alpha-light:light;
}
static int hub_native_glyph_bounds(const struct hub_native_pixels *pixels,int x,int y,int width,int height,
                                    int polarity,struct hub_native_glyph_crop *out) {
    if (!out || !hub_native_glyph_view_valid(pixels) || (polarity!=0 && polarity!=1) ||
        x<0 || y<0 || x>=pixels->width || y>=pixels->height || width<=0 || height<=0 ||
        width>256 || height>256 || width>pixels->width-x || height>pixels->height-y) return 0;
    int left=x+width,top=y+height,right=-1,bottom=-1;
    for (int py=y;py<y+height;py++) for (int px=x;px<x+width;px++)
        if (hub_native_glyph_coverage(pixels,px,py,polarity)) {
            if (px<left) left=px;
            if (px>right) right=px;
            if (py<top) top=py;
            if (py>bottom) bottom=py;
        }
    if (right<left || bottom<top) return 0;
    *out=(struct hub_native_glyph_crop){.pixels=*pixels,.x=left,.y=top,
        .width=right-left+1,.height=bottom-top+1,.polarity=polarity};
    return 1;
}
/* family: 0 Source, 1 Browse, 2 Waveform. The rightmost column-connected
 * component brighter than the modal band background is a candidate digit.
 * A dominant light background or nonuniform art can make extraction fail; this
 * is conservative measurement, not proof that the body is a USB caption.
 * Source labels begin above the bottom third, so both vertical families use
 * the lower half. Waveform retains its entire horizontal body. */
static int source_badge_native_caption_digit(const struct hub_native_pixels *pixels,int family,
                                             struct hub_native_glyph_crop *out) {
    if (!out || !hub_native_glyph_view_valid(pixels) || family<0 || family>2) return 0;
    int top=family<2 ? pixels->height/2:0;
    unsigned histogram[256]={0};
    for (int y=top;y<pixels->height;y++) for (int x=0;x<pixels->width;x++) {
        unsigned coverage=hub_native_glyph_coverage(pixels,x,y,0);
        if (coverage>255) return 0;
        histogram[coverage]++;
    }
    unsigned background=0;
    for (unsigned light=1;light<256;light++)
        if (histogram[light]>histogram[background]) background=light;
    uint32_t representative=0;int sampled=0;
    for (int y=top;!sampled && y<pixels->height;y++) for (int x=0;x<pixels->width;x++)
        if (hub_native_glyph_coverage(pixels,x,y,0)==background) {
            source_badge_native_pixel_argb(pixels,x,y,&representative);sampled=1;break;
        }
    int right=-1,left=-1;
    for (int x=pixels->width-1;x>=0;x--) {
        int ink=0;
        for (int y=top;y<pixels->height;y++)
            if (hub_native_glyph_coverage(pixels,x,y,0)>background) { ink=1;break; }
        if (right<0) { if (ink) right=left=x; }
        else if (!ink) break;
        else left=x;
    }
    if (right<0) return 0;
    int ink_top=pixels->height,ink_bottom=-1;
    for (int y=top;y<pixels->height;y++) for (int x=left;x<=right;x++)
        if (hub_native_glyph_coverage(pixels,x,y,0)>background) {
            if (y<ink_top) ink_top=y;
            if (y>ink_bottom) ink_bottom=y;
        }
    if (ink_bottom<ink_top) return 0;
    *out=(struct hub_native_glyph_crop){.pixels=*pixels,.x=left,.y=ink_top,
        .width=right-left+1,.height=ink_bottom-ink_top+1,.polarity=0,
        .background_argb=representative,.background_luminance=background,
        .matted_background=background!=0};
    return 1;
}
static int source_badge_native_player_digit(const struct hub_native_pixels *pixels,int polarity,
                                            struct hub_native_glyph_crop *out) {
    if (!hub_native_glyph_view_valid(pixels) || pixels->width<=4 || pixels->height<=4) return 0;
    return hub_native_glyph_bounds(pixels,2,2,pixels->width-4,pixels->height-4,polarity,out);
}
static int hub_native_glyph_crop_valid(const struct hub_native_glyph_crop *crop) {
    return crop && hub_native_glyph_view_valid(&crop->pixels) && crop->x>=0 && crop->y>=0 &&
        crop->x<crop->pixels.width && crop->y<crop->pixels.height &&
        crop->width>0 && crop->height>0 && crop->width<=256 && crop->height<=256 &&
        crop->width<=crop->pixels.width-crop->x &&
        crop->height<=crop->pixels.height-crop->y && (crop->polarity==0 || crop->polarity==1);
}
static int source_badge_native_glyph_compare(const struct hub_native_glyph_crop *caption,
                                            const struct hub_native_glyph_crop *player,
                                            struct hub_native_glyph_score *score) {
    if (!score) return 0;
    memset(score,0,sizeof *score);score->mean_error_ppm=UINT_MAX;
    if (!hub_native_glyph_crop_valid(caption) || !hub_native_glyph_crop_valid(player)) return 0;
    if (caption->width!=player->width || caption->height!=player->height) return 1;
    score->same_dimensions=1;
    score->compared_pixels=(unsigned)caption->width*(unsigned)caption->height;
    for (int y=0;y<caption->height;y++) for (int x=0;x<caption->width;x++) {
        unsigned a=hub_native_glyph_coverage(&caption->pixels,caption->x+x,caption->y+y,caption->polarity);
        unsigned b=hub_native_glyph_coverage(&player->pixels,player->x+x,player->y+y,player->polarity);
        unsigned error=a>b ? a-b:b-a;
        score->absolute_error+=error;score->different_pixels+=error!=0;
        if (error>score->max_error) score->max_error=error;
    }
    score->exact=score->absolute_error==0;
    score->mean_error_ppm=(unsigned)(score->absolute_error*1000000/((uint64_t)255*score->compared_pixels));
    return 1;
}
/* Try both native foreground polarities without scaling or thresholding. A
 * non-exact score is measurement only; callers must not treat it as font proof. */
static int source_badge_native_glyph_match(const struct hub_native_glyph_crop *caption,
                                          const struct hub_native_pixels *player,
                                          struct hub_native_glyph_crop *best,
                                          struct hub_native_glyph_score *score) {
    if (!best || !score) return 0;
    memset(best,0,sizeof *best);
    memset(score,0,sizeof *score);
    score->mean_error_ppm=UINT_MAX;
    if (!hub_native_glyph_crop_valid(caption)) return 0;
    int found=0;
    for (int polarity=0;polarity<2;polarity++) {
        struct hub_native_glyph_crop candidate;struct hub_native_glyph_score measured;
        if (!source_badge_native_player_digit(player,polarity,&candidate) ||
            !source_badge_native_glyph_compare(caption,&candidate,&measured)) continue;
        if (!found || measured.mean_error_ppm<score->mean_error_ppm) {
            *best=candidate;*score=measured;found=1;
        }
    }
    return found;
}
/* Metadata-only diagnostics. These report raw BGRA/alpha discrepancies and
 * white-ink source-over predictions; they never change strict mask equality.
 * Channel order in error[] is B,G,R,A, matching native premultiplied bytes. */
struct hub_native_glyph_channel_error {
    uint64_t absolute_error;
    unsigned different_pixels,max_error;
};
struct hub_native_glyph_pixel_stats {
    unsigned pixels,non_gray,not_white_premultiplied,alpha_min,alpha_max,max_rgb_alpha_error;
};
struct hub_native_glyph_diagnostic {
    int same_dimensions;
    struct hub_native_glyph_pixel_stats caption,player;
    struct hub_native_glyph_channel_error raw[4];
    /* Rounding modes: floor, nearest, ceil. Caller supplies exact native matte. */
    struct hub_native_glyph_channel_error matte[3][4];
    uint32_t matte_argb;
};
static void hub_native_glyph_channel_note(struct hub_native_glyph_channel_error *error,unsigned a,unsigned b) {
    unsigned difference=a>b ? a-b:b-a;
    error->absolute_error+=difference;error->different_pixels+=difference!=0;
    if (difference>error->max_error) error->max_error=difference;
}
static void hub_native_glyph_stats_note(struct hub_native_glyph_pixel_stats *stats,const unsigned char *p) {
    if (!stats->pixels || p[3]<stats->alpha_min) stats->alpha_min=p[3];
    if (!stats->pixels || p[3]>stats->alpha_max) stats->alpha_max=p[3];
    stats->pixels++;stats->non_gray+=p[0]!=p[1] || p[1]!=p[2];
    stats->not_white_premultiplied+=p[0]!=p[3] || p[1]!=p[3] || p[2]!=p[3];
    for (unsigned channel=0;channel<3;channel++) {
        unsigned error=p[channel]>p[3] ? p[channel]-p[3]:p[3]-p[channel];
        if (error>stats->max_rgb_alpha_error) stats->max_rgb_alpha_error=error;
    }
}
static int source_badge_native_glyph_diagnose(const struct hub_native_glyph_crop *caption,
                                             const struct hub_native_glyph_crop *player,uint32_t matte_argb,
                                             struct hub_native_glyph_diagnostic *out) {
    if (!out) return 0;
    memset(out,0,sizeof *out);out->matte_argb=matte_argb;
    if (!hub_native_glyph_crop_valid(caption) || !hub_native_glyph_crop_valid(player)) return 0;
    if (caption->width!=player->width || caption->height!=player->height) return 1;
    out->same_dimensions=1;
    for (int y=0;y<caption->height;y++) for (int x=0;x<caption->width;x++) {
        const unsigned char *a=source_badge_native_pixel_at(&caption->pixels,caption->x+x,caption->y+y);
        const unsigned char *b=source_badge_native_pixel_at(&player->pixels,player->x+x,player->y+y);
        if (!a || !b) return 0;
        hub_native_glyph_stats_note(&out->caption,a);hub_native_glyph_stats_note(&out->player,b);
        unsigned coverage=hub_native_glyph_coverage(&player->pixels,player->x+x,player->y+y,player->polarity);
        if (coverage>255) return 0;
        for (unsigned channel=0;channel<4;channel++) {
            hub_native_glyph_channel_note(&out->raw[channel],a[channel],b[channel]);
            unsigned background=(matte_argb>>(8*channel))&255;
            unsigned numerator=coverage*255+background*(255-coverage);
            for (unsigned rounding=0;rounding<3;rounding++) {
                unsigned bias=rounding==0 ? 0:rounding==1 ? 127:254;
                unsigned predicted=(numerator+bias)/255;
                hub_native_glyph_channel_note(&out->matte[rounding][channel],a[channel],predicted);
            }
        }
    }
    return 1;
}
#endif

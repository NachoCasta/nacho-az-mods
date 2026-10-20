/* SPDX-License-Identifier: GPL-3.0-only */
/* Synthetic pixels only: no native artwork or font data. */
#include <assert.h>
#include <stdio.h>
#include "source-badge-native-glyph.h"
static unsigned char body[40*40*4],player[22*22*4];
static struct hub_native_pixels view(unsigned char *data,int width,int height) {
    return (struct hub_native_pixels){data,width,height,2,4,width*4,(size_t)width*height*4};
}
static void pixel(unsigned char *data,int width,int x,int y,unsigned value,unsigned alpha) {
    unsigned char *p=data+(y*width+x)*4;p[0]=p[1]=p[2]=(unsigned char)value;p[3]=(unsigned char)alpha;
}
int main(void) {
    struct hub_native_pixels caption=view(body,40,40),number=view(player,22,22);
    struct hub_native_glyph_crop digit,glyph;struct hub_native_glyph_score score;
    /* Separate icon, caption letters, then a three-column antialiased digit. */
    for (int y=1;y<10;y++) for (int x=30;x<38;x++) pixel(body,40,x,y,255,255);
    for (int y=30;y<35;y++) for (int x=3;x<20;x++) pixel(body,40,x,y,255,255);
    for (int y=29;y<36;y++) for (int x=31;x<34;x++) {
        unsigned coverage=x==31 ? 64:x==32 ? 255:128;
        pixel(body,40,x,y,coverage,coverage);
        pixel(player,22,x-24,y-23,coverage,coverage);
    }
    assert(source_badge_native_caption_digit(&caption,0,&digit));
    assert(digit.x==31 && digit.y==29 && digit.width==3 && digit.height==7);
    assert(source_badge_native_glyph_match(&digit,&number,&glyph,&score));
    assert(score.exact && score.same_dimensions && score.compared_pixels==21 && !score.absolute_error && !score.mean_error_ppm);
    assert(glyph.x==7 && glyph.y==6 && glyph.polarity==0);
    struct hub_native_glyph_diagnostic diagnostic;
    assert(source_badge_native_glyph_diagnose(&digit,&glyph,0,&diagnostic));
    assert(diagnostic.same_dimensions && diagnostic.caption.pixels==21);
    assert(!diagnostic.caption.non_gray && !diagnostic.caption.not_white_premultiplied);
    for (unsigned channel=0;channel<4;channel++) {
        assert(!diagnostic.raw[channel].absolute_error);
        assert(!diagnostic.matte[1][channel].absolute_error);
    }
    /* A single coverage difference cannot pass an exact match. */
    pixel(player,22,8,7,254,255);
    assert(source_badge_native_glyph_match(&digit,&number,&glyph,&score));
    assert(!score.exact && score.different_pixels==1 && score.absolute_error==1 && score.max_error==1);
    /* Opaque light background and dark ink select inverse luminance coverage. */
    memset(player,255,sizeof player);
    for (int y=6;y<13;y++) for (int x=7;x<10;x++) {
        unsigned coverage=x==7 ? 64:x==8 ? 255:128;
        pixel(player,22,x,y,255-coverage,255);
    }
    assert(source_badge_native_glyph_match(&digit,&number,&glyph,&score));
    assert(score.exact && glyph.polarity==1);
    /* Source caption starts above the old bottom-third boundary. */
    memset(body,0,sizeof body);
    for (int y=21;y<29;y++) for (int x=31;x<34;x++) pixel(body,40,x,y,255,255);
    assert(source_badge_native_caption_digit(&caption,0,&glyph));
    assert(glyph.y==21 && glyph.height==8 && !glyph.background_luminance);
    /* Browse's opaque gray matte must not become an all-band component. */
    for (int y=0;y<40;y++) for (int x=0;x<40;x++) pixel(body,40,x,y,35,255);
    for (int y=25;y<32;y++) for (int x=31;x<34;x++) {
        unsigned coverage=x==31 ? 64:x==32 ? 255:128;
        pixel(body,40,x,y,35+(220*coverage+127)/255,255);
    }
    assert(source_badge_native_caption_digit(&caption,1,&glyph));
    assert(glyph.x==31 && glyph.y==25 && glyph.width==3 && glyph.height==7);
    assert(glyph.background_argb==0xff232323u && glyph.background_luminance==35 && glyph.matted_background);
    /* Same geometry with a matte is not silently normalized into exact equality. */
    struct hub_native_glyph_crop reference;
    assert(source_badge_native_player_digit(&number,1,&reference));
    assert(source_badge_native_glyph_compare(&glyph,&reference,&score));
    assert(score.same_dimensions && !score.exact && score.absolute_error);
    assert(source_badge_native_glyph_diagnose(&glyph,&reference,0xff232323u,&diagnostic));
    assert(diagnostic.same_dimensions && diagnostic.caption.alpha_min==255 && diagnostic.caption.alpha_max==255);
    assert(diagnostic.caption.not_white_premultiplied && !diagnostic.caption.non_gray);
    for (unsigned channel=0;channel<4;channel++) assert(!diagnostic.matte[1][channel].absolute_error);
    assert(diagnostic.raw[0].absolute_error);
    /* Restore the earlier transparent caption for remaining extraction checks. */
    memset(body,0,sizeof body);
    for (int y=1;y<10;y++) for (int x=30;x<38;x++) pixel(body,40,x,y,255,255);
    for (int y=29;y<36;y++) for (int x=31;x<34;x++) pixel(body,40,x,y,255,255);
    /* Border ink is ignored; unscaled geometry mismatches are reported. */
    memset(player,0,sizeof player);
    for (int y=0;y<22;y++) for (int x=0;x<22;x++)
        if (x<2 || y<2 || x>=20 || y>=20) pixel(player,22,x,y,255,255);
    pixel(player,22,8,8,255,255);
    assert(source_badge_native_player_digit(&number,0,&glyph));
    assert(glyph.width==1 && glyph.height==1);
    assert(source_badge_native_glyph_compare(&digit,&glyph,&score));
    assert(!score.same_dimensions && !score.exact && score.mean_error_ppm==UINT_MAX);
    /* Family crop boundaries differ; rightmost icon is allowed in Waveform. */
    assert(source_badge_native_caption_digit(&caption,1,&glyph));assert(glyph.x==31);
    assert(source_badge_native_caption_digit(&caption,2,&glyph));assert(glyph.x==30 && glyph.y==1 && glyph.width==8);
    memset(body,0,sizeof body);pixel(body,40,35,32,255,0);
    assert(!source_badge_native_caption_digit(&caption,0,&glyph)); /* Hidden RGB is not ink. */
    assert(!source_badge_native_caption_digit(&caption,-1,&glyph));
    assert(!source_badge_native_caption_digit(NULL,0,&glyph));
    caption.width=257;assert(!hub_native_glyph_view_valid(&caption));caption.width=40;
    caption.extent--;assert(!hub_native_glyph_view_valid(&caption));caption.extent++;
    caption.line_stride=-1;assert(!hub_native_glyph_view_valid(&caption));caption.line_stride=160;
    caption.format=1;assert(!hub_native_glyph_view_valid(&caption));caption.format=2;
    assert(!source_badge_native_player_digit(&number,2,&glyph));
    assert(!source_badge_native_glyph_compare(NULL,&glyph,&score));
    assert(!source_badge_native_glyph_diagnose(NULL,&glyph,0,&diagnostic));
    assert(!diagnostic.same_dimensions && !diagnostic.caption.pixels);
    memset(&glyph,0xa5,sizeof glyph);memset(&score,0xa5,sizeof score);
    assert(!source_badge_native_glyph_match(NULL,&number,&glyph,&score));
    assert(!glyph.pixels.data && !glyph.width && !glyph.height);
    assert(!score.exact && !score.compared_pixels && score.mean_error_ppm==UINT_MAX);
    puts("native glyph masks: family crop, exact/unscaled coverage, polarity, border and bounds checks pass");
    return 0;
}

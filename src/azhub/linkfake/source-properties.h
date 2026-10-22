/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HUB_SOURCE_PROPERTIES_H
#define HUB_SOURCE_PROPERTIES_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
struct hub_source_properties {
    char name[128];
    unsigned char color,name_valid,color_valid;
};
/* Exact EP147 native mount-point PropertyTable data: 0x94 bytes, with a
 * terminated UTF-16LE name in the first 32 units and background color at 0x7c.
 * This is not DEVSETTING's waveform-color field or the network byte order. */
static int hub_source_properties_decode(const unsigned char *data,size_t size,
                                        struct hub_source_properties *out) {
    if (!out) return 0;
    memset(out,0,sizeof *out);
    if (!data || size!=0x94) return 0;
    out->color_valid=data[0x7c]<=8;
    out->color=out->color_valid ? data[0x7c]:0;
    size_t used=0;int terminated=0,valid=1,visible=0;
    for (size_t i=0;i<32;i++) {
        uint32_t code=data[i*2]|((uint32_t)data[i*2+1]<<8);
        if (!code) {terminated=1;break;}
        if (code>=0xd800 && code<=0xdbff) {
            if (++i>=32) {valid=0;break;}
            uint32_t low=data[i*2]|((uint32_t)data[i*2+1]<<8);
            if (low<0xdc00 || low>0xdfff) {valid=0;break;}
            code=0x10000+((code-0xd800)<<10)+(low-0xdc00);
        } else if ((code>=0xdc00 && code<=0xdfff) || code<32 || code==127) {
            valid=0;break;
        }
        if (code!=32) visible=1;
        if (code<0x80) out->name[used++]=(char)code;
        else if (code<0x800) {
            out->name[used++]=(char)(0xc0|(code>>6));
            out->name[used++]=(char)(0x80|(code&63));
        } else if (code<0x10000) {
            out->name[used++]=(char)(0xe0|(code>>12));
            out->name[used++]=(char)(0x80|((code>>6)&63));
            out->name[used++]=(char)(0x80|(code&63));
        } else {
            out->name[used++]=(char)(0xf0|(code>>18));
            out->name[used++]=(char)(0x80|((code>>12)&63));
            out->name[used++]=(char)(0x80|((code>>6)&63));
            out->name[used++]=(char)(0x80|(code&63));
        }
    }
    out->name_valid=valid && terminated && visible;
    if (!out->name_valid) used=0;
    out->name[used]=0;
    return 1;
}
#endif

/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef NUSB_NAME_POLICY_H
#define NUSB_NAME_POLICY_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
/* A complete, visible UTF-8 name, with the native 31 UTF-16-unit limit.
 * Reject controls and malformed encodings rather than repairing configured text. */
static int nusb_valid_name(const char *s,size_t bound,size_t max_units) {
    if (!s || !bound) return 0;
    size_t n=0,units=0; int visible=0;
    while(n<bound && s[n]) {
        uint32_t c=(unsigned char)s[n++]; unsigned extra=0; uint32_t min=0;
        if(c>=0xc2 && c<=0xdf) {c&=31;extra=1;min=0x80;}
        else if(c>=0xe0 && c<=0xef) {c&=15;extra=2;min=0x800;}
        else if(c>=0xf0 && c<=0xf4) {c&=7;extra=3;min=0x10000;}
        else if(c>=128) return 0;
        if(extra>bound-n) return 0;
        for(unsigned j=0;j<extra;j++) {
            unsigned char b=(unsigned char)s[n++];
            if((b&0xc0)!=0x80)return 0;
            c=(c<<6)|(b&63);
        }
        if(c<min || c>0x10ffff || (c>=0xd800 && c<=0xdfff) || c<32 || (c>=127 && c<=159)) return 0;
        units+=c>0xffff ? 2:1;
        if(units>max_units)return 0;
        if(c!=32 && c!=0xa0 && c!=0x3000)visible=1;
    }
    return n<bound && visible;
}
/* Copy a valid volume label into the native limit without splitting Unicode.
 * Configured names are never passed through this truncation path. */
static int nusb_label(const char *s,char out[128]) {
    out[0]=0;
    if(!nusb_valid_name(s,512,255))return 0;
    size_t n=0,units=0;
    while(s[n]) {
        unsigned char c=(unsigned char)s[n];
        size_t bytes=c<128 ? 1:(c<224 ? 2:(c<240 ? 3:4));
        size_t add=bytes==4 ? 2:1;
        if(units+add>31 || n+bytes>=128)break;
        n+=bytes;units+=add;
    }
    memcpy(out,s,n);out[n]=0;
    if(!nusb_valid_name(out,128,31)) {out[0]=0;return 0;}
    return 1;
}
static int nusb_native_port(const unsigned char *media) {
    if(!media || media[0]!=1 || media[1] || media[2] || media[3])return -1;
    return media[4]==2 ? 0:(media[4]==0x22 ? 1:-1);
}
#endif

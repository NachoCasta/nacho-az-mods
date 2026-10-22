/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HUB_PUBLIC_WIRE_NAME_H
#define HUB_PUBLIC_WIRE_NAME_H
#include <stddef.h>
#include <stdint.h>
/* Native Link NFS lookup strings are UTF16LE. Decode strictly to filesystem
 * UTF8, accepting one optional final NUL but never embedded NUL/surrogates. */
static int hub_public_name_decode(const unsigned char *data,size_t length,char out[256],size_t *written) {
    if (!data || !length || length>1024 || length%2) return 0;
    size_t used=0;
    for (size_t i=0;i<length;i+=2) {
        uint32_t code=(uint32_t)data[i]|((uint32_t)data[i+1]<<8);
        if (!code) { if (i+2!=length) return 0; break; }
        if (code>=0xd800 && code<=0xdbff) {
            if (i+3>=length) return 0;
            uint32_t low=(uint32_t)data[i+2]|((uint32_t)data[i+3]<<8);
            if (low<0xdc00 || low>0xdfff) return 0;
            code=0x10000+((code-0xd800)<<10)+(low-0xdc00); i+=2;
        } else if (code>=0xdc00 && code<=0xdfff) return 0;
        unsigned bytes=code<0x80 ? 1 : code<0x800 ? 2 : code<0x10000 ? 3 : 4;
        if (used+bytes>255) return 0;
        if (bytes==1) out[used++]=(char)code;
        else {
            out[used++]=(char)((bytes==2 ? 0xc0 : bytes==3 ? 0xe0 : 0xf0)|(code>>(6*(bytes-1))));
            for (unsigned n=bytes-1;n;--n) out[used++]=(char)(0x80|((code>>(6*(n-1)))&0x3f));
        }
    }
    if (!used) return 0;
    out[used]=0; *written=used; return 1;
}
/* Encode one catalog UTF8 component as UTF16LE, rejecting noncanonical UTF8. */
static int hub_public_name_encode(const char *text,size_t length,unsigned char out[512],size_t *written) {
    if (!text || !length || length>255) return 0;
    size_t used=0;
    for (size_t i=0;i<length;) {
        unsigned char first=(unsigned char)text[i++]; uint32_t code; unsigned extra;
        if (first<0x80) { code=first; extra=0; }
        else if (first>=0xc2 && first<=0xdf) { code=first&31; extra=1; }
        else if (first>=0xe0 && first<=0xef) { code=first&15; extra=2; }
        else if (first>=0xf0 && first<=0xf4) { code=first&7; extra=3; }
        else return 0;
        if (extra>length-i) return 0;
        for (unsigned n=0;n<extra;++n) {
            unsigned char next=(unsigned char)text[i++];
            if ((next&0xc0)!=0x80) return 0;
            code=(code<<6)|(next&63);
        }
        if (!code || (extra==1 && code<0x80) || (extra==2 && code<0x800) ||
            (extra==3 && code<0x10000) || code>0x10ffff || (code>=0xd800 && code<=0xdfff)) return 0;
        if (code<0x10000) { out[used++]=(unsigned char)code; out[used++]=(unsigned char)(code>>8); }
        else {
            code-=0x10000; uint32_t high=0xd800+(code>>10),low=0xdc00+(code&1023);
            out[used++]=(unsigned char)high; out[used++]=(unsigned char)(high>>8);
            out[used++]=(unsigned char)low; out[used++]=(unsigned char)(low>>8);
        }
    }
    *written=used; return 1;
}
#endif

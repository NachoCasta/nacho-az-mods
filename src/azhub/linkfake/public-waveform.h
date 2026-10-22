/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HUB_PUBLIC_WAVEFORM_H
#define HUB_PUBLIC_WAVEFORM_H
#include <errno.h>
#include <stdlib.h>
#include "public-db-wire.h"
/* Write one of the two fixed-size legacy preview parts into a900-byte buffer. */
static int hub_wave_preview_part(const unsigned char *section,size_t length,unsigned tiny,unsigned char output[900]) {
    unsigned count=tiny ? 100 : 400;
    if (!section || length!=20+count || memcmp(section,tiny ? "PWV2" : "PWAV",4) ||
        hub_db_u32(section+4)!=20 || hub_db_u32(section+8)!=length || hub_db_u32(section+12)!=count) {
        errno=EINVAL; return -1;
    }
    if (tiny) memcpy(output+800,section+20,count);
    else for (unsigned i=0;i<count;++i) {
        output[i*2]=section[20+i]&31; output[i*2+1]=section[20+i]>>5;
    }
    return 0;
}
/* Legacy detail has nineteen leading bytes ignored by Beat Link. Their native
 * meaning remains unverified; initialize them to zero for the standalone
 * provider. Keep every original PWV3 waveform sample unchanged. */
static int hub_wave_detail_convert(const unsigned char *section,size_t length,
        unsigned char **output,uint32_t *size) {
    *output=NULL; *size=0;
    if (!section || length<24 || memcmp(section,"PWV3",4) || hub_db_u32(section+4)!=24 ||
        hub_db_u32(section+8)!=length || hub_db_u32(section+12)!=1 ||
        hub_db_u32(section+16)!=length-24) { errno=EINVAL; return -1; }
    size_t count=length-24;
    if (count>HUB_DB_FIELD_MAX-19) { errno=EFBIG; return -1; }
    unsigned char *data=calloc(1,count+19);
    if (!data) { errno=ENOMEM; return -1; }
    memcpy(data+19,section+24,count); *output=data; *size=(uint32_t)count+19; return 0;
}
#endif

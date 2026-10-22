/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HUB_PUBLIC_BEATGRID_H
#define HUB_PUBLIC_BEATGRID_H
#include <errno.h>
#include <stdlib.h>
#include "public-db-wire.h"
/* Convert exported PQTZ beats to the legacy dbserver layout. Preserve each
 * recorded tempo/time; do not regenerate a constant-tempo grid. Unknown wire
 * fields use the public-provider convention, pending native-client validation. */
static void hub_grid_le32(unsigned char *out,uint32_t value) {
    for (unsigned i=0;i<4;++i) out[i]=(unsigned char)(value>>(8*i));
}
static int hub_grid_convert(const unsigned char *section,size_t length,unsigned char **output,uint32_t *size) {
    *output=NULL; *size=0;
    if (!section || length<24 || memcmp(section,"PQTZ",4) || hub_db_u32(section+4)!=24 ||
        hub_db_u32(section+8)!=length) { errno=EINVAL; return -1; }
    uint32_t count=hub_db_u32(section+20);
    if (count!=(length-24)/8 || (length-24)%8) { errno=EINVAL; return -1; }
    if (count>(HUB_DB_FIELD_MAX-20)/16) { errno=EFBIG; return -1; }
    uint32_t bytes=20+count*16; unsigned char *data=calloc(1,bytes);
    if (!data) { errno=ENOMEM; return -1; }
    hub_grid_le32(data,0x80000); hub_grid_le32(data+4,count); hub_grid_le32(data+8,count*16);
    hub_grid_le32(data+12,1); hub_grid_le32(data+16,1);
    for (uint32_t i=0;i<count;++i) {
        const unsigned char *in=section+24+i*8; unsigned char *out=data+20+i*16;
        out[0]=in[1]; out[1]=in[0]; out[2]=in[3]; out[3]=in[2];
        hub_grid_le32(out+4,hub_db_u32(in+4)); memset(out+8,255,8);
    }
    *output=data; *size=bytes; return 0;
}
#endif

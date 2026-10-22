/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "public-db-wire.h"
static int normalize(void) {
    unsigned char *input=malloc(HUB_DB_FRAME_MAX*2),*output=malloc(HUB_DB_FRAME_MAX);
    assert(input && output); size_t length=fread(input,1,HUB_DB_FRAME_MAX*2,stdin),offset=0;
    assert(!ferror(stdin) && feof(stdin));
    while (offset<length) {
        struct hub_db_message message; size_t used,written;
        assert(hub_db_message_read(input+offset,length-offset,&message,&used)==1 && used);
        assert(!hub_db_message_write(&message,output,HUB_DB_FRAME_MAX,&written));
        assert(fwrite(output,1,written,stdout)==written); offset+=used;
    }
    free(input); free(output); return 0;
}
int main(int argc,char **argv) {
    if (argc==2 && !strcmp(argv[1],"normalize")) return normalize();
    assert(argc==1);
    const unsigned char text[]={0,65,0xd8,0x3c,0xdf,0xb5,0,0},blob[]={1,2};
    struct hub_db_message original={.transaction=17,.type=0x4101,.count=3};
    original.args[0]=(struct hub_db_field){.kind=HUB_DB_NUMBER,.number=7};
    original.args[1]=(struct hub_db_field){.kind=HUB_DB_STRING,.length=sizeof text,.data=text};
    original.args[2]=(struct hub_db_field){.kind=HUB_DB_BINARY,.length=sizeof blob,.data=blob};
    unsigned char bytes[2048],again[2048]; size_t length,used,written;
    assert(!hub_db_message_write(&original,bytes,sizeof bytes,&length));
    struct hub_db_message parsed;
    for (size_t n=0;n<length;++n) assert(!hub_db_message_read(bytes,n,&parsed,&used) && !used);
    assert(hub_db_message_read(bytes,length,&parsed,&used)==1 && used==length);
    assert(parsed.transaction==17 && parsed.type==0x4101 && parsed.count==3);
    assert(!hub_db_message_write(&parsed,again,sizeof again,&written) && written==length && !memcmp(bytes,again,length));
    assert(hub_db_message_write(&parsed,again,length-1,&written)<0 && !written);
    memcpy(bytes+length,bytes,length);
    assert(hub_db_message_read(bytes,length*2,&parsed,&used)==1 && used==length);
    bytes[20]=2; assert(hub_db_message_read(bytes,length,&parsed,&used)<0 && !used); bytes[20]=6;
    hub_db_put32(bytes+16,1); assert(hub_db_message_read(bytes,length,&parsed,&used)<0); /* Too few type tags. */
    original.count=16;
    for (unsigned i=0;i<16;++i) original.args[i]=(struct hub_db_field){.kind=HUB_DB_NUMBER,.number=i};
    assert(!hub_db_message_write(&original,bytes,sizeof bytes,&length));
    assert(hub_db_message_read(bytes,length,&parsed,&used)==1 && parsed.args[15].number==15);
    original.count=3; original.args[0].number=0;
    original.args[1]=(struct hub_db_field){.kind=HUB_DB_BINARY};
    assert(!hub_db_message_write(&original,bytes,sizeof bytes,&length));
    assert(hub_db_message_read(bytes,length,&parsed,&used)==1 && parsed.args[1].length==0 && parsed.args[2].number==2);
    original.args[1].data=blob; original.args[1].length=2;
    assert(hub_db_message_write(&original,bytes,sizeof bytes,&length)<0); /* Ambiguous zero prefix. */
    unsigned char malformed[]={38,0,0,0,2,0xd8,0,0,0}; used=0; struct hub_db_field field;
    assert(hub_db_field_read(malformed,sizeof malformed,&used,&field)<0);
    malformed[0]=20; hub_db_put32(malformed+1,HUB_DB_FIELD_MAX+1); used=0;
    assert(hub_db_field_read(malformed,sizeof malformed,&used,&field)<0);
    puts("PASS: bounded dbserver framing, all truncated prefixes, coalesced frames, Unicode, type arrays and empty binary fields");
}

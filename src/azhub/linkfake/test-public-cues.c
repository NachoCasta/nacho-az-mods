/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "public-cues.h"
static void dump(const char *path) {
    FILE *file=fopen(path,"rb"); assert(file && !fseek(file,0,SEEK_END));
    long size=ftell(file); assert(size>=12 && size<=4*1024*1024 && !fseek(file,0,SEEK_SET));
    unsigned char *data=malloc((size_t)size); assert(data && fread(data,1,size,file)==(size_t)size && !fclose(file));
    assert(!memcmp(data,"PMAI",4) && hub_db_u32(data+8)==(uint32_t)size);
    size_t offset=hub_db_u32(data+4); assert(offset>=12 && offset<=(size_t)size);
    while (offset<(size_t)size) {
        assert((size_t)size-offset>=12); uint32_t length=hub_db_u32(data+offset+8);
        assert(length>=12 && length<=(size_t)size-offset);
        if (!memcmp(data+offset,"PCOB",4)) {
            struct hub_cue_list list; assert(!hub_cue_list_parse(&list,data+offset,length));
            for (unsigned i=0;i<list.count;++i) {
                struct hub_cue *cue=list.entries+i;
                printf("%u\t%u\t%u\t%u\t%u\t%u\n",list.kind,cue->hot,cue->type,cue->status,cue->time_ms,cue->loop_ms);
            }
            hub_cue_list_destroy(&list);
        }
        offset+=length;
    }
    free(data);
}
int main(int argc,char **argv) {
    if (argc==2) { dump(argv[1]); return 0; }
    assert(argc==1);
    unsigned char extended[76]={0}; memcpy(extended,"PCO2",4); hub_db_put32(extended+4,20);
    hub_db_put32(extended+8,sizeof extended); extended[17]=1;
    unsigned char *ext=extended+20; memcpy(ext,"PCP2",4); hub_db_put32(ext+4,16); hub_db_put32(ext+8,56);
    ext[16]=2; hub_db_put32(ext+20,12345); hub_db_put32(ext+24,23456); ext[28]=3;
    hub_db_put32(ext+40,8);
    const unsigned char comment[]={0,65,0xd8,0x3d,0xde,0,0,0}; memcpy(ext+44,comment,8);
    ext[52]=1; ext[53]=48; ext[54]=90; ext[55]=255;
    struct hub_cue_wire extended_wire={0};
    assert(!hub_cue_extended_append(&extended_wire,extended,sizeof extended));
    assert(extended_wire.size==90 && extended_wire.memory==1 && !extended_wire.hot);
    assert(extended_wire.data[6]==2 && extended_wire.data[34]==3 && extended_wire.data[72]==8);
    assert(extended_wire.data[74]==65 && extended_wire.data[76]==0x3d && extended_wire.data[77]==0xd8);
    assert(extended_wire.data[86]==1 && extended_wire.data[89]==255);
    assert(hub_cue_extended_append(&extended_wire,extended,sizeof extended)<0 && extended_wire.size==90);
    hub_cue_wire_destroy(&extended_wire);
    for (size_t length=0;length<sizeof extended;++length)
        assert(hub_cue_extended_append(&extended_wire,extended,length)<0 && !extended_wire.data);
    ext[48]=0; assert(hub_cue_extended_append(&extended_wire,extended,sizeof extended)<0); ext[48]=0xde;
    hub_db_put32(ext+40,65535); assert(hub_cue_extended_append(&extended_wire,extended,sizeof extended)<0);
    hub_db_put32(extended+8,60); hub_db_put32(ext+8,40);
    assert(!hub_cue_extended_append(&extended_wire,extended,60) && extended_wire.size==82);
    hub_cue_wire_destroy(&extended_wire);
    unsigned char data[136]={0}; memcpy(data,"PCOB",4); hub_db_put32(data+4,24);
    hub_db_put32(data+8,sizeof data); data[19]=2;
    for (unsigned i=0;i<2;++i) {
        unsigned char *entry=data+24+56*i; memcpy(entry,"PCPT",4);
        hub_db_put32(entry+4,28); hub_db_put32(entry+8,56); hub_db_put32(entry+12,i);
        hub_db_put32(entry+16,4+i); entry[28]=(unsigned char)(i+1);
        hub_db_put32(entry+32,12345+i); hub_db_put32(entry+36,i ? 23456 : UINT32_MAX);
    }
    struct hub_cue_list list; assert(!hub_cue_list_parse(&list,data,sizeof data));
    assert(list.count==2 && !list.kind && !list.entries[0].hot && list.entries[0].loop_ms==UINT32_MAX);
    assert(list.entries[1].hot==1 && list.entries[1].type==2 && list.entries[1].status==5 && list.entries[1].loop_ms==23456);
    hub_cue_list_destroy(&list);
    struct hub_cue_wire wire={0}; assert(!hub_cue_wire_append(&wire,data,sizeof data));
    assert(wire.size==72 && wire.hot==1 && wire.memory==1 && wire.data[0]==0 && wire.data[1]==1);
    assert(wire.data[36]==1 && wire.data[38]==1);
    assert(wire.data[12]==0x3b && wire.data[13]==7); /* floor(12345*150/1000)=1851. */
    assert(hub_cue_wire_append(&wire,data,sizeof data)<0 && wire.size==72); /* Duplicate kind. */
    hub_cue_wire_destroy(&wire);
    for (size_t length=0;length<sizeof data;++length) assert(hub_cue_list_parse(&list,data,length)<0 && !list.entries);
    data[19]=3; assert(hub_cue_list_parse(&list,data,sizeof data)<0); data[19]=2;
    hub_db_put32(data+32,UINT32_MAX); assert(hub_cue_list_parse(&list,data,sizeof data)<0);
    hub_db_put32(data+32,56); data[52]=3; assert(hub_cue_list_parse(&list,data,sizeof data)<0);
    puts("PASS: bounded PCOB entries, memory/hot cues, loops, status preservation and malformed records");
}

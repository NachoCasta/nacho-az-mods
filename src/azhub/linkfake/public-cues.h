/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HUB_PUBLIC_CUES_H
#define HUB_PUBLIC_CUES_H
#include <errno.h>
#include <stdlib.h>
#include "public-db-wire.h"
struct hub_cue { uint32_t hot,type,status,time_ms,loop_ms; };
struct hub_cue_list { struct hub_cue *entries; uint32_t count,kind; };
static void hub_cue_list_destroy(struct hub_cue_list *list) {
    free(list->entries); memset(list,0,sizeof *list);
}
/* Decode one original PCOB section. Status is retained verbatim: its meaning
 * for deleted cues is not established, so parsing must not silently drop rows.
 * The caller owns source identity/lifetime and combines memory/hot sections. */
static int hub_cue_list_parse(struct hub_cue_list *list,const unsigned char *data,size_t length) {
    memset(list,0,sizeof *list);
    if (!data || length<24 || memcmp(data,"PCOB",4) || hub_db_u32(data+4)!=24 ||
        hub_db_u32(data+8)!=length || hub_db_u32(data+12)>1) { errno=EINVAL; return -1; }
    uint32_t count=((uint32_t)data[18]<<8)|data[19];
    if (count>(length-24)/56) { errno=EINVAL; return -1; }
    struct hub_cue *entries=calloc(count ? count : 1,sizeof *entries);
    if (!entries) { errno=ENOMEM; return -1; }
    size_t offset=24;
    for (uint32_t i=0;i<count;++i) {
        if (length-offset<56) goto malformed;
        const unsigned char *entry=data+offset;
        uint32_t header=hub_db_u32(entry+4),size=hub_db_u32(entry+8);
        if (memcmp(entry,"PCPT",4) || header!=28 || size<56 || size>length-offset ||
            (entry[28]!=1 && entry[28]!=2)) goto malformed;
        entries[i]=(struct hub_cue){.hot=hub_db_u32(entry+12),.status=hub_db_u32(entry+16),
            .type=entry[28],.time_ms=hub_db_u32(entry+32),.loop_ms=hub_db_u32(entry+36)};
        offset+=size;
    }
    if (offset!=length) goto malformed;
    list->entries=entries; list->count=count; list->kind=hub_db_u32(data+12); return 0;
malformed:
    free(entries); errno=EINVAL; return -1;
}
struct hub_cue_wire { unsigned char *data; uint32_t size,hot,memory,kinds; };
static void hub_cue_wire_destroy(struct hub_cue_wire *wire) {
    free(wire->data); memset(wire,0,sizeof *wire);
}
static void hub_cue_le32(unsigned char *out,uint32_t value) {
    for (unsigned i=0;i<4;++i) out[i]=(unsigned char)(value>>(i*8));
}
/* Legacy cue positions use half frames. Unknown entry fields remain zero;
 * native interpretation and deleted-entry policy still require validation. */
static int hub_cue_wire_append(struct hub_cue_wire *wire,const unsigned char *data,size_t length) {
    struct hub_cue_list list;
    if (hub_cue_list_parse(&list,data,length)<0) return -1;
    if ((wire->kinds&(1U<<list.kind)) || list.count>(HUB_DB_FIELD_MAX-wire->size)/36) {
        hub_cue_list_destroy(&list); errno=EINVAL; return -1;
    }
    for (unsigned i=0;i<list.count;++i) if (list.entries[i].hot>255) {
        hub_cue_list_destroy(&list); errno=EINVAL; return -1;
    }
    uint32_t size=wire->size+list.count*36;
    unsigned char *output=realloc(wire->data,size ? size : 1);
    if (!output) { hub_cue_list_destroy(&list); errno=ENOMEM; return -1; }
    wire->data=output;
    for (unsigned i=0;i<list.count;++i) {
        const struct hub_cue *cue=list.entries+i; unsigned char *entry=output+wire->size+i*36;
        memset(entry,0,36); entry[0]=(unsigned char)(cue->type==2); entry[1]=1; entry[2]=(unsigned char)cue->hot;
        hub_cue_le32(entry+12,(uint32_t)((uint64_t)cue->time_ms*150/1000));
        if (cue->type==2) hub_cue_le32(entry+16,(uint32_t)((uint64_t)cue->loop_ms*150/1000));
        if (cue->hot) ++wire->hot; else ++wire->memory;
    }
    wire->size=size; wire->kinds|=1U<<list.kind; hub_cue_list_destroy(&list); return 0;
}
/* Validate UTF16BE comments including their terminator before swapping bytes. */
static int hub_cue_comment_valid(const unsigned char *data,uint32_t size) {
    if (!size) return 1;
    if (size<2 || (size&1) || data[size-2] || data[size-1]) return 0;
    for (uint32_t i=0;i<size-2;i+=2) {
        unsigned code=((unsigned)data[i]<<8)|data[i+1];
        if (!code || (code>=0xdc00 && code<=0xdfff)) return 0;
        if (code>=0xd800 && code<=0xdbff) {
            if (i+4>=size) return 0;
            unsigned low=((unsigned)data[i+2]<<8)|data[i+3];
            if (low<0xdc00 || low>0xdfff) return 0;
            i+=2;
        }
    }
    return 1;
}
/* Original EXT fields are big-endian, but extended network comments/numbers
 * are little-endian. Unidentified network fields are initialized to zero. */
static int hub_cue_extended_append(struct hub_cue_wire *wire,const unsigned char *data,size_t length) {
    if (!data || length<20 || memcmp(data,"PCO2",4) || hub_db_u32(data+4)!=20 ||
        hub_db_u32(data+8)!=length || hub_db_u32(data+12)>1) { errno=EINVAL; return -1; }
    uint32_t kind=hub_db_u32(data+12),count=((uint32_t)data[16]<<8)|data[17];
    if (wire->kinds&(1U<<kind)) { errno=EINVAL; return -1; }
    size_t offset=20,total=wire->size;
    for (unsigned i=0;i<count;++i) {
        if (length-offset<40) goto malformed_extended;
        const unsigned char *entry=data+offset; uint32_t size=hub_db_u32(entry+8);
        if (memcmp(entry,"PCP2",4) || hub_db_u32(entry+4)!=16 || size<40 || size>length-offset ||
            (size>40 && size<44) || hub_db_u32(entry+12)>255 || (entry[16]!=1 && entry[16]!=2)) goto malformed_extended;
        uint32_t comment=size>=44 ? hub_db_u32(entry+40) : 0;
        if (comment>65534 || comment>size-(size>=44 ? 44 : 40) ||
            (comment && !hub_cue_comment_valid(entry+44,comment))) goto malformed_extended;
        if (total>HUB_DB_FIELD_MAX-82-comment) { errno=EFBIG; return -1; }
        total+=82+comment; offset+=size;
    }
    if (offset!=length) goto malformed_extended;
    unsigned char *output=realloc(wire->data,total ? total : 1);
    if (!output) { errno=ENOMEM; return -1; }
    wire->data=output; offset=20;
    for (unsigned i=0;i<count;++i) {
        const unsigned char *entry=data+offset; uint32_t size=hub_db_u32(entry+8);
        uint32_t comment=size>=44 ? hub_db_u32(entry+40) : 0,hot=hub_db_u32(entry+12);
        unsigned char *out=output+wire->size; memset(out,0,82+comment);
        hub_cue_le32(out,82+comment); out[4]=(unsigned char)hot; out[6]=entry[16];
        hub_cue_le32(out+12,hub_db_u32(entry+20)); hub_cue_le32(out+16,hub_db_u32(entry+24)); out[34]=entry[28];
        out[72]=(unsigned char)comment; out[73]=(unsigned char)(comment>>8);
        for (unsigned j=0;j<comment;j+=2) { out[74+j]=entry[45+j]; out[75+j]=entry[44+j]; }
        for (unsigned j=0;j<4;++j) if (size>44+comment+j) out[78+comment+j]=entry[44+comment+j];
        wire->size+=82+comment; if (hot) ++wire->hot; else ++wire->memory; offset+=size;
    }
    wire->kinds|=1U<<kind; return 0;
malformed_extended:
    errno=EINVAL; return -1;
}
#endif

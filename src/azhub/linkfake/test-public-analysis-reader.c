/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include "public-analysis-reader.h"
#include "public-pdb-source.h"
static int extract(const char *directory,const char *relative,const char *tag,const char *occurrence) {
    assert(strlen(tag)==4); char *end; unsigned long which=strtoul(occurrence,&end,10);
    assert(*occurrence && !*end && which<HUB_ANALYSIS_TAG_COUNT_MAX);
    struct hub_public_root root; assert(!hub_public_root_init(&root) && !hub_public_root_attach(&root,directory,1));
    struct hub_public_pdb_source source; assert(!hub_public_pdb_source_init(&source,&root,1,relative,strlen(relative)));
    struct hub_analysis_reader reader;
    assert(!hub_analysis_init(&reader,(uint64_t)source.expected.st_size,hub_public_pdb_source_read,&source,
        hub_db_u32((const unsigned char *)tag),(unsigned)which));
    int result; while (!(result=hub_analysis_step(&reader))) assert(!root.active);
    assert(result==1); size_t length; const unsigned char *data=hub_analysis_ready(&reader,&length);
    assert(data && fwrite(data,1,length,stdout)==length && !root.active);
    hub_analysis_destroy(&reader); hub_public_root_withdraw(&root); assert(!hub_public_root_destroy(&root)); return 0;
}
struct memory { unsigned char *data; size_t length,largest; int withdrawn; };
static int read_memory(void *opaque,uint64_t offset,void *out,size_t length) {
    struct memory *memory=opaque;
    if (memory->withdrawn) { errno=ESTALE; return -1; }
    if (offset>memory->length || length>memory->length-offset) { errno=EINVAL; return -1; }
    if (length>memory->largest) memory->largest=length;
    memcpy(out,memory->data+offset,length); return 0;
}
int main(int argc,char **argv) {
    if (argc==6 && !strcmp(argv[1],"extract")) return extract(argv[2],argv[3],argv[4],argv[5]);
    assert(argc==1);
    unsigned char data[9064]={0}; memcpy(data,"PMAI",4); hub_db_put32(data+4,28); hub_db_put32(data+8,sizeof data);
    memcpy(data+28,"PCOB",4); hub_db_put32(data+32,12); hub_db_put32(data+36,12);
    memcpy(data+40,"PCOB",4); hub_db_put32(data+44,24); hub_db_put32(data+48,sizeof data-40);
    for (unsigned i=52;i<sizeof data;++i) data[i]=(unsigned char)i;
    struct memory memory={data,sizeof data,0,0}; struct hub_analysis_reader reader; size_t length;
    assert(!hub_analysis_init(&reader,sizeof data,read_memory,&memory,0x50434f42,1));
    assert(!hub_analysis_step(&reader) && !hub_analysis_ready(&reader,&length));
    assert(!hub_analysis_step(&reader) && !hub_analysis_ready(&reader,&length));
    assert(!hub_analysis_step(&reader) && !hub_analysis_ready(&reader,&length));
    assert(hub_analysis_step(&reader)==1 && memory.largest<=8192);
    const unsigned char *result=hub_analysis_ready(&reader,&length);
    assert(result && length==sizeof data-40 && !memcmp(result,data+40,length)); hub_analysis_destroy(&reader);
    assert(!hub_analysis_init(&reader,sizeof data,read_memory,&memory,0x50434f42,0));
    assert(hub_analysis_step(&reader)==1); hub_analysis_destroy(&reader);
    assert(!hub_analysis_init(&reader,sizeof data,read_memory,&memory,0x50575634,0));
    assert(!hub_analysis_step(&reader) && !hub_analysis_step(&reader));
    assert(hub_analysis_step(&reader)<0 && errno==ENOENT); hub_analysis_destroy(&reader);
    assert(!hub_analysis_init(&reader,sizeof data,read_memory,&memory,0x50434f42,1));
    assert(!hub_analysis_step(&reader) && !hub_analysis_step(&reader)); memory.withdrawn=1;
    assert(hub_analysis_step(&reader)<0 && errno==ESTALE && !hub_analysis_ready(&reader,&length)); hub_analysis_destroy(&reader);
    memory.withdrawn=0; hub_db_put32(data+36,0);
    assert(!hub_analysis_init(&reader,sizeof data,read_memory,&memory,0x50434f42,0));
    assert(hub_analysis_step(&reader)<0 && errno==EINVAL); hub_analysis_destroy(&reader);
    hub_db_put32(data+8,1); assert(hub_analysis_init(&reader,sizeof data,read_memory,&memory,0,0)<0); hub_analysis_destroy(&reader);
    puts("PASS: incremental ANLZ tag selection, duplicate occurrence, chunk bounds, missing/malformed sections and source withdrawal");
}

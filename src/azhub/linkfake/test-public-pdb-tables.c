/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include "public-pdb-source.h"
#include "public-pdb-tables.h"
static void put16(unsigned char *p,unsigned n) { p[0]=(unsigned char)n; p[1]=(unsigned char)(n>>8); }
static void put32(unsigned char *p,unsigned n) { put16(p,n); put16(p+2,n>>16); }
static void text(unsigned char *p,const char *value) {
    size_t length=strlen(value); assert(length<126); p[0]=(unsigned char)((length+1)*2+1); memcpy(p+1,value,length);
}
static int emit(void *opaque,const struct hub_public_pdb_entity *entity) {
    (void)opaque; printf("%u\t%u\t%u\t%u\t%u\t%d\t",entity->table,entity->id,entity->parent,
        entity->sort_order,entity->track_id,entity->folder);
    for (const unsigned char *p=(const unsigned char *)entity->name;*p;++p) printf("%02x",*p);
    putchar('\n'); return 0;
}
static int scan(const char *directory,const char *relative) {
    struct hub_public_root root; assert(!hub_public_root_init(&root) && !hub_public_root_attach(&root,directory,1));
    struct hub_public_pdb_source source; assert(!hub_public_pdb_source_init(&source,&root,1,relative,strlen(relative)));
    const unsigned tables[]={1,2,3,4,5,7,8,13};
    for (unsigned i=0;i<sizeof tables/sizeof *tables;++i) {
        struct hub_public_pdb_reader reader;
        assert(!hub_public_pdb_table_init(&reader,(uint64_t)source.expected.st_size,hub_public_pdb_source_read,&source,tables[i]));
        int result; while ((result=hub_public_pdb_entities_step(&reader,emit,NULL))==0) assert(!root.active);
        if (result!=1) fprintf(stderr,"Table failure type=%u page=%u rows=%u error=%d\n",tables[i],reader.current,reader.count,errno);
        assert(result==1); hub_public_pdb_destroy(&reader);
    }
    hub_public_root_withdraw(&root); assert(!hub_public_root_destroy(&root)); return 0;
}
struct memory { unsigned char *bytes; size_t length; };
static int read_memory(void *opaque,uint64_t offset,void *out,size_t length) {
    struct memory *memory=opaque;
    if (offset>memory->length || length>memory->length-offset) { errno=EIO; return -1; }
    memcpy(out,memory->bytes+offset,length); return 0;
}
static int verify(void *opaque,const struct hub_public_pdb_entity *entity) {
    unsigned *count=opaque; ++*count; assert(entity->table==1 && entity->id==7 && !strcmp(entity->name,"Genre")); return 0;
}
int main(int argc,char **argv) {
    if (argc==4 && !strcmp(argv[1],"scan")) return scan(argv[2],argv[3]);
    assert(argc==1);
    unsigned char data[1536]={0}; put32(data+4,512); put32(data+8,1); put32(data+28,1);
    put32(data+36,1); put32(data+40,1); put32(data+516,1); put32(data+520,1);
    data[536]=1; put32(data+552,7); text(data+556,"Genre"); put16(data+1020,1);
    struct memory memory={data,1024}; struct hub_public_pdb_reader reader; unsigned count=0;
    assert(!hub_public_pdb_table_init(&reader,1024,read_memory,&memory,1));
    assert(hub_public_pdb_entities_step(&reader,verify,&count)==1 && count==1); hub_public_pdb_destroy(&reader);
    /* A partial final index group leaves room for packed data beyond the
     * boundary obtained by incorrectly reserving all sixteen offsets. */
    put16(data+1018,450); put32(data+1002,7); text(data+1006,"Genre");
    assert(!hub_public_pdb_table_init(&reader,1024,read_memory,&memory,1));
    assert(hub_public_pdb_entities_step(&reader,verify,&count)==1 && count==2);
    hub_public_pdb_destroy(&reader); put16(data+1018,0);
    /* More than255 allocated offsets, with only the last row still present.
     * The transaction index deliberately differs from the allocation count. */
    unsigned char wide[8192]={0}; put32(wide+4,4096); put32(wide+8,1);
    put32(wide+28,1); put32(wide+36,1); put32(wide+40,1);
    put32(wide+4100,1); put32(wide+4104,1); put16(wide+4120,284);
    put16(wide+4130,133); put32(wide+4136,7); text(wide+4140,"Genre");
    put16(wide+8192-17*36-4,1U<<11);
    struct memory wide_memory={wide,sizeof wide};
    assert(!hub_public_pdb_table_init(&reader,sizeof wide,read_memory,&wide_memory,1));
    assert(hub_public_pdb_entities_step(&reader,verify,&count)==1 && count==3);
    hub_public_pdb_destroy(&reader);
    memcpy(data+1024,data+512,512); memory.length=1536; put32(data+40,2); put32(data+524,1);
    assert(!hub_public_pdb_table_init(&reader,1536,read_memory,&memory,1));
    assert(hub_public_pdb_entities_step(&reader,verify,&count)==0);
    assert(hub_public_pdb_entities_step(&reader,verify,&count)<0); hub_public_pdb_destroy(&reader);
    unsigned char page[512]={0}; struct hub_public_pdb_entity entity; size_t row=40;
    put16(page+row,0x60); put32(page+row+4,9); page[row+9]=10; text(page+row+10,"Near");
    assert(hub_public_pdb_entity_decode(page,row,sizeof page,2,&entity) && !strcmp(entity.name,"Near"));
    put16(page+row,0x64); put16(page+row+10,300); text(page+row+300,"Far");
    assert(hub_public_pdb_entity_decode(page,row,sizeof page,2,&entity) && !strcmp(entity.name,"Far"));
    put16(page+row+10,600); assert(!hub_public_pdb_entity_decode(page,row,sizeof page,2,&entity));
    memset(page,0,sizeof page); put32(page+row+8,11); put32(page+row+12,12); page[row+21]=22; text(page+row+22,"Album");
    assert(hub_public_pdb_entity_decode(page,row,sizeof page,3,&entity) && entity.id==12 && entity.parent==11);
    memset(page,0,sizeof page); put32(page+row,7); put32(page+row+8,2); put32(page+row+12,19);
    put32(page+row+16,1); text(page+row+20,"Folder");
    assert(hub_public_pdb_entity_decode(page,row,sizeof page,7,&entity) && entity.parent==7 && entity.sort_order==2 && entity.folder);
    assert(!hub_public_pdb_entity_decode(page,row,row+19,7,&entity));
    memset(page,0,sizeof page); put32(page+row,3); put32(page+row+4,27); put32(page+row+8,19);
    assert(hub_public_pdb_entity_decode(page,row,sizeof page,8,&entity) && entity.id==19 && entity.track_id==27 && entity.sort_order==3);
    assert(!hub_public_pdb_entity_decode(page,row,row+11,8,&entity));
    puts("PASS: bounded named-table page traversal/cycle, artist near/far bounds, album references and playlist tree/membership decoding");
}

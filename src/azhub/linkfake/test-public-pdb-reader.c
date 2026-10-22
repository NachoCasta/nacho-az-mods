/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include "public-pdb-source.h"
static void put16(unsigned char *p,unsigned value) { p[0]=(unsigned char)value; p[1]=(unsigned char)(value>>8); }
static void put32(unsigned char *p,uint32_t value) { put16(p,value); put16(p+2,value>>16); }
static void fixture(unsigned char data[1024]) {
    memset(data,0,1024); put32(data+4,512); put32(data+8,1);
    put32(data+36,1); put32(data+40,1); put32(data+516,1); put32(data+524,2); data[536]=1;
    size_t row=552,offset=136; put32(data+row+72,17);
    put32(data+row+16,123456); put32(data+row+56,12850); put16(data+row+84,321); data[row+89]=4;
    const char *strings[]={"","Title","/Contents/track.mp3","Comment"}; const unsigned indices[]={14,17,20,16};
    for (unsigned i=0;i<4;++i) {
        size_t length=strlen(strings[i]); put16(data+row+94+indices[i]*2,(unsigned)offset);
        data[row+offset]=(unsigned char)((length+1)*2+1);
        memcpy(data+row+offset+1,strings[i],length); offset+=length+1;
    }
    put16(data+1020,1);
}
struct memory { unsigned char *data; size_t length; };
static int memory_read(void *opaque,uint64_t offset,void *out,size_t length) {
    struct memory *memory=opaque;
    if (offset>memory->length || length>memory->length-offset) { errno=EIO; return -1; }
    memcpy(out,memory->data+offset,length); return 0;
}
static int verify(void *opaque,const struct hub_public_pdb_track *track) {
    unsigned *count=opaque; ++*count;
    assert(track->id==17 && !strcmp(track->path,"/Contents/track.mp3") &&
        !strcmp(track->title,"Title") && !track->analysis_path[0]);
    assert(track->file_size==123456 && track->bpm100==12850 && track->duration==321 && track->rating==4);
    assert(!strcmp(track->comment,"Comment")); return 0;
}
static int cancel(void *opaque,const struct hub_public_pdb_track *track) {
    (void)opaque; (void)track; errno=ECANCELED; return -1;
}
static void print_hex(const char *text) {
    for (const unsigned char *p=(const unsigned char *)text;*p;++p) printf("%02x",*p);
}
static int emit(void *opaque,const struct hub_public_pdb_track *track) {
    printf("%u\t",track->id); print_hex(track->path); putchar('\t');
    print_hex(track->analysis_path); putchar('\t'); print_hex(track->title);
    if (opaque) {
        putchar('\t'); print_hex(track->comment);
        printf("\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u",track->file_size,
            track->artwork_id,track->key_id,track->label_id,track->bitrate,track->bpm100,
            track->genre_id,track->album_id,track->artist_id,track->duration,track->rating);
    }
    putchar('\n'); return 0;
}
static int scan(const char *directory,const char *relative,int full) {
    struct hub_public_root root; assert(!hub_public_root_init(&root));
    assert(!hub_public_root_attach(&root,directory,1));
    struct hub_public_pdb_source source;
    assert(!hub_public_pdb_source_init(&source,&root,1,relative,strlen(relative)));
    struct hub_public_pdb_reader reader;
    assert(!hub_public_pdb_init(&reader,(uint64_t)source.expected.st_size,hub_public_pdb_source_read,&source));
    int result; while ((result=hub_public_pdb_step(&reader,emit,full ? &full : NULL))==0) assert(!root.active);
    hub_public_pdb_destroy(&reader); hub_public_root_withdraw(&root); assert(!hub_public_root_destroy(&root));
    return result==1 ? 0 : 1;
}
int main(int argc,char **argv) {
    if (argc==4 && (!strcmp(argv[1],"scan") || !strcmp(argv[1],"scan-full")))
        return scan(argv[2],argv[3],!strcmp(argv[1],"scan-full"));
    assert(argc==1);
    unsigned char data[1536]; fixture(data); struct memory memory={data,1024};
    struct hub_public_pdb_reader reader; unsigned count=0;
    assert(!hub_public_pdb_init(&reader,memory.length,memory_read,&memory));
    assert(hub_public_pdb_step(&reader,verify,&count)==1 && count==1);
    assert(hub_public_pdb_step(&reader,verify,&count)==1 && count==1); hub_public_pdb_destroy(&reader);
    assert(!hub_public_pdb_init(&reader,memory.length,memory_read,&memory));
    assert(hub_public_pdb_step(&reader,cancel,NULL)<0 && errno==ECANCELED);
    assert(hub_public_pdb_step(&reader,verify,&count)<0 && errno==ECANCELED); hub_public_pdb_destroy(&reader);
    const unsigned offsets[]={4,8,36,516,520,1018};
    for (unsigned i=0;i<sizeof offsets/sizeof *offsets;++i) {
        fixture(data); put32(data+offsets[i],0xffffffffU);
        int result=hub_public_pdb_init(&reader,memory.length,memory_read,&memory);
        if (!result) result=hub_public_pdb_step(&reader,verify,&count);
        assert(result<0); hub_public_pdb_destroy(&reader);
    }
    fixture(data); memcpy(data+1024,data+512,512); memory.length=1536;
    put32(data+40,2); put32(data+524,1);
    assert(!hub_public_pdb_init(&reader,memory.length,memory_read,&memory));
    assert(hub_public_pdb_step(&reader,verify,&count)==0);
    assert(hub_public_pdb_step(&reader,verify,&count)<0); hub_public_pdb_destroy(&reader);
    put32(data+524,2); put32(data+1028,2);
    assert(!hub_public_pdb_init(&reader,memory.length,memory_read,&memory));
    assert(hub_public_pdb_step(&reader,verify,&count)==0);
    assert(hub_public_pdb_step(&reader,verify,&count)<0); hub_public_pdb_destroy(&reader);
    fixture(data); memory.length=1024;
    char directory[]="/tmp/azhub-pdb.XXXXXX"; assert(mkdtemp(directory));
    char path[512],old[512]; snprintf(path,sizeof path,"%s/export.pdb",directory);
    snprintf(old,sizeof old,"%s/old.pdb",directory);
    int fd=open(path,O_CREAT|O_EXCL|O_WRONLY,0600); assert(fd>=0);
    assert(write(fd,data,1024)==1024 && !close(fd));
    struct hub_public_root root; assert(!hub_public_root_init(&root));
    assert(!hub_public_root_attach(&root,directory,1));
    struct hub_public_pdb_source source; assert(!hub_public_pdb_source_init(&source,&root,1,"export.pdb",10));
    assert(!hub_public_pdb_init(&reader,1024,hub_public_pdb_source_read,&source) && !root.active);
    hub_public_root_withdraw(&root); assert(hub_public_root_drained(&root));
    assert(hub_public_pdb_step(&reader,verify,&count)<0 && errno==ESTALE); hub_public_pdb_destroy(&reader);
    assert(!hub_public_root_attach(&root,directory,2));
    assert(!hub_public_pdb_source_init(&source,&root,2,"export.pdb",10));
    assert(!hub_public_pdb_init(&reader,1024,hub_public_pdb_source_read,&source));
    assert(!rename(path,old)); fd=open(path,O_CREAT|O_EXCL|O_WRONLY,0600); assert(fd>=0);
    assert(write(fd,data,1024)==1024 && !close(fd));
    assert(hub_public_pdb_step(&reader,verify,&count)<0 && errno==ESTALE && !root.active);
    hub_public_pdb_destroy(&reader); hub_public_root_withdraw(&root); assert(!hub_public_root_destroy(&root));
    assert(!unlink(path) && !unlink(old) && !rmdir(directory));
    puts("PASS: bounded incremental C PDB tracks, malformed bounds/cycle/duplicate, callback cancellation and source withdrawal/replacement");
}

/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <sys/vfs.h>
static int fixture_mac_filesystem;
static int fixture_fstatfs(int fd,struct statfs *info) {
    int result=fstatfs(fd,info);
    if (!result && fixture_mac_filesystem) info->f_type=0x482b;
    return result;
}
#define fstatfs(fd,info) fixture_fstatfs(fd,info)
#include "public-library-export.h"
#include "public-nfs-tree.h"
#undef fstatfs
static void put16(unsigned char *p,unsigned n) { p[0]=(unsigned char)n; p[1]=(unsigned char)(n>>8); }
static void put32(unsigned char *p,unsigned n) { put16(p,n); put16(p+2,n>>16); }
static void pdb_fixture(unsigned char data[1024],const char *path) {
    memset(data,0,1024); put32(data+4,512); put32(data+8,1); put32(data+36,1); put32(data+40,1);
    put32(data+516,1); data[536]=1; put32(data+552+72,17);
    const char *strings[]={"","Title",path,""}; const unsigned indexes[]={14,17,20,16}; size_t offset=136;
    for (unsigned i=0;i<4;++i) {
        size_t length=strlen(strings[i]); assert(length<126);
        put16(data+552+94+indexes[i]*2,(unsigned)offset);
        data[552+offset]=(unsigned char)((length+1)*2+1);
        memcpy(data+552+offset+1,strings[i],length); offset+=length+1;
    }
    put16(data+1020,1);
}
static void write_file(const char *path,const void *data,size_t length) {
    int fd=open(path,O_CREAT|O_TRUNC|O_WRONLY,0600); assert(fd>=0);
    assert(write(fd,data,length)==(ssize_t)length && !close(fd));
}
static void *worker(void *opaque) {
    int result;
    while ((result=hub_public_library_files_step(opaque))==0) {}
    return (void *)(intptr_t)result;
}
static int finish(struct hub_public_library_files *library) {
    int result; while ((result=hub_public_library_files_step(library))==0) {}
    return result;
}
static void hidden_library_fixture(void) {
    char directory[]="/tmp/azhub-mac-library.XXXXXX";assert(mkdtemp(directory));
    char pioneer[512],rekordbox[512],pdb[512],contents[512],track[512];
    snprintf(pioneer,sizeof pioneer,"%s/.PIONEER",directory);assert(!mkdir(pioneer,0700));
    snprintf(rekordbox,sizeof rekordbox,"%s/.PIONEER/rekordbox",directory);assert(!mkdir(rekordbox,0700));
    snprintf(pdb,sizeof pdb,"%s/.PIONEER/rekordbox/export.pdb",directory);
    snprintf(contents,sizeof contents,"%s/Contents",directory);assert(!mkdir(contents,0700));
    snprintf(track,sizeof track,"%s/Contents/track.mp3",directory);write_file(track,"abc",3);
    unsigned char data[1024];pdb_fixture(data,"/Contents/track.mp3");write_file(pdb,data,sizeof data);
    struct hub_public_root root;assert(!hub_public_root_init(&root));
    assert(!hub_public_root_attach(&root,directory,1));
    fixture_mac_filesystem=1;
    struct hub_public_library_files library;unsigned char nonce[16]={1},handle[32];
    assert(!hub_public_library_files_init(&library,&root,1,nonce));
    assert(!root.active && finish(&library)==1 && library.tracks==1 && !library.missing_audio);
    const char *relative=".PIONEER/rekordbox/export.pdb";
    assert(hub_public_catalog_find(&library.catalog,relative,strlen(relative),handle));
    const char *ordinary="PIONEER/rekordbox/export.pdb";
    assert(!hub_public_catalog_find(&library.catalog,ordinary,strlen(ordinary),handle));
    hub_public_library_files_destroy(&library);
    fixture_mac_filesystem=0;
    assert(!hub_public_root_attach(&root,directory,2));
    assert(hub_public_library_files_init(&library,&root,2,nonce)<0 && errno==ENOENT);
    assert(!root.active);
    hub_public_library_files_destroy(&library);assert(!hub_public_root_destroy(&root));
    assert(!unlink(pdb) && !unlink(track) && !rmdir(rekordbox) && !rmdir(pioneer) && !rmdir(contents) && !rmdir(directory));
}
static int scan(const char *directory) {
    struct hub_public_root root; assert(!hub_public_root_init(&root));
    assert(!hub_public_root_attach(&root,directory,1));
    struct hub_public_library_files library; unsigned char nonce[16]={1};
    assert(!hub_public_library_files_init(&library,&root,1,nonce));
    assert(!hub_public_library_ready_catalog(&library));
    int result; while ((result=hub_public_library_files_step(&library))==0) assert(!root.active);
    if (result==1) printf("{\"tracks\":%u,\"missing_audio\":%u,\"missing_analysis\":%u,\"entries\":%u,\"path_bytes\":%zu,\"entry_capacity\":%u,\"entity_rows\":%u,\"missing_names\":%u,\"missing_artwork\":%u}\n",
        library.tracks,library.missing_audio,library.missing_analysis,library.catalog.count,
        library.catalog.path_bytes,library.catalog.capacity,library.entities.count,library.missing_names,library.missing_artwork);
    else fprintf(stderr,"Library scan failed: %d\n",errno);
    hub_public_library_files_destroy(&library); assert(!hub_public_root_destroy(&root)); return result==1 ? 0 : 1;
}
int main(int argc,char **argv) {
    if (argc==3 && !strcmp(argv[1],"scan")) return scan(argv[2]);
    assert(argc==1);
    hidden_library_fixture();
    char directory[]="/tmp/azhub-library.XXXXXX"; assert(mkdtemp(directory));
    char pioneer[512],rekordbox[512],contents[512],pdb[512],track[512],secret[512];
    snprintf(pioneer,sizeof pioneer,"%s/PIONEER",directory); assert(!mkdir(pioneer,0700));
    snprintf(rekordbox,sizeof rekordbox,"%s/PIONEER/rekordbox",directory); assert(!mkdir(rekordbox,0700));
    snprintf(contents,sizeof contents,"%s/Contents",directory); assert(!mkdir(contents,0700));
    snprintf(pdb,sizeof pdb,"%s/PIONEER/rekordbox/export.pdb",directory);
    snprintf(track,sizeof track,"%s/Contents/track.mp3",directory);
    snprintf(secret,sizeof secret,"%s/secret",directory);
    write_file(track,"abc",3); write_file(secret,"hidden",6);
    unsigned char data[1024]; pdb_fixture(data,"/Contents/track.mp3"); write_file(pdb,data,sizeof data);
    struct hub_public_root root,neighbor; assert(!hub_public_root_init(&root) && !hub_public_root_init(&neighbor));
    assert(!hub_public_root_attach(&root,directory,1) && !hub_public_root_attach(&neighbor,directory,1));
    struct hub_public_library_files a,b; unsigned char nonce[16]={1},other[16]={2},handle[32];
    assert(!hub_public_library_files_init(&a,&root,1,nonce));
    assert(!hub_public_library_files_init(&b,&neighbor,1,other));
    assert(!hub_public_library_ready_catalog(&a) && !hub_public_library_ready_tracks(&a) && !root.active);
    {
        struct hub_public_library_export staging={&a,{&a.catalog,40000,2049}};
        unsigned char query[40]={0}; const uint32_t words[]={1,0,2,100005,1,5,0,0,0,0};
        for (unsigned i=0;i<10;++i) hub_public_rpc_store(query+i*4,words[i]);
        struct hub_public_rpc_reply response;
        hub_public_library_export_dispatch(&staging,query,sizeof query,&response);
        assert(!response.length && !root.active);
    }
    assert(finish(&a)==1 && hub_public_library_ready_catalog(&a));
    assert(hub_public_library_ready_entities(&a));
    assert(a.tracks==1 && !a.missing_audio && a.catalog.count==6 && !root.active);
    const struct hub_public_track_index *metadata=hub_public_library_ready_tracks(&a); assert(metadata);
    const struct hub_public_track_row *metadata_row=hub_public_track_index_find(metadata,1,17);
    assert(metadata_row && !strcmp(metadata->text+metadata_row->title,"Title"));
    assert(!hub_public_track_index_find(metadata,2,17));
    assert(hub_public_catalog_find(&a.catalog,"Contents/track.mp3",18,handle));
    struct hub_public_file_ref ref; assert(hub_public_catalog_resolve(&a.catalog,handle,&ref));
    struct hub_public_nfs_reply reply; hub_public_nfs_read(&root,&ref,0,3,&reply);
    assert(reply.length==80 && !memcmp(reply.bytes+76,"abc",3));
    assert(!hub_public_catalog_find(&a.catalog,"secret",6,handle));
    hub_public_library_files_cancel(&a); assert(!hub_public_library_ready_catalog(&a) && !hub_public_library_ready_tracks(&a));
    assert(!hub_public_library_ready_entities(&a));
    assert(!hub_public_track_index_find(metadata,1,17));
    assert(!strcmp(metadata->text+metadata_row->title,"Title")); /* In-flight immutable data survives retirement. */
    assert(hub_public_root_drained(&root));
    assert(finish(&b)==1 && hub_public_library_ready_catalog(&b));
    assert(hub_public_library_ready_tracks(&b) && hub_public_track_index_find(&b.metadata,1,17));
    hub_public_library_files_destroy(&a); hub_public_library_files_destroy(&b);
    assert(!hub_public_root_destroy(&neighbor));
    assert(!hub_public_root_attach(&root,directory,2));
    assert(!hub_public_library_files_init(&a,&root,2,nonce));
    pthread_t thread; assert(!pthread_create(&thread,NULL,worker,&a));
    hub_public_library_files_cancel(&a);
    assert(!pthread_join(thread,NULL) && !hub_public_library_ready_catalog(&a));
    assert(hub_public_library_files_step(&a)<0 && errno==ECANCELED && hub_public_root_drained(&root));
    hub_public_library_files_destroy(&a);
    pdb_fixture(data,"/../secret"); write_file(pdb,data,sizeof data);
    assert(!hub_public_root_attach(&root,directory,3));
    assert(!hub_public_library_files_init(&a,&root,3,nonce));
    assert(hub_public_library_files_step(&a)<0 && errno==EINVAL && !hub_public_library_ready_catalog(&a));
    assert(hub_public_root_drained(&root)); hub_public_library_files_destroy(&a);
    pdb_fixture(data,"/Contents/missing.mp3"); write_file(pdb,data,sizeof data);
    assert(!hub_public_root_attach(&root,directory,4));
    assert(!hub_public_library_files_init(&a,&root,4,nonce));
    assert(finish(&a)==1 && a.missing_audio==1 && a.catalog.count==4);
    assert(hub_public_library_ready_tracks(&a) && hub_public_track_index_find(&a.metadata,4,17));
    hub_public_library_files_destroy(&a);
    pdb_fixture(data,"/Contents/track.mp3"); write_file(pdb,data,sizeof data);
    assert(!hub_public_root_attach(&root,directory,5));
    assert(!hub_public_library_files_init(&a,&root,5,nonce));
    write_file(pdb,data,1000);
    assert(hub_public_library_files_step(&a)<0 && errno==ESTALE && !hub_public_library_ready_catalog(&a));
    hub_public_library_files_destroy(&a);
    write_file(pdb,data,sizeof data);
    assert(!hub_public_root_attach(&root,directory,6));
    assert(!hub_public_library_files_init(&a,&root,6,nonce));
    assert(finish(&a)==1);
    assert(hub_public_catalog_find(&a.catalog,"",0,handle));
    struct hub_public_library_export service={&a,{&a.catalog,40000,2049}};
    unsigned char call[72]={0};
    const uint32_t header[]={1,0,2,100003,2,1,0,0,0,0};
    for (unsigned i=0;i<10;++i) hub_public_rpc_store(call+i*4,header[i]);
    memcpy(call+40,handle,32); struct hub_public_rpc_reply rpc;
    hub_public_library_export_dispatch(&service,call,sizeof call,&rpc);
    assert(rpc.length==96 && !hub_public_rpc_word(rpc.bytes+24));
    write_file(pdb,data,1000);
    hub_public_library_export_dispatch(&service,call,4,&rpc);
    assert(!rpc.length && hub_public_library_ready_catalog(&a));
    hub_public_rpc_store(call+20,8);
    hub_public_library_export_dispatch(&service,call,sizeof call,&rpc);
    assert(rpc.length==24 && hub_public_rpc_word(rpc.bytes+20)==3 && hub_public_library_ready_catalog(&a));
    hub_public_rpc_store(call+20,1);
    hub_public_library_export_dispatch(&service,call,sizeof call,&rpc);
    assert(rpc.length==28 && hub_public_rpc_word(rpc.bytes+24)==70 && !hub_public_library_ready_catalog(&a));
    assert(!hub_public_library_ready_tracks(&a) && !hub_public_track_index_find(&a.metadata,6,17));
    assert(hub_public_root_drained(&root));
    hub_public_library_files_destroy(&a);
    /* Stage real playlist tables alongside the track and file catalog. */
    unsigned char complete[2048]={0}; pdb_fixture(complete,"/Contents/track.mp3");
    put32(complete+8,3);
    put32(complete+44,7); put32(complete+52,2); put32(complete+56,2);
    put32(complete+60,8); put32(complete+68,3); put32(complete+72,3);
    put32(complete+1028,2); put32(complete+1032,7); complete[1048]=1;
    put32(complete+1064+12,4); complete[1064+20]=11; memcpy(complete+1064+21,"List",4);
    put16(complete+1532,1);
    put32(complete+1540,3); put32(complete+1544,8); complete[1560]=1;
    put32(complete+1576,1); put32(complete+1580,17); put32(complete+1584,4);
    put16(complete+2044,1);
    write_file(pdb,complete,sizeof complete);
    assert(!hub_public_root_attach(&root,directory,7));
    assert(!hub_public_library_files_init(&a,&root,7,nonce));
    assert(hub_public_library_files_step(&a)==0 && a.tracks==1);
    assert(!hub_public_library_ready_catalog(&a) && !hub_public_library_ready_entities(&a));
    assert(finish(&a)==1);
    const struct hub_entity_index *entities=hub_public_library_ready_entities(&a); unsigned entity_count;
    assert(entities && entities->count==2);
    const struct hub_entity_row *members=hub_entity_index_find(entities,7,8,4,&entity_count);
    assert(members && entity_count==1 && members->track_id==17);
    hub_public_library_files_cancel(&a);
    assert(!hub_public_library_ready_entities(&a) && !hub_entity_index_find(entities,7,8,4,&entity_count));
    assert(members->track_id==17 && hub_public_root_drained(&root));
    hub_public_library_files_destroy(&a);
    put32(complete+1580,18); /* Missing referenced track prevents any publication. */
    write_file(pdb,complete,sizeof complete);
    assert(!hub_public_root_attach(&root,directory,8));
    assert(!hub_public_library_files_init(&a,&root,8,nonce));
    assert(finish(&a)<0 && errno==EINVAL && !hub_public_library_ready_catalog(&a));
    assert(!hub_public_library_ready_entities(&a) && !hub_public_library_ready_tracks(&a));
    hub_public_library_files_destroy(&a);
    unsigned char with_art[2560]={0}; memcpy(with_art,complete,sizeof complete);
    put32(with_art+1580,17); put32(with_art+8,4);
    put32(with_art+76,13); put32(with_art+84,4); put32(with_art+88,4);
    put32(with_art+2052,4); put32(with_art+2056,13); with_art[2072]=1;
    put32(with_art+2088,9);
    const char *art_relative="/Contents/art.jpg"; size_t art_length=strlen(art_relative);
    with_art[2092]=(unsigned char)((art_length+1)*2+1); memcpy(with_art+2093,art_relative,art_length);
    put16(with_art+2556,1);
    char art_path[512]; snprintf(art_path,sizeof art_path,"%s/Contents/art.jpg",directory);
    char art_high[512],derived[4096]; snprintf(art_high,sizeof art_high,"%s/Contents/art_m.jpg",directory);
    assert(!hub_public_artwork_high_path("/Contents/art.jpg",derived) && !strcmp(derived,"Contents/art_m.jpg"));
    assert(hub_public_artwork_high_path("/../secret.jpg",derived)<0);
    assert(hub_public_artwork_high_path("/Contents/art",derived)==1);
    write_file(art_high,"synthetic high artwork catalog bytes",36);
    write_file(art_path,"synthetic artwork bytes for catalog",34); write_file(pdb,with_art,sizeof with_art);
    assert(!hub_public_root_attach(&root,directory,9));
    assert(!hub_public_library_files_init(&a,&root,9,nonce));
    assert(!hub_public_library_ready_catalog(&a));
    assert(finish(&a)==1 && !a.missing_artwork && !a.missing_high_artwork && a.entities.count==3);
    assert(hub_public_catalog_find(&a.catalog,"Contents/art_m.jpg",18,handle));
    assert(hub_public_catalog_find(&a.catalog,"Contents/art.jpg",16,handle));
    assert(!hub_public_catalog_find(&a.catalog,"secret",6,handle));
    hub_public_library_files_destroy(&a); assert(!unlink(art_path) && !unlink(art_high));
    assert(!hub_public_root_attach(&root,directory,10));
    assert(!hub_public_library_files_init(&a,&root,10,nonce));
    assert(finish(&a)==1 && a.missing_artwork==1 && a.missing_high_artwork==1 && a.entities.count==3);
    assert(!hub_public_catalog_find(&a.catalog,"Contents/art.jpg",16,handle));
    hub_public_library_files_destroy(&a); assert(!hub_public_root_destroy(&root));
    assert(!unlink(pdb) && !unlink(track) && !unlink(secret));
    assert(!rmdir(rekordbox) && !rmdir(pioneer) && !rmdir(contents) && !rmdir(directory));
    puts("PASS: staged PDB RPC catalog, hidden unrelated file, concurrent cancel/neighbor, unsafe/missing paths and pre/post-ready PDB change retirement");
}

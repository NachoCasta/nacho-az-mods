/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include "public-db-menus.h"
static void request(struct hub_db_backend *backend,unsigned type,unsigned count,uint32_t a,uint32_t b,uint32_t c,uint32_t d) {
    struct hub_db_message message={.transaction=9,.type=type,.count=count}; const uint32_t values[]={a,b,c,d};
    for (unsigned i=0;i<count;++i) message.args[i]=(struct hub_db_field){.kind=HUB_DB_NUMBER,.number=values[i]};
    assert(!hub_db_backend_begin(backend,&message));
}
static struct hub_db_message response(struct hub_db_backend *backend,unsigned type) {
    struct hub_db_message reply; int result; unsigned steps=0;
    do { result=hub_db_backend_next(backend,&reply); assert(++steps<2048); } while (result==2);
    assert(result==1 && reply.type==type);
    unsigned char bytes[16384]; size_t length,used; struct hub_db_message decoded;
    assert(!hub_db_message_write(&reply,bytes,sizeof bytes,&length));
    assert(hub_db_message_read(bytes,length,&decoded,&used)==1 && used==length);
    return reply;
}
int main(void) {
    unsigned char wave_tag[28]={0},*wave=NULL; uint32_t wave_size=0;
    memcpy(wave_tag,"PWV3",4); hub_db_put32(wave_tag+4,24); hub_db_put32(wave_tag+8,28);
    hub_db_put32(wave_tag+12,1); hub_db_put32(wave_tag+16,4);
    wave_tag[24]=0; wave_tag[25]=31; wave_tag[26]=224; wave_tag[27]=255;
    assert(!hub_wave_detail_convert(wave_tag,sizeof wave_tag,&wave,&wave_size) && wave_size==23);
    assert(!memcmp(wave+19,wave_tag+24,4)); free(wave);
    hub_db_put32(wave_tag+12,2);
    assert(hub_wave_detail_convert(wave_tag,sizeof wave_tag,&wave,&wave_size)<0 && !wave && !wave_size);
    hub_db_put32(wave_tag+12,1); hub_db_put32(wave_tag+16,5);
    assert(hub_wave_detail_convert(wave_tag,sizeof wave_tag,&wave,&wave_size)<0 && errno==EINVAL);
    unsigned char grid_tag[40]={0},*grid=NULL; uint32_t grid_size=0;
    memcpy(grid_tag,"PQTZ",4); hub_db_put32(grid_tag+4,24); hub_db_put32(grid_tag+8,40);
    hub_db_put32(grid_tag+20,2);
    grid_tag[25]=4; grid_tag[26]=0x30; grid_tag[27]=0x39; hub_db_put32(grid_tag+28,123456);
    grid_tag[33]=1; grid_tag[34]=0x31; grid_tag[35]=0x40; hub_db_put32(grid_tag+36,123950);
    assert(!hub_grid_convert(grid_tag,sizeof grid_tag,&grid,&grid_size) && grid_size==52);
    assert(grid[20]==4 && grid[22]==0x39 && grid[23]==0x30 && grid[24]==0x40 && grid[25]==0xe2);
    assert(grid[36]==1 && grid[38]==0x40 && grid[39]==0x31 && grid[40]==0x2e && grid[41]==0xe4);
    free(grid);
    hub_db_put32(grid_tag+20,3);
    assert(hub_grid_convert(grid_tag,sizeof grid_tag,&grid,&grid_size)<0 && !grid && !grid_size);
    hub_db_put32(grid_tag+20,2); hub_db_put32(grid_tag+4,25);
    assert(hub_grid_convert(grid_tag,sizeof grid_tag,&grid,&grid_size)<0 && errno==EINVAL);
    char directory[]="/tmp/azhub-db-menu.XXXXXX"; assert(mkdtemp(directory));
    char path[512]; snprintf(path,sizeof path,"%s/PDB",directory);
    int fd=open(path,O_CREAT|O_EXCL|O_WRONLY,0600); assert(fd>=0 && !ftruncate(fd,28) && !close(fd));
    struct hub_public_root root; assert(!hub_public_root_init(&root) && !hub_public_root_attach(&root,directory,1));
    struct hub_public_library_files library={0};
    char audio[512]; snprintf(audio,sizeof audio,"%s/a.mp3",directory);
    fd=open(audio,O_CREAT|O_EXCL|O_WRONLY,0600); assert(fd>=0 && write(fd,"abc",3)==3 && !close(fd));
    const unsigned char nonce[16]={1}; unsigned char handle[32];
    assert(!hub_public_catalog_init(&library.catalog,&root,1,nonce)); library.initialized=1;
    assert(!hub_public_catalog_register(&library.catalog,"a.mp3",5,handle));
    char analysis_path[512]; snprintf(analysis_path,sizeof analysis_path,"%s/ANLZ.DAT",directory);
    unsigned char analysis_data[56]={0}; memcpy(analysis_data,"PMAI",4);
    hub_db_put32(analysis_data+4,28); hub_db_put32(analysis_data+8,sizeof analysis_data);
    memcpy(analysis_data+28,"PWV4",4); hub_db_put32(analysis_data+32,24); hub_db_put32(analysis_data+36,28);
    analysis_data[55]=42;
    fd=open(analysis_path,O_CREAT|O_EXCL|O_WRONLY,0600);
    assert(fd>=0 && write(fd,analysis_data,sizeof analysis_data)==sizeof analysis_data && !close(fd));
    assert(!hub_public_catalog_register(&library.catalog,"ANLZ.DAT",8,handle));
    assert(!hub_public_pdb_source_init(&library.source,&root,1,"PDB",3));
    assert(!hub_public_track_index_init(&library.metadata,1) && !hub_entity_index_init(&library.entities,1));
    struct hub_public_pdb_track track={.id=1,.artist_id=7,.file_size=3,.duration=123,.bpm100=12800,.title="Zulu",.path="a.mp3",.analysis_path="/ANLZ.DAT"};
    assert(!hub_public_track_index_add(&library.metadata,&track));
    track.id=2; strcpy(track.title,"alpha"); strcpy(track.path,"missing.MP3"); assert(!hub_public_track_index_add(&library.metadata,&track));
    struct hub_public_pdb_entity entity={.table=7,.id=1,.folder=1,.name="Folder"};
    assert(!hub_entity_index_add(&library.entities,&entity));
    entity.id=2; entity.parent=1; entity.folder=0; strcpy(entity.name,"Playlist"); assert(!hub_entity_index_add(&library.entities,&entity));
    entity=(struct hub_public_pdb_entity){.table=8,.id=2,.track_id=2,.sort_order=1}; assert(!hub_entity_index_add(&library.entities,&entity));
    entity.track_id=1; entity.sort_order=2; assert(!hub_entity_index_add(&library.entities,&entity));
    entity=(struct hub_public_pdb_entity){.table=2,.id=7,.name="Artist \xf0\x9f\x8e\xb5"}; assert(!hub_entity_index_add(&library.entities,&entity));
    entity=(struct hub_public_pdb_entity){.table=13,.id=9,.name="/ANLZ.DAT"}; assert(!hub_entity_index_add(&library.entities,&entity));
    assert(!hub_public_track_index_seal(&library.metadata) && !hub_entity_index_seal(&library.entities)); library.state=HUB_LIBRARY_READY;
    struct hub_db_library_view view; assert(!hub_db_library_view_init(&view,&library));
    struct hub_db_session connection={.requester=3};
    struct hub_db_backend a={.view=&view,.connection=&connection},b={.view=&view,.connection=&connection};
    const unsigned dmst=0x03010401;
    /* Synthetic opaque bytes exercise transport; the independent Java control
     * separately verifies JPEG decoding. */
    request(&a,0x2003,2,dmst,9,0,0); struct hub_db_message art_reply=response(&a,0x4002);
    assert(art_reply.args[3].length==56 && !memcmp(art_reply.args[3].data,analysis_data,56));
    request(&a,0x2003,2,dmst,99,0,0); assert(!response(&a,0x4002).args[3].length);
    request(&a,0x2003,3,dmst,9,1,0); assert(!response(&a,0x4002).args[3].length); /* Missing companion. */
    request(&a,0x2003,3,dmst,9,2,0); response(&a,0x4003); /* Unanalyzed art not supported. */
    fd=open(analysis_path,O_WRONLY); assert(fd>=0 && !ftruncate(fd,9000) && !close(fd));
    request(&a,0x2003,2,dmst,9,0,0);
    assert(hub_db_backend_next(&a,&art_reply)==2 && a.artwork.filled==8192 && !root.active);
    fd=open(analysis_path,O_WRONLY); assert(fd>=0 && !ftruncate(fd,8999) && !close(fd));
    assert(hub_db_backend_next(&a,&art_reply)<0 && errno==ESTALE && !root.active);
    hub_db_backend_release_analysis(&a); assert(!a.artwork.data);
    fd=open(analysis_path,O_WRONLY); assert(fd>=0 && !ftruncate(fd,56) && !close(fd));
    request(&a,0x1004,2,dmst,0,0,0); assert(response(&a,0x4000).args[1].number==2);
    request(&b,0x1105,4,dmst,0,0,1); assert(response(&b,0x4000).args[1].number==1);
    request(&a,0x3000,3,dmst,0,64,0); response(&a,0x4001);
    connection.extended_menu=1;
    struct hub_db_message extended=response(&a,0x4101);
    assert(extended.args[1].number==2 && extended.count==16 && extended.args[15].number==12800);
    assert(extended.args[13].number==2 && extended.args[14].kind==HUB_DB_STRING && extended.args[14].length==2);
    connection.extended_menu=0;
    extended=response(&a,0x4101); assert(extended.args[1].number==1 && extended.count==12); response(&a,0x4201);
    request(&b,0x3000,3,dmst,0,64,0); response(&b,0x4001);
    assert(response(&b,0x4101).args[6].number==1); response(&b,0x4201);
    request(&b,0x1105,4,dmst,0,2,0); assert(response(&b,0x4000).args[1].number==2);
    request(&b,0x3000,3,dmst,1,1,0); response(&b,0x4001);
    struct hub_db_message row=response(&b,0x4101); assert(row.args[1].number==1 && row.args[9].number==2 && row.args[0].number==2); response(&b,0x4201);
    request(&a,0x1000,1,0x04010401,0,0,0); response(&a,0x4003); /* Requester spoof. */
    request(&a,0x1000,1,0x03010301,0,0,0); response(&a,0x4003); /* Native/private slot. */
    request(&a,0x3000,3,dmst,0,65,0); response(&a,0x4003);
    request(&a,0x2002,2,dmst,1,0,0); assert(response(&a,0x4000).args[1].number==11);
    request(&a,0x3000,3,dmst,0,64,0); response(&a,0x4001);
    for (unsigned i=0;i<11;++i) {
        row=response(&a,0x4101);
        if (i==1) assert(row.args[1].number==7 && row.args[3].length==20);
        if (i==6) assert(row.args[1].number==123);
    }
    response(&a,0x4201);
    request(&a,0x2102,2,dmst,2,0,0); response(&a,0x4003); /* Unexported media cannot be loaded. */
    request(&a,0x2102,2,dmst,1,0,0); assert(response(&a,0x4000).args[1].number==7);
    request(&a,0x3000,3,dmst,0,64,0); response(&a,0x4001);
    for (unsigned i=0;i<7;++i) {
        row=response(&a,0x4101);
        if (i==0) assert(row.args[1].number==1 && row.args[6].number==4);
        if (i==1) assert(row.args[1].number==123);
        if (i==4) assert(row.args[0].number==3 && row.args[1].number==1 && row.args[3].length==12);
    }
    response(&a,0x4201);
    request(&a,0x2c04,4,dmst,1,0x34565750,0x00544144);
    row=response(&a,0x4f02);
    assert(row.args[2].number==32 && row.args[3].length==32 && row.args[3].data[0]==28);
    assert(!memcmp(row.args[3].data+4,analysis_data+28,28) && !root.active);
    request(&a,0x2c04,4,dmst,1,0x5a5a5a5a,0x00544144);
    row=response(&a,0x4f02); assert(!row.args[2].number && !row.args[3].length);
    request(&a,0x2c04,4,dmst,1,0x34565750,0x12345678); response(&a,0x4003);
    request(&a,0x2b04,3,dmst,1,0,0); response(&a,0x4003); /* Missing EXT allows legacy fallback. */
    unsigned char preview_file[568]={0}; memcpy(preview_file,"PMAI",4);
    hub_db_put32(preview_file+4,28); hub_db_put32(preview_file+8,sizeof preview_file);
    memcpy(preview_file+28,"PWAV",4); hub_db_put32(preview_file+32,20);
    hub_db_put32(preview_file+36,420); hub_db_put32(preview_file+40,400);
    memset(preview_file+48,255,400);
    memcpy(preview_file+448,"PWV2",4); hub_db_put32(preview_file+452,20);
    hub_db_put32(preview_file+456,120); hub_db_put32(preview_file+460,100);
    memset(preview_file+468,0x12,100);
    fd=open(analysis_path,O_WRONLY|O_TRUNC); assert(fd>=0);
    assert(write(fd,preview_file,sizeof preview_file)==sizeof preview_file && !close(fd));
    request(&a,0x2004,4,dmst,1,1,0); row=response(&a,0x4402);
    assert(row.count==4 && row.args[3].length==900 && row.args[3].data[0]==31 && row.args[3].data[1]==7);
    assert(row.args[3].data[800]==0x12 && row.args[3].data[899]==0x12);
    request(&a,0x2004,4,dmst,1,1,0);
    for (unsigned steps=0;!a.analysis_part;++steps) { assert(steps<10 && hub_db_backend_next(&a,&row)==2); }
    fd=open(analysis_path,O_WRONLY); assert(fd>=0 && !ftruncate(fd,567) && !close(fd));
    assert(hub_db_backend_next(&a,&row)<0 && errno==ESTALE && !root.active);
    /* A missing tiny section must not publish a partial primary preview. */
    hub_db_put32(preview_file+8,448);
    fd=open(analysis_path,O_WRONLY|O_TRUNC); assert(fd>=0);
    assert(write(fd,preview_file,448)==448 && !close(fd));
    request(&a,0x2004,4,dmst,1,1,0); row=response(&a,0x4402);
    assert(!row.args[3].length && !row.args[2].number);
    fd=open(analysis_path,O_WRONLY|O_TRUNC); assert(fd>=0);
    assert(write(fd,analysis_data,sizeof analysis_data)==sizeof analysis_data && !close(fd));
    unsigned char cue_file[188]={0}; memcpy(cue_file,"PMAI",4);
    hub_db_put32(cue_file+4,28); hub_db_put32(cue_file+8,sizeof cue_file);
    for (unsigned i=0;i<2;++i) {
        unsigned char *tag=cue_file+28+i*80; memcpy(tag,"PCOB",4);
        hub_db_put32(tag+4,24); hub_db_put32(tag+8,80); hub_db_put32(tag+12,i); tag[19]=1;
        unsigned char *entry=tag+24; memcpy(entry,"PCPT",4); hub_db_put32(entry+4,28); hub_db_put32(entry+8,56);
        hub_db_put32(entry+12,i); entry[28]=(unsigned char)(i+1); hub_db_put32(entry+32,1000); hub_db_put32(entry+36,2000);
    }
    fd=open(analysis_path,O_WRONLY|O_TRUNC); assert(fd>=0);
    assert(write(fd,cue_file,sizeof cue_file)==sizeof cue_file && !close(fd));
    request(&a,0x2104,2,dmst,1,0,0); row=response(&a,0x4702);
    assert(row.count==9 && row.args[3].length==72 && row.args[5].number==1 && row.args[6].number==1);
    assert(row.args[3].data[12]==150 && row.args[3].data[36]==1 && row.args[3].data[52]==44);
    request(&a,0x2104,2,dmst,1,0,0);
    for (unsigned steps=0;!a.analysis_part;++steps) { assert(steps<10 && hub_db_backend_next(&a,&row)==2); }
    fd=open(analysis_path,O_WRONLY); assert(fd>=0 && !ftruncate(fd,187) && !close(fd));
    assert(hub_db_backend_next(&a,&row)<0 && errno==ESTALE && a.cues.size==36 && !root.active);
    hub_db_backend_release_analysis(&a); assert(!a.cues.data);
    fd=open(analysis_path,O_WRONLY|O_TRUNC); assert(fd>=0);
    assert(write(fd,analysis_data,sizeof analysis_data)==sizeof analysis_data && !close(fd));
    struct hub_library_analysis changing;
    assert(!hub_library_analysis_init(&changing,&library,1,0,0x50575634,0));
    assert(!hub_library_analysis_step(&changing));
    fd=open(analysis_path,O_WRONLY); assert(fd>=0 && !ftruncate(fd,55) && !close(fd));
    assert(hub_library_analysis_step(&changing)<0 && errno==ESTALE && !root.active);
    hub_library_analysis_destroy(&changing);
    fd=open(analysis_path,O_WRONLY); assert(fd>=0 && write(fd,analysis_data,sizeof analysis_data)==sizeof analysis_data && !close(fd));
    char moved[512]; snprintf(moved,sizeof moved,"%s/ANLZ.old",directory);
    assert(!rename(analysis_path,moved));
    fd=open(analysis_path,O_CREAT|O_EXCL|O_WRONLY,0600);
    assert(fd>=0 && write(fd,analysis_data,sizeof analysis_data)==sizeof analysis_data && !close(fd));
    assert(hub_library_analysis_init(&changing,&library,1,0,0x50575634,0)<0 && errno==ESTALE);
    hub_library_analysis_destroy(&changing); assert(!unlink(analysis_path) && !rename(moved,analysis_path));
    request(&a,0x2c04,4,dmst,1,0x34565750,0x00544144);
    assert(hub_db_backend_next(&a,&row)==2);
    hub_public_track_index_retire(&library.metadata);
    assert(hub_db_backend_next(&a,&row)<0);
    hub_db_backend_destroy(&a); hub_db_backend_destroy(&b);
    hub_db_library_view_destroy(&view); hub_public_library_files_destroy(&library);
    hub_public_root_withdraw(&root); assert(!hub_public_root_destroy(&root));
    assert(!unlink(path) && !unlink(audio) && !unlink(analysis_path) && !rmdir(directory));
    puts("PASS: source-backed track/playlist/metadata menus, ordering, per-session isolation, pagination, DMST rejection and retirement");
}

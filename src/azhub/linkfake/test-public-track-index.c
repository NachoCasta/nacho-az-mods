/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include "public-track-index.h"
#include "public-pdb-source.h"
static void *reader(void *opaque) {
    struct hub_public_track_index *index=opaque;
    for (unsigned i=0;i<10000;++i) {
        const struct hub_public_track_row *row=hub_public_track_index_find(index,1,17);
        if (row) assert(row->id==17 && !strcmp(index->text+row->title,"Track17"));
    }
    return NULL;
}
static void hex(const char *text) { for (const unsigned char *p=(const unsigned char *)text;*p;++p) printf("%02x",*p); }
static int scan(const char *directory,const char *relative) {
    struct hub_public_root root; assert(!hub_public_root_init(&root));
    assert(!hub_public_root_attach(&root,directory,1));
    struct hub_public_pdb_source source; assert(!hub_public_pdb_source_init(&source,&root,1,relative,strlen(relative)));
    struct hub_public_pdb_reader parser; assert(!hub_public_pdb_init(&parser,(uint64_t)source.expected.st_size,hub_public_pdb_source_read,&source));
    struct hub_public_track_index index; assert(!hub_public_track_index_init(&index,1));
    int result; while ((result=hub_public_pdb_step(&parser,hub_public_track_index_add,&index))==0) assert(!root.active);
    assert(result==1 && !hub_public_track_index_seal(&index));
    fprintf(stderr,"INDEX tracks=%u row_capacity=%u row_bytes=%zu text_bytes=%zu text_capacity=%zu\n",
        index.count,index.capacity,sizeof(struct hub_public_track_row),index.text_bytes,index.text_capacity);
    for (unsigned i=0;i<index.count;++i) {
        const struct hub_public_track_row *row=hub_public_track_index_find(&index,1,index.rows[i].id); assert(row);
        printf("%u\t",row->id); hex(index.text+row->path); putchar('\t'); hex(index.text+row->analysis_path);
        putchar('\t'); hex(index.text+row->title); putchar('\t'); hex(index.text+row->comment);
        printf("\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\n",row->file_size,row->artwork_id,
            row->key_id,row->label_id,row->bitrate,row->bpm100,row->genre_id,row->album_id,row->artist_id,row->duration,row->rating);
    }
    hub_public_track_index_retire(&index); hub_public_track_index_destroy(&index); hub_public_pdb_destroy(&parser);
    hub_public_root_withdraw(&root); assert(!hub_public_root_destroy(&root)); return 0;
}
int main(int argc,char **argv) {
    if (argc==4 && !strcmp(argv[1],"scan")) return scan(argv[2],argv[3]);
    assert(argc==1);
    struct hub_public_track_index a,b; assert(!hub_public_track_index_init(&a,1));
    assert(!hub_public_track_index_init(&b,1));
    struct hub_public_pdb_track track={0}; strcpy(track.path,"/Contents/track.mp3"); strcpy(track.comment,"Comment");
    for (unsigned i=2048;i;--i) {
        track.id=i; track.bpm100=12000+i; track.duration=(uint16_t)i;
        snprintf(track.title,sizeof track.title,"Track%u",i);
        assert(!hub_public_track_index_add(&a,&track));
        assert(!hub_public_track_index_find(&a,1,i));
    }
    track.id=17; strcpy(track.title,"Neighbor"); assert(!hub_public_track_index_add(&b,&track));
    assert(!hub_public_track_index_seal(&a) && !hub_public_track_index_seal(&b));
    for (unsigned i=1;i<=2048;++i) {
        const struct hub_public_track_row *row=hub_public_track_index_find(&a,1,i); char expected[32];
        snprintf(expected,sizeof expected,"Track%u",i);
        assert(row && row->bpm100==12000+i && row->duration==i && !strcmp(a.text+row->title,expected));
        assert(!strcmp(a.text+row->path,"/Contents/track.mp3") && !strcmp(a.text+row->comment,"Comment"));
    }
    assert(!hub_public_track_index_find(&a,2,17) && !hub_public_track_index_find(&a,1,2049));
    const struct hub_public_track_row *retained=hub_public_track_index_find(&a,1,17);
    pthread_t thread; assert(!pthread_create(&thread,NULL,reader,&a));
    hub_public_track_index_retire(&a); assert(!pthread_join(thread,NULL));
    assert(!hub_public_track_index_find(&a,1,17) && !strcmp(a.text+retained->title,"Track17"));
    assert(!strcmp(b.text+hub_public_track_index_find(&b,1,17)->title,"Neighbor"));
    hub_public_track_index_destroy(&a); hub_public_track_index_destroy(&b);
    assert(!hub_public_track_index_init(&a,3));
    assert(!hub_public_track_index_add(&a,&track) && !hub_public_track_index_add(&a,&track));
    assert(hub_public_track_index_seal(&a)<0 && !hub_public_track_index_find(&a,3,17));
    hub_public_track_index_destroy(&a);
    assert(!hub_public_track_index_init(&a,4)); hub_public_track_index_retire(&a);
    assert(hub_public_track_index_seal(&a)<0 && !hub_public_track_index_find(&a,4,17));
    hub_public_track_index_destroy(&a);
    puts("PASS: compact2048-track index, growth/sort/field preservation, staged/generation/neighbor isolation, duplicate refusal and concurrent retirement");
}

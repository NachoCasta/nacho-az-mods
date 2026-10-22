/* SPDX-License-Identifier: GPL-3.0-only */
/* Immutable per-source track index after sealing. One builder owns mutations;
 * callers must join all readers before destroy. Never stores native pointers. */
#ifndef HUB_PUBLIC_TRACK_INDEX_H
#define HUB_PUBLIC_TRACK_INDEX_H
#include "public-pdb-reader.h"
#define HUB_PUBLIC_TRACK_TEXT_MAX (16U*1024U*1024U)
enum hub_public_track_index_state { HUB_TRACK_BUILDING,HUB_TRACK_READY,HUB_TRACK_RETIRED,HUB_TRACK_FAILED };
struct hub_public_track_row {
    uint32_t id,file_size,artwork_id,key_id,label_id,bitrate,bpm100,genre_id,album_id,artist_id;
    uint32_t path,analysis_path,title,comment;
    uint16_t duration;
    uint8_t rating;
};
struct hub_public_track_index {
    uint64_t generation;
    unsigned count,capacity;
    size_t text_bytes,text_capacity;
    struct hub_public_track_row *rows;
    char *text;
    int state,error;
};
static int hub_public_track_index_init(struct hub_public_track_index *index,uint64_t generation) {
    memset(index,0,sizeof *index);
    if (!generation) { index->state=HUB_TRACK_FAILED; index->error=EINVAL; errno=EINVAL; return -1; }
    index->generation=generation; return 0;
}
static int hub_public_track_index_fail(struct hub_public_track_index *index,int error) {
    index->error=error; __atomic_store_n(&index->state,HUB_TRACK_FAILED,__ATOMIC_RELEASE);
    errno=error; return -1;
}
static int hub_public_track_index_add(void *opaque,const struct hub_public_pdb_track *track) {
    struct hub_public_track_index *index=opaque;
    if (__atomic_load_n(&index->state,__ATOMIC_ACQUIRE)!=HUB_TRACK_BUILDING) { errno=EINVAL; return -1; }
    if (!track || !track->id) return hub_public_track_index_fail(index,EINVAL);
    if (index->count==HUB_PDB_TRACKS_MAX) return hub_public_track_index_fail(index,ENOSPC);
    const char *strings[]={track->path,track->analysis_path,track->title,track->comment};
    size_t lengths[4],bytes=0;
    for (unsigned i=0;i<4;++i) {
        lengths[i]=strnlen(strings[i],4096);
        if (lengths[i]==4096) return hub_public_track_index_fail(index,EINVAL);
        bytes+=lengths[i]+1;
    }
    if (bytes>HUB_PUBLIC_TRACK_TEXT_MAX-index->text_bytes) return hub_public_track_index_fail(index,ENOSPC);
    if (index->count==index->capacity) {
        unsigned capacity=index->capacity ? index->capacity*2 : 32;
        if (capacity>HUB_PDB_TRACKS_MAX) capacity=HUB_PDB_TRACKS_MAX;
        struct hub_public_track_row *rows=realloc(index->rows,capacity*sizeof *rows);
        if (!rows) return hub_public_track_index_fail(index,ENOMEM);
        index->rows=rows; index->capacity=capacity;
    }
    size_t wanted=index->text_bytes+bytes;
    if (wanted>index->text_capacity) {
        size_t capacity=index->text_capacity ? index->text_capacity : 4096;
        while (capacity<wanted) capacity*=2;
        char *text=realloc(index->text,capacity);
        if (!text) return hub_public_track_index_fail(index,ENOMEM);
        index->text=text; index->text_capacity=capacity;
    }
    uint32_t offsets[4];
    for (unsigned i=0;i<4;++i) {
        offsets[i]=(uint32_t)index->text_bytes;
        memcpy(index->text+index->text_bytes,strings[i],lengths[i]+1);
        index->text_bytes+=lengths[i]+1;
    }
    struct hub_public_track_row row={track->id,track->file_size,track->artwork_id,track->key_id,
        track->label_id,track->bitrate,track->bpm100,track->genre_id,track->album_id,track->artist_id,
        offsets[0],offsets[1],offsets[2],offsets[3],track->duration,track->rating};
    index->rows[index->count++]=row; return 0;
}
static int hub_public_track_compare(const void *a,const void *b) {
    uint32_t x=((const struct hub_public_track_row *)a)->id,y=((const struct hub_public_track_row *)b)->id;
    return (x>y)-(x<y);
}
static int hub_public_track_index_seal(struct hub_public_track_index *index) {
    if (__atomic_load_n(&index->state,__ATOMIC_ACQUIRE)!=HUB_TRACK_BUILDING) { errno=EINVAL; return -1; }
    if (index->count) qsort(index->rows,index->count,sizeof *index->rows,hub_public_track_compare);
    for (unsigned i=1;i<index->count;++i)
        if (index->rows[i-1].id==index->rows[i].id) return hub_public_track_index_fail(index,EINVAL);
    int expected=HUB_TRACK_BUILDING;
    if (!__atomic_compare_exchange_n(&index->state,&expected,HUB_TRACK_READY,0,__ATOMIC_RELEASE,__ATOMIC_ACQUIRE)) {
        errno=ECANCELED; return -1;
    }
    return 0;
}
static const struct hub_public_track_row *hub_public_track_index_find(
        const struct hub_public_track_index *index,uint64_t generation,uint32_t id) {
    if (generation!=index->generation || !id ||
        __atomic_load_n(&index->state,__ATOMIC_ACQUIRE)!=HUB_TRACK_READY) return NULL;
    unsigned begin=0,end=index->count;
    while (begin<end) {
        unsigned middle=begin+(end-begin)/2; uint32_t found=index->rows[middle].id;
        if (found==id) return index->rows+middle;
        if (found<id) begin=middle+1; else end=middle;
    }
    return NULL;
}
/* Retirement blocks new lookups. In-flight callers may finish reading immutable
 * memory; this never holds a filesystem lease or delays USB unmount. */
static void hub_public_track_index_retire(struct hub_public_track_index *index) {
    __atomic_store_n(&index->state,HUB_TRACK_RETIRED,__ATOMIC_RELEASE);
}
static void hub_public_track_index_destroy(struct hub_public_track_index *index) {
    free(index->rows); free(index->text); memset(index,0,sizeof *index);
}
#endif

/* SPDX-License-Identifier: GPL-3.0-only */
/* One source-owned immutable entity index. Build/seal on one worker; retirement
 * may run concurrently. Join readers/builders before freeing storage. */
#ifndef HUB_PUBLIC_ENTITY_INDEX_H
#define HUB_PUBLIC_ENTITY_INDEX_H
#include "public-pdb-tables.h"
#define HUB_ENTITY_ROWS_MAX 262144U
#define HUB_ENTITY_TEXT_MAX (8U*1024U*1024U)
enum hub_entity_state { HUB_ENTITY_BUILDING,HUB_ENTITY_READY,HUB_ENTITY_RETIRED,HUB_ENTITY_FAILED };
struct hub_entity_row { uint32_t table,id,parent,sort_order,track_id,name,folder; };
struct hub_entity_index {
    uint64_t generation;
    struct hub_entity_row *rows;
    char *text;
    unsigned count,capacity;
    size_t text_bytes,text_capacity;
    int state,error;
};
static int hub_entity_index_fail(struct hub_entity_index *index,int error) {
    index->error=error; __atomic_store_n(&index->state,HUB_ENTITY_FAILED,__ATOMIC_RELEASE);
    errno=error; return -1;
}
static int hub_entity_index_init(struct hub_entity_index *index,uint64_t generation) {
    memset(index,0,sizeof *index); index->generation=generation;
    return generation ? 0 : hub_entity_index_fail(index,EINVAL);
}
static int hub_entity_index_add(void *opaque,const struct hub_public_pdb_entity *entity) {
    struct hub_entity_index *index=opaque;
    if (__atomic_load_n(&index->state,__ATOMIC_ACQUIRE)!=HUB_ENTITY_BUILDING) { errno=EINVAL; return -1; }
    if (!entity || !entity->id) return hub_entity_index_fail(index,EINVAL);
    switch (entity->table) {
    case 1: case 2: case 3: case 4: case 5: case 7: case 13: break;
    case 8: if (entity->track_id) break; /* A membership must reference a track. */
        return hub_entity_index_fail(index,EINVAL);
    default: return hub_entity_index_fail(index,EINVAL);
    }
    size_t length=strnlen(entity->name,sizeof entity->name);
    if (length==sizeof entity->name) return hub_entity_index_fail(index,EINVAL);
    /* Memberships have no name; all share the initial empty string. */
    size_t bytes=entity->table==8 && index->text_bytes ? 0 : length+1;
    if (index->count==HUB_ENTITY_ROWS_MAX || bytes>HUB_ENTITY_TEXT_MAX-index->text_bytes)
        return hub_entity_index_fail(index,ENOSPC);
    if (index->count==index->capacity) {
        unsigned capacity=index->capacity ? index->capacity*2 : 32;
        if (capacity>HUB_ENTITY_ROWS_MAX) capacity=HUB_ENTITY_ROWS_MAX;
        void *rows=realloc(index->rows,capacity*sizeof *index->rows);
        if (!rows) return hub_entity_index_fail(index,ENOMEM);
        index->rows=rows; index->capacity=capacity;
    }
    /* Reserve offset0 as an empty string independently of insertion order. */
    size_t wanted=index->text_bytes+bytes+(!index->text_bytes);
    if (wanted>HUB_ENTITY_TEXT_MAX) return hub_entity_index_fail(index,ENOSPC);
    if (wanted>index->text_capacity) {
        size_t capacity=index->text_capacity ? index->text_capacity : 4096;
        while (capacity<wanted) capacity*=2;
        char *text=realloc(index->text,capacity);
        if (!text) return hub_entity_index_fail(index,ENOMEM);
        index->text=text; index->text_capacity=capacity;
    }
    if (!index->text_bytes) index->text[index->text_bytes++]=0;
    uint32_t name=0;
    if (entity->table!=8) {
        name=(uint32_t)index->text_bytes; memcpy(index->text+name,entity->name,length+1);
        index->text_bytes+=length+1;
    }
    struct hub_entity_row row={entity->table,entity->id,entity->parent,entity->sort_order,
        entity->track_id,name,entity->folder!=0};
    index->rows[index->count++]=row; return 0;
}
static int hub_entity_compare(const void *left,const void *right) {
    const struct hub_entity_row *a=left,*b=right;
    if (a->table!=b->table) return (a->table>b->table)-(a->table<b->table);
    if (a->id!=b->id) return (a->id>b->id)-(a->id<b->id);
    if (a->table==8) return (a->sort_order>b->sort_order)-(a->sort_order<b->sort_order);
    return 0;
}
static int hub_entity_index_seal(struct hub_entity_index *index) {
    if (__atomic_load_n(&index->state,__ATOMIC_ACQUIRE)!=HUB_ENTITY_BUILDING) { errno=EINVAL; return -1; }
    if (index->count) qsort(index->rows,index->count,sizeof *index->rows,hub_entity_compare);
    for (unsigned i=1;i<index->count;++i)
        if (!hub_entity_compare(index->rows+i-1,index->rows+i)) return hub_entity_index_fail(index,EINVAL);
    int expected=HUB_ENTITY_BUILDING;
    if (!__atomic_compare_exchange_n(&index->state,&expected,HUB_ENTITY_READY,0,__ATOMIC_RELEASE,__ATOMIC_ACQUIRE)) {
        errno=ECANCELED; return -1;
    }
    return 0;
}
/* Returns a contiguous range: one named entity, or an ordered playlist's
 * memberships. Returned storage stays valid until all readers join/destroy. */
static const struct hub_entity_row *hub_entity_index_find(const struct hub_entity_index *index,
        uint64_t generation,uint32_t table,uint32_t id,unsigned *count) {
    *count=0;
    if (generation!=index->generation || __atomic_load_n(&index->state,__ATOMIC_ACQUIRE)!=HUB_ENTITY_READY) return NULL;
    unsigned begin=0,end=index->count;
    while (begin<end) {
        unsigned middle=begin+(end-begin)/2; const struct hub_entity_row *row=index->rows+middle;
        if (row->table<table || (row->table==table && row->id<id)) begin=middle+1;
        else end=middle;
    }
    unsigned first=begin;
    while (begin<index->count && index->rows[begin].table==table && index->rows[begin].id==id) ++begin;
    *count=begin-first; return *count ? index->rows+first : NULL;
}
static void hub_entity_index_retire(struct hub_entity_index *index) {
    __atomic_store_n(&index->state,HUB_ENTITY_RETIRED,__ATOMIC_RELEASE);
}
static void hub_entity_index_destroy(struct hub_entity_index *index) {
    free(index->rows); free(index->text); memset(index,0,sizeof *index);
}
#endif

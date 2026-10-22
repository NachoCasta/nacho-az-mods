/* SPDX-License-Identifier: GPL-3.0-only */
/* Bounded classic PDB named entities and playlist membership, using the same
 * source-owned page I/O as tracks. Consumers validate cross-table references. */
#ifndef HUB_PUBLIC_PDB_TABLES_H
#define HUB_PUBLIC_PDB_TABLES_H
#include "public-pdb-reader.h"
#define HUB_PDB_TABLE_ROWS_MAX 1000000U
struct hub_public_pdb_entity {
    uint32_t table,id,parent,sort_order,track_id;
    int folder;
    char name[4096];
};
typedef int (*hub_public_pdb_entity_fn)(void *,const struct hub_public_pdb_entity *);
static int hub_public_pdb_entity_decode(const unsigned char *page,size_t row,size_t end,
                                       uint32_t table,struct hub_public_pdb_entity *entity) {
    memset(entity,0,sizeof *entity); entity->table=table;
    if (row>end) return 0;
    size_t minimum,offset;
    switch (table) {
    case 1: case 4: case 13:
        minimum=4;
        if (end-row<minimum) return 0;
        entity->id=hub_pdb_u32(page+row); offset=row+4; break;
    case 2:
        minimum=10;
        if (end-row<minimum) return 0;
        entity->id=hub_pdb_u32(page+row+4);
        if (hub_pdb_u16(page+row)==0x64) {
            minimum=12; if (end-row<minimum) return 0;
            offset=row+hub_pdb_u16(page+row+10);
        } else if (hub_pdb_u16(page+row)==0x60) offset=row+page[row+9];
        else return 0;
        break;
    case 3:
        minimum=22;
        if (end-row<minimum) return 0;
        entity->id=hub_pdb_u32(page+row+12); entity->parent=hub_pdb_u32(page+row+8);
        offset=row+page[row+21]; break;
    case 5:
        minimum=8;
        if (end-row<minimum) return 0;
        entity->id=hub_pdb_u32(page+row); offset=row+8; break;
    case 7:
        minimum=20;
        if (end-row<minimum) return 0;
        entity->parent=hub_pdb_u32(page+row); entity->sort_order=hub_pdb_u32(page+row+8);
        entity->id=hub_pdb_u32(page+row+12); entity->folder=hub_pdb_u32(page+row+16)!=0;
        offset=row+20; break;
    case 8:
        if (end-row<12) return 0;
        entity->sort_order=hub_pdb_u32(page+row); entity->track_id=hub_pdb_u32(page+row+4);
        entity->id=hub_pdb_u32(page+row+8); return 1;
    default: return 0;
    }
    return offset>=row+minimum && hub_public_pdb_string(page,offset,end,entity->name);
}
static int hub_public_pdb_entities_step(struct hub_public_pdb_reader *reader,
                                       hub_public_pdb_entity_fn emit,void *context) {
    if (reader->failed) { errno=reader->failed; return -1; }
    if (reader->done) return 1;
    uint32_t current=reader->current,size=reader->page_size;
    if (!emit || !reader->table_type || !current || current>=reader->pages ||
        (reader->visited[current/8]&(1U<<(current%8)))) return hub_public_pdb_fail(reader,EINVAL);
    reader->visited[current/8]|=(unsigned char)(1U<<(current%8));
    if (reader->read(reader->context,(uint64_t)current*size,reader->page,size)<0)
        return hub_public_pdb_fail(reader,errno);
    const unsigned char *page=reader->page;
    if (hub_pdb_u32(page+4)!=current || hub_pdb_u32(page+8)!=reader->table_type)
        return hub_public_pdb_fail(reader,EINVAL);
    if (!(page[27]&0x40)) {
        /* The allocated-offset count is the low thirteen bits at byte24.
         * Byte34 is a transaction index, not a larger row count. */
        unsigned rows=hub_pdb_u16(page+24)&0x1fff,groups=(rows+15)/16;
        /* The final group stores only its actual row offsets. Reserving a
         * full sixteen-offset group would reject valid packed row data. */
        size_t index_bytes=rows ? (groups-1)*36+4+2*((rows-1)%16+1) : 0;
        if (index_bytes>size-40) return hub_public_pdb_fail(reader,EINVAL);
        size_t end=size-index_bytes;
        for (unsigned group=0;group<groups;++group) {
            size_t base=size-group*36; unsigned flags=hub_pdb_u16(page+base-4);
            for (unsigned i=0;i<16;++i) if (flags&(1U<<i)) {
                size_t row=40+hub_pdb_u16(page+base-6-i*2);
                struct hub_public_pdb_entity entity;
                if (group*16+i>=rows || row>=end || reader->count>=HUB_PDB_TABLE_ROWS_MAX ||
                    !hub_public_pdb_entity_decode(page,row,end,reader->table_type,&entity))
                    return hub_public_pdb_fail(reader,EINVAL);
                ++reader->count;
                if (emit(context,&entity)<0) return hub_public_pdb_fail(reader,errno ? errno : ECANCELED);
            }
        }
    }
    if (current==reader->last) { reader->done=1; return 1; }
    reader->current=hub_pdb_u32(page+12); return 0;
}
#endif

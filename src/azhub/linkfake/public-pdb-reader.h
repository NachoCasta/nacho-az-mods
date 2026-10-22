/* SPDX-License-Identifier: GPL-3.0-only */
/* Independent classic export.pdb reader. Format reference: Deep Symmetry
 * crate-digger rekordbox_pdb.ksy, including the corrected13-bit page count.
 * No generated parser is included.
 * One step processes at most one page; caller owns cancellation/publication. */
#ifndef HUB_PUBLIC_PDB_READER_H
#define HUB_PUBLIC_PDB_READER_H
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define HUB_PDB_SIZE_MAX (256U*1024U*1024U)
#define HUB_PDB_TRACKS_MAX 100000U
#define HUB_PDB_ID_BUCKETS 262144U
struct hub_public_pdb_track {
    uint32_t id,file_size,artwork_id,key_id,label_id,bitrate,bpm100,genre_id,album_id,artist_id;
    uint16_t duration;
    uint8_t rating;
    char path[4096],analysis_path[4096],title[4096],comment[4096];
};
typedef int (*hub_public_pdb_read_fn)(void *,uint64_t,void *,size_t);
typedef int (*hub_public_pdb_track_fn)(void *,const struct hub_public_pdb_track *);
struct hub_public_pdb_reader {
    hub_public_pdb_read_fn read;
    void *context;
    uint32_t page_size,pages,current,last,count,table_type;
    unsigned char *page,*visited;
    uint32_t *ids;
    int done,failed;
};
static uint16_t hub_pdb_u16(const unsigned char *p) { return (uint16_t)p[0]|((uint16_t)p[1]<<8); }
static uint32_t hub_pdb_u32(const unsigned char *p) {
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static void hub_public_pdb_destroy(struct hub_public_pdb_reader *reader) {
    free(reader->page); free(reader->visited); free(reader->ids);
    memset(reader,0,sizeof *reader);
}
static int hub_public_pdb_fail(struct hub_public_pdb_reader *reader,int error) {
    reader->failed=error; errno=error; return -1;
}
static int hub_public_pdb_table_init(struct hub_public_pdb_reader *reader,uint64_t size,
                               hub_public_pdb_read_fn read,void *context,uint32_t table_type) {
    memset(reader,0,sizeof *reader); reader->read=read; reader->context=context;
    reader->table_type=table_type;
    unsigned char header[28];
    if (!read || size<sizeof header || size>HUB_PDB_SIZE_MAX) return hub_public_pdb_fail(reader,EINVAL);
    if (read(context,0,header,sizeof header)<0) return hub_public_pdb_fail(reader,errno);
    uint32_t bytes=hub_pdb_u32(header+4),tables=hub_pdb_u32(header+8);
    if (bytes<512 || bytes>65536 || (bytes&(bytes-1)) || size%bytes ||
        !tables || tables>64 || 28+tables*16>bytes || hub_pdb_u32(header) || hub_pdb_u32(header+24))
        return hub_public_pdb_fail(reader,EINVAL);
    reader->page_size=bytes; reader->pages=(uint32_t)(size/bytes);
    reader->page=malloc(bytes); reader->visited=calloc((reader->pages+7)/8,1);
    if (!table_type) reader->ids=calloc(HUB_PDB_ID_BUCKETS,sizeof *reader->ids);
    if (!reader->page || !reader->visited || (!table_type && !reader->ids)) return hub_public_pdb_fail(reader,ENOMEM);
    if (read(context,0,reader->page,bytes)<0) return hub_public_pdb_fail(reader,errno);
    if (memcmp(header,reader->page,sizeof header)) return hub_public_pdb_fail(reader,ESTALE);
    unsigned found=0;
    for (unsigned i=0;i<tables;++i) {
        const unsigned char *table=reader->page+28+i*16;
        if (hub_pdb_u32(table)==table_type) {
            ++found; reader->current=hub_pdb_u32(table+8); reader->last=hub_pdb_u32(table+12);
        }
    }
    if (!found) return hub_public_pdb_fail(reader,ENOENT);
    if (found!=1 || !reader->current || reader->current>=reader->pages ||
        !reader->last || reader->last>=reader->pages) return hub_public_pdb_fail(reader,EINVAL);
    return 0;
}
static int hub_public_pdb_init(struct hub_public_pdb_reader *reader,uint64_t size,
                               hub_public_pdb_read_fn read,void *context) {
    return hub_public_pdb_table_init(reader,size,read,context,0);
}
static int hub_public_pdb_string(const unsigned char *page,size_t offset,size_t end,char out[4096]) {
    if (offset>=end) return 0;
    unsigned kind=page[offset]; size_t bytes,start;
    if (kind==0x40 || kind==0x90) {
        if (end-offset<4) return 0;
        size_t total=hub_pdb_u16(page+offset+1);
        if (total<4 || total>end-offset) return 0;
        bytes=total-4; start=offset+4;
    } else {
        if (!(kind&1) || kind<3) return 0;
        bytes=(kind>>1)-1; start=offset+1;
        if (bytes>end-start) return 0;
    }
    if (kind!=0x90) {
        if (bytes>=4096) return 0;
        for (size_t i=0;i<bytes;++i) if (!page[start+i] || page[start+i]>127) return 0;
        memcpy(out,page+start,bytes); out[bytes]=0; return 1;
    }
    if (bytes%2) return 0;
    size_t used=0;
    for (size_t i=0;i<bytes;i+=2) {
        uint32_t code=hub_pdb_u16(page+start+i);
        if (!code) return 0;
        if (code>=0xd800 && code<=0xdbff) {
            if (bytes-i<4) return 0;
            uint32_t low=hub_pdb_u16(page+start+i+2);
            if (low<0xdc00 || low>0xdfff) return 0;
            code=0x10000+((code-0xd800)<<10)+(low-0xdc00); i+=2;
        } else if (code>=0xdc00 && code<=0xdfff) return 0;
        unsigned count=code<128 ? 1 : code<2048 ? 2 : code<65536 ? 3 : 4;
        if (used+count>=4096) return 0;
        if (count==1) out[used++]=(char)code;
        else {
            out[used++]=(char)((count==2 ? 0xc0 : count==3 ? 0xe0 : 0xf0)|(code>>(6*(count-1))));
            for (unsigned n=count-1;n;--n) out[used++]=(char)(0x80|((code>>(6*(n-1)))&63));
        }
    }
    out[used]=0; return 1;
}
static int hub_public_pdb_text(const unsigned char *page,size_t row,size_t end,unsigned index,char out[4096]) {
    size_t offset=row+hub_pdb_u16(page+row+94+index*2);
    return offset>=row+136 && hub_public_pdb_string(page,offset,end,out);
}
/* Returns1 when complete,0 after a page,-1 on terminal error/cancellation.
 * Rows emitted before a later error must never be published as a full library. */
static int hub_public_pdb_step(struct hub_public_pdb_reader *reader,
                              hub_public_pdb_track_fn emit,void *context) {
    if (reader->failed) { errno=reader->failed; return -1; }
    if (reader->table_type) return hub_public_pdb_fail(reader,EINVAL);
    if (reader->done) return 1;
    uint32_t current=reader->current,size=reader->page_size;
    if (!emit || !current || current>=reader->pages ||
        (reader->visited[current/8]&(1U<<(current%8)))) return hub_public_pdb_fail(reader,EINVAL);
    reader->visited[current/8]|=(unsigned char)(1U<<(current%8));
    if (reader->read(reader->context,(uint64_t)current*size,reader->page,size)<0)
        return hub_public_pdb_fail(reader,errno);
    const unsigned char *page=reader->page;
    if (hub_pdb_u32(page+4)!=current || hub_pdb_u32(page+8)) return hub_public_pdb_fail(reader,EINVAL);
    if (!(page[27]&0x40)) {
        /* The allocated-offset count is the low thirteen bits at byte24.
         * Byte34 is a transaction index, not a larger row count. */
        unsigned rows=hub_pdb_u16(page+24)&0x1fff,groups=(rows+15)/16;
        size_t index_bytes=rows ? (groups-1)*36+4+2*((rows-1)%16+1) : 0;
        if (index_bytes>size-40) return hub_public_pdb_fail(reader,EINVAL);
        size_t end=size-index_bytes;
        for (unsigned group=0;group<groups;++group) {
            size_t base=size-group*36; unsigned flags=hub_pdb_u16(page+base-4);
            for (unsigned i=0;i<16;++i) if (flags&(1U<<i)) {
                size_t row=40+hub_pdb_u16(page+base-6-i*2);
                if (group*16+i>=rows || end<136 || row>end-136) return hub_public_pdb_fail(reader,EINVAL);
                struct hub_public_pdb_track track; track.id=hub_pdb_u32(page+row+72);
                track.file_size=hub_pdb_u32(page+row+16); track.artwork_id=hub_pdb_u32(page+row+28);
                track.key_id=hub_pdb_u32(page+row+32); track.label_id=hub_pdb_u32(page+row+40);
                track.bitrate=hub_pdb_u32(page+row+48); track.bpm100=hub_pdb_u32(page+row+56);
                track.genre_id=hub_pdb_u32(page+row+60); track.album_id=hub_pdb_u32(page+row+64);
                track.artist_id=hub_pdb_u32(page+row+68); track.duration=hub_pdb_u16(page+row+84);
                track.rating=page[row+89];
                if (!track.id || reader->count>=HUB_PDB_TRACKS_MAX) return hub_public_pdb_fail(reader,EINVAL);
                uint32_t bucket=(track.id*2654435761U)&(HUB_PDB_ID_BUCKETS-1);
                while (reader->ids[bucket] && reader->ids[bucket]!=track.id) bucket=(bucket+1)&(HUB_PDB_ID_BUCKETS-1);
                if (reader->ids[bucket]) return hub_public_pdb_fail(reader,EINVAL);
                reader->ids[bucket]=track.id; ++reader->count;
                if (!hub_public_pdb_text(page,row,end,20,track.path) ||
                    !hub_public_pdb_text(page,row,end,14,track.analysis_path) ||
                    !hub_public_pdb_text(page,row,end,16,track.comment) ||
                    !hub_public_pdb_text(page,row,end,17,track.title)) return hub_public_pdb_fail(reader,EINVAL);
                if (emit(context,&track)<0) return hub_public_pdb_fail(reader,errno ? errno : ECANCELED);
            }
        }
    }
    if (current==reader->last) { reader->done=1; return 1; }
    reader->current=hub_pdb_u32(page+12); return 0;
}
#endif

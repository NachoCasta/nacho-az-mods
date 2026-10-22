/* SPDX-License-Identifier: GPL-3.0-only */
/* Validate sealed private staging before publishing the combined library. */
#ifndef HUB_PUBLIC_LIBRARY_METADATA_H
#define HUB_PUBLIC_LIBRARY_METADATA_H
#include "public-track-index.h"
#include "public-entity-index.h"
static const struct hub_entity_row *hub_library_entity(const struct hub_entity_index *entities,
        uint32_t table,uint32_t id) {
    unsigned count; return hub_entity_index_find(entities,entities->generation,table,id,&count);
}
static int hub_library_metadata_validate(const struct hub_public_track_index *tracks,
        const struct hub_entity_index *entities,unsigned *missing_names) {
    *missing_names=0;
    if (tracks->generation!=entities->generation ||
        __atomic_load_n(&tracks->state,__ATOMIC_ACQUIRE)!=HUB_TRACK_READY ||
        __atomic_load_n(&entities->state,__ATOMIC_ACQUIRE)!=HUB_ENTITY_READY) { errno=EINVAL; return -1; }
    /* Native libraries can contain dangling optional name IDs. Resolve those
     * as absent metadata, while retaining the playable track. */
    const unsigned tables[]={13,5,4,1,3,2};
    for (unsigned i=0;i<tracks->count;++i) {
        const struct hub_public_track_row *row=tracks->rows+i;
        const uint32_t ids[]={row->artwork_id,row->key_id,row->label_id,row->genre_id,row->album_id,row->artist_id};
        for (unsigned j=0;j<6;++j) if (ids[j] && !hub_library_entity(entities,tables[j],ids[j])) ++*missing_names;
    }
    unsigned char *marks=calloc(entities->count ? entities->count : 1,1);
    if (!marks) { errno=ENOMEM; return -1; }
    for (unsigned i=0;i<entities->count;++i) {
        const struct hub_entity_row *row=entities->rows+i;
        if (row->table==8) {
            const struct hub_entity_row *playlist=hub_library_entity(entities,7,row->id);
            if (!playlist || playlist->folder || !hub_public_track_index_find(tracks,tracks->generation,row->track_id)) goto invalid;
        }
        if (row->table!=7 || marks[i]==2) continue;
        const struct hub_entity_row *cursor=row;
        while (cursor) {
            size_t position=(size_t)(cursor-entities->rows);
            if (marks[position]==1) goto invalid;
            if (marks[position]==2) break;
            marks[position]=1;
            if (!cursor->parent) break;
            cursor=hub_library_entity(entities,7,cursor->parent);
            if (!cursor || !cursor->folder) goto invalid;
        }
        cursor=row;
        while (cursor && marks[cursor-entities->rows]==1) {
            marks[cursor-entities->rows]=2;
            cursor=cursor->parent ? hub_library_entity(entities,7,cursor->parent) : NULL;
        }
    }
    free(marks); return 0;
invalid:
    free(marks); errno=EINVAL; return -1;
}
#endif

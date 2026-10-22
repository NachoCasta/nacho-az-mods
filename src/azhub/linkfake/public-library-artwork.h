/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HUB_PUBLIC_LIBRARY_ARTWORK_H
#define HUB_PUBLIC_LIBRARY_ARTWORK_H
#include "public-library-files.h"
#include "public-db-wire.h"
struct hub_library_artwork {
    struct hub_public_library_files *library;
    struct hub_public_pdb_source source;
    unsigned char *data;
    uint32_t size,filled;
    uint64_t generation;
};
static void hub_library_artwork_destroy(struct hub_library_artwork *art) {
    free(art->data); memset(art,0,sizeof *art);
}
static int hub_library_artwork_init(struct hub_library_artwork *art,struct hub_public_library_files *library,uint32_t id,unsigned high) {
    memset(art,0,sizeof *art); art->library=library; art->generation=library->metadata.generation;
    if (hub_public_library_files_validate(library)<0) return -1;
    const struct hub_entity_index *entities=hub_public_library_ready_entities(library);
    if (!entities) { errno=ESTALE; return -1; }
    const struct hub_entity_row *row=hub_library_entity(entities,13,id);
    if (!row || !entities->text[row->name]) { errno=ENOENT; return -1; }
    char relative[4096]; unsigned char handle[32]; struct hub_public_file_ref registered;
    if (high) {
        int result=hub_public_artwork_high_path(entities->text+row->name,relative);
        if (result) { if (result>0) errno=ENOENT; return -1; }
    } else if (hub_public_library_relative(entities->text+row->name,relative)<0) return -1;
    if (!hub_public_catalog_find(&library->catalog,relative,strlen(relative),handle)) { errno=ENOENT; return -1; }
    if (!hub_public_catalog_resolve(&library->catalog,handle,&registered) || registered.directory) { errno=ESTALE; return -1; }
    if (hub_public_pdb_source_init(&art->source,registered.owner,art->generation,relative,strlen(relative))<0) return -1;
    if (registered.device!=art->source.file.device || registered.inode!=art->source.file.inode) { errno=ESTALE; return -1; }
    if ((uint64_t)art->source.expected.st_size>HUB_DB_FIELD_MAX) { errno=EFBIG; return -1; }
    art->size=(uint32_t)art->source.expected.st_size; art->data=malloc(art->size);
    if (!art->data) { errno=ENOMEM; return -1; }
    return 0;
}
static int hub_library_artwork_step(struct hub_library_artwork *art) {
    if (!hub_public_library_ready_entities(art->library) || art->library->metadata.generation!=art->generation) {
        errno=ESTALE; return -1;
    }
    size_t length=art->size-art->filled; if (length>8192) length=8192;
    if (hub_public_pdb_source_read(&art->source,art->filled,art->data+art->filled,length)<0) return -1;
    art->filled+=(uint32_t)length;
    if (art->filled<art->size) return 0;
    if (hub_public_library_files_validate(art->library)<0) return -1;
    return 1;
}
#endif

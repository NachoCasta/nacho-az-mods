/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include "public-library-metadata.h"
int main(void) {
    for (unsigned mode=0;mode<5;++mode) {
        struct hub_public_track_index tracks; struct hub_entity_index entities;
        assert(!hub_public_track_index_init(&tracks,1) && !hub_entity_index_init(&entities,1));
        struct hub_public_pdb_track track={.id=17,.album_id=99};
        assert(!hub_public_track_index_add(&tracks,&track));
        struct hub_public_pdb_entity folder={.table=7,.id=1,.folder=1,.name="Folder"};
        struct hub_public_pdb_entity list={.table=7,.id=2,.parent=1,.name="List"};
        struct hub_public_pdb_entity member={.table=8,.id=2,.track_id=17,.sort_order=1};
        if (mode==1) { folder.parent=2; list.folder=1; } /* Hierarchy cycle. */
        if (mode==2) list.parent=9; /* Missing parent. */
        if (mode==3) member.id=1; /* Folder cannot contain track membership. */
        if (mode==4) member.track_id=18;
        assert(!hub_entity_index_add(&entities,&folder) && !hub_entity_index_add(&entities,&list));
        assert(!hub_entity_index_add(&entities,&member));
        assert(!hub_public_track_index_seal(&tracks) && !hub_entity_index_seal(&entities));
        unsigned missing; int result=hub_library_metadata_validate(&tracks,&entities,&missing);
        assert(missing==1);
        if (!mode) assert(!result); else assert(result<0 && errno==EINVAL);
        hub_entity_index_destroy(&entities); hub_public_track_index_destroy(&tracks);
    }
    puts("PASS: optional absent names, playlist membership references, folder typing and hierarchy cycle validation");
}

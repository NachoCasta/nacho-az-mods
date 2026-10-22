/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include "public-pdb-source.h"
#include "public-entity-index.h"
static int scan(const char *directory,const char *relative) {
    struct hub_public_root root; assert(!hub_public_root_init(&root));
    assert(!hub_public_root_attach(&root,directory,1));
    struct hub_public_pdb_source source;
    assert(!hub_public_pdb_source_init(&source,&root,1,relative,strlen(relative)));
    struct hub_entity_index index; assert(!hub_entity_index_init(&index,1));
    const unsigned tables[]={1,2,3,4,5,7,8,13};
    for (unsigned i=0;i<sizeof tables/sizeof *tables;++i) {
        struct hub_public_pdb_reader reader;
        assert(!hub_public_pdb_table_init(&reader,(uint64_t)source.expected.st_size,
            hub_public_pdb_source_read,&source,tables[i]));
        int result;
        while ((result=hub_public_pdb_entities_step(&reader,hub_entity_index_add,&index))==0) assert(!root.active);
        assert(result==1); hub_public_pdb_destroy(&reader);
    }
    assert(!hub_entity_index_seal(&index));
    for (unsigned i=0;i<index.count;) {
        unsigned count; const struct hub_entity_row *first=index.rows+i;
        const struct hub_entity_row *rows=hub_entity_index_find(&index,1,first->table,first->id,&count);
        assert(rows==first && count);
        for (unsigned n=0;n<count;++n) {
            const struct hub_entity_row *row=rows+n;
            printf("%u\t%u\t%u\t%u\t%u\t%u\t",row->table,row->id,row->parent,
                row->sort_order,row->track_id,row->folder);
            for (const unsigned char *p=(const unsigned char *)index.text+row->name;*p;++p) printf("%02x",*p);
            putchar('\n');
        }
        i+=count;
    }
    fprintf(stderr,"Entity index: rows=%u row_capacity=%u text_bytes=%zu text_capacity=%zu allocated=%zu\n",
        index.count,index.capacity,index.text_bytes,index.text_capacity,
        index.capacity*sizeof *index.rows+index.text_capacity);
    hub_entity_index_retire(&index); hub_entity_index_destroy(&index);
    hub_public_root_withdraw(&root); assert(!hub_public_root_destroy(&root)); return 0;
}
int main(int argc,char **argv) {
    if (argc==4 && !strcmp(argv[1],"scan")) return scan(argv[2],argv[3]);
    assert(argc==1);
    struct hub_entity_index a,b; unsigned count;
    assert(!hub_entity_index_init(&a,11) && !hub_entity_index_init(&b,12));
    struct hub_public_pdb_entity e={.table=7,.id=4,.name="Playlist"};
    assert(!hub_entity_index_add(&a,&e));
    strcpy(e.name,"Other source"); assert(!hub_entity_index_add(&b,&e));
    assert(!hub_entity_index_find(&a,11,7,4,&count) && !count);
    e=(struct hub_public_pdb_entity){.table=8,.id=4,.sort_order=2,.track_id=8};
    assert(!hub_entity_index_add(&a,&e)); e.sort_order=1; e.track_id=9;
    assert(!hub_entity_index_add(&a,&e));
    assert(!hub_entity_index_seal(&a) && !hub_entity_index_seal(&b));
    const struct hub_entity_row *rows=hub_entity_index_find(&a,11,8,4,&count);
    assert(rows && count==2 && rows[0].track_id==9 && rows[1].track_id==8 && !a.text[rows[0].name]);
    assert(!hub_entity_index_find(&a,12,8,4,&count) && !count);
    rows=hub_entity_index_find(&a,11,7,4,&count);
    assert(rows && !strcmp(a.text+rows->name,"Playlist"));
    hub_entity_index_retire(&a);
    assert(!hub_entity_index_find(&a,11,7,4,&count) && !count);
    assert(!strcmp(a.text+rows->name,"Playlist")); /* Accepted reader can finish. */
    rows=hub_entity_index_find(&b,12,7,4,&count);
    assert(rows && !strcmp(b.text+rows->name,"Other source"));
    assert(hub_entity_index_seal(&a)<0);
    hub_entity_index_destroy(&a); hub_entity_index_destroy(&b);
    for (unsigned table=7;table<=8;++table) {
        assert(!hub_entity_index_init(&a,1)); e.table=table;
        assert(!hub_entity_index_add(&a,&e) && !hub_entity_index_add(&a,&e));
        assert(hub_entity_index_seal(&a)<0 && a.state==HUB_ENTITY_FAILED);
        hub_entity_index_destroy(&a);
    }
    puts("PASS: entity isolation, unpublished staging, ordered membership ranges, duplicate rejection and retirement");
}

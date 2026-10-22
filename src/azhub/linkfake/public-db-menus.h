/* SPDX-License-Identifier: GPL-3.0-only */
/* Source-owned browse ordering, shared by sessions until all readers join. */
#ifndef HUB_PUBLIC_DB_MENUS_H
#define HUB_PUBLIC_DB_MENUS_H
#ifndef HUB_DB_OBSERVE_REQUEST
#define HUB_DB_OBSERVE_REQUEST(request) ((void)0)
#endif
#include "public-library-files.h"
#include "public-db-session.h"
#include "public-library-analysis.h"
#include "public-beatgrid.h"
#include "public-waveform.h"
#include "public-cues.h"
#include "public-library-artwork.h"
struct hub_db_title { const char *text; const struct hub_public_track_row *track; };
struct hub_db_tree { const struct hub_entity_row *row; };
struct hub_db_library_view {
    struct hub_public_library_files *library;
    uint64_t generation;
    struct hub_db_title *titles;
    struct hub_db_tree *tree;
    unsigned track_count,tree_count;
};
static int hub_db_title_compare(const void *left,const void *right) {
    const struct hub_db_title *a=left,*b=right;
    const unsigned char *x=(const unsigned char *)a->text,*y=(const unsigned char *)b->text;
    while (*x && *y) {
        unsigned p=*x++,q=*y++; if (p>='A' && p<='Z') p+=32; if (q>='A' && q<='Z') q+=32;
        if (p!=q) return (p>q)-(p<q);
    }
    if (*x!=*y) return (*x>*y)-(*x<*y);
    return (a->track->id>b->track->id)-(a->track->id<b->track->id);
}
static int hub_db_tree_compare(const void *left,const void *right) {
    const struct hub_entity_row *a=((const struct hub_db_tree *)left)->row,*b=((const struct hub_db_tree *)right)->row;
    if (a->parent!=b->parent) return (a->parent>b->parent)-(a->parent<b->parent);
    if (a->sort_order!=b->sort_order) return (a->sort_order>b->sort_order)-(a->sort_order<b->sort_order);
    return (a->id>b->id)-(a->id<b->id);
}
static void hub_db_library_view_destroy(struct hub_db_library_view *view) {
    free(view->titles); free(view->tree); memset(view,0,sizeof *view);
}
static int hub_db_library_view_init(struct hub_db_library_view *view,struct hub_public_library_files *library) {
    memset(view,0,sizeof *view);
    const struct hub_public_track_index *tracks=hub_public_library_ready_tracks(library);
    const struct hub_entity_index *entities=hub_public_library_ready_entities(library);
    if (!tracks || !entities) { errno=ESTALE; return -1; }
    view->library=library; view->generation=tracks->generation; view->track_count=tracks->count;
    for (unsigned i=0;i<entities->count;++i) view->tree_count+=entities->rows[i].table==7;
    view->titles=calloc(view->track_count ? view->track_count : 1,sizeof *view->titles);
    view->tree=calloc(view->tree_count ? view->tree_count : 1,sizeof *view->tree);
    if (!view->titles || !view->tree) { hub_db_library_view_destroy(view); errno=ENOMEM; return -1; }
    for (unsigned i=0;i<view->track_count;++i) {
        view->titles[i].track=tracks->rows+i; view->titles[i].text=tracks->text+tracks->rows[i].title;
    }
    unsigned count=0;
    for (unsigned i=0;i<entities->count;++i) if (entities->rows[i].table==7) view->tree[count++].row=entities->rows+i;
    if (view->track_count) qsort(view->titles,view->track_count,sizeof *view->titles,hub_db_title_compare);
    if (view->tree_count) qsort(view->tree,view->tree_count,sizeof *view->tree,hub_db_tree_compare);
    return 0;
}
/* Strict UTF8 to terminated UTF16BE for menu strings, up to4095 UTF8 bytes. */
static int hub_db_menu_text(const char *text,unsigned char output[8192],uint32_t *length) {
    size_t size=strnlen(text,4096),used=0;
    if (size==4096) return -1;
    for (size_t i=0;i<size;) {
        unsigned c=(unsigned char)text[i++],extra; uint32_t code;
        if (c<128) { code=c; extra=0; }
        else if (c>=0xc2 && c<=0xdf) { code=c&31; extra=1; }
        else if (c>=0xe0 && c<=0xef) { code=c&15; extra=2; }
        else if (c>=0xf0 && c<=0xf4) { code=c&7; extra=3; }
        else return -1;
        if (extra>size-i) return -1;
        for (unsigned n=0;n<extra;++n) { c=(unsigned char)text[i++]; if ((c&0xc0)!=0x80) return -1; code=(code<<6)|(c&63); }
        if ((extra==1 && code<128) || (extra==2 && code<2048) || (extra==3 && code<65536) ||
            code>0x10ffff || (code>=0xd800 && code<=0xdfff)) return -1;
        if (code<65536) { output[used++]=(unsigned char)(code>>8); output[used++]=(unsigned char)code; }
        else {
            code-=65536; unsigned high=0xd800+(code>>10),low=0xdc00+(code&1023);
            output[used++]=(unsigned char)(high>>8); output[used++]=(unsigned char)high;
            output[used++]=(unsigned char)(low>>8); output[used++]=(unsigned char)low;
        }
    }
    output[used++]=0; output[used++]=0; *length=(uint32_t)used; return 0;
}
enum hub_db_menu_kind { HUB_MENU_ROOT,HUB_MENU_TRACKS,HUB_MENU_FOLDER,HUB_MENU_PLAYLIST,HUB_MENU_METADATA,HUB_MENU_LOADINFO };
struct hub_db_menu { unsigned used,key,kind,id,first,count; };
struct hub_db_backend {
    struct hub_db_library_view *view;
    struct hub_db_session *connection;
    struct hub_db_menu menus[16],current;
    unsigned phase,offset,end;
    uint32_t transaction,single_type,single_first,single_second;
    unsigned char text[8192];
    struct hub_library_analysis analysis;
    unsigned char *analysis_payload;
    uint32_t analysis_length,analysis_request,analysis_part;
    struct hub_cue_wire cues;
    struct hub_library_artwork artwork;
};
static void hub_db_backend_release_analysis(struct hub_db_backend *backend) {
    hub_library_analysis_destroy(&backend->analysis);
    hub_cue_wire_destroy(&backend->cues);
    hub_library_artwork_destroy(&backend->artwork);
    free(backend->analysis_payload); backend->analysis_payload=NULL; backend->analysis_length=0;
}
/* Call after the session worker stops, before destroying its source view. */
static void hub_db_backend_destroy(struct hub_db_backend *backend) {
    hub_db_backend_release_analysis(backend);
}
static int hub_db_backend_alive(void *opaque) {
    struct hub_db_backend *backend=opaque; struct hub_public_library_files *library=backend->view->library;
    return library && library->metadata.generation==backend->view->generation &&
        hub_public_library_ready_tracks(library) && hub_public_library_ready_entities(library);
}
static int hub_db_backend_single(struct hub_db_backend *backend,uint32_t type,uint32_t first,uint32_t second) {
    backend->phase=1; backend->single_type=type; backend->single_first=first; backend->single_second=second; return 0;
}
static int hub_db_backend_begin(void *opaque,const struct hub_db_message *request) {
    HUB_DB_OBSERVE_REQUEST(request);
    struct hub_db_backend *backend=opaque; backend->phase=0; backend->transaction=request->transaction;
    hub_db_backend_release_analysis(backend);
    if (!hub_db_backend_alive(backend) || hub_public_library_files_validate(backend->view->library)<0) return -1;
    if (request->type==1) return 0;
    if (request->type==0x3007) return hub_db_backend_single(backend,0x4000,request->type,0);
    if (!request->count || request->args[0].kind!=HUB_DB_NUMBER) goto reject;
    uint32_t dmst=request->args[0].number;
    /* Public rekordbox-style provider slot4/type1 only. Private local slots
     * and another requester's DMST must never address this backend. */
    if ((dmst>>24)!=backend->connection->requester || (dmst&65535)!=0x0401) goto reject;
    if (request->type==0x2003) {
        if ((request->count!=2 && request->count!=3) || request->args[1].kind!=HUB_DB_NUMBER) goto reject;
        unsigned high=request->count==3;
        if (high && (request->args[2].kind!=HUB_DB_NUMBER || request->args[2].number!=1)) goto reject;
        if (hub_library_artwork_init(&backend->artwork,backend->view->library,request->args[1].number,high)<0) {
            if (errno!=ENOENT) return -1;
            backend->phase=15;
        } else backend->phase=14;
        return 0;
    }
    if (request->type==0x2104 || request->type==0x2b04) {
        unsigned extended=request->type==0x2b04;
        if (request->count!=(extended ? 3U : 2U) || request->args[1].kind!=HUB_DB_NUMBER) goto reject;
        if (extended && (request->args[2].kind!=HUB_DB_NUMBER || request->args[2].number)) goto reject;
        backend->analysis_request=request->type; backend->analysis_part=0;
        if (hub_library_analysis_init(&backend->analysis,backend->view->library,request->args[1].number,
                extended ? 1 : 0,extended ? 0x50434f32 : 0x50434f42,0)<0) {
            if (errno!=ENOENT) return -1;
            if (extended) return hub_db_backend_single(backend,0x4003,request->type,0);
            backend->phase=11;
        } else backend->phase=10;
        return 0;
    }
    if (request->type==0x2004) {
        if (request->count!=4 && request->count!=5) goto reject;
        for (unsigned i=1;i<4;++i) if (request->args[i].kind!=HUB_DB_NUMBER) goto reject;
        if (request->args[3].number || (request->count==5 &&
            (request->args[4].kind!=HUB_DB_BINARY || request->args[4].length))) goto reject;
        backend->analysis_request=request->type; backend->analysis_part=0;
        if (hub_library_analysis_init(&backend->analysis,backend->view->library,request->args[2].number,0,0x50574156,0)<0) {
            if (errno!=ENOENT) return -1;
            backend->phase=11;
        } else backend->phase=10;
        return 0;
    }
    if (request->type==0x2204 || request->type==0x2904) {
        unsigned detail=request->type==0x2904;
        if (request->count!=(detail ? 3U : 2U) || request->args[1].kind!=HUB_DB_NUMBER) goto reject;
        if (detail && (request->args[2].kind!=HUB_DB_NUMBER || request->args[2].number)) goto reject;
        backend->analysis_request=request->type;
        if (hub_library_analysis_init(&backend->analysis,backend->view->library,request->args[1].number,
                detail ? 1 : 0,detail ? 0x50575633 : 0x5051545a,0)<0) {
            if (errno!=ENOENT) return -1;
            backend->phase=11;
        } else backend->phase=10;
        return 0;
    }
    if (request->type==0x2c04) {
        backend->analysis_request=request->type;
        if (request->count!=4) goto reject;
        for (unsigned i=1;i<4;++i) if (request->args[i].kind!=HUB_DB_NUMBER) goto reject;
        unsigned extension;
        switch (request->args[3].number) {
        case 0x00544144: extension=0; break;
        case 0x00545845: extension=1; break;
        case 0x00584532: extension=2; break;
        default: goto reject;
        }
        uint32_t reversed=request->args[2].number;
        uint32_t tag=((reversed&255)<<24)|((reversed&0xff00)<<8)|((reversed>>8)&0xff00)|(reversed>>24);
        if (hub_library_analysis_init(&backend->analysis,backend->view->library,request->args[1].number,extension,tag,0)<0) {
            if (errno!=ENOENT) return -1;
            backend->phase=11;
        } else backend->phase=10;
        return 0;
    }
    unsigned key=(dmst>>8)&65535;
    struct hub_db_menu *stored=NULL,*empty=NULL;
    for (unsigned i=0;i<16;++i) {
        if (backend->menus[i].used && backend->menus[i].key==key) stored=backend->menus+i;
        else if (!backend->menus[i].used && !empty) empty=backend->menus+i;
    }
    if (request->type==0x3000) {
        if (!stored || request->count<3 || request->args[1].kind!=HUB_DB_NUMBER || request->args[2].kind!=HUB_DB_NUMBER) goto reject;
        unsigned offset=request->args[1].number,limit=request->args[2].number;
        if (offset>stored->count || !limit || limit>64) goto reject;
        backend->current=*stored; backend->offset=offset;
        backend->end=offset+(limit<stored->count-offset ? limit : stored->count-offset); backend->phase=2; return 0;
    }
    struct hub_db_menu menu={.used=1,.key=key};
    struct hub_db_library_view *view=backend->view;
    const struct hub_entity_index *entities=&view->library->entities;
    if (request->type==0x1000) { menu.kind=HUB_MENU_ROOT; menu.count=2; }
    else if (request->type==0x1004) {
        if (request->count<2 || request->args[1].kind!=HUB_DB_NUMBER || request->args[1].number) goto reject;
        menu.kind=HUB_MENU_TRACKS; menu.count=view->track_count;
    } else if (request->type==0x1105) {
        if (request->count<4) goto reject;
        for (unsigned i=1;i<4;++i) if (request->args[i].kind!=HUB_DB_NUMBER) goto reject;
        if (request->args[1].number || request->args[3].number>1) goto reject;
        menu.id=request->args[2].number;
        const struct hub_entity_row *node=hub_library_entity(entities,7,menu.id);
        if (request->args[3].number) {
            if (menu.id && (!node || !node->folder)) goto reject;
            menu.kind=HUB_MENU_FOLDER;
            while (menu.first<view->tree_count && view->tree[menu.first].row->parent<menu.id) ++menu.first;
            while (menu.first+menu.count<view->tree_count && view->tree[menu.first+menu.count].row->parent==menu.id) ++menu.count;
        } else {
            if (!node || node->folder) goto reject;
            menu.kind=HUB_MENU_PLAYLIST;
            const struct hub_entity_row *members=hub_entity_index_find(entities,view->generation,8,menu.id,&menu.count);
            if (members) menu.first=(unsigned)(members-entities->rows);
        }
    } else if (request->type==0x2002 || request->type==0x2102) {
        if (request->count<2 || request->args[1].kind!=HUB_DB_NUMBER) goto reject;
        menu.id=request->args[1].number;
        const struct hub_public_track_row *track=hub_public_track_index_find(&view->library->metadata,view->generation,menu.id);
        if (!track) goto reject;
        if (request->type==0x2102) {
            const char *path=view->library->metadata.text+track->path; size_t length=strlen(path);
            /* Decoder1 is the native-emulator-confirmed MP3 mapping. Other
             * formats remain a research gate, never silently mapped to MP3. */
            if (length<4 || path[length-4]!='.' || (path[length-3]!='m' && path[length-3]!='M') ||
                (path[length-2]!='p' && path[length-2]!='P') || path[length-1]!='3') goto reject;
            char relative[4096]; unsigned char handle[32];
            if (hub_public_library_relative(path,relative)<0 ||
                !hub_public_catalog_find(&view->library->catalog,relative,strlen(relative),handle)) goto reject;
            menu.kind=HUB_MENU_LOADINFO; menu.count=7;
        } else { menu.kind=HUB_MENU_METADATA; menu.count=11; }
    } else goto reject;
    if (!stored) stored=empty;
    if (!stored) goto reject;
    *stored=menu; return hub_db_backend_single(backend,0x4000,request->type,menu.count);
reject:
    return hub_db_backend_single(backend,0x4003,request->type,0);
}
static int hub_db_backend_next(void *opaque,struct hub_db_message *reply) {
    struct hub_db_backend *backend=opaque;
    if (!hub_db_backend_alive(backend)) return -1;
    if (!backend->phase) return 0;
    memset(reply,0,sizeof *reply); reply->transaction=backend->transaction;
    for (unsigned i=0;i<12;++i) reply->args[i].kind=HUB_DB_NUMBER;
    if (backend->phase==12) { hub_db_backend_release_analysis(backend); backend->phase=0; return 0; }
    if (backend->phase==14) {
        int result=hub_library_artwork_step(&backend->artwork);
        if (result<0) return -1;
        if (!result) return 2;
        backend->phase=15;
    }
    if (backend->phase==15) {
        reply->type=0x4002; reply->count=4; reply->args[0].number=0x2003;
        reply->args[2].number=backend->artwork.size;
        reply->args[3]=(struct hub_db_field){.kind=HUB_DB_BINARY,.length=backend->artwork.size,.data=backend->artwork.data};
        backend->phase=12; return 1;
    }
    if (backend->phase==10) {
        int result=hub_library_analysis_step(&backend->analysis);
        if (!result) return 2;
        if (result<0) {
            if (errno!=ENOENT) return -1;
            if (backend->analysis_request==0x2b04 && !backend->analysis_part) {
                hub_db_backend_release_analysis(backend);
                hub_db_backend_single(backend,0x4003,0x2b04,0); return 2;
            }
            if (backend->analysis_request==0x2104 || backend->analysis_request==0x2b04) hub_library_analysis_destroy(&backend->analysis);
            else hub_db_backend_release_analysis(backend);
            backend->phase=11;
        } else {
            size_t length; const unsigned char *data=hub_analysis_ready(&backend->analysis.reader,&length);
            if (!data || length>HUB_DB_FIELD_MAX-4) return -1;
            if (backend->analysis_request==0x2104 || backend->analysis_request==0x2b04) {
                unsigned extended=backend->analysis_request==0x2b04;
                if ((extended ? hub_cue_extended_append(&backend->cues,data,length) : hub_cue_wire_append(&backend->cues,data,length))<0) return -1;
                if (!backend->analysis_part) {
                    hub_analysis_destroy(&backend->analysis.reader); backend->analysis_part=1;
                    if (hub_analysis_init(&backend->analysis.reader,(uint64_t)backend->analysis.source.expected.st_size,
                            hub_library_analysis_read,&backend->analysis,extended ? 0x50434f32 : 0x50434f42,1)<0) return -1;
                    return 2;
                }
            } else if (backend->analysis_request==0x2004) {
                if (!backend->analysis_part) {
                    backend->analysis_payload=calloc(1,900); if (!backend->analysis_payload) return -1;
                }
                if (hub_wave_preview_part(data,length,backend->analysis_part,backend->analysis_payload)<0) return -1;
                if (!backend->analysis_part) {
                    /* Reuse the captured file fingerprint across both sections. */
                    hub_analysis_destroy(&backend->analysis.reader); backend->analysis_part=1;
                    if (hub_analysis_init(&backend->analysis.reader,(uint64_t)backend->analysis.source.expected.st_size,
                            hub_library_analysis_read,&backend->analysis,0x50575632,0)<0) return -1;
                    return 2;
                }
                backend->analysis_length=900;
            } else if (backend->analysis_request==0x2204) {
                if (hub_grid_convert(data,length,&backend->analysis_payload,&backend->analysis_length)<0) return -1;
            } else if (backend->analysis_request==0x2904) {
                if (hub_wave_detail_convert(data,length,&backend->analysis_payload,&backend->analysis_length)<0) return -1;
            } else {
                backend->analysis_payload=malloc(length+4); if (!backend->analysis_payload) return -1;
                /* Link prepends a little-endian section length before the original
                 * big-endian ANLZ envelope; Beat Link skips this four-byte prefix. */
                for (unsigned i=0;i<4;++i) backend->analysis_payload[i]=(unsigned char)(length>>(i*8));
                memcpy(backend->analysis_payload+4,data,length); backend->analysis_length=(uint32_t)length+4;
            }
            hub_library_analysis_destroy(&backend->analysis); backend->phase=11;
        }
    }
    if (backend->phase==11) {
        if (backend->analysis_request==0x2b04) {
            reply->type=0x4e02; reply->count=5; reply->args[0].number=0x2b04;
            reply->args[2].number=backend->cues.size;
            reply->args[3]=(struct hub_db_field){.kind=HUB_DB_BINARY,.length=backend->cues.size,.data=backend->cues.data};
            reply->args[4].number=backend->cues.hot+backend->cues.memory; backend->phase=12; return 1;
        }
        if (backend->analysis_request==0x2104) {
            reply->type=0x4702; reply->count=9; reply->args[0].number=0x2104;
            reply->args[2].number=backend->cues.size;
            reply->args[3]=(struct hub_db_field){.kind=HUB_DB_BINARY,.length=backend->cues.size,.data=backend->cues.data};
            reply->args[4].number=36; reply->args[5].number=backend->cues.hot; reply->args[6].number=backend->cues.memory;
            reply->args[8]=(struct hub_db_field){.kind=HUB_DB_BINARY}; backend->phase=12; return 1;
        }
        reply->type=backend->analysis_request==0x2204 ? 0x4602 : backend->analysis_request==0x2904 ? 0x4a02 : 0x4f02;
        reply->count=5; reply->args[0].number=backend->analysis_request;
        if (backend->analysis_request==0x2004) { reply->type=0x4402; reply->count=4; }
        reply->args[2].number=backend->analysis_length;
        reply->args[3]=(struct hub_db_field){.kind=HUB_DB_BINARY,.length=backend->analysis_length,.data=backend->analysis_payload};
        reply->args[4].number=1; backend->phase=12; return 1;
    }
    if (backend->phase==1) {
        reply->type=backend->single_type; reply->count=2;
        reply->args[0].number=backend->single_first; reply->args[1].number=backend->single_second;
        backend->phase=0; return 1;
    }
    if (backend->phase==2) {
        reply->type=0x4001; reply->count=2; reply->args[0].number=1; reply->args[1].number=backend->offset;
        backend->phase=3; return 1;
    }
    if (backend->offset==backend->end) {
        reply->type=0x4201; reply->count=2; reply->args[0].number=1; backend->phase=0; return 1;
    }
    unsigned at=backend->offset++,id=0,kind=0,parent=0,position=0,artwork=0;
    const char *text=""; struct hub_db_library_view *view=backend->view;
    const struct hub_public_track_index *tracks=&view->library->metadata;
    const struct hub_entity_index *entities=&view->library->entities;
    const struct hub_public_track_row *track=NULL;
    switch (backend->current.kind) {
    case HUB_MENU_ROOT: text=at ? "PLAYLIST" : "TRACK"; kind=at ? 0x84 : 0x83; break;
    case HUB_MENU_TRACKS: track=view->titles[at].track; break;
    case HUB_MENU_FOLDER: {
        const struct hub_entity_row *row=view->tree[backend->current.first+at].row;
        text=entities->text+row->name; id=row->id; kind=row->folder ? 1 : 8; break;
    }
    case HUB_MENU_PLAYLIST: {
        const struct hub_entity_row *row=entities->rows+backend->current.first+at;
        track=hub_public_track_index_find(tracks,view->generation,row->track_id);
        if (!track) return -1;
        parent=backend->current.id; position=row->sort_order; break;
    }
    case HUB_MENU_METADATA: {
        track=hub_public_track_index_find(tracks,view->generation,backend->current.id);
        if (!track) return -1;
        if (!at) { artwork=track->artwork_id; break; }
        if (at<=5) {
            const unsigned tables[]={2,3,1,5,4},kinds[]={7,2,6,15,14};
            const uint32_t ids[]={track->artist_id,track->album_id,track->genre_id,track->key_id,track->label_id};
            id=ids[at-1]; kind=kinds[at-1]; const struct hub_entity_row *row=hub_library_entity(entities,tables[at-1],id);
            if (row) text=entities->text+row->name;
        } else if (at<=9) {
            const unsigned kinds[]={11,13,10,16}; const uint32_t values[]={track->duration,track->bpm100,track->rating,track->bitrate};
            kind=kinds[at-6]; id=values[at-6];
        } else { text=tracks->text+track->comment; kind=0x23; }
        track=NULL; break;
    }
    case HUB_MENU_LOADINFO: {
        track=hub_public_track_index_find(tracks,view->generation,backend->current.id);
        if (!track) return -1;
        switch (at) {
        case 0: id=1; kind=4; break;
        case 1: id=track->duration; kind=11; break;
        case 2: id=track->bpm100; kind=13; break;
        case 3: text=tracks->text+track->comment; id=track->id; kind=0x23; break;
        case 4: text=tracks->text+track->path; id=track->id; parent=track->file_size; break;
        case 5: id=1; kind=0x2f; break;
        case 6: {
            id=track->key_id; kind=15; const struct hub_entity_row *key=hub_library_entity(entities,5,id);
            if (key) text=entities->text+key->name;
            break;
        }
        }
        track=NULL; break;
    }
    }
    if (track) { text=tracks->text+track->title; id=track->id; kind=4; }
    uint32_t length; if (hub_db_menu_text(text,backend->text,&length)<0) return -1;
    static const unsigned char empty[2]={0,0};
    reply->type=0x4101; reply->count=12; reply->args[0].number=parent; reply->args[1].number=id;
    reply->args[2].number=length;
    reply->args[3]=(struct hub_db_field){.kind=HUB_DB_STRING,.length=length,.data=backend->text};
    reply->args[4].number=2; reply->args[5]=(struct hub_db_field){.kind=HUB_DB_STRING,.length=2,.data=empty};
    reply->args[6].number=kind; reply->args[8].number=artwork; reply->args[9].number=position;
    /* Native AZ setup [requester,20] requests the four trailing menu fields
     * also used by the independently exercised public-provider prototype. */
    if (backend->connection->extended_menu) {
        reply->count=16;
        reply->args[12]=(struct hub_db_field){.kind=HUB_DB_NUMBER,.number=0};
        reply->args[13]=(struct hub_db_field){.kind=HUB_DB_NUMBER,.number=2};
        reply->args[14]=(struct hub_db_field){.kind=HUB_DB_STRING,.length=2,.data=empty};
        const struct hub_public_track_row *extended_track=kind==4 ?
            hub_public_track_index_find(tracks,view->generation,
                backend->current.kind==HUB_MENU_LOADINFO ? backend->current.id : id) : NULL;
        reply->args[15]=(struct hub_db_field){.kind=HUB_DB_NUMBER,.number=extended_track ? extended_track->bpm100 : 0};
    }
    return 1;
}
#endif

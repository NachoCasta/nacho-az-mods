/* SPDX-License-Identifier: GPL-3.0-only */
/* Single-worker staged file export from one classic library. The root is a
 * dedicated public-service root, never a native/private library object.
 * Do not publish the catalog until ready_catalog returns non-NULL. */
#ifndef HUB_PUBLIC_LIBRARY_FILES_H
#define HUB_PUBLIC_LIBRARY_FILES_H
#include "public-pdb-source.h"
#include "public-file-catalog.h"
#include "public-library-metadata.h"
#include "library-path.h"
enum hub_public_library_state { HUB_LIBRARY_BUILDING,HUB_LIBRARY_READY,HUB_LIBRARY_FAILED };
struct hub_public_library_files {
    struct hub_public_pdb_reader reader;
    struct hub_public_pdb_source source;
    struct hub_public_file_catalog catalog;
    struct hub_public_track_index metadata;
    struct hub_entity_index entities;
    int initialized,canceled,state,error;
    unsigned tracks,missing_audio,missing_analysis,missing_artwork,missing_high_artwork,missing_names,table_cursor;
};
static int hub_public_library_relative(const char *input,char relative[4096]) {
    if (!input) { errno=EINVAL; return -1; }
    size_t length=strnlen(input,4096);
    if (!length || length==4096) { errno=EINVAL; return -1; }
    if (*input=='/') { ++input; --length; }
    if (!length || input[length-1]=='/' || memchr(input,'\\',length)) { errno=EINVAL; return -1; }
    for (size_t begin=0;begin<length;) {
        size_t end=begin;
        while (end<length && input[end]!='/') ++end;
        size_t count=end-begin;
        if (!count || count>255 || (count==1 && input[begin]=='.') ||
            (count==2 && input[begin]=='.' && input[begin+1]=='.')) { errno=EINVAL; return -1; }
        begin=end+1;
    }
    memcpy(relative,input,length); relative[length]=0; return 0;
}
/* Beat Link Util.highResolutionPath inserts _m before the final word extension.
 * Normalize first; only derive a sibling of the explicitly indexed image. */
static int hub_public_artwork_high_path(const char *input,char output[4096]) {
    if (hub_public_library_relative(input,output)<0) return -1;
    char *dot=strrchr(output,'.'),*slash=strrchr(output,'/');
    if (!dot || (slash && dot<slash) || !dot[1]) { errno=ENOENT; return 1; }
    for (const char *p=dot+1;*p;++p) if (!((*p>='a' && *p<='z') || (*p>='A' && *p<='Z') ||
            (*p>='0' && *p<='9') || *p=='_')) { errno=ENOENT; return 1; }
    size_t length=strlen(output);
    if (length>4093) { errno=ENAMETOOLONG; return -1; }
    memmove(dot+2,dot,strlen(dot)+1); dot[0]='_'; dot[1]='m'; return 0;
}
/* Returns1 for unavailable optional media,0 for registration,-1 for fatal
 * source/budget/path failure. Never scan or expose unrelated USB contents. */
static int hub_public_library_add(struct hub_public_library_files *library,const char *path) {
    if (__atomic_load_n(&library->canceled,__ATOMIC_ACQUIRE)) { errno=ECANCELED; return -1; }
    char relative[4096];
    if (hub_public_library_relative(path,relative)<0) return -1;
    size_t length=strlen(relative); unsigned char handle[32];
    if (hub_public_catalog_register(&library->catalog,relative,length,handle)<0) {
        if (errno==ENOENT || errno==ENOTDIR || errno==EACCES || errno==EPERM || errno==ELOOP) return 1;
        return -1;
    }
    for (size_t i=0;i<length;++i) if (relative[i]=='/') {
        relative[i]=0;
        int result=hub_public_catalog_register_node(&library->catalog,relative,i,1,handle);
        relative[i]='/'; if (result<0) return -1;
    }
    return 0;
}
static int hub_public_library_track(void *opaque,const struct hub_public_pdb_track *track) {
    struct hub_public_library_files *library=opaque;
    int result=hub_public_library_add(library,track->path);
    if (result<0) return -1;
    library->missing_audio+=(unsigned)result;
    if (track->analysis_path[0]) {
        result=hub_public_library_add(library,track->analysis_path);
        if (result<0) return -1;
        library->missing_analysis+=(unsigned)result;
        size_t length=strlen(track->analysis_path);
        if (length>=4 && !strcmp(track->analysis_path+length-4,".DAT")) {
            char companion[4096]; memcpy(companion,track->analysis_path,length+1);
            const char *extensions[]={".EXT",".2EX"};
            for (unsigned i=0;i<2;++i) {
                memcpy(companion+length-4,extensions[i],4);
                result=hub_public_library_add(library,companion);
                if (result<0) return -1;
                library->missing_analysis+=(unsigned)result;
            }
        }
    }
    if (hub_public_track_index_add(&library->metadata,track)<0) return -1;
    ++library->tracks; return 0;
}
static int hub_public_library_entity(void *opaque,const struct hub_public_pdb_entity *entity) {
    struct hub_public_library_files *library=opaque;
    if (entity->table==13) {
        int result=entity->name[0] ? hub_public_library_add(library,entity->name) : 1;
        if (result<0) return -1;
        library->missing_artwork+=(unsigned)result;
        char high[4096];
        result=entity->name[0] ? hub_public_artwork_high_path(entity->name,high) : 1;
        if (!result) result=hub_public_library_add(library,high);
        if (result<0) return -1;
        library->missing_high_artwork+=(unsigned)result;
    }
    return hub_entity_index_add(&library->entities,entity);
}
static int hub_public_library_fail(struct hub_public_library_files *library,int error) {
#ifdef LF_PUBLIC_LIBRARY_VALIDATION_TRACE
    if(library->error!=error) fprintf(stderr,"public library validation failed generation=%llu errno=%d\n",(unsigned long long)library->source.file.generation,error);
#endif
    library->error=error;
    hub_public_track_index_retire(&library->metadata);
    hub_entity_index_retire(&library->entities);
    if (library->initialized) hub_public_catalog_withdraw(&library->catalog);
    __atomic_store_n(&library->state,HUB_LIBRARY_FAILED,__ATOMIC_RELEASE);
    errno=error; return -1;
}
/* Always call destroy after init, including partial initialization failure. */
static int hub_public_library_files_init(struct hub_public_library_files *library,
        struct hub_public_root *root,uint64_t generation,const unsigned char nonce[16]) {
    memset(library,0,sizeof *library);
    if (hub_public_track_index_init(&library->metadata,generation)<0)
        return hub_public_library_fail(library,errno);
    if (hub_entity_index_init(&library->entities,generation)<0)
        return hub_public_library_fail(library,errno);
    if (hub_public_catalog_init(&library->catalog,root,generation,nonce)<0)
        return hub_public_library_fail(library,errno);
    library->initialized=1;
    char pdb[64]; unsigned char handle[32];
    struct hub_public_file_lease lease=HUB_PUBLIC_FILE_LEASE_INIT;
    if (hub_public_node_open(root,generation,"",0,1,&lease)<0)
        return hub_public_library_fail(library,errno);
    struct statfs filesystem;
    int result=fstatfs(lease.fd,&filesystem),error=errno;
    hub_public_file_finish(&lease);
    if (result<0) return hub_public_library_fail(library,error);
    snprintf(pdb,sizeof pdb,"%s/rekordbox/export.pdb",hub_library_directory(filesystem.f_type));
    if (hub_public_catalog_register_node(&library->catalog,"",0,1,handle)<0 ||
        hub_public_pdb_source_init(&library->source,root,generation,pdb,strlen(pdb))<0 ||
        hub_public_library_add(library,pdb)!=0 ||
        hub_public_pdb_init(&library->reader,(uint64_t)library->source.expected.st_size,
                            hub_public_pdb_source_read,&library->source)<0)
        return hub_public_library_fail(library,errno);
    return 0;
}
static int hub_public_library_files_step(struct hub_public_library_files *library) {
    if (__atomic_load_n(&library->canceled,__ATOMIC_ACQUIRE)) return hub_public_library_fail(library,ECANCELED);
    int state=__atomic_load_n(&library->state,__ATOMIC_ACQUIRE);
    if (state==HUB_LIBRARY_FAILED) { errno=library->error; return -1; }
    if (state==HUB_LIBRARY_READY) return 1;
    static const unsigned tables[]={0,1,2,3,4,5,7,8,13};
    const unsigned table_count=sizeof tables/sizeof *tables;
    /* At most one table page is decoded per step. Absent optional tables take
     * a step as well; dangling name IDs remain measurable absent metadata. */
    if (library->table_cursor<table_count && !library->reader.page) {
        if (hub_public_pdb_table_init(&library->reader,(uint64_t)library->source.expected.st_size,
                hub_public_pdb_source_read,&library->source,tables[library->table_cursor])<0) {
            if (errno!=ENOENT) return hub_public_library_fail(library,errno);
            hub_public_pdb_destroy(&library->reader); ++library->table_cursor; return 0;
        }
    }
    int result=1;
    if (library->table_cursor<table_count) {
        result=library->table_cursor ?
            hub_public_pdb_entities_step(&library->reader,hub_public_library_entity,library) :
            hub_public_pdb_step(&library->reader,hub_public_library_track,library);
        if (result==1) {
            hub_public_pdb_destroy(&library->reader); ++library->table_cursor;
            if (library->table_cursor<table_count) return 0;
        }
    }
    if (result<0) return hub_public_library_fail(library,errno);
    if (result==1) {
        unsigned char byte;
        if (__atomic_load_n(&library->canceled,__ATOMIC_ACQUIRE)) return hub_public_library_fail(library,ECANCELED);
        if (hub_public_track_index_seal(&library->metadata)<0)
            return hub_public_library_fail(library,errno);
        if (hub_entity_index_seal(&library->entities)<0 ||
            hub_library_metadata_validate(&library->metadata,&library->entities,&library->missing_names)<0)
            return hub_public_library_fail(library,errno);
        if (hub_public_pdb_source_read(&library->source,0,&byte,1)<0)
            return hub_public_library_fail(library,errno);
        if (__atomic_load_n(&library->canceled,__ATOMIC_ACQUIRE)) return hub_public_library_fail(library,ECANCELED);
        hub_public_pdb_destroy(&library->reader);
        __atomic_store_n(&library->state,HUB_LIBRARY_READY,__ATOMIC_RELEASE);
    }
    return result;
}
static struct hub_public_file_catalog *hub_public_library_ready_catalog(struct hub_public_library_files *library) {
    if (__atomic_load_n(&library->canceled,__ATOMIC_ACQUIRE) ||
        __atomic_load_n(&library->state,__ATOMIC_ACQUIRE)!=HUB_LIBRARY_READY) return NULL;
    return &library->catalog;
}
static const struct hub_public_track_index *hub_public_library_ready_tracks(struct hub_public_library_files *library) {
    if (!hub_public_library_ready_catalog(library) ||
        __atomic_load_n(&library->metadata.state,__ATOMIC_ACQUIRE)!=HUB_TRACK_READY) return NULL;
    return &library->metadata;
}
static const struct hub_entity_index *hub_public_library_ready_entities(struct hub_public_library_files *library) {
    if (!hub_public_library_ready_catalog(library) ||
        __atomic_load_n(&library->entities.state,__ATOMIC_ACQUIRE)!=HUB_ENTITY_READY) return NULL;
    return &library->entities;
}
/* Run on the same owning worker as step/dispatch, never concurrently with step.
 * A changed PDB retires this incarnation; rebuilding requires a fresh catalog. */
static int hub_public_library_files_validate(struct hub_public_library_files *library) {
    if (!hub_public_library_ready_catalog(library)) { errno=ESTALE; return -1; }
    unsigned char byte;
    if (hub_public_pdb_source_read(&library->source,0,&byte,1)<0)
        return hub_public_library_fail(library,errno);
    return 0;
}
/* May run concurrently with worker steps, after successful init. */
static void hub_public_library_files_cancel(struct hub_public_library_files *library) {
    __atomic_store_n(&library->canceled,1,__ATOMIC_RELEASE);
    hub_public_track_index_retire(&library->metadata);
    hub_entity_index_retire(&library->entities);
    hub_public_catalog_withdraw(&library->catalog);
}
/* Stop/join workers and dispatchers first. Root lifetime belongs to caller. */
static void hub_public_library_files_destroy(struct hub_public_library_files *library) {
    hub_public_pdb_destroy(&library->reader);
    hub_public_track_index_retire(&library->metadata);
    hub_public_track_index_destroy(&library->metadata);
    hub_entity_index_retire(&library->entities);
    hub_entity_index_destroy(&library->entities);
    if (library->initialized) {
        hub_public_catalog_withdraw(&library->catalog); hub_public_catalog_destroy(&library->catalog);
        library->initialized=0;
    }
}
#endif

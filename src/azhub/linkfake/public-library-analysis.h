/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HUB_PUBLIC_LIBRARY_ANALYSIS_H
#define HUB_PUBLIC_LIBRARY_ANALYSIS_H
#include "public-library-files.h"
#include "public-analysis-reader.h"
struct hub_library_analysis {
    struct hub_public_library_files *library;
    uint64_t generation;
    struct hub_public_pdb_source source;
    struct hub_analysis_reader reader;
};
static int hub_library_analysis_read(void *opaque,uint64_t offset,void *data,size_t length) {
    struct hub_library_analysis *analysis=opaque;
    if (!hub_public_library_ready_tracks(analysis->library) ||
        analysis->library->metadata.generation!=analysis->generation) { errno=ESTALE; return -1; }
    return hub_public_pdb_source_read(&analysis->source,offset,data,length);
}
static void hub_library_analysis_destroy(struct hub_library_analysis *analysis) {
    hub_analysis_destroy(&analysis->reader); memset(analysis,0,sizeof *analysis);
}
/* extension:0=DAT,1=EXT,2=2EX. Never accept a remote filesystem path. */
static int hub_library_analysis_init(struct hub_library_analysis *analysis,struct hub_public_library_files *library,
        uint32_t track_id,unsigned extension,uint32_t tag,unsigned occurrence) {
    memset(analysis,0,sizeof *analysis); analysis->library=library; analysis->generation=library->metadata.generation;
    if (extension>2) { errno=EINVAL; return -1; }
    if (hub_public_library_files_validate(library)<0) return -1;
    const struct hub_public_track_row *track=hub_public_track_index_find(&library->metadata,analysis->generation,track_id);
    if (!track) { errno=ENOENT; return -1; }
    const char *input=library->metadata.text+track->analysis_path; size_t length=strlen(input);
    if (!length) { errno=ENOENT; return -1; }
    if (length<4 || strcmp(input+length-4,".DAT")) { errno=EINVAL; return -1; }
    char path[4096],relative[4096];
    if (length>=sizeof path) { errno=ENAMETOOLONG; return -1; }
    memcpy(path,input,length+1);
    const char *extensions[]={"DAT","EXT","2EX"}; memcpy(path+length-3,extensions[extension],3);
    if (hub_public_library_relative(path,relative)<0) return -1;
    unsigned char handle[32];
    if (!hub_public_catalog_find(&library->catalog,relative,strlen(relative),handle)) { errno=ENOENT; return -1; }
    struct hub_public_file_ref registered;
    if (!hub_public_catalog_resolve(&library->catalog,handle,&registered) || registered.directory) { errno=ESTALE; return -1; }
    if (hub_public_pdb_source_init(&analysis->source,registered.owner,analysis->generation,relative,strlen(relative))<0) return -1;
    if (registered.device!=analysis->source.file.device || registered.inode!=analysis->source.file.inode) { errno=ESTALE; return -1; }
    return hub_analysis_init(&analysis->reader,(uint64_t)analysis->source.expected.st_size,
        hub_library_analysis_read,analysis,tag,occurrence);
}
static int hub_library_analysis_step(struct hub_library_analysis *analysis) {
    if (!hub_public_library_ready_tracks(analysis->library) || analysis->library->metadata.generation!=analysis->generation) {
        errno=ESTALE; return -1;
    }
    int result=hub_analysis_step(&analysis->reader);
    if (result==1 && hub_public_library_files_validate(analysis->library)<0) return -1;
    return result;
}
#endif

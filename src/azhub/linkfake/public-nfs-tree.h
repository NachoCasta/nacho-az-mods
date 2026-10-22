/* SPDX-License-Identifier: GPL-3.0-only */
/* Directory operations over trusted catalog entries only. Remote names never
 * authorize or register new filesystem paths. Wire-name decoding is separate. */
#ifndef HUB_PUBLIC_NFS_TREE_H
#define HUB_PUBLIC_NFS_TREE_H
#include "public-file-catalog.h"
#include "public-wire-name.h"
static int hub_public_reference_stat(const struct hub_public_file_ref *ref,struct stat *info) {
    struct hub_public_file_lease lease=HUB_PUBLIC_FILE_LEASE_INIT;
    if (hub_public_node_open(ref->owner,ref->generation,ref->relative,ref->length,ref->directory,&lease)<0) return -1;
    int result=fstat(lease.fd,info),error=errno;
    if (!result && (info->st_dev!=ref->device || info->st_ino!=ref->inode)) { result=-1; error=ESTALE; }
    hub_public_file_finish(&lease); errno=error; return result;
}
static void hub_public_tree_attributes(unsigned char *out,const struct hub_public_file_ref *ref,
                                       const struct stat *info) {
    uint32_t size=ref->directory ? 0 : (uint32_t)info->st_size;
    const uint32_t attributes[17]={ref->directory ? 2U : 1U,ref->directory ? 0040555U : 0100444U,
        1,0,0,size,4096,0,(uint32_t)(((uint64_t)size+511)/512),1,ref->file_id,
        hub_public_nfs_time(info->st_atim.tv_sec),(uint32_t)(info->st_atim.tv_nsec/1000),
        hub_public_nfs_time(info->st_mtim.tv_sec),(uint32_t)(info->st_mtim.tv_nsec/1000),
        hub_public_nfs_time(info->st_ctim.tv_sec),(uint32_t)(info->st_ctim.tv_nsec/1000)};
    for (unsigned i=0;i<17;++i) hub_public_xdr_word(out+i*4,attributes[i]);
}
static uint32_t hub_public_tree_stat(const struct hub_public_file_ref *ref,struct stat *info) {
    if (hub_public_reference_stat(ref,info)<0) return hub_public_nfs_error(errno);
    if (!ref->directory && (info->st_size<0 || (uint64_t)info->st_size>UINT32_MAX)) return 27;
    return 0;
}
static void hub_public_nfs_getattr(struct hub_public_file_catalog *catalog,const unsigned char handle[32],
                                  struct hub_public_nfs_reply *reply) {
    struct hub_public_file_ref ref;
    if (!hub_public_catalog_resolve(catalog,handle,&ref)) { hub_public_nfs_status(reply,70); return; }
    struct stat info; uint32_t status=hub_public_tree_stat(&ref,&info);
    hub_public_nfs_status(reply,status);
    if (!status) { hub_public_tree_attributes(reply->bytes+4,&ref,&info); reply->length=72; }
}
static void hub_public_nfs_lookup(struct hub_public_file_catalog *catalog,const unsigned char parent[32],
        const char *name,size_t length,struct hub_public_nfs_reply *reply) {
    struct hub_public_file_ref directory,child;
    if (!hub_public_catalog_resolve(catalog,parent,&directory)) { hub_public_nfs_status(reply,70); return; }
    struct stat info; uint32_t status=hub_public_tree_stat(&directory,&info);
    if (status) { hub_public_nfs_status(reply,status); return; }
    if (!directory.directory) { hub_public_nfs_status(reply,20); return; }
    if (!name || !length || length>255 || memchr(name,0,length) || memchr(name,'/',length) ||
        memchr(name,'\\',length) || (length==1 && name[0]=='.') ||
        (length==2 && name[0]=='.' && name[1]=='.')) { hub_public_nfs_status(reply,2); return; }
    size_t total=directory.length+(directory.length ? 1 : 0)+length;
    if (total>=HUB_PUBLIC_PATH_MAX) { hub_public_nfs_status(reply,63); return; }
    char path[HUB_PUBLIC_PATH_MAX]; memcpy(path,directory.relative,directory.length);
    size_t offset=directory.length;
    if (offset) path[offset++]='/';
    memcpy(path+offset,name,length); path[total]=0;
    unsigned char handle[32];
    if (!hub_public_catalog_find(catalog,path,total,handle)) { hub_public_nfs_status(reply,2); return; }
    if (!hub_public_catalog_resolve(catalog,handle,&child)) { hub_public_nfs_status(reply,70); return; }
    status=hub_public_tree_stat(&child,&info); hub_public_nfs_status(reply,status);
    if (!status) {
        memcpy(reply->bytes+4,handle,32); hub_public_tree_attributes(reply->bytes+36,&child,&info);
        reply->length=104;
    }
}
static void hub_public_nfs_readdir(struct hub_public_file_catalog *catalog,const unsigned char handle[32],
        uint32_t cookie,uint32_t count,struct hub_public_nfs_reply *reply) {
    struct hub_public_file_ref directory;
    if (!hub_public_catalog_resolve(catalog,handle,&directory)) { hub_public_nfs_status(reply,70); return; }
    struct stat info; uint32_t status=hub_public_tree_stat(&directory,&info);
    hub_public_nfs_status(reply,status); if (status) return;
    if (!directory.directory) { hub_public_nfs_status(reply,20); return; }
    if (count<8 || count>8192) { hub_public_nfs_status(reply,5); return; }
    pthread_mutex_lock(&catalog->mutex);
    if (catalog->withdrawn) { hub_public_nfs_status(reply,70); goto done; }
    if (cookie>catalog->count) { hub_public_nfs_status(reply,5); goto done; }
    size_t position=4; int eof=1;
    for (uint32_t index=cookie;index<catalog->count;++index) {
        const struct hub_public_catalog_entry *entry=&catalog->entries[index];
        size_t prefix=directory.length;
        if (entry->length<=prefix || (prefix && (memcmp(entry->relative,directory.relative,prefix) ||
            entry->relative[prefix]!='/'))) continue;
        if (prefix) ++prefix;
        const char *name=entry->relative+prefix; size_t length=entry->length-prefix;
        if (!length || memchr(name,'/',length)) continue;
        unsigned char wire[512]; size_t bytes;
        if (!hub_public_name_encode(name,length,wire,&bytes)) { hub_public_nfs_status(reply,5); goto done; }
        size_t padded=(bytes+3)&~(size_t)3;
        if (position-4+16+padded+8>count) {
            if (position==4) { hub_public_nfs_status(reply,5); goto done; }
            eof=0; break;
        }
        hub_public_xdr_word(reply->bytes+position,1);
        hub_public_xdr_word(reply->bytes+position+4,index+1);
        hub_public_xdr_word(reply->bytes+position+8,(uint32_t)bytes);
        memcpy(reply->bytes+position+12,wire,bytes);
        memset(reply->bytes+position+12+bytes,0,padded-bytes);
        hub_public_xdr_word(reply->bytes+position+12+padded,index+1);
        position+=16+padded;
    }
    hub_public_xdr_word(reply->bytes+position,0); hub_public_xdr_word(reply->bytes+position+4,(uint32_t)eof);
    reply->length=position+8;
done:
    pthread_mutex_unlock(&catalog->mutex);
}
#endif

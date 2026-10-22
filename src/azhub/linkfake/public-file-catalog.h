/* SPDX-License-Identifier: GPL-3.0-only */
/* Source-owned opaque handles. Caller supplies a fresh unpredictable 128-bit
 * instance nonce; never reuse it across reloads. Handles are not authorization. */
#ifndef HUB_PUBLIC_FILE_CATALOG_H
#define HUB_PUBLIC_FILE_CATALOG_H
#include <stdlib.h>
#include "public-nfs-read.h"
#define HUB_PUBLIC_CATALOG_MAX 65536
#define HUB_PUBLIC_CATALOG_PATH_BYTES_MAX (8U*1024U*1024U)
#define HUB_PUBLIC_CATALOG_BUCKETS 1024
struct hub_public_catalog_entry {
    dev_t device;
    ino_t inode;
    size_t length;
    char *relative;
    uint32_t next;
    int directory;
};
struct hub_public_file_catalog {
    pthread_mutex_t mutex;
    struct hub_public_root *root;
    uint64_t generation;
    unsigned char nonce[16];
    unsigned count,capacity;
    size_t path_bytes;
    int withdrawn;
    uint32_t buckets[HUB_PUBLIC_CATALOG_BUCKETS];
    struct hub_public_catalog_entry *entries;
};
static int hub_public_catalog_init(struct hub_public_file_catalog *catalog,
        struct hub_public_root *root,uint64_t generation,const unsigned char nonce[16]) {
    unsigned nonzero=0;
    if (!catalog || !root || !generation || !nonce) { errno=EINVAL; return -1; }
    for (unsigned i=0;i<16;++i) nonzero|=nonce[i];
    if (!nonzero) { errno=EINVAL; return -1; }
    memset(catalog,0,sizeof *catalog);
    int error=pthread_mutex_init(&catalog->mutex,NULL);
    if (error) { errno=error; return -1; }
    catalog->root=root; catalog->generation=generation;
    memcpy(catalog->nonce,nonce,16); return 0;
}
static void hub_public_catalog_handle(const struct hub_public_file_catalog *catalog,
                                     unsigned id,unsigned char handle[32]) {
    memcpy(handle,catalog->nonce,16);
    hub_public_xdr_word(handle+16,(uint32_t)(catalog->generation>>32));
    hub_public_xdr_word(handle+20,(uint32_t)catalog->generation);
    hub_public_xdr_word(handle+24,id);
    hub_public_xdr_word(handle+28,0x48554231);
}
static unsigned hub_public_catalog_bucket(const char *relative,size_t length) {
    uint32_t hash=2166136261U;
    for (size_t i=0;i<length;++i) hash=(hash^(unsigned char)relative[i])*16777619U;
    return hash%HUB_PUBLIC_CATALOG_BUCKETS;
}
/* Only trusted library/catalog paths may be registered. Never pass an unchecked
 * remote path here. No eviction: issued handles retain identity until withdrawal. */
static int hub_public_catalog_register_node(struct hub_public_file_catalog *catalog,
        const char *relative,size_t length,int directory,unsigned char handle[32]) {
    struct hub_public_file_ref reference={0};
    if (!relative || !handle || (!length && !directory) || length>=HUB_PUBLIC_PATH_MAX) { errno=EINVAL; return -1; }
    unsigned bucket=hub_public_catalog_bucket(relative,length);
    pthread_mutex_lock(&catalog->mutex);
    int result=-1,error=ESTALE;
    if (catalog->withdrawn) goto done;
    for (uint32_t id=catalog->buckets[bucket];id;id=catalog->entries[id-1].next) {
        unsigned i=id-1;
        if (catalog->entries[i].length==length &&
            !memcmp(catalog->entries[i].relative,relative,length)) {
            if (catalog->entries[i].directory!=!!directory) { error=EINVAL; goto done; }
            hub_public_catalog_handle(catalog,i+1,handle); result=0; goto done;
        }
    }
    if (catalog->count==HUB_PUBLIC_CATALOG_MAX ||
        length+1>HUB_PUBLIC_CATALOG_PATH_BYTES_MAX-catalog->path_bytes) { error=ENOSPC; goto done; }
    if (hub_public_node_capture(catalog->root,catalog->generation,relative,length,
                               catalog->count+1,directory,&reference)<0) { error=errno; goto done; }
    if (catalog->count==catalog->capacity) {
        unsigned capacity=catalog->capacity ? catalog->capacity*2 : 32;
        struct hub_public_catalog_entry *entries=realloc(catalog->entries,capacity*sizeof *entries);
        if (!entries) { error=ENOMEM; goto done; }
        catalog->entries=entries; catalog->capacity=capacity;
    }
    char *path=malloc(length+1);
    if (!path) { error=ENOMEM; goto done; }
    memcpy(path,reference.relative,length+1);
    struct hub_public_catalog_entry entry={reference.device,reference.inode,length,path,catalog->buckets[bucket],!!directory};
    catalog->entries[catalog->count++]=entry;
    catalog->buckets[bucket]=catalog->count;
    catalog->path_bytes+=length+1;
    hub_public_catalog_handle(catalog,catalog->count,handle); result=0;
done:
    pthread_mutex_unlock(&catalog->mutex);
    if (result<0) errno=error;
    return result;
}
static int hub_public_catalog_register(struct hub_public_file_catalog *catalog,
        const char *relative,size_t length,unsigned char handle[32]) {
    return hub_public_catalog_register_node(catalog,relative,length,0,handle);
}
static int hub_public_catalog_resolve(void *context,const unsigned char *handle,
                                     struct hub_public_file_ref *out) {
    struct hub_public_file_catalog *catalog=context;
    if (!catalog || !handle || !out) return 0;
    pthread_mutex_lock(&catalog->mutex);
    int found=0;
    uint32_t id=((uint32_t)handle[24]<<24)|((uint32_t)handle[25]<<16)|
                ((uint32_t)handle[26]<<8)|handle[27];
    if (!catalog->withdrawn && id && id<=catalog->count) {
        unsigned char expected[32];
        hub_public_catalog_handle(catalog,id,expected);
        if (!memcmp(expected,handle,32)) {
            const struct hub_public_catalog_entry *entry=&catalog->entries[id-1];
            out->owner=catalog->root; out->generation=catalog->generation;
            out->device=entry->device; out->inode=entry->inode;
            out->file_id=id; out->length=entry->length;
            out->directory=entry->directory;
            memcpy(out->relative,entry->relative,entry->length+1); found=1;
        }
    }
    pthread_mutex_unlock(&catalog->mutex); return found;
}
static void hub_public_catalog_withdraw(struct hub_public_file_catalog *catalog) {
    pthread_mutex_lock(&catalog->mutex); catalog->withdrawn=1;
    pthread_mutex_unlock(&catalog->mutex);
    hub_public_root_withdraw(catalog->root);
}
/* Caller must first stop/join dispatchers; references never retain file leases. */
static void hub_public_catalog_destroy(struct hub_public_file_catalog *catalog) {
    for (unsigned i=0;i<catalog->count;++i) free(catalog->entries[i].relative);
    free(catalog->entries); catalog->entries=NULL;
    catalog->count=catalog->capacity=0; catalog->path_bytes=0;
    pthread_mutex_destroy(&catalog->mutex);
}
static int hub_public_catalog_find(struct hub_public_file_catalog *catalog,const char *path,size_t length,
                                    unsigned char handle[32]) {
    unsigned bucket=hub_public_catalog_bucket(path,length);
    pthread_mutex_lock(&catalog->mutex);
    int found=0;
    if (!catalog->withdrawn) {
        for (uint32_t id=catalog->buckets[bucket];id;id=catalog->entries[id-1].next) {
            const struct hub_public_catalog_entry *entry=&catalog->entries[id-1];
            if (entry->length==length && !memcmp(entry->relative,path,length)) {
                hub_public_catalog_handle(catalog,id,handle); found=1; break;
            }
        }
    }
    pthread_mutex_unlock(&catalog->mutex); return found;
}
#endif

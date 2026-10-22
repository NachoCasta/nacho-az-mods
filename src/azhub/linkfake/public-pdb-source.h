/* SPDX-License-Identifier: GPL-3.0-only */
/* Generation-owned PDB read adapter. Never retain a descriptor across chunks
 * or worker steps. Caller must revalidate before publishing staged metadata. */
#ifndef HUB_PUBLIC_PDB_SOURCE_H
#define HUB_PUBLIC_PDB_SOURCE_H
#include "public-pdb-reader.h"
#include "public-nfs-read.h"
struct hub_public_pdb_source { struct hub_public_file_ref file; struct stat expected; };
static int hub_public_pdb_same(const struct stat *a,const struct stat *b) {
    return a->st_dev==b->st_dev && a->st_ino==b->st_ino && a->st_size==b->st_size &&
        a->st_mtim.tv_sec==b->st_mtim.tv_sec && a->st_mtim.tv_nsec==b->st_mtim.tv_nsec &&
        a->st_ctim.tv_sec==b->st_ctim.tv_sec && a->st_ctim.tv_nsec==b->st_ctim.tv_nsec;
}
static int hub_public_pdb_source_init(struct hub_public_pdb_source *source,struct hub_public_root *root,
        uint64_t generation,const char *relative,size_t length) {
    memset(source,0,sizeof *source);
    if (hub_public_file_capture(root,generation,relative,length,1,&source->file)<0) return -1;
    struct hub_public_file_lease lease=HUB_PUBLIC_FILE_LEASE_INIT;
    if (hub_public_file_open(root,generation,relative,length,&lease)<0) return -1;
    int result=fstat(lease.fd,&source->expected),error=errno;
    if (!result && (source->expected.st_dev!=source->file.device || source->expected.st_ino!=source->file.inode ||
        source->expected.st_size<28 || (uint64_t)source->expected.st_size>HUB_PDB_SIZE_MAX)) { result=-1; error=ESTALE; }
    hub_public_file_finish(&lease); errno=error; return result;
}
static int hub_public_pdb_source_read(void *opaque,uint64_t offset,void *buffer,size_t length) {
    struct hub_public_pdb_source *source=opaque;
    if (offset>(uint64_t)source->expected.st_size || length>(uint64_t)source->expected.st_size-offset) {
        errno=EINVAL; return -1;
    }
    size_t done=0;
    while (done<length) {
        struct hub_public_file_lease lease=HUB_PUBLIC_FILE_LEASE_INIT;
        const struct hub_public_file_ref *ref=&source->file;
        if (hub_public_file_open(ref->owner,ref->generation,ref->relative,ref->length,&lease)<0) return -1;
        struct stat info; int error=0;
        if (fstat(lease.fd,&info)<0) error=errno;
        else if (!hub_public_pdb_same(&info,&source->expected)) {
#ifdef LF_PUBLIC_LIBRARY_VALIDATION_TRACE
            fprintf(stderr,"PDB validation mismatch generation=%llu inode=%llu/%llu size=%llu/%llu mtime=%lld.%ld/%lld.%ld ctime=%lld.%ld/%lld.%ld\n",
                (unsigned long long)ref->generation,(unsigned long long)source->expected.st_ino,(unsigned long long)info.st_ino,
                (unsigned long long)source->expected.st_size,(unsigned long long)info.st_size,
                (long long)source->expected.st_mtim.tv_sec,source->expected.st_mtim.tv_nsec,(long long)info.st_mtim.tv_sec,info.st_mtim.tv_nsec,
                (long long)source->expected.st_ctim.tv_sec,source->expected.st_ctim.tv_nsec,(long long)info.st_ctim.tv_sec,info.st_ctim.tv_nsec);
#endif
            error=ESTALE;
        }
        size_t count=length-done;
        if (count>HUB_PUBLIC_READ_MAX) count=HUB_PUBLIC_READ_MAX;
        ssize_t bytes=-1;
        if (!error) {
            do { bytes=hub_public_file_read(&lease,(unsigned char *)buffer+done,count,(off_t)(offset+done)); }
            while (bytes<0 && errno==EINTR);
            if (bytes<=0) error=bytes<0 ? errno : EIO;
            else if (fstat(lease.fd,&info)<0) error=errno;
            else if (!hub_public_pdb_same(&info,&source->expected)) error=ESTALE;
        }
        hub_public_file_finish(&lease);
        if (error) { errno=error; return -1; }
        done+=(size_t)bytes;
    }
    return 0;
}
#endif

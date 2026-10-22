/* SPDX-License-Identifier: GPL-3.0-only */
/* NFSv2 READ body over authorized, source-owned references. RPC framing,
 * opaque handle ownership and catalog authorization belong to callers. */
#ifndef HUB_PUBLIC_NFS_READ_H
#define HUB_PUBLIC_NFS_READ_H
#include "public-file-root.h"
#define HUB_PUBLIC_NFS_READ_REPLY_MAX (76+HUB_PUBLIC_READ_MAX)
struct hub_public_file_ref {
    struct hub_public_root *owner;
    uint64_t generation;
    dev_t device;
    ino_t inode;
    uint32_t file_id;
    int directory;
    size_t length;
    char relative[HUB_PUBLIC_PATH_MAX];
};
struct hub_public_nfs_reply {
    size_t length;
    unsigned char bytes[HUB_PUBLIC_NFS_READ_REPLY_MAX];
};
static void hub_public_xdr_word(unsigned char *out,uint32_t word) {
    out[0]=(unsigned char)(word>>24); out[1]=(unsigned char)(word>>16);
    out[2]=(unsigned char)(word>>8); out[3]=(unsigned char)word;
}
static uint32_t hub_public_nfs_error(int error) {
    switch (error) {
    case ESTALE: case ENOENT: case ENOTDIR: case ELOOP: return 70;
    case EACCES: case EPERM: return 13;
    default: return 5;
    }
}
static void hub_public_nfs_status(struct hub_public_nfs_reply *reply,uint32_t status) {
    hub_public_xdr_word(reply->bytes,status); reply->length=4;
}
static int hub_public_node_capture(struct hub_public_root *root,uint64_t generation,
                                  const char *relative,size_t length,uint32_t identity,
                                  int directory,struct hub_public_file_ref *reference) {
    if (!identity) { errno=EINVAL; return -1; }
    struct hub_public_file_lease lease=HUB_PUBLIC_FILE_LEASE_INIT;
    if (hub_public_node_open(root,generation,relative,length,directory,&lease)<0) return -1;
    struct stat info;
    int result=fstat(lease.fd,&info),error=errno;
    if (!result) {
        reference->owner=root; reference->generation=generation;
        reference->device=info.st_dev; reference->inode=info.st_ino;
        reference->file_id=identity; reference->length=length;
        reference->directory=!!directory;
        memcpy(reference->relative,relative,length); reference->relative[length]=0;
    }
    hub_public_file_finish(&lease); errno=error;
    return result;
}
static int hub_public_file_capture(struct hub_public_root *root,uint64_t generation,
        const char *relative,size_t length,uint32_t identity,struct hub_public_file_ref *reference) {
    return hub_public_node_capture(root,generation,relative,length,identity,0,reference);
}
static uint32_t hub_public_nfs_time(time_t seconds) {
    if (seconds<0) return 0;
    return (uint64_t)seconds>UINT32_MAX ? UINT32_MAX : (uint32_t)seconds;
}
static void hub_public_nfs_read(struct hub_public_root *root,const struct hub_public_file_ref *reference,
                                uint32_t offset,uint32_t count,struct hub_public_nfs_reply *reply) {
    hub_public_nfs_status(reply,5);
    if (!reference || reference->owner!=root || !reference->file_id) {
        hub_public_nfs_status(reply,70); return;
    }
    if (reference->directory) { hub_public_nfs_status(reply,21); return; }
    if (count>HUB_PUBLIC_READ_MAX) return;
    struct hub_public_file_lease lease=HUB_PUBLIC_FILE_LEASE_INIT;
    if (hub_public_file_open(root,reference->generation,reference->relative,reference->length,&lease)<0) {
        hub_public_nfs_status(reply,hub_public_nfs_error(errno)); return;
    }
    struct stat info;
    if (fstat(lease.fd,&info)<0) { hub_public_nfs_status(reply,5); goto done; }
    if (info.st_dev!=reference->device || info.st_ino!=reference->inode) {
        hub_public_nfs_status(reply,70); goto done;
    }
    if (info.st_size<0 || (uint64_t)info.st_size>UINT32_MAX) {
        hub_public_nfs_status(reply,27); goto done;
    }
    ssize_t bytes=hub_public_file_read(&lease,reply->bytes+76,count,(off_t)offset);
    if (bytes<0) { hub_public_nfs_status(reply,hub_public_nfs_error(errno)); goto done; }
    /* RFC1094 fattr: regular read-only file, synthetic uid/gid/fsid/fileid.
     * Never serialize host pointers or raw native library object identities. */
    uint32_t attributes[17]={1,0100444,1,0,0,(uint32_t)info.st_size,4096,0,
        (uint32_t)(((uint64_t)info.st_size+511)/512),1,reference->file_id,
        hub_public_nfs_time(info.st_atim.tv_sec),(uint32_t)(info.st_atim.tv_nsec/1000),
        hub_public_nfs_time(info.st_mtim.tv_sec),(uint32_t)(info.st_mtim.tv_nsec/1000),
        hub_public_nfs_time(info.st_ctim.tv_sec),(uint32_t)(info.st_ctim.tv_nsec/1000)};
    hub_public_xdr_word(reply->bytes,0);
    for (unsigned i=0;i<17;++i) hub_public_xdr_word(reply->bytes+4+i*4,attributes[i]);
    hub_public_xdr_word(reply->bytes+72,(uint32_t)bytes);
    size_t padded=((size_t)bytes+3)&~(size_t)3;
    memset(reply->bytes+76+bytes,0,padded-(size_t)bytes);
    reply->length=76+padded;
done:
    /* Release before network send; a slow peer must not pin the USB mount. */
    hub_public_file_finish(&lease);
}
#endif

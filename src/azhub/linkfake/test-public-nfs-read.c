/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include "public-nfs-read.h"
static uint32_t word(const unsigned char *data) {
    return ((uint32_t)data[0]<<24)|((uint32_t)data[1]<<16)|((uint32_t)data[2]<<8)|data[3];
}
static void put_file(const char *name,const unsigned char *bytes,size_t size) {
    int fd=open(name,O_CREAT|O_EXCL|O_WRONLY,0600); assert(fd>=0);
    assert(write(fd,bytes,size)==(ssize_t)size); assert(!close(fd));
}
int main(int argc,char **argv) {
    if (argc==6 && !strcmp(argv[1],"emit")) {
        struct hub_public_root root; struct hub_public_file_ref reference;
        struct hub_public_nfs_reply reply; assert(!hub_public_root_init(&root));
        assert(!hub_public_root_attach(&root,argv[2],1));
        assert(!hub_public_file_capture(&root,1,argv[3],strlen(argv[3]),7,&reference));
        char *end; unsigned long offset=strtoul(argv[4],&end,10); assert(!*end && offset<=UINT32_MAX);
        unsigned long count=strtoul(argv[5],&end,10); assert(!*end && count<=HUB_PUBLIC_READ_MAX);
        hub_public_nfs_read(&root,&reference,(uint32_t)offset,(uint32_t)count,&reply);
        assert(fwrite(reply.bytes,1,reply.length,stdout)==reply.length);
        hub_public_root_withdraw(&root); assert(hub_public_root_drained(&root));
        assert(!hub_public_root_destroy(&root)); return 0;
    }
    assert(argc==1);
    char directory[]="/tmp/azhub-nfs-read.XXXXXX"; assert(mkdtemp(directory));
    char path[512],old[512]; snprintf(path,sizeof path,"%s/Track.bin",directory);
    snprintf(old,sizeof old,"%s/old.bin",directory);
    unsigned char content[10003]; for (size_t i=0;i<sizeof content;++i) content[i]=(unsigned char)(i*17);
    put_file(path,content,sizeof content);
    struct hub_public_root root,neighbor; assert(!hub_public_root_init(&root));
    assert(!hub_public_root_init(&neighbor));
    assert(!hub_public_root_attach(&root,directory,1) && !hub_public_root_attach(&neighbor,directory,1));
    struct hub_public_file_ref reference,neighbor_ref;
    assert(!hub_public_file_capture(&root,1,"Track.bin",9,2,&reference));
    assert(!hub_public_file_capture(&neighbor,1,"Track.bin",9,2,&neighbor_ref));
    struct hub_public_nfs_reply reply;
    const uint32_t counts[]={0,1,3,8191,8192};
    for (unsigned i=0;i<sizeof counts/sizeof *counts;++i) {
        memset(&reply,0xcc,sizeof reply);
        hub_public_nfs_read(&root,&reference,0,counts[i],&reply);
        assert(word(reply.bytes)==0 && word(reply.bytes+4)==1);
        assert(word(reply.bytes+8)==0100444 && word(reply.bytes+24)==sizeof content);
        assert(word(reply.bytes+44)==2 && word(reply.bytes+72)==counts[i]);
        assert(reply.length==76+((counts[i]+3)&~3u));
        assert(!memcmp(reply.bytes+76,content,counts[i]));
        for (size_t p=76+counts[i];p<reply.length;++p) assert(!reply.bytes[p]);
    }
    hub_public_nfs_read(&root,&reference,10000,8192,&reply);
    assert(word(reply.bytes+72)==3 && !memcmp(reply.bytes+76,content+10000,3));
    hub_public_nfs_read(&root,&reference,sizeof content,8192,&reply);
    assert(word(reply.bytes)==0 && reply.length==76 && !word(reply.bytes+72));
    hub_public_nfs_read(&root,&reference,0,8193,&reply); assert(reply.length==4 && word(reply.bytes)==5);
    hub_public_nfs_read(&root,&neighbor_ref,0,1,&reply); assert(reply.length==4 && word(reply.bytes)==70);
    assert(!rename(path,old)); put_file(path,(const unsigned char *)"replacement",11);
    hub_public_nfs_read(&root,&reference,0,1,&reply); assert(reply.length==4 && word(reply.bytes)==70);
    hub_public_root_withdraw(&root); assert(hub_public_root_drained(&root));
    hub_public_nfs_read(&root,&reference,0,1,&reply); assert(word(reply.bytes)==70);
    assert(!hub_public_file_capture(&neighbor,1,"Track.bin",9,3,&neighbor_ref));
    hub_public_nfs_read(&neighbor,&neighbor_ref,0,11,&reply);
    assert(word(reply.bytes)==0 && word(reply.bytes+72)==11 && !memcmp(reply.bytes+76,"replacement",11));
    hub_public_root_withdraw(&neighbor); assert(hub_public_root_drained(&neighbor));
    assert(!hub_public_root_destroy(&root) && !hub_public_root_destroy(&neighbor));
    assert(!unlink(path) && !unlink(old) && !rmdir(directory));
    puts("PASS: NFSv2 READ layout, partial/EOF/padding, capacity, source ownership, inode replacement and withdrawal");
    return 0;
}

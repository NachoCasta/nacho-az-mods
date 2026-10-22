/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include "public-file-catalog.h"
struct reader_context { struct hub_public_file_catalog *catalog; unsigned char handle[32]; };
static void *reader(void *opaque) {
    struct reader_context *context=opaque;
    for (unsigned i=0;i<1000;++i) {
        struct hub_public_file_ref ref;
        if (hub_public_catalog_resolve(context->catalog,context->handle,&ref)) {
            struct hub_public_nfs_reply reply;
            hub_public_nfs_read(context->catalog->root,&ref,0,3,&reply);
            assert(reply.length==80 || reply.length==4);
        }
    }
    return NULL;
}
int main(void) {
    char directory[]="/tmp/azhub-catalog.XXXXXX"; assert(mkdtemp(directory));
    char path[512]; snprintf(path,sizeof path,"%s/track",directory);
    int fd=open(path,O_CREAT|O_EXCL|O_WRONLY,0600); assert(fd>=0);
    assert(write(fd,"abc",3)==3 && !close(fd));
    struct hub_public_root root,neighbor;
    assert(!hub_public_root_init(&root) && !hub_public_root_init(&neighbor));
    assert(!hub_public_root_attach(&root,directory,1));
    assert(!hub_public_root_attach(&neighbor,directory,1));
    struct hub_public_file_catalog *a=calloc(1,sizeof *a),*b=calloc(1,sizeof *b);
    assert(a && b);
    unsigned char nonce[16]={1},other_nonce[16]={2},zero[16]={0},handle[32],again[32];
    assert(hub_public_catalog_init(a,&root,1,zero)<0 && errno==EINVAL);
    assert(!hub_public_catalog_init(a,&root,1,nonce));
    assert(!hub_public_catalog_init(b,&neighbor,1,other_nonce));
    assert(!hub_public_catalog_register(a,"track",5,handle));
    assert(!hub_public_catalog_register(a,"track",5,again));
    assert(!memcmp(handle,again,32) && a->count==1 && !root.active);
    struct hub_public_file_ref ref;
    assert(hub_public_catalog_resolve(a,handle,&ref));
    assert(!hub_public_catalog_resolve(b,handle,&ref));
    for (unsigned i=0;i<32;++i) {
        memcpy(again,handle,32); again[i]^=1;
        assert(!hub_public_catalog_resolve(a,again,&ref));
    }
    assert(hub_public_catalog_register(a,"../track",8,again)<0);
    assert(!hub_public_catalog_register(b,"track",5,again));
    /* Grow beyond the old256-entry bound and across repeated reallocations.
     * Hard links are synthetic distinct authorized paths, not a USB library. */
    for (unsigned i=0;i<1024;++i) {
        char name[32],alias[512]; unsigned char grown[32];
        snprintf(name,sizeof name,"track-%u",i);
        snprintf(alias,sizeof alias,"%s/%s",directory,name);
        assert(!link(path,alias));
        assert(!hub_public_catalog_register(a,name,strlen(name),grown));
        assert(hub_public_catalog_resolve(a,grown,&ref));
        struct hub_public_nfs_reply body; hub_public_nfs_read(&root,&ref,0,3,&body);
        assert(body.length==80 && !memcmp(body.bytes+76,"abc",3));
        assert(!unlink(alias));
    }
    assert(a->count==1025 && a->capacity==2048);
    assert(a->path_bytes<16384 && sizeof *a<16384);
    assert(hub_public_catalog_resolve(a,handle,&ref));
    unsigned char duplicate[32];
    assert(!hub_public_catalog_register(a,"track",5,duplicate));
    assert(!memcmp(duplicate,handle,32) && a->count==1025);
    struct reader_context context={.catalog=a}; memcpy(context.handle,handle,32);
    pthread_t thread; assert(!pthread_create(&thread,NULL,reader,&context));
    hub_public_catalog_withdraw(a); assert(!pthread_join(thread,NULL));
    assert(hub_public_root_drained(&root));
    assert(!hub_public_catalog_resolve(a,handle,&ref));
    assert(hub_public_catalog_register(a,"track",5,handle)<0 && errno==ESTALE);
    assert(hub_public_catalog_resolve(b,again,&ref));
    struct hub_public_nfs_reply reply; hub_public_nfs_read(&neighbor,&ref,0,3,&reply);
    assert(reply.length==80 && !memcmp(reply.bytes+76,"abc",3));
    hub_public_catalog_destroy(a);
    assert(!hub_public_root_attach(&root,directory,2));
    assert(!hub_public_catalog_init(a,&root,2,nonce));
    assert(!hub_public_catalog_register(a,"track",5,again));
    assert(!hub_public_catalog_resolve(a,context.handle,&ref));
    hub_public_catalog_withdraw(a); hub_public_catalog_withdraw(b);
    hub_public_catalog_destroy(a); hub_public_catalog_destroy(b);
    hub_public_root_destroy(&root); hub_public_root_destroy(&neighbor);
    free(a); free(b); assert(!unlink(path) && !rmdir(directory));
    puts("PASS:1025 source-owned handles, compact dynamic growth, stable deduplication, tamper/neighbor rejection, generation and concurrent withdrawal");
}

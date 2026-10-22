/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include "public-rpc-export.h"
static void request(unsigned char *data,uint32_t program,uint32_t version,uint32_t procedure) {
    const uint32_t header[]={17,0,2,program,version,procedure,0,0,0,0};
    memset(data,0,128);
    for (unsigned i=0;i<10;++i) hub_public_rpc_store(data+i*4,header[i]);
}
int main(void) {
    char directory[]="/tmp/azhub-export.XXXXXX"; assert(mkdtemp(directory));
    struct hub_public_root root; assert(!hub_public_root_init(&root));
    assert(!hub_public_root_attach(&root,directory,1));
    struct hub_public_file_catalog catalog; unsigned char nonce[16]={1},handle[32],call[128];
    assert(!hub_public_catalog_init(&catalog,&root,1,nonce));
    assert(!hub_public_catalog_register_node(&catalog,"",0,1,handle));
    struct hub_public_export export={.catalog=&catalog,.mount_port=40000,.nfs_port=2049}; struct hub_public_rpc_reply reply;
    request(call,100000,2,3);
    hub_public_rpc_store(call+40,100005); hub_public_rpc_store(call+44,1); hub_public_rpc_store(call+48,17);
    for (size_t size=0;size<56;++size) {
        hub_public_rpc_export_dispatch(&export,call,size,&reply);
        assert(reply.length!=28 && !root.active);
    }
    hub_public_rpc_export_dispatch(&export,call,56,&reply);
    assert(reply.length==28 && hub_public_rpc_word(reply.bytes+24)==40000);
    hub_public_rpc_store(call+40,100003); hub_public_rpc_store(call+44,2);
    hub_public_rpc_export_dispatch(&export,call,56,&reply);
    assert(hub_public_rpc_word(reply.bytes+24)==2049);
    hub_public_rpc_store(call+48,6); hub_public_rpc_export_dispatch(&export,call,56,&reply);
    assert(hub_public_rpc_word(reply.bytes+24)==0);
    request(call,100000,2,1); hub_public_rpc_export_dispatch(&export,call,40,&reply);
    assert(reply.length==24 && hub_public_rpc_word(reply.bytes+20)==3); /* No SET. */
    request(call,100005,1,5); hub_public_rpc_export_dispatch(&export,call,40,&reply);
    assert(reply.length==44 && !memcmp(reply.bytes+32,"/C/\0",4));
    assert(!hub_public_export_set_group(&export,"192.0.2.2/255.255.0.0"));
    hub_public_rpc_export_dispatch(&export,call,40,&reply);
    assert(reply.length==76 && hub_public_rpc_word(reply.bytes+36)==1 && hub_public_rpc_word(reply.bytes+40)==21);
    assert(!memcmp(reply.bytes+44,"192.0.2.2/255.255.0.0",21));
    assert(!hub_public_rpc_word(reply.bytes+68) && !hub_public_rpc_word(reply.bytes+72));
    assert(hub_public_export_set_group(&export,"192.0.2.2/255.0.255.0")<0);
    assert(hub_public_export_set_group(&export,"bad/group")<0);
    assert(!hub_public_export_set_group(&export,NULL));
    request(call,100005,1,1); hub_public_rpc_store(call+40,3); memcpy(call+44,"/C/",3);
    hub_public_rpc_export_dispatch(&export,call,48,&reply);
    assert(reply.length==60 && !memcmp(reply.bytes+28,handle,32));
    /* UTF16LE mount path, including terminal NUL. */
    const unsigned char path[]={'/',0,'C',0,'/',0,0,0};
    hub_public_rpc_store(call+40,8); memcpy(call+44,path,8);
    hub_public_rpc_export_dispatch(&export,call,52,&reply); assert(reply.length==60);
    hub_public_rpc_store(call+20,3); hub_public_rpc_export_dispatch(&export,call,52,&reply);
    assert(reply.length==24 && !hub_public_rpc_word(reply.bytes+20));
    struct hub_public_file_ref ref; assert(hub_public_catalog_resolve(&catalog,handle,&ref));
    hub_public_rpc_store(call+20,1); hub_public_rpc_store(call+16,3);
    hub_public_rpc_export_dispatch(&export,call,52,&reply);
    assert(reply.length==32 && hub_public_rpc_word(reply.bytes+20)==2 && hub_public_rpc_word(reply.bytes+24)==1);
    hub_public_catalog_withdraw(&catalog);
    hub_public_rpc_store(call+16,1); hub_public_rpc_export_dispatch(&export,call,52,&reply);
    assert(reply.length==28 && hub_public_rpc_word(reply.bytes+24)==2);
    request(call,100005,1,5); hub_public_rpc_export_dispatch(&export,call,40,&reply);
    assert(reply.length==28 && !hub_public_rpc_word(reply.bytes+24));
    hub_public_catalog_destroy(&catalog); assert(!hub_public_root_destroy(&root)); assert(!rmdir(directory));
    puts("PASS: read-only portmapper, ASCII/UTF16LE mount/export, malformed/version bounds, peer UMNT isolation and withdrawal");
}

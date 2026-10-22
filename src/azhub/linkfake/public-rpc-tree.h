/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HUB_PUBLIC_RPC_TREE_H
#define HUB_PUBLIC_RPC_TREE_H
#include "public-rpc-read.h"
#include "public-nfs-tree.h"
#include "public-wire-name.h"
static void hub_public_rpc_tree_dispatch(void *opaque,const void *data,size_t length,
                                         struct hub_public_rpc_reply *reply) {
    struct hub_public_file_catalog *catalog=opaque;
    struct hub_public_rpc_call call;
    if (!hub_public_rpc_nfs_begin(data,length,&call,reply)) return;
    if (call.procedure==0 || call.procedure==6) {
        hub_public_rpc_read_dispatch(catalog->root,data,length,hub_public_catalog_resolve,catalog,reply); return;
    }
    uint32_t status=0; size_t name_length=0; char name[256];
    if (call.procedure!=1 && call.procedure!=4 && call.procedure!=16 && call.procedure!=17) status=3;
    else if (call.procedure==1 && call.arguments.length!=32) status=4;
    else if (call.procedure==16 && (call.arguments.length!=40 ||
             hub_public_rpc_word(call.arguments.data+36)>8192)) status=4;
    else if (call.procedure==17 && call.arguments.length!=32) status=4;
    else if (call.procedure==4) {
        if (call.arguments.length<36) status=4;
        else {
            uint32_t bytes=hub_public_rpc_word(call.arguments.data+32);
            if (bytes>1024 || call.arguments.length!=36+(((size_t)bytes+3)&~(size_t)3) ||
                !hub_public_name_decode(call.arguments.data+36,bytes,name,&name_length)) status=4;
        }
    }
    if (status) { reply->length=hub_public_rpc_accept(reply->bytes,sizeof reply->bytes,call.xid,status); return; }
    struct hub_public_nfs_reply body;
    if (call.procedure==1) hub_public_nfs_getattr(catalog,call.arguments.data,&body);
    else if (call.procedure==4) hub_public_nfs_lookup(catalog,call.arguments.data,name,name_length,&body);
    else if (call.procedure==16) hub_public_nfs_readdir(catalog,call.arguments.data,
        hub_public_rpc_word(call.arguments.data+32),hub_public_rpc_word(call.arguments.data+36),&body);
    else {
        /* Read-only export capacity is synthetic; never expose host disk usage. */
        hub_public_nfs_getattr(catalog,call.arguments.data,&body);
        if (!hub_public_rpc_word(body.bytes)) {
            const uint32_t values[]={0,8192,4096,1,0,0};
            for (unsigned i=0;i<6;++i) hub_public_xdr_word(body.bytes+i*4,values[i]);
            body.length=24;
        }
    }
    memcpy(reply->bytes+24,body.bytes,body.length); reply->length=24+body.length;
}
#endif

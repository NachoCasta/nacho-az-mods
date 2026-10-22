/* SPDX-License-Identifier: GPL-3.0-only */
/* Read-only NFS NULL/READ RPC dispatch. Socket peer policy and opaque handle
 * ownership stay with the source-specific caller/catalog. AUTH_SYS fields
 * are syntax-checked assertions, never used as trusted OS credentials. */
#ifndef HUB_PUBLIC_RPC_READ_H
#define HUB_PUBLIC_RPC_READ_H
#include "public-rpc.h"
#include "public-nfs-read.h"
#define HUB_PUBLIC_RPC_READ_REPLY_MAX (24+HUB_PUBLIC_NFS_READ_REPLY_MAX)
struct hub_public_rpc_reply { size_t length; unsigned char bytes[HUB_PUBLIC_RPC_READ_REPLY_MAX]; };
typedef int (*hub_public_file_resolver)(void *,const unsigned char *,struct hub_public_file_ref *);
static int hub_public_rpc_credential_valid(const struct hub_public_rpc_auth *auth) {
    if (!auth->flavor) return !auth->body.length;
    if (auth->flavor!=1) return 0;
    struct hub_public_rpc_cursor cursor={auth->body.data,auth->body.length,0};
    uint32_t ignored,length,groups;
    if (!hub_public_rpc_take_word(&cursor,&ignored) || !hub_public_rpc_take_word(&cursor,&length) || length>255) return 0;
    size_t padded=((size_t)length+3)&~(size_t)3;
    if (padded>cursor.length-cursor.offset) return 0;
    cursor.offset+=padded;
    if (!hub_public_rpc_take_word(&cursor,&ignored) || !hub_public_rpc_take_word(&cursor,&ignored) ||
        !hub_public_rpc_take_word(&cursor,&groups) || groups>16) return 0;
    return cursor.length-cursor.offset==(size_t)groups*4;
}
static void hub_public_rpc_auth_error(struct hub_public_rpc_reply *reply,uint32_t xid,uint32_t reason) {
    const uint32_t words[5]={xid,1,1,1,reason};
    for (unsigned i=0;i<5;++i) hub_public_rpc_store(reply->bytes+i*4,words[i]);
    reply->length=20;
}
static int hub_public_rpc_program_begin(const void *data,size_t length,uint32_t program,uint32_t version,
                                   struct hub_public_rpc_call *call,
                                   struct hub_public_rpc_reply *reply) {
    reply->length=0;
    enum hub_public_rpc_result decoded=hub_public_rpc_decode(data,length,call);
    if (decoded==HUB_RPC_WRONG_VERSION) {
        reply->length=hub_public_rpc_version_reject(reply->bytes,sizeof reply->bytes,call->xid); return 0;
    }
    if (decoded!=HUB_RPC_CALL) return 0;
    if (!hub_public_rpc_credential_valid(&call->credential)) {
        hub_public_rpc_auth_error(reply,call->xid,call->credential.flavor>1 ? 5 : 1); return 0;
    }
    if (call->verifier.flavor || call->verifier.body.length) { hub_public_rpc_auth_error(reply,call->xid,3); return 0; }
    uint32_t status=0;
    if (call->program!=program) status=1;
    else if (call->version!=version) status=2;
    reply->length=hub_public_rpc_accept(reply->bytes,sizeof reply->bytes,call->xid,status);
    if (status==2) {
        hub_public_rpc_store(reply->bytes+24,version); hub_public_rpc_store(reply->bytes+28,version); reply->length=32;
    }
    return !status;
}
static int hub_public_rpc_nfs_begin(const void *data,size_t length,struct hub_public_rpc_call *call,
                                   struct hub_public_rpc_reply *reply) {
    return hub_public_rpc_program_begin(data,length,100003,2,call,reply);
}
static void hub_public_rpc_read_dispatch(struct hub_public_root *root,const void *data,size_t length,
                                        hub_public_file_resolver resolve,void *context,
                                        struct hub_public_rpc_reply *reply) {
    struct hub_public_rpc_call call;
    if (!hub_public_rpc_nfs_begin(data,length,&call,reply)) return;
    uint32_t status=0;
    if (call.procedure!=0 && call.procedure!=6) status=3;
    else if ((call.procedure==0 && call.arguments.length) ||
             (call.procedure==6 && (call.arguments.length!=44 ||
              hub_public_rpc_word(call.arguments.data+36)>HUB_PUBLIC_READ_MAX))) status=4;
    reply->length=hub_public_rpc_accept(reply->bytes,sizeof reply->bytes,call.xid,status);
    if (status || !call.procedure) return;
    struct hub_public_file_ref reference={0};
    if (!resolve || !resolve(context,call.arguments.data,&reference)) {
        hub_public_rpc_store(reply->bytes+24,70); reply->length=28; return;
    }
    struct hub_public_nfs_reply body;
    hub_public_nfs_read(root,&reference,hub_public_rpc_word(call.arguments.data+32),
                        hub_public_rpc_word(call.arguments.data+36),&body);
    memcpy(reply->bytes+24,body.bytes,body.length); reply->length+=body.length;
}
#endif

/* SPDX-License-Identifier: GPL-3.0-only */
/* Bounded ONC RPCv2 datagram decoding. Auth fields are parsed, not trusted.
 * Dispatchers must decide credential policy and validate procedure arguments. */
#ifndef HUB_PUBLIC_RPC_H
#define HUB_PUBLIC_RPC_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#define HUB_PUBLIC_RPC_AUTH_MAX 400
#define HUB_PUBLIC_RPC_DATAGRAM_MAX 65507
struct hub_public_rpc_span { const unsigned char *data; size_t length; };
struct hub_public_rpc_auth { uint32_t flavor; struct hub_public_rpc_span body; };
struct hub_public_rpc_call {
    uint32_t xid,program,version,procedure;
    struct hub_public_rpc_auth credential,verifier;
    struct hub_public_rpc_span arguments;
};
enum hub_public_rpc_result { HUB_RPC_BAD=0,HUB_RPC_CALL=1,HUB_RPC_WRONG_VERSION=2,HUB_RPC_IGNORE=3 };
struct hub_public_rpc_cursor { const unsigned char *data; size_t length,offset; };
static uint32_t hub_public_rpc_word(const unsigned char *data) {
    return ((uint32_t)data[0]<<24)|((uint32_t)data[1]<<16)|((uint32_t)data[2]<<8)|data[3];
}
static int hub_public_rpc_take_word(struct hub_public_rpc_cursor *cursor,uint32_t *word) {
    if (cursor->offset>cursor->length || cursor->length-cursor->offset<4) return 0;
    *word=hub_public_rpc_word(cursor->data+cursor->offset); cursor->offset+=4; return 1;
}
static int hub_public_rpc_take_auth(struct hub_public_rpc_cursor *cursor,struct hub_public_rpc_auth *auth) {
    uint32_t length;
    if (!hub_public_rpc_take_word(cursor,&auth->flavor) || !hub_public_rpc_take_word(cursor,&length) ||
        length>HUB_PUBLIC_RPC_AUTH_MAX) return 0;
    size_t padded=((size_t)length+3)&~(size_t)3;
    if (cursor->offset>cursor->length || padded>cursor->length-cursor->offset) return 0;
    auth->body.data=cursor->data+cursor->offset; auth->body.length=length;
    cursor->offset+=padded; return 1;
}
static enum hub_public_rpc_result hub_public_rpc_decode(const void *data,size_t length,
                                                         struct hub_public_rpc_call *out) {
    memset(out,0,sizeof *out);
    if (!data || length<12 || length>HUB_PUBLIC_RPC_DATAGRAM_MAX) return HUB_RPC_BAD;
    struct hub_public_rpc_cursor cursor={data,length,0};
    struct hub_public_rpc_call call; memset(&call,0,sizeof call);
    uint32_t kind,version;
    if (!hub_public_rpc_take_word(&cursor,&call.xid) || !hub_public_rpc_take_word(&cursor,&kind) ||
        !hub_public_rpc_take_word(&cursor,&version)) return HUB_RPC_BAD;
    if (kind!=0) return HUB_RPC_IGNORE; /* Never answer an RPC reply. */
    if (version!=2) { out->xid=call.xid; return HUB_RPC_WRONG_VERSION; }
    if (!hub_public_rpc_take_word(&cursor,&call.program) ||
        !hub_public_rpc_take_word(&cursor,&call.version) ||
        !hub_public_rpc_take_word(&cursor,&call.procedure) ||
        !hub_public_rpc_take_auth(&cursor,&call.credential) ||
        !hub_public_rpc_take_auth(&cursor,&call.verifier)) return HUB_RPC_BAD;
    call.arguments.data=cursor.data+cursor.offset;
    call.arguments.length=cursor.length-cursor.offset;
    *out=call; return HUB_RPC_CALL;
}
static void hub_public_rpc_store(unsigned char *out,uint32_t value) {
    out[0]=(unsigned char)(value>>24); out[1]=(unsigned char)(value>>16);
    out[2]=(unsigned char)(value>>8); out[3]=(unsigned char)value;
}
/* Accepted reply header with AUTH_NONE verifier. Body follows these24 bytes. */
static size_t hub_public_rpc_accept(unsigned char *out,size_t capacity,uint32_t xid,uint32_t status) {
    if (capacity<24 || status>5) return 0;
    const uint32_t words[6]={xid,1,0,0,0,status};
    for (unsigned i=0;i<6;++i) hub_public_rpc_store(out+i*4,words[i]);
    return 24;
}
static size_t hub_public_rpc_version_reject(unsigned char *out,size_t capacity,uint32_t xid) {
    if (capacity<24) return 0;
    const uint32_t words[6]={xid,1,1,0,2,2};
    for (unsigned i=0;i<6;++i) hub_public_rpc_store(out+i*4,words[i]);
    return 24;
}
#endif

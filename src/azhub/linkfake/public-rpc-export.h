/* SPDX-License-Identifier: GPL-3.0-only */
/* Stateless read-only mount/portmapper facade for one owned source endpoint. */
#ifndef HUB_PUBLIC_RPC_EXPORT_H
#define HUB_PUBLIC_RPC_EXPORT_H
#include <arpa/inet.h>
#include "public-rpc-tree.h"
struct hub_public_export {
    struct hub_public_file_catalog *catalog;
    uint16_t mount_port,nfs_port;
    char group[64]; /* Descriptive network group; admission remains endpoint policy. */
};
static int hub_public_export_set_group(struct hub_public_export *export,const char *group) {
    if (!group || !*group) { export->group[0]=0; return 0; }
    size_t length=strnlen(group,sizeof export->group);
    if (length>=sizeof export->group) { errno=EINVAL; return -1; }
    char copy[64]; memcpy(copy,group,length+1); char *slash=strchr(copy,'/');
    struct in_addr address,mask;
    if (!slash) { errno=EINVAL; return -1; }
    *slash++=0;
    if (inet_pton(AF_INET,copy,&address)!=1 || inet_pton(AF_INET,slash,&mask)!=1) { errno=EINVAL; return -1; }
    uint32_t inverse=~ntohl(mask.s_addr);
    if (inverse&(inverse+1)) { errno=EINVAL; return -1; }
    memcpy(export->group,group,length+1); return 0;
}
static int hub_public_export_path(const unsigned char *data,size_t length) {
    if (length<4) return 0;
    uint32_t bytes=hub_public_rpc_word(data);
    if (bytes>1024 || length!=4+(((size_t)bytes+3)&~(size_t)3)) return 0;
    const unsigned char *name=data+4;
    if ((bytes==2 && !memcmp(name,"/C",2)) || (bytes==3 && !memcmp(name,"/C/",3))) return 1;
    char decoded[256]; size_t used;
    return hub_public_name_decode(name,bytes,decoded,&used) &&
        ((used==2 && !memcmp(decoded,"/C",2)) || (used==3 && !memcmp(decoded,"/C/",3)));
}
static void hub_public_rpc_export_dispatch(void *opaque,const void *data,size_t length,
                                           struct hub_public_rpc_reply *reply) {
    struct hub_public_export *export=opaque;
    struct hub_public_rpc_call peek,call;
    /* Program selection never bypasses the shared decode/auth validation. */
    if (hub_public_rpc_decode(data,length,&peek)==HUB_RPC_CALL && peek.program==100003) {
        hub_public_rpc_tree_dispatch(export->catalog,data,length,reply); return;
    }
    uint32_t program=peek.program==100005 ? 100005 : 100000;
    uint32_t version=program==100005 ? 1 : 2;
    if (!hub_public_rpc_program_begin(data,length,program,version,&call,reply)) return;
    uint32_t status=0;
    if (program==100000) {
        if (call.procedure!=0 && call.procedure!=3) status=3;
        else if ((call.procedure==0 && call.arguments.length) ||
                 (call.procedure==3 && call.arguments.length!=16)) status=4;
        if (!status && call.procedure==3) {
            uint32_t requested=hub_public_rpc_word(call.arguments.data),
                requested_version=hub_public_rpc_word(call.arguments.data+4),
                protocol=hub_public_rpc_word(call.arguments.data+8),port=0;
            if (protocol==17) {
                if (requested==100003 && requested_version==2) port=export->nfs_port;
                if (requested==100005 && requested_version==1) port=export->mount_port;
            }
            hub_public_rpc_store(reply->bytes+24,port); reply->length=28;
        }
    } else {
        if (call.procedure!=0 && call.procedure!=1 && call.procedure!=3 && call.procedure!=4 && call.procedure!=5) status=3;
        else if ((call.procedure==0 || call.procedure==4 || call.procedure==5) && call.arguments.length) status=4;
        else if ((call.procedure==1 || call.procedure==3) && !hub_public_export_path(call.arguments.data,call.arguments.length)) status=4;
        if (!status && (call.procedure==1 || call.procedure==5)) {
            unsigned char root_handle[32]; struct hub_public_nfs_reply attributes;
            int found=hub_public_catalog_find(export->catalog,"",0,root_handle);
            if (found) {
                hub_public_nfs_getattr(export->catalog,root_handle,&attributes);
                found=!hub_public_rpc_word(attributes.bytes);
            }
            if (call.procedure==1) {
                hub_public_rpc_store(reply->bytes+24,found ? 0 : 2); reply->length=28;
                if (found) { memcpy(reply->bytes+28,root_handle,32); reply->length=60; }
            } else if (!found) { hub_public_rpc_store(reply->bytes+24,0); reply->length=28; }
            else {
                /* One export with an optional native-client network group. */
                hub_public_rpc_store(reply->bytes+24,1); hub_public_rpc_store(reply->bytes+28,3);
                memcpy(reply->bytes+32,"/C/\0",4);
                size_t group_length=strnlen(export->group,sizeof export->group);
                if (group_length && group_length<sizeof export->group) {
                    size_t padded=(group_length+3)&~(size_t)3;
                    hub_public_rpc_store(reply->bytes+36,1);
                    hub_public_rpc_store(reply->bytes+40,(uint32_t)group_length);
                    memset(reply->bytes+44,0,padded+8); memcpy(reply->bytes+44,export->group,group_length);
                    reply->length=52+padded;
                } else {
                    hub_public_rpc_store(reply->bytes+36,0); hub_public_rpc_store(reply->bytes+40,0);
                    reply->length=44;
                }
            }
        }
        /* UMNT/UMNTALL acknowledge only this peer's stateless mount. They must
         * never withdraw the source or disrupt another client's handles. */
    }
    if (status) reply->length=hub_public_rpc_accept(reply->bytes,sizeof reply->bytes,call.xid,status);
}
#endif

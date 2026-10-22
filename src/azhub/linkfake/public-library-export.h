/* SPDX-License-Identifier: GPL-3.0-only */
/* Single-owner worker dispatch for a fully staged PDB file catalog. */
#ifndef HUB_PUBLIC_LIBRARY_EXPORT_H
#define HUB_PUBLIC_LIBRARY_EXPORT_H
#include "public-library-files.h"
#include "public-rpc-export.h"
struct hub_public_library_export {
    struct hub_public_library_files *library;
    struct hub_public_export rpc;
};
static int hub_public_library_rpc_uses_files(const struct hub_public_rpc_call *call) {
    if (!hub_public_rpc_credential_valid(&call->credential) || call->verifier.flavor || call->verifier.body.length) return 0;
    size_t length=call->arguments.length; const unsigned char *args=call->arguments.data;
    if (call->program==100005 && call->version==1)
        return (call->procedure==5 && !length) ||
            (call->procedure==1 && hub_public_export_path(args,length));
    if (call->program!=100003 || call->version!=2) return 0;
    switch (call->procedure) {
    case 1: case 17: return length==32;
    case 6: return length==44 && hub_public_rpc_word(args+36)<=8192;
    case 16: return length==40 && hub_public_rpc_word(args+36)<=8192;
    case 4: {
        if (length<36) return 0;
        uint32_t bytes=hub_public_rpc_word(args+32); char name[256]; size_t used;
        return bytes<=1024 && length==36+(((size_t)bytes+3)&~(size_t)3) &&
            hub_public_name_decode(args+36,bytes,name,&used);
    }
    default: return 0;
    }
}
static void hub_public_library_export_dispatch(void *opaque,const void *data,size_t length,
                                               struct hub_public_rpc_reply *reply) {
    struct hub_public_library_export *service=opaque;
    if (__atomic_load_n(&service->library->state,__ATOMIC_ACQUIRE)==HUB_LIBRARY_BUILDING) {
        reply->length=0; return;
    }
    /* On failure the initialized catalog stays alive but withdrawn, allowing
     * NFS STALE and empty EXPORT replies until its owner stops the endpoint. */
    struct hub_public_rpc_call call;
    if (hub_public_rpc_decode(data,length,&call)==HUB_RPC_CALL &&
        hub_public_library_rpc_uses_files(&call) && hub_public_library_ready_catalog(service->library))
        (void)hub_public_library_files_validate(service->library);
    hub_public_rpc_export_dispatch(&service->rpc,data,length,reply);
}
#endif

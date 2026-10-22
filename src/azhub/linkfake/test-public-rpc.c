/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "public-rpc.h"
int main(void) {
    unsigned char storage[1025]={0},*data=storage+1; /* Intentionally unaligned. */
    const uint32_t header[]={0x12345678,0,2,100003,2,6,0,0,0,0};
    for (unsigned i=0;i<10;++i) hub_public_rpc_store(data+i*4,header[i]);
    struct hub_public_rpc_call call;
    for (size_t size=0;size<40;++size) {
        memset(&call,0xcc,sizeof call);
        assert(hub_public_rpc_decode(data,size,&call)==HUB_RPC_BAD);
        assert(!call.arguments.data && !call.arguments.length);
    }
    assert(hub_public_rpc_decode(data,40,&call)==HUB_RPC_CALL);
    assert(call.xid==0x12345678 && call.program==100003 && call.version==2 && call.procedure==6);
    assert(!call.arguments.length && !call.credential.body.length);
    for (unsigned i=0;i<44;++i) data[40+i]=(unsigned char)i;
    assert(hub_public_rpc_decode(data,84,&call)==HUB_RPC_CALL && call.arguments.length==44);
    assert(call.arguments.data==data+40 && !memcmp(call.arguments.data,data+40,44));
    hub_public_rpc_store(data+4,1);
    assert(hub_public_rpc_decode(data,84,&call)==HUB_RPC_IGNORE && !call.arguments.data);
    hub_public_rpc_store(data+4,0); hub_public_rpc_store(data+8,3);
    assert(hub_public_rpc_decode(data,84,&call)==HUB_RPC_WRONG_VERSION && call.xid==0x12345678);
    hub_public_rpc_store(data+8,2);
    /* Credential padding must be present before decoding the verifier. */
    hub_public_rpc_store(data+24,1); hub_public_rpc_store(data+28,3);
    data[32]='a';data[33]='b';data[34]='c';data[35]=0;
    hub_public_rpc_store(data+36,0);hub_public_rpc_store(data+40,0);
    assert(hub_public_rpc_decode(data,44,&call)==HUB_RPC_CALL);
    assert(call.credential.flavor==1 && call.credential.body.length==3 && call.arguments.length==0);
    assert(!memcmp(call.credential.body.data,"abc",3)); /* Not AUTH_SYS validation. */
    assert(hub_public_rpc_decode(data,43,&call)==HUB_RPC_BAD && !call.arguments.data);
    hub_public_rpc_store(data+28,401);
    assert(hub_public_rpc_decode(data,900,&call)==HUB_RPC_BAD);
    hub_public_rpc_store(data+28,UINT32_MAX);
    assert(hub_public_rpc_decode(data,900,&call)==HUB_RPC_BAD);
    assert(hub_public_rpc_decode(data,HUB_PUBLIC_RPC_DATAGRAM_MAX+1,&call)==HUB_RPC_BAD);
    assert(hub_public_rpc_decode(NULL,40,&call)==HUB_RPC_BAD);
    unsigned char reply[26]; memset(reply,0xcc,sizeof reply);
    for (size_t size=0;size<24;++size) assert(!hub_public_rpc_accept(reply,size,7,0));
    assert(reply[0]==0xcc);
    assert(hub_public_rpc_accept(reply,24,7,0)==24);
    assert(hub_public_rpc_word(reply)==7 && hub_public_rpc_word(reply+4)==1);
    assert(hub_public_rpc_word(reply+8)==0 && hub_public_rpc_word(reply+20)==0);
    assert(reply[24]==0xcc && !hub_public_rpc_accept(reply,sizeof reply,7,6));
    assert(hub_public_rpc_version_reject(reply,sizeof reply,9)==24);
    assert(hub_public_rpc_word(reply+8)==1 && hub_public_rpc_word(reply+20)==2);
    puts("PASS: RPCv2 unaligned/truncated calls, auth bounds/padding, reply suppression and accepted/version headers");
    return 0;
}

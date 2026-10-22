/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include "public-rpc-tree.h"
static uint32_t word(const unsigned char *p) { return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3]; }
int main(void) {
    char decoded[256]; size_t decoded_length;
    const unsigned char unicode[]={0xc1,0,0x3c,0xd8,0xb5,0xdf,0,0};
    assert(hub_public_name_decode(unicode,sizeof unicode,decoded,&decoded_length));
    assert(decoded_length==6 && !memcmp(decoded,"\xc3\x81\xf0\x9f\x8e\xb5",6));
    unsigned char encoded[512]; size_t encoded_length;
    assert(hub_public_name_encode(decoded,decoded_length,encoded,&encoded_length));
    assert(encoded_length==6 && !memcmp(encoded,unicode,6));
    assert(!hub_public_name_encode("\xc0\xaf",2,encoded,&encoded_length));
    assert(!hub_public_name_encode("\xed\xa0\x80",3,encoded,&encoded_length));
    assert(!hub_public_name_decode(unicode,3,decoded,&decoded_length));
    const unsigned char invalid[][4]={{0,0xd8,0,0},{0,0xdc,'a',0},{0,0,'a',0}};
    for (unsigned i=0;i<3;++i) assert(!hub_public_name_decode(invalid[i],4,decoded,&decoded_length));
    unsigned char long_name[512];
    for (unsigned i=0;i<256;++i) { long_name[i*2]='x'; long_name[i*2+1]=0; }
    assert(!hub_public_name_decode(long_name,sizeof long_name,decoded,&decoded_length));
    char root_path[]="/tmp/azhub-tree.XXXXXX"; assert(mkdtemp(root_path));
    char folder[512],file[512],secret[512],symbol[512],old[512];
    snprintf(folder,sizeof folder,"%s/Music",root_path); assert(!mkdir(folder,0700));
    snprintf(file,sizeof file,"%s/Music/track",root_path);
    snprintf(old,sizeof old,"%s/Music/old",root_path);
    int fd=open(file,O_CREAT|O_EXCL|O_WRONLY,0600); assert(fd>=0);
    assert(write(fd,"abc",3)==3 && !close(fd));
    snprintf(secret,sizeof secret,"%s/secret",root_path);
    fd=open(secret,O_CREAT|O_EXCL|O_WRONLY,0600); assert(fd>=0 && !close(fd));
    snprintf(symbol,sizeof symbol,"%s/link",root_path); assert(!symlink(folder,symbol));
    struct hub_public_root root; assert(!hub_public_root_init(&root));
    assert(!hub_public_root_attach(&root,root_path,1));
    struct hub_public_file_catalog catalog; unsigned char nonce[16]={1},rh[32],dh[32],fh[32],bad[32];
    assert(!hub_public_catalog_init(&catalog,&root,1,nonce));
    assert(!hub_public_catalog_register_node(&catalog,"",0,1,rh));
    assert(!hub_public_catalog_register_node(&catalog,"Music",5,1,dh));
    assert(!hub_public_catalog_register(&catalog,"Music/track",11,fh));
    unsigned char call[88]={0};
    const uint32_t header[]={9,0,2,100003,2,4,0,0,0,0};
    for (unsigned i=0;i<10;++i) hub_public_rpc_store(call+i*4,header[i]);
    memcpy(call+40,rh,32); hub_public_rpc_store(call+72,10);
    const unsigned char music[]={'M',0,'u',0,'s',0,'i',0,'c',0}; memcpy(call+76,music,10);
    struct hub_public_rpc_reply rpc;
    for (size_t bytes=0;bytes<sizeof call;++bytes) {
        hub_public_rpc_tree_dispatch(&catalog,call,bytes,&rpc);
        assert(rpc.length!=128 && !root.active && catalog.count==3);
    }
    hub_public_rpc_tree_dispatch(&catalog,call,sizeof call,&rpc);
    assert(rpc.length==128 && !memcmp(rpc.bytes+28,dh,32));
    hub_public_rpc_store(call+20,1);
    hub_public_rpc_tree_dispatch(&catalog,call,72,&rpc);
    assert(rpc.length==96 && hub_public_rpc_word(rpc.bytes+28)==2);
    hub_public_rpc_store(call+20,8);
    hub_public_rpc_tree_dispatch(&catalog,call,sizeof call,&rpc);
    assert(rpc.length==24 && hub_public_rpc_word(rpc.bytes+20)==3);
    assert(hub_public_catalog_register_node(&catalog,"link",4,1,bad)<0);
    assert(hub_public_catalog_register_node(&catalog,"Music/track",11,1,bad)<0);
    struct hub_public_nfs_reply reply;
    hub_public_nfs_getattr(&catalog,rh,&reply); assert(reply.length==72 && word(reply.bytes+4)==2);
    hub_public_nfs_lookup(&catalog,rh,"Music",5,&reply);
    assert(reply.length==104 && !memcmp(reply.bytes+4,dh,32) && word(reply.bytes+36)==2);
    hub_public_nfs_lookup(&catalog,dh,"track",5,&reply);
    assert(reply.length==104 && !memcmp(reply.bytes+4,fh,32) && word(reply.bytes+56)==3);
    hub_public_nfs_lookup(&catalog,rh,"secret",6,&reply);
    assert(reply.length==4 && word(reply.bytes)==2); /* Existing but unauthorized. */
    hub_public_nfs_lookup(&catalog,rh,"../secret",9,&reply); assert(word(reply.bytes)==2);
    hub_public_nfs_lookup(&catalog,fh,"x",1,&reply); assert(word(reply.bytes)==20);
    char second[512]; snprintf(second,sizeof second,"%s/two",root_path); assert(!link(file,second));
    assert(!hub_public_catalog_register(&catalog,"two",3,bad));
    hub_public_nfs_readdir(&catalog,rh,0,40,&reply);
    assert(reply.length==40 && word(reply.bytes)==0 && word(reply.bytes+28)==2 && word(reply.bytes+36)==0);
    hub_public_nfs_readdir(&catalog,rh,2,40,&reply);
    assert(reply.length==36 && word(reply.bytes+24)==4 && word(reply.bytes+32)==1);
    hub_public_nfs_readdir(&catalog,rh,4,40,&reply);
    assert(reply.length==12 && word(reply.bytes+8)==1);
    hub_public_nfs_readdir(&catalog,rh,5,40,&reply); assert(word(reply.bytes)==5);
    hub_public_nfs_readdir(&catalog,rh,0,8,&reply); assert(word(reply.bytes)==5);
    hub_public_nfs_readdir(&catalog,fh,0,40,&reply); assert(word(reply.bytes)==20);
    assert(!rename(file,old));
    fd=open(file,O_CREAT|O_EXCL|O_WRONLY,0600); assert(fd>=0 && !close(fd));
    hub_public_nfs_lookup(&catalog,dh,"track",5,&reply); assert(word(reply.bytes)==70);
    assert(!root.active);
    hub_public_catalog_withdraw(&catalog); assert(hub_public_root_drained(&root));
    hub_public_nfs_getattr(&catalog,rh,&reply); assert(word(reply.bytes)==70);
    hub_public_nfs_lookup(&catalog,rh,"Music",5,&reply); assert(word(reply.bytes)==70);
    hub_public_catalog_destroy(&catalog); assert(!hub_public_root_destroy(&root));
    assert(!unlink(symbol) && !unlink(secret) && !unlink(file) && !unlink(old) && !unlink(second));
    assert(!rmdir(folder) && !rmdir(root_path));
    puts("PASS: UTF16LE/surrogate validation, RPC GETATTR/LOOKUP framing/truncation/write refusal, authorized tree and replacement/withdrawal STALE");
}

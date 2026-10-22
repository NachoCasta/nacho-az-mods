/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include "public-rpc-read.h"
struct fixture { struct hub_public_file_ref reference; unsigned calls; };
static int resolve(void *context,const unsigned char *handle,struct hub_public_file_ref *out) {
    struct fixture *fixture=context; ++fixture->calls;
    for (unsigned i=0;i<32;++i) if (handle[i]!=0x5a) return 0;
    *out=fixture->reference; return 1;
}
static void make_call(unsigned char *data) {
    const uint32_t header[]={7,0,2,100003,2,6,0,0,0,0};
    for (unsigned i=0;i<10;++i) hub_public_rpc_store(data+i*4,header[i]);
    memset(data+40,0x5a,32);
    hub_public_rpc_store(data+72,0); hub_public_rpc_store(data+76,8192); hub_public_rpc_store(data+80,0);
}
int main(void) {
    char directory[]="/tmp/azhub-rpc-read.XXXXXX"; assert(mkdtemp(directory));
    char path[512]; snprintf(path,sizeof path,"%s/Track.bin",directory);
    unsigned char content[8192]; for (unsigned i=0;i<sizeof content;++i) content[i]=(unsigned char)(i*19);
    int fd=open(path,O_CREAT|O_EXCL|O_WRONLY,0600); assert(fd>=0);
    assert(write(fd,content,sizeof content)==sizeof content && !close(fd));
    struct hub_public_root root,other; assert(!hub_public_root_init(&root));
    assert(!hub_public_root_init(&other) && !hub_public_root_attach(&root,directory,1));
    struct fixture fixture={0}; assert(!hub_public_file_capture(&root,1,"Track.bin",9,33,&fixture.reference));
    unsigned char call[128]; make_call(call);
    struct { unsigned char before[8]; struct hub_public_rpc_reply value; unsigned char after[8]; } guarded;
    memset(&guarded,0xcc,sizeof guarded);
    for (size_t length=0;length<84;++length) {
        hub_public_rpc_read_dispatch(&root,call,length,resolve,&fixture,&guarded.value);
        assert(!fixture.calls);
        assert(!guarded.value.length || (guarded.value.length==24 && hub_public_rpc_word(guarded.value.bytes+20)==4));
    }
    hub_public_rpc_read_dispatch(&root,call,84,resolve,&fixture,&guarded.value);
    assert(fixture.calls==1 && guarded.value.length==HUB_PUBLIC_RPC_READ_REPLY_MAX);
    assert(hub_public_rpc_word(guarded.value.bytes)==7 && hub_public_rpc_word(guarded.value.bytes+20)==0);
    assert(hub_public_rpc_word(guarded.value.bytes+24)==0 && hub_public_rpc_word(guarded.value.bytes+96)==8192);
    assert(!memcmp(guarded.value.bytes+100,content,sizeof content));
    for (unsigned i=0;i<8;++i) assert(guarded.before[i]==0xcc && guarded.after[i]==0xcc);
    fixture.calls=0; hub_public_rpc_store(call+76,8193);
    hub_public_rpc_read_dispatch(&root,call,84,resolve,&fixture,&guarded.value);
    assert(!fixture.calls && hub_public_rpc_word(guarded.value.bytes+20)==4);
    make_call(call); hub_public_rpc_store(call+20,8);
    hub_public_rpc_read_dispatch(&root,call,84,resolve,&fixture,&guarded.value);
    assert(!fixture.calls && hub_public_rpc_word(guarded.value.bytes+20)==3);
    make_call(call); hub_public_rpc_store(call+16,3);
    hub_public_rpc_read_dispatch(&root,call,84,resolve,&fixture,&guarded.value);
    assert(!fixture.calls && guarded.value.length==32 && hub_public_rpc_word(guarded.value.bytes+20)==2);
    make_call(call); hub_public_rpc_store(call+8,3);
    hub_public_rpc_read_dispatch(&root,call,84,resolve,&fixture,&guarded.value);
    assert(!fixture.calls && guarded.value.length==24 && hub_public_rpc_word(guarded.value.bytes+8)==1);
    make_call(call); hub_public_rpc_store(call+4,1);
    hub_public_rpc_read_dispatch(&root,call,84,resolve,&fixture,&guarded.value);
    assert(!fixture.calls && !guarded.value.length);
    make_call(call); hub_public_rpc_store(call+24,99);
    hub_public_rpc_read_dispatch(&root,call,84,resolve,&fixture,&guarded.value);
    assert(!fixture.calls && guarded.value.length==20 && hub_public_rpc_word(guarded.value.bytes+16)==5);
    /* AUTH_SYS with a three-byte machine name and no supplemental groups. */
    make_call(call); memmove(call+64,call+40,44); memset(call+24,0,40);
    hub_public_rpc_store(call+24,1); hub_public_rpc_store(call+28,24);
    hub_public_rpc_store(call+36,3); memcpy(call+40,"abc",3);
    hub_public_rpc_read_dispatch(&root,call,108,resolve,&fixture,&guarded.value);
    assert(fixture.calls==1 && guarded.value.length==HUB_PUBLIC_RPC_READ_REPLY_MAX);
    fixture.calls=0; hub_public_rpc_store(call+52,17);
    hub_public_rpc_read_dispatch(&root,call,108,resolve,&fixture,&guarded.value);
    assert(!fixture.calls && guarded.value.length==20 && hub_public_rpc_word(guarded.value.bytes+16)==1);
    make_call(call); call[40]=0;
    hub_public_rpc_read_dispatch(&root,call,84,resolve,&fixture,&guarded.value);
    assert(guarded.value.length==28 && hub_public_rpc_word(guarded.value.bytes+24)==70);
    make_call(call);
    hub_public_rpc_read_dispatch(&other,call,84,resolve,&fixture,&guarded.value);
    assert(guarded.value.length==28 && hub_public_rpc_word(guarded.value.bytes+24)==70);
    hub_public_root_withdraw(&root); assert(hub_public_root_drained(&root));
    hub_public_rpc_read_dispatch(&root,call,84,resolve,&fixture,&guarded.value);
    assert(guarded.value.length==28 && hub_public_rpc_word(guarded.value.bytes+24)==70);
    assert(!hub_public_root_destroy(&root) && !hub_public_root_destroy(&other));
    assert(!unlink(path) && !rmdir(directory));
    puts("PASS: RPC READ integration, truncated/malformed rejection before lookup, AUTH_NONE/SYS policy, bounded reply and withdrawn/foreign handles");
    return 0;
}

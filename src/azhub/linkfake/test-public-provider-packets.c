/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include "public-provider-packets.h"
static void dump(const unsigned char *bytes,size_t size) {
    for (size_t i=0;i<size;++i) printf("%02x",bytes[i]);
    puts("");
}
int main(int argc,char **argv) {
    (void)argv;
    struct hub_public_provider_identity identity;
    const unsigned char ip[]={192,0,2,2},mac[]={2,0,0,0,17,41};
    assert(!hub_public_provider_identity_init(&identity,41,ip,mac,"Hub USB1 Research"));
    unsigned char output[296],request[48]={0}; memcpy(request,"Qspt1WmJOL",10);
    size_t length=hub_public_provider_activation(41,0x16,output); assert(length==48);
    if (argc==2) dump(output,length);
    length=hub_public_provider_activation(41,0x47,output); assert(length==72);
    if (argc==2) dump(output,length);
    request[10]=0x10; length=hub_public_provider_status(&identity,request,sizeof request,2546,output);
    assert(length==296 && output[33]==41 && output[35]==4);
    if (argc==2) dump(output,length);
    request[10]=5; request[47]=4;
    length=hub_public_provider_status(&identity,request,sizeof request,2546,output);
    assert(length==192 && output[166]==9 && output[167]==242);
    if (argc==2) dump(output,length);
    length=hub_public_provider_discovery(&identity,2,6,output); assert(length==50);
    if (argc==2) dump(output,length);
    length=hub_public_provider_discovery(&identity,6,0,output); assert(length==54);
    if (argc==2) { dump(output,length); return 0; }
    for (size_t size=0;size<48;++size) assert(!hub_public_provider_status(&identity,request,size,0,output));
    request[47]=3; assert(!hub_public_provider_status(&identity,request,48,0,output)); request[47]=4;
    request[0]=0; assert(!hub_public_provider_status(&identity,request,48,0,output)); request[0]='Q';
    assert(hub_public_provider_status(&identity,request,48,100000,output)==192 && output[166]==255 && output[167]==255);
    assert(!hub_public_provider_discovery(&identity,0,0,output));
    assert(hub_public_provider_discovery(&identity,0,3,output)==44 && !memcmp(output+38,mac,6));
    assert(hub_public_provider_discovery(&identity,4,1,output)==42 && output[36]==41);
    assert(hub_public_provider_discovery(&identity,10,0,output)==37);
    assert(!hub_public_provider_identity_init(&identity,48,ip,mac,"USB \xf0\x9f\x8e\xb5"));
    assert(identity.label[8]==0xd8 && identity.label[9]==0x3c && identity.label[10]==0xdf && identity.label[11]==0xb5);
    assert(hub_public_provider_identity_init(&identity,41,ip,mac,"\xc0\xaf")<0);
    char long_label[62]; memset(long_label,'x',61); long_label[61]=0;
    assert(hub_public_provider_identity_init(&identity,41,ip,mac,long_label)<0);
    puts("PASS: public provider packet boundaries, collection slot, counts, stable identity and strict Unicode labels");
    return 0;
}

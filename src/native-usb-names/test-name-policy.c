/* SPDX-License-Identifier: GPL-3.0-only */
#include "name-policy.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    char out[128],name[512];
    assert(nusb_valid_name("Nacho Chips",128,31));
    assert(nusb_valid_name("MiXeD Case",128,31));
    assert(nusb_valid_name("  MiXeD  ",128,31));
    assert(!nusb_valid_name("",128,31));
    assert(!nusb_valid_name("   ",128,31));
    assert(!nusb_valid_name("\xc2\xa0",128,31));
    assert(!nusb_valid_name("\xe3\x80\x80",128,31));
    assert(nusb_valid_name("Espa\xc3\xb1ol \xf0\x9f\x8e\xb5",128,31));
    assert(!nusb_valid_name("\xc0\xaf",128,31));
    assert(!nusb_valid_name("\xed\xa0\x80",128,31));
    assert(!nusb_valid_name("\xf4\x90\x80\x80",128,31));
    assert(!nusb_valid_name("x\n",128,31));
    assert(!nusb_valid_name("\xc2\x85",128,31));
    assert(!nusb_valid_name("\xf0\x9f",128,31));
    memset(name,'x',sizeof name);assert(!nusb_valid_name(name,128,31));
    name[31]=0;assert(nusb_valid_name(name,128,31));
    name[31]='x';name[32]=0;assert(!nusb_valid_name(name,128,31));
    assert(nusb_label("CAMI",out) && !strcmp(out,"CAMI"));
    assert(!nusb_label("",out) && !out[0]);
    assert(!nusb_label("\xff",out) && !out[0]);
    memset(name,'x',30);strcpy(name+30,"\xf0\x9f\x8e\xb5");
    assert(nusb_label(name,out) && strlen(out)==30);
    memset(name,'x',29);strcpy(name+29,"\xf0\x9f\x8e\xb5");
    assert(nusb_label(name,out) && strlen(out)==33);
    for(unsigned i=0;i<31;i++)memcpy(name+i*2,"\xc2\xa0",2);
    strcpy(name+62,"Visible only after the native limit");
    assert(!nusb_label(name,out) && !out[0]);
    for(unsigned a=0;a<256;a++) for(unsigned b=0;b<256;b++) {
        unsigned char media[8]={1,0,0,0,(unsigned char)a,(unsigned char)b,0,0};
        assert(nusb_native_port(media)==(a==2 ? 0:(a==0x22 ? 1:-1)));
        media[0]=2;assert(nusb_native_port(media)==-1);
    }
    puts("Native name policy: case, Unicode, bounds, truncation and 131072 native/foreign identity controls pass");
    return 0;
}

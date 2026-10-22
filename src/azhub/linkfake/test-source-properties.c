/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include "source-properties.h"
static void unit(unsigned char *b,size_t i,unsigned value) {
    b[i*2]=(unsigned char)value;b[i*2+1]=(unsigned char)(value>>8);
}
int main(void) {
    unsigned char data[0x94]={0};struct hub_source_properties p;
    const char *name="Nacho Chips";
    for (size_t i=0;name[i];i++) unit(data,i,(unsigned char)name[i]);
    data[0x7c]=6;
    assert(hub_source_properties_decode(data,sizeof data,&p));
    assert(p.name_valid && !strcmp(p.name,name) && p.color_valid && p.color==6);
    memset(data,0,sizeof data);unit(data,0,'C');unit(data,1,0xe1);
    unit(data,2,'m');unit(data,3,'i');unit(data,4,0xd83c);unit(data,5,0xdfb5);
    assert(hub_source_properties_decode(data,sizeof data,&p));
    assert(p.name_valid && !strcmp(p.name,"C\xc3\xa1mi\xf0\x9f\x8e\xb5"));
    data[0x7c]=255;
    assert(hub_source_properties_decode(data,sizeof data,&p) && p.name_valid && !p.color_valid && !p.color);
    unit(data,5,0);assert(hub_source_properties_decode(data,sizeof data,&p) && !p.name_valid);
    memset(data,0,sizeof data);for (int i=0;i<32;i++) unit(data,i,'x');
    assert(hub_source_properties_decode(data,sizeof data,&p) && !p.name_valid);
    unit(data,31,0);assert(hub_source_properties_decode(data,sizeof data,&p) && p.name_valid && strlen(p.name)==31);
    memset(data,0,sizeof data);unit(data,0,' ');
    assert(hub_source_properties_decode(data,sizeof data,&p) && !p.name_valid);
    assert(!hub_source_properties_decode(data,sizeof data-1,&p) && !p.name_valid);
    assert(!hub_source_properties_decode(NULL,sizeof data,&p));
    assert(!hub_source_properties_decode(data,sizeof data,NULL));
    return 0;
}

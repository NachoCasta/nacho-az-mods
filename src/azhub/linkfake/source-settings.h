/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef AZHUB_SOURCE_SETTINGS_H
#define AZHUB_SOURCE_SETTINGS_H
#include <stdint.h>
#include <stdio.h>
#include <string.h>
/* EP147 DeviceSettingManager -> IPLinkset_DevSetting wire order:
 * WaveSelect, waveform color, cue list, key style, position, and three extras.
 * DEVSETTING.DAT is distinct from MYSETTING.DAT (player My Settings). */
static inline uint32_t hub_settings_le32(const unsigned char *p) {
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static inline uint16_t hub_settings_crc(const unsigned char *p,size_t n) {
    uint16_t crc=0;
    while(n--) {
        crc^=(uint16_t)*p++<<8;
        for(unsigned i=0;i<8;i++)crc=(uint16_t)((crc<<1)^((crc&0x8000)?0x1021:0));
    }
    return crc;
}
static inline int hub_settings_decode(const unsigned char *data,size_t n,unsigned char out[8]) {
    if(n!=140 || hub_settings_le32(data)!=96 ||
       memcmp(data+4,"PIONEER DJ",10) || hub_settings_le32(data+100)!=32 ||
       hub_settings_le32(data+104)!=0x12345678 ||
       hub_settings_le32(data+136)!=hub_settings_crc(data+104,32))return 0;
    /* Native file loader treats zero as unspecified. Reject unknown visual enums. */
    unsigned color=data[114],position=data[117];
    if((color && color!=1 && color!=3 && color!=4) || position>2)return 0;
    memcpy(out,data+113,8);return 1;
}
static inline int hub_settings_read(const char *directory,unsigned char out[8]) {
    char path[512];unsigned char data[141];
    if(snprintf(path,sizeof path,"%s/PIONEER/DEVSETTING.DAT",directory)>=(int)sizeof path)return 0;
    FILE *f=fopen(path,"rb");if(!f)return 0;
    size_t n=fread(data,1,sizeof data,f);int failed=ferror(f);fclose(f);
    return !failed && hub_settings_decode(data,n,out);
}
/* Native defaults, updated only from a populated native USB1 settings row.
 * Empty deck packets contain zero rows; they are not a preference update. */
static inline void hub_settings_observe(unsigned char defaults[8],const unsigned char *p,size_t n) {
    if(n<0xe1)return;
    if((p[0xda]!=1 && p[0xda]!=3 && p[0xda]!=4) ||
       (p[0xdd]!=1 && p[0xdd]!=2))return;
    for(unsigned i=0;i<8;i++)if(p[0xd9+i])defaults[i]=p[0xd9+i];
}
static inline void hub_settings_merge(unsigned char out[8],const unsigned char defaults[8],const unsigned char source[8]) {
    for(unsigned i=0;i<8;i++)out[i]=source[i]?source[i]:defaults[i];
}
/* Derived from EP147 15e21f0 -> 10d3f18; packet payload starts at +0x24.
 * USB1 kind0, SD kind2, USB2 kind1, USB3 kind8. */
static inline int hub_settings_write(unsigned char *p,size_t n,int slot,const unsigned char row[8]) {
    size_t offset=slot==3?0xd9:slot==2?0xf9:slot==7?0x468:slot==8?0x474:0;
    if(!offset || offset+8>n)return 0;
    memcpy(p+offset,row,8);return 1;
}
static inline void hub_settings_clear_foreign(unsigned char *p,size_t n,const unsigned char defaults[8]) {
    const size_t full[]={0xd9,0xf9,0x3fd,0x41d,0x468,0x474};
    for(unsigned i=0;i<sizeof full/sizeof full[0];i++)
        if(full[i]+8<=n)memcpy(p+full[i],defaults,8);
    /* Export/cloud rows omit WaveSelect and/or cue-list bytes. */
    const size_t partial[]={0x408,0x40e};
    const unsigned indices[]={1,3,4,5,6,7};
    for(unsigned i=0;i<sizeof partial/sizeof partial[0];i++)
        if(partial[i]+6<=n)for(unsigned j=0;j<6;j++)p[partial[i]+j]=defaults[indices[j]];
    const size_t other[]={0xe4,0xea};
    const unsigned other_indices[]={1,4,5,6,7};
    for(unsigned i=0;i<2;i++)if(other[i]+6<=n) {
        p[other[i]]=defaults[1];
        for(unsigned j=1;j<5;j++)p[other[i]+j+1]=defaults[other_indices[j]];
    }
}
#endif

/* SPDX-License-Identifier: GPL-3.0-only */
/* Public computer-library packet encoding, reconstructed from the isolated
 * provider prototype. Transport, identity conflict detection and peer admission
 * belong to the endpoint owner. These helpers never send packets themselves. */
#ifndef HUB_PUBLIC_PROVIDER_PACKETS_H
#define HUB_PUBLIC_PROVIDER_PACKETS_H
#include <string.h>
#include "public-wire-name.h"
struct hub_public_provider_identity {
    unsigned char number,ip[4],mac[6],label[122];
    size_t label_bytes;
};
static int hub_public_provider_identity_init(struct hub_public_provider_identity *identity,
        unsigned number,const unsigned char ip[4],const unsigned char mac[6],const char *label) {
    if (!identity || !number || number>255 || !ip || !mac || !label) return -1;
    unsigned char encoded[512]; size_t length;
    if (!hub_public_name_encode(label,strnlen(label,256),encoded,&length) || length>120) return -1;
    memset(identity,0,sizeof *identity); identity->number=(unsigned char)number;
    memcpy(identity->ip,ip,4); memcpy(identity->mac,mac,6); identity->label_bytes=length;
    for (size_t i=0;i<length;i+=2) { identity->label[i]=encoded[i+1]; identity->label[i+1]=encoded[i]; }
    return 0;
}
static void hub_public_provider_header(unsigned char *packet,size_t length,unsigned kind,int discovery) {
    memset(packet,0,length); memcpy(packet,"Qspt1WmJOL",10); packet[10]=(unsigned char)kind;
    memcpy(packet+(discovery ? 12 : 11),"rekordbox",9);
    if (discovery) { packet[32]=1; packet[33]=3; packet[35]=(unsigned char)length; }
    else { packet[31]=1; packet[32]=1; }
}
/* kind10 initial, kind0/4 claim (sequence1..3), kind2 identity, kind6 keepalive.
 * A stable identity uses sequence6 for periodic kind2 packets. */
static size_t hub_public_provider_discovery(const struct hub_public_provider_identity *identity,
        unsigned kind,unsigned sequence,unsigned char output[54]) {
    size_t length=kind==10 ? 37 : kind==0 ? 44 : kind==4 ? 42 : kind==2 ? 50 : kind==6 ? 54 : 0;
    if (!length || ((kind==0 || kind==4) && (!sequence || sequence>3)) ||
        (kind==2 && (!sequence || sequence>6))) return 0;
    hub_public_provider_header(output,length,kind,1);
    switch (kind) {
    case 10: output[33]=2; output[36]=3; break;
    case 0: output[36]=(unsigned char)sequence; output[37]=4; memcpy(output+38,identity->mac,6); break;
    case 4: output[33]=2; output[36]=identity->number; output[37]=(unsigned char)sequence; break;
    case 2:
        memcpy(output+36,identity->ip,4); memcpy(output+40,identity->mac,6);
        output[46]=identity->number; output[47]=(unsigned char)sequence; output[48]=4; output[49]=1; break;
    case 6:
        output[36]=identity->number; output[37]=1; memcpy(output+38,identity->mac,6);
        memcpy(output+44,identity->ip,4); output[48]=2; output[52]=4; output[53]=8; break;
    }
    return length;
}
static size_t hub_public_provider_activation(unsigned number,unsigned kind,unsigned char output[72]) {
    if (!number || number>255 || (kind!=0x16 && kind!=0x47)) return 0;
    size_t length=kind==0x16 ? 48 : 72;
    hub_public_provider_header(output,length,kind,0); output[33]=(unsigned char)number;
    if (kind==0x47) {
        output[35]=36; output[36]=(unsigned char)number; output[37]=4;
        output[40]=0x12; output[41]=0x34; output[42]=0x56; output[43]=0x78; output[47]=1;
        const unsigned char flags[]={1,2,3,1,2,1}; memcpy(output+48,flags,sizeof flags);
    }
    return length;
}
/* Reply only to the supported public collection slot4. Input is a complete
 * admitted UDP datagram; malformed/unknown packets produce no response. */
static size_t hub_public_provider_status(const struct hub_public_provider_identity *identity,
        const unsigned char *request,size_t length,uint32_t tracks,unsigned char output[296]) {
    if (!request || length<36 || memcmp(request,"Qspt1WmJOL",10)) return 0;
    unsigned kind=request[10];
    if (kind==0x10) {
        hub_public_provider_header(output,296,0x11,0);
        output[33]=identity->number; output[34]=1; output[35]=4; output[36]=identity->number; output[37]=1;
        memcpy(output+40,identity->label,identity->label_bytes); return 296;
    }
    if (kind!=5 || length<48 || request[44] || request[45] || request[46] || request[47]!=4) return 0;
    hub_public_provider_header(output,192,6,0); output[33]=identity->number; output[35]=156;
    output[39]=identity->number; output[43]=4;
    memcpy(output+44,identity->label,identity->label_bytes);
    if (tracks>65535) tracks=65535;
    output[166]=(unsigned char)(tracks>>8); output[167]=(unsigned char)tracks; output[170]=1;
    return 192;
}
#endif

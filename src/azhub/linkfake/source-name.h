/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef AZHUB_SOURCE_NAME_H
#define AZHUB_SOURCE_NAME_H
#include <dirent.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static inline int hub_hex_digit(unsigned char c) {
    if(c>='0' && c<='9')return c-'0';
    if(c>='a' && c<='f')return c-'a'+10;
    if(c>='A' && c<='F')return c-'A'+10;
    return -1;
}
/* udev escapes unsafe path bytes as \xHH; the resulting label is UTF-8. */
static inline void hub_decode_label(const char *encoded,char *out,size_t size) {
    if(!size)return;
    size_t n=0;
    while(*encoded && n+1<size) {
        unsigned char c=(unsigned char)*encoded++;
        if(c=='\\' && encoded[0]=='x' && encoded[1] && encoded[2] &&
           hub_hex_digit(encoded[1])>=0 && hub_hex_digit(encoded[2])>=0) {
            c=(unsigned char)(hub_hex_digit(encoded[1])*16+hub_hex_digit(encoded[2]));encoded+=3;
        }
        if(c<32 || c==127)c=' ';
        out[n++]=(char)c;
    }
    while(n && out[n-1]==' ')n--;
    out[n]=0;size_t lead=0;while(out[lead]==' ')lead++;
    if(lead)memmove(out,out+lead,n-lead+1);
}
static inline int hub_label_from_links(const char *directory,const char *device,char *out,size_t size) {
    DIR *dir=opendir(directory);if(!dir)return 0;
    struct dirent *entry;int found=0;unsigned inspected=0;
    while((entry=readdir(dir)) && inspected++<256) {
        if(entry->d_name[0]=='.')continue;
        char link[PATH_MAX],target[PATH_MAX];
        if(snprintf(link,sizeof link,"%s/%s",directory,entry->d_name)>=(int)sizeof link)continue;
        if(!realpath(link,target) || strcmp(target,device))continue;
        hub_decode_label(entry->d_name,out,size);
        if(*out) {found=1;break;}
    }
    closedir(dir);return found;
}
/* Per-device udev data also works when several volumes share the same label. */
static inline int hub_label_from_udev(const char *path,char *out,size_t size) {
    FILE *file=fopen(path,"r");if(!file)return 0;
    char line[2048],plain[128]={0};int found=0;
    while(fgets(line,sizeof line,file)) {
        line[strcspn(line,"\r\n")]=0;
        if(!strncmp(line,"E:ID_FS_LABEL_ENC=",18)) {
            hub_decode_label(line+18,out,size);
            if(*out) {found=1;break;}
        } else if(!strncmp(line,"E:ID_FS_LABEL=",14)) snprintf(plain,sizeof plain,"%.*s",(int)sizeof plain-1,line+14);
    }
    fclose(file);
    if(!found && *plain) {
        for(char *p=plain;*p;p++)if((unsigned char)*p<32 || (unsigned char)*p==127)*p=' ';
        size_t n=strlen(plain);while(n && plain[n-1]==' ')plain[--n]=0;
        const char *start=plain;while(*start==' ')start++;
        snprintf(out,size,"%s",start);found=*out!=0;
    }
    return found;
}
static inline int hub_device_label(const char *node,const char *device,char *out,size_t size) {
    char path[PATH_MAX];unsigned major,minor;
    if(snprintf(path,sizeof path,"%s/dev",node)<(int)sizeof path) {
        FILE *f=fopen(path,"r");
        if(f) {
            int valid=fscanf(f,"%u:%u",&major,&minor)==2;fclose(f);
            if(valid) {
                snprintf(path,sizeof path,"/run/udev/data/b%u:%u",major,minor);
                if(hub_label_from_udev(path,out,size))return 1;
            }
        }
    }
    return hub_label_from_links("/dev/disk/by-label",device,out,size);
}
/* Last nested USB port chain, excluding the controller/root-port component. */
static inline void hub_port_name(const char *topology,char *out,size_t size) {
    snprintf(out,size,"USB");
    const char *part=topology;
    while(*part) {
        while(*part=='/')part++;
        const char *end=strchr(part,'/');if(!end)end=part+strlen(part);
        const char *p=part;
        while(p<end && *p>='0' && *p<='9')p++;
        if(p>part && p<end && *p++=='-') {
            const char *root=p;while(p<end && *p>='0' && *p<='9')p++;
            const char *dot=p;
            if(p>root && p<end && *p=='.') {
                int valid=1;
                while(p<end) {
                    if(*p++!='.') {valid=0;break;}
                    const char *child=p;while(p<end && *p>='0' && *p<='9')p++;
                    if(child==p) {valid=0;break;}
                }
                if(valid)snprintf(out,size,"USB %.*s",(int)(end-dot-1),dot+1);
            }
        }
        part=end;
    }
}
/* UTF-16BE field including a reserved zero terminator; never split a surrogate. */
static inline void hub_name_utf16be(const char *name,unsigned char *out,size_t units) {
    memset(out,0,units*2);size_t n=0;const unsigned char *p=(const unsigned char *)name;
    while(*p && n+1<units) {
        uint32_t code=*p++;unsigned extra=0;uint32_t minimum=0;
        if(code>=0xc2 && code<=0xdf) {code&=31;extra=1;minimum=0x80;}
        else if(code>=0xe0 && code<=0xef) {code&=15;extra=2;minimum=0x800;}
        else if(code>=0xf0 && code<=0xf4) {code&=7;extra=3;minimum=0x10000;}
        else if(code>=0x80)code=0xfffd;
        const unsigned char *next=p;int valid=1;
        for(unsigned j=0;j<extra;j++) {
            if((*next&0xc0)!=0x80) {valid=0;break;}
            code=(code<<6)|(*next++&63);
        }
        if(extra && (!valid || code<minimum || code>0x10ffff || (code>=0xd800 && code<=0xdfff)))code=0xfffd;
        else p=next;
        if(code>0xffff) {
            if(n+2>=units)break;
            code-=0x10000;uint16_t high=(uint16_t)(0xd800+(code>>10));
            out[n*2]=(unsigned char)(high>>8);out[n++*2+1]=(unsigned char)high;
            code=0xdc00+(code&1023);
        }
        out[n*2]=(unsigned char)(code>>8);out[n++*2+1]=(unsigned char)code;
    }
}
#endif

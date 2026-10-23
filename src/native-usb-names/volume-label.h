/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef NUSB_VOLUME_LABEL_H
#define NUSB_VOLUME_LABEL_H
/* Udev decoding/link lookup adapted from the vetted USB Hub Support helpers.
 * This standalone copy has no hub mount, topology or runtime dependency. */
#include <dirent.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static inline int nusb_hex_digit(unsigned char c) {
    if(c>='0' && c<='9')return c-'0';
    if(c>='a' && c<='f')return c-'a'+10;
    if(c>='A' && c<='F')return c-'A'+10;
    return -1;
}
/* udev escapes unsafe path bytes as \xHH; the resulting label is UTF-8. */
static inline void nusb_decode_label(const char *encoded,char *out,size_t size) {
    if(!size)return;
    size_t n=0;
    while(*encoded && n+1<size) {
        unsigned char c=(unsigned char)*encoded++;
        if(c=='\\' && encoded[0]=='x' && encoded[1] && encoded[2] &&
           nusb_hex_digit(encoded[1])>=0 && nusb_hex_digit(encoded[2])>=0) {
            c=(unsigned char)(nusb_hex_digit(encoded[1])*16+nusb_hex_digit(encoded[2]));encoded+=3;
        }
        if(c<32 || c==127)c=' ';
        out[n++]=(char)c;
    }
    while(n && out[n-1]==' ')n--;
    out[n]=0;size_t lead=0;while(out[lead]==' ')lead++;
    if(lead)memmove(out,out+lead,n-lead+1);
}
static inline int nusb_label_from_links(const char *directory,const char *device,char *out,size_t size) {
    DIR *dir=opendir(directory);if(!dir)return 0;
    struct dirent *entry;int found=0;unsigned inspected=0;
    while((entry=readdir(dir)) && inspected++<256) {
        if(entry->d_name[0]=='.')continue;
        char link[PATH_MAX],target[PATH_MAX];
        if(snprintf(link,sizeof link,"%s/%s",directory,entry->d_name)>=(int)sizeof link)continue;
        if(!realpath(link,target) || strcmp(target,device))continue;
        nusb_decode_label(entry->d_name,out,size);
        if(*out) {found=1;break;}
    }
    closedir(dir);return found;
}
/* Per-device udev data also works when several volumes share the same label. */
static inline int nusb_label_from_udev(const char *path,char *out,size_t size) {
    FILE *file=fopen(path,"re");if(!file)return 0;
    char line[2048],plain[128]={0};int found=0;
    while(fgets(line,sizeof line,file)) {
        line[strcspn(line,"\r\n")]=0;
        if(!strncmp(line,"E:ID_FS_LABEL_ENC=",18)) {
            nusb_decode_label(line+18,out,size);
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

#endif

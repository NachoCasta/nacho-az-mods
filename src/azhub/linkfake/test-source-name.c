/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include "source-name.h"
int main(void) {
    char label[128];unsigned char field[64];
    hub_decode_label("NACHO\\x20PLUGS",label,sizeof label);assert(!strcmp(label,"NACHO PLUGS"));
    hub_decode_label("  CAMI  ",label,sizeof label);assert(!strcmp(label,"CAMI"));
    hub_decode_label("x\\x0Ay",label,sizeof label);assert(!strcmp(label,"x y"));
    hub_port_name("/sys/usb2/2-1/2-1.4/2-1.4:1.0/block/sdb/sdb1",label,sizeof label);assert(!strcmp(label,"USB 4"));
    hub_port_name("/sys/usb2/2-1/2-1.4/2-1.4.2/2-1.4.2:1.0",label,sizeof label);assert(!strcmp(label,"USB 4.2"));
    hub_port_name("/sys/usb2/2-1/2-1:1.0",label,sizeof label);assert(!strcmp(label,"USB"));
    hub_name_utf16be("CAMI",field,32);assert(!memcmp(field,"\0C\0A\0M\0I\0\0",10));
    hub_name_utf16be("Mu\xc3\xb1oz",field,32);assert(field[4]==0 && field[5]==0xf1);
    hub_name_utf16be("\xf0\x9f\x8e\xb5",field,32);assert(!memcmp(field,"\xd8\x3c\xdf\xb5\0\0",6));
    hub_name_utf16be("\xf0\x9f\x8e\xb5",field,2);assert(field[0]==0 && field[1]==0);
    hub_name_utf16be("\xff",field,32);assert(field[0]==0xff && field[1]==0xfd && field[2]==0);
    char dir[]="/tmp/koi-name-XXXXXX";assert(mkdtemp(dir));char data[512],device[512],link[512],links[512];
    snprintf(data,sizeof data,"%s/udev",dir);FILE *f=fopen(data,"w");assert(f);
    fputs("E:ID_FS_LABEL=NACHO_PLUGS\nE:ID_FS_LABEL_ENC=NACHO\\x20PLUGS\n",f);fclose(f);
    assert(hub_label_from_udev(data,label,sizeof label));assert(!strcmp(label,"NACHO PLUGS"));
    snprintf(device,sizeof device,"%s/device",dir);f=fopen(device,"w");assert(f);fclose(f);
    snprintf(links,sizeof links,"%s/labels",dir);assert(!mkdir(links,0700));
    snprintf(link,sizeof link,"%s/labels/CAMI",dir);assert(!symlink(device,link));
    assert(hub_label_from_links(links,device,label,sizeof label));assert(!strcmp(label,"CAMI"));
    assert(!hub_label_from_links(links,"/dev/nonexistent",label,sizeof label));
    unlink(link);rmdir(links);unlink(device);unlink(data);rmdir(dir);
    puts("source naming: 14 label/port/UTF16 cases passed");return 0;
}

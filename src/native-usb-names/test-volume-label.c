/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include "volume-label.h"
#include "name-policy.h"
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
int main(void) {
    assert(nusb_native_port(NULL)==-1);
    char temp[]="/tmp/nusb-label-XXXXXX";assert(mkdtemp(temp));
    char file[512],device[512],link[512],out[512],name[128];
    snprintf(file,sizeof file,"%s/udev",temp);
    FILE *f=fopen(file,"w");assert(f);
    fputs("E:ID_FS_LABEL=NACHO_CHIPS\nE:ID_FS_LABEL_ENC=Nacho\\x20Chips\n",f);fclose(f);
    assert(nusb_label_from_udev(file,out,sizeof out) && !strcmp(out,"Nacho Chips"));
    f=fopen(file,"w");assert(f);fputs("E:ID_FS_LABEL_ENC=Espa\\xc3\\xb1ol\\x20\\xf0\\x9f\\x8e\\xb5\n",f);fclose(f);
    assert(nusb_label_from_udev(file,out,sizeof out) && nusb_label(out,name) && !strcmp(name,"Espa\xc3\xb1ol \xf0\x9f\x8e\xb5"));
    f=fopen(file,"w");assert(f);fputs("E:ID_FS_LABEL_ENC=\\xff\n",f);fclose(f);
    assert(nusb_label_from_udev(file,out,sizeof out) && !nusb_label(out,name));
    f=fopen(file,"w");assert(f);fputs("E:ID_FS_LABEL=   CAMI   \n",f);fclose(f);
    assert(nusb_label_from_udev(file,out,sizeof out) && !strcmp(out,"CAMI"));
    f=fopen(file,"w");assert(f);fputs("E:ID_FS_LABEL=   \n",f);fclose(f);
    assert(!nusb_label_from_udev(file,out,sizeof out));
    assert(!nusb_label_from_udev("/no/such/nusb-fixture",out,sizeof out));
    snprintf(device,sizeof device,"%s/device",temp);f=fopen(device,"w");assert(f);fclose(f);
    snprintf(link,sizeof link,"%s/CAMI",temp);assert(!symlink(device,link));
    assert(nusb_label_from_links(temp,device,out,sizeof out) && !strcmp(out,"CAMI"));
    assert(!nusb_label_from_links(temp,"/unrelated/device",out,sizeof out));
    unlink(link);unlink(device);unlink(file);rmdir(temp);
    puts("Real udev-file, escaped Unicode, invalid-label, whitespace and device-matched symlink controls pass");
}

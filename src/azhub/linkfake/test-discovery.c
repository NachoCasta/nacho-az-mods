/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include "discovery.h"
int main(void) {
    const struct { const char *path; int nested; } cases[]={
        {"/sys/devices/usb2/2-1/2-1.1/2-1.1:1.0/host1/target1:0:0",1},
        {"/sys/devices/usb4/4-1/4-1.12/4-1.12.3/host2",1},
        {"/sys/devices/usb1/1-1/1-1:1.0/host0/target0:0:0",0},
        {"/sys/devices/usb2/2-1/2-1:1.1/host0",0},
        {"/sys/devices/pci0000:00/0000:00:03.0/host0",0},
        {"/sys/devices/usb2/2-1./host0",0},
        {"/sys/devices/usb2/2-1..2/host0",0},
        {"/sys/devices/usb2/not2-1.1/host0",0}
    };
    for (unsigned i=0;i<sizeof cases/sizeof cases[0];i++)
        if (azhub_nested_usb_path(cases[i].path)!=cases[i].nested) {
            fprintf(stderr,"wrong topology classification: %s\n",cases[i].path);return 1;
        }
    puts("USB topology classification passed (8 cases)");return 0;
}

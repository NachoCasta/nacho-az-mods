/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
static int fixture_access(const char *path,int mode) {(void)path;(void)mode;return 0;}
#define access fixture_access
#include "discovery.h"
#undef access
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
    /* More than eight observed routes must not truncate a valid eighth port
     * merely because lexicographic sorting puts port10 before port2. */
    char temporary[]="/tmp/azhub-discovery-XXXXXX";assert(mkdtemp(temporary));
    char hub[PATH_MAX],classes[PATH_MAX],mounts[PATH_MAX],output[PATH_MAX],attribute[PATH_MAX];
    snprintf(hub,sizeof hub,"%s/3-2",temporary);assert(!mkdir(hub,0700));
    snprintf(classes,sizeof classes,"%s/class",temporary);assert(!mkdir(classes,0700));
    snprintf(mounts,sizeof mounts,"%s/mounts",temporary);snprintf(output,sizeof output,"%s/config",temporary);
    snprintf(attribute,sizeof attribute,"%s/3-2/maxchild",temporary);
    FILE *f=fopen(attribute,"w");assert(f);fputs("10\n",f);fclose(f);
    f=fopen(mounts,"w");assert(f);
    for (int i=0;i<9;i++) {
        unsigned port=i==8 ? 10:(unsigned)i+1;
        char device[PATH_MAX],link[PATH_MAX];
        snprintf(device,sizeof device,"%s/3-2/3-2.%u",temporary,port);assert(!mkdir(device,0700));
        snprintf(link,sizeof link,"%s/class/sd%c1",temporary,'a'+i);assert(!symlink(device,link));
        fprintf(f,"/dev/sd%c1 /media/usb/sd%c1 vfat rw 0 0\n",'a'+i,'a'+i);
    }
    fclose(f);assert(azhub_discover(mounts,classes,output)==8);
    f=fopen(output,"r");assert(f);char line[512];int rows=0,eighth=0,unassigned=0;
    while(fgets(line,sizeof line,f)) {
        if(!strncmp(line,"#hub-port ",10)) {
            rows++;if(!strncmp(line,"#hub-port 8 ",12))eighth++;
            if(!strncmp(line,"#hub-port 0 ",12))unassigned++;
        }
    }
    fclose(f);assert(rows==9 && eighth==1 && unassigned==1);
    for(int i=0;i<9;i++) {
        char path[PATH_MAX];snprintf(path,sizeof path,"%s/class/sd%c1",temporary,'a'+i);unlink(path);
        snprintf(path,sizeof path,"%s/3-2/3-2.%u",temporary,i==8 ? 10:(unsigned)i+1);rmdir(path);
    }
    unlink(attribute);unlink(output);unlink(mounts);rmdir(classes);rmdir(hub);rmdir(temporary);
    puts("USB topology classification and excess-route/empty-port reservation scanner controls passed");return 0;
}

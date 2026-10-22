/* SPDX-License-Identifier: GPL-3.0-only */
/* Hub mount scanner. The community adapter reconciles source lifecycle. */
#ifndef AZHUB_DISCOVERY_H
#define AZHUB_DISCOVERY_H
#include <ctype.h>
#include <limits.h>
#include <mntent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "source-name.h"
#include "port-topology.h"

static int azhub_nested_usb_path(const char *path) {
    const char *part=path;
    while (*part) {
        while (*part=='/') part++;
        const char *end=strchr(part,'/');
        if (!end) end=part+strlen(part);
        const char *p=part;
        while (p<end && isdigit((unsigned char)*p)) p++;
        if (p>part && p<end && *p++=='-') {
            const char *port=p;
            while (p<end && isdigit((unsigned char)*p)) p++;
            if (p>port && p<end && *p=='.') {
                int valid=1;
                while (p<end) {
                    if (*p++!='.') { valid=0; break; }
                    const char *child=p;
                    while (p<end && isdigit((unsigned char)*p)) p++;
                    if (p==child) { valid=0; break; }
                }
                if (valid) return 1;
            }
        }
        part=end;
    }
    return 0;
}
struct azhub_candidate { char mount[256],topology[PATH_MAX],device[64],name[128],port_route[96]; unsigned port_number; };
static int azhub_candidate_cmp(const void *a,const void *b) {
    const struct azhub_candidate *x=a,*y=b;
    int order=strcmp(x->topology,y->topology);
    return order ? order : strcmp(x->device,y->device);
}
/* Returns source count, -1 on I/O error. A completed file replaces output atomically. */
static int azhub_discover(const char *mounts,const char *sysblock,const char *output) {
    struct azhub_candidate found[32];
    unsigned count=0;
    FILE *table=setmntent(mounts,"re");
    if (!table) return -1;
    struct mntent *m,entry; char mount_buffer[8192];
    while ((m=getmntent_r(table,&entry,mount_buffer,sizeof mount_buffer)) && count<32) {
        if (strncmp(m->mnt_fsname,"/dev/sd",7) || strncmp(m->mnt_dir,"/media/usb/",11)) continue;
        const char *device=strrchr(m->mnt_fsname,'/');
        device=device ? device+1 : m->mnt_fsname;
#ifdef LF_SAFE_EJECT
        /* Do not open labels or library files on a reserved/ejected connection. */
        if (safe_eject_blocked_device(device)) continue;
#endif
        int safe=1;
        for (const char *p=device;*p;p++) if (!isalnum((unsigned char)*p)) safe=0;
        for (const char *p=m->mnt_dir;*p;p++)
            if (!isalnum((unsigned char)*p) && !strchr("/._-",*p)) safe=0;
        if (!safe || strlen(device)>=64 || strlen(m->mnt_dir)>=256) continue;
        char node[PATH_MAX],resolved[PATH_MAX],database[512];
        if (snprintf(node,sizeof node,"%s/%s",sysblock,device)>=(int)sizeof node) continue;
        if (!realpath(node,resolved) || !azhub_nested_usb_path(resolved)) continue;
        struct hub_port_route route;struct hub_port_sysfs topology;
        if (!hub_port_route_parse(resolved,&route) || !hub_port_sysfs_init(&topology,resolved,&route)) continue;
        unsigned port_number=hub_port_number(&route,hub_port_sysfs_children,&topology);

        snprintf(database,sizeof database,"%s/PIONEER/rekordbox/exportLibrary.db",m->mnt_dir);
        if (access(database,R_OK)) continue;
        int duplicate=0;
        for (unsigned i=0;i<count;i++) if (!strcmp(found[i].device,device)) duplicate=1;
        if (duplicate) continue;
        found[count].port_number=port_number;
        int route_length=snprintf(found[count].port_route,sizeof found[count].port_route,"%u-%u",route.bus,route.root);
        for (unsigned level=0;level<route.depth;level++)
            route_length+=snprintf(found[count].port_route+route_length,
                sizeof found[count].port_route-(size_t)route_length,".%u",route.ports[level]);
        strcpy(found[count].device,device);
        strcpy(found[count].mount,m->mnt_dir);
        strcpy(found[count].topology,resolved);
        if(!hub_device_label(node,m->mnt_fsname,found[count].name,sizeof found[count].name))
            hub_port_name(resolved,found[count].name,sizeof found[count].name);
        count++;
    }
    endmntent(table);
    qsort(found,count,sizeof found[0],azhub_candidate_cmp);
    /* Keep every observed route in presence reconciliation, including an active
     * route whose rank moved past8. The loader alone enforces eight transports. */
    char temporary[PATH_MAX];
    if (snprintf(temporary,sizeof temporary,"%s.%ld.tmp",output,(long)getpid())>=(int)sizeof temporary) return -1;
    FILE *config=fopen(temporary,"we");
    if (!config) return -1;
    static const unsigned players[]={7,7,8,8,7,8,7,8};
    static const unsigned slots[]={3,7,3,7,8,8,2,2};
    int failed=0;
    for (unsigned i=0;i<count;i++)
        if (fprintf(config,"#hub-port %u %s\n%u 10.0.2.%u %u %s %s\n",found[i].port_number,found[i].port_route,players[i%8],47+players[i%8],slots[i%8],found[i].mount,found[i].name)<0) failed=1;
    if (fclose(config)) failed=1;
    if (failed || rename(temporary,output)) { unlink(temporary); return -1; }
    return count>8 ? 8:(int)count;
}
#endif
#ifdef AZHUB_DISCOVERY_MAIN
int main(int argc,char **argv) {
    if (argc!=4) { fprintf(stderr,"usage: discover MOUNTS SYS_CLASS_BLOCK OUTPUT\n"); return 2; }
    int count=azhub_discover(argv[1],argv[2],argv[3]);
    printf("discovered=%d\n",count);
    return count<0 ? 1 : 0;
}
#endif

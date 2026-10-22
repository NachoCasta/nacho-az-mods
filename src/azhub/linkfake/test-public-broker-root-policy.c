/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include "public-broker-root-policy.h"
int main(void) {
    char base[]="/tmp/azhub-root-policy-XXXXXX";assert(mkdtemp(base));
    char usb[512],mount[512],table[512],sys[512],nested[512],node[512];
    snprintf(usb,sizeof usb,"%s/usb",base);assert(!mkdir(usb,0700));
    snprintf(mount,sizeof mount,"%s/usb/sdc1",base);assert(!mkdir(mount,0700));
    snprintf(table,sizeof table,"%s/mounts",base);
    snprintf(sys,sizeof sys,"%s/block",base);assert(!mkdir(sys,0700));
    snprintf(nested,sizeof nested,"%s/3-2.1",base);assert(!mkdir(nested,0700));
    snprintf(node,sizeof node,"%s/block/sdc1",base);assert(!symlink(nested,node));
    int fd=open(mount,O_RDONLY|O_DIRECTORY|O_CLOEXEC);assert(fd>=0);struct stat st;assert(!fstat(fd,&st));
    struct hub_public_broker_source source={.identity=1,.device=st.st_dev,.inode=st.st_ino,.player=41,
        .mac={2,0,0,0,0,1},.interface="azpub0",.label="Hub USB 1"};strcpy(source.directory,mount);
    FILE *f=fopen(table,"we");assert(f);fprintf(f,"/dev/sdc1 %s vfat rw 0 0\n",mount);assert(!fclose(f));
    assert(!hub_public_broker_root_policy(&source,fd,usb,table,sys));
    /* Missing/ambiguous mount, wrong capability and direct native topology
     * must not authorize an otherwise valid descriptor. */
    int other=open(usb,O_RDONLY|O_DIRECTORY|O_CLOEXEC);assert(other>=0);
    assert(hub_public_broker_root_policy(&source,other,usb,table,sys)<0 && errno==ESTALE);close(other);
    f=fopen(table,"ae");assert(f);fprintf(f,"/dev/sdc1 %s vfat rw 0 0\n",mount);assert(!fclose(f));
    assert(hub_public_broker_root_policy(&source,fd,usb,table,sys)<0);
    f=fopen(table,"we");assert(f);fprintf(f,"/dev/sdc1 %s vfat rw 0 0\n",mount);assert(!fclose(f));
    assert(!unlink(node) && !symlink(usb,node));
    assert(hub_public_broker_root_policy(&source,fd,usb,table,sys)<0);
    assert(!unlink(node) && !unlink(table));close(fd);
    assert(!rmdir(mount) && !rmdir(usb) && !rmdir(sys) && !rmdir(nested) && !rmdir(base));
    puts("PASS: dynamic mounted hub root, independent pinned identity, ambiguous mounts and native topology refusal");
}

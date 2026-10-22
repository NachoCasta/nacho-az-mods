/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include "public-broker-source.h"
int main(void) {
    struct hub_public_broker_source source={.identity=UINT64_MAX,.device=2049,.inode=1234,
        .player=41,.mac={2,0,0,0,0,3},.directory="/media/usb/sdc1",.interface="azpub0",.label="Hub USB 1"},out;
    unsigned char bytes[4096],saved[4096];size_t length=0;
    assert(!hub_public_broker_source_encode(&source,bytes,sizeof bytes,&length));
    memcpy(saved,bytes,length);
    assert(!hub_public_broker_source_decode(bytes,length,&out));
    assert(out.identity==UINT64_MAX && out.device==2049 && out.inode==1234 && out.player==41);
    assert(!strcmp(out.directory,source.directory) && !strcmp(out.label,source.label));
    for (size_t n=0;n<length;++n) assert(hub_public_broker_source_decode(bytes,n,&out)<0);
    assert(hub_public_broker_source_decode(bytes,length+1,&out)<0);
    const unsigned reserved[]={0,1,2,3,35,42,43,44,45,46,47};
    for (unsigned i=0;i<sizeof reserved/sizeof *reserved;++i) {
        memcpy(bytes,saved,length);bytes[reserved[i]]=2;
        assert(hub_public_broker_source_decode(bytes,length,&out)<0);
    }
    memcpy(bytes,saved,length);bytes[48]=0;assert(hub_public_broker_source_decode(bytes,length,&out)<0);
    memcpy(bytes,saved,length);bytes[36]=255;assert(hub_public_broker_source_decode(bytes,length,&out)<0);
    const char *paths[]={"relative","/","/media/../usb","/media/./usb","/media//usb","/media/usb/","/media/usb\n"};
    for (unsigned i=0;i<sizeof paths/sizeof *paths;++i) {
        strcpy(source.directory,paths[i]);assert(hub_public_broker_source_encode(&source,bytes,sizeof bytes,&length)<0);
    }
    strcpy(source.directory,"/media/usb/sdc1");
    source.mac[0]=1;assert(!hub_public_broker_source_valid(&source));source.mac[0]=2;
    strcpy(source.interface,"azpub0;id");assert(!hub_public_broker_source_valid(&source));
    strcpy(source.interface,"azpub0");source.player=256;assert(!hub_public_broker_source_valid(&source));source.player=41;
    memset(source.directory,'a',sizeof source.directory);source.directory[0]='/';source.directory[3071]=0;
    memset(source.label,'b',255);source.label[255]=0;
    assert(!hub_public_broker_source_encode(&source,bytes,sizeof bytes,&length));
    assert(!hub_public_broker_source_decode(bytes,length,&out));
    assert(hub_public_broker_source_encode(&source,bytes,length-1,&length)<0);
    char temporary[]="/tmp/azhub-source-XXXXXX";assert(mkdtemp(temporary));
    char root[256],old[256],link[256],via[256];
    snprintf(root,sizeof root,"%s/root",temporary);snprintf(old,sizeof old,"%s/old",temporary);
    snprintf(link,sizeof link,"%s/link",temporary);snprintf(via,sizeof via,"%s/link/child",temporary);
    assert(!mkdir(root,0700));struct stat state;assert(!stat(root,&state));
    strcpy(source.directory,root);strcpy(source.label,"Hub USB 1");
    source.device=state.st_dev;source.inode=state.st_ino;
    int pinned=hub_public_broker_source_open(&source);assert(pinned>=0);
    assert(fcntl(pinned,F_GETFD)&FD_CLOEXEC);
    assert(!rename(root,old) && !mkdir(root,0700));
    assert(hub_public_broker_source_open(&source)<0 && errno==ESTALE);
    assert(!fstat(pinned,&state) && (uint64_t)state.st_ino==source.inode);close(pinned);
    assert(!rmdir(root) && !symlink(old,root));
    assert(hub_public_broker_source_open(&source)<0);assert(!unlink(root));
    char child[256];snprintf(child,sizeof child,"%s/old/child",temporary);
    assert(!mkdir(child,0700) && !stat(child,&state) && !symlink(old,link));
    strcpy(source.directory,via);source.inode=state.st_ino;
    assert(hub_public_broker_source_open(&source)<0);
    assert(!unlink(link) && !rmdir(child) && !rmdir(old) && !rmdir(temporary));
    puts("PASS: bounded SOURCE identity/network payload; exact lengths, reserved fields, truncation and canonical path controls");
    return 0;
}

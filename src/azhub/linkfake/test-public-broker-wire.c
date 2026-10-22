/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <dirent.h>
#include "public-broker-wire.h"
static unsigned descriptors(void) {
    DIR *d=opendir("/proc/self/fd");assert(d);unsigned count=0;struct dirent *e;
    while ((e=readdir(d))) if (strcmp(e->d_name,".") && strcmp(e->d_name,"..")) ++count;
    closedir(d);return count;
}
static void raw(int fd,const void *packet,size_t size,const int *fds,unsigned count) {
    struct iovec v={(void *)packet,size};
    union { struct cmsghdr align;unsigned char bytes[CMSG_SPACE(4*sizeof(int))]; } control={0};
    struct msghdr message={.msg_iov=&v,.msg_iovlen=1};
    if (count) {
        assert(count<=4);message.msg_control=control.bytes;message.msg_controllen=CMSG_SPACE(count*sizeof(int));
        struct cmsghdr *c=CMSG_FIRSTHDR(&message);c->cmsg_level=SOL_SOCKET;c->cmsg_type=SCM_RIGHTS;
        c->cmsg_len=CMSG_LEN(count*sizeof(int));memcpy(CMSG_DATA(c),fds,count*sizeof(int));
    }
    assert(sendmsg(fd,&message,MSG_NOSIGNAL)==(ssize_t)size);
}
int main(void) {
    unsigned baseline=descriptors();int pair[2];
    assert(!socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_NONBLOCK|SOCK_CLOEXEC,0,pair));
    struct hub_public_broker_frame sent={.op=HUB_BROKER_HELLO,.token={1}},received={0};
    assert(!hub_public_broker_peer(pair[0],getpid(),geteuid(),-1));
    assert(hub_public_broker_peer(pair[0],getpid()+1,geteuid(),-1)<0);
    assert(hub_public_broker_peer(pair[0],getpid(),geteuid()+1,-1)<0);
    int self=open("/proc/self/exe",O_PATH|O_CLOEXEC),other=open("/dev/null",O_RDONLY|O_CLOEXEC);
    assert(self>=0 && other>=0);
    assert(!hub_public_broker_peer(pair[0],0,geteuid(),self));
    assert(hub_public_broker_peer(pair[0],0,geteuid(),other)<0);
    assert(hub_public_broker_peer(pair[0],0,geteuid(),-1)<0);
    assert(hub_public_broker_receive(pair[1],&received)==0);
    assert(!hub_public_broker_send(pair[0],&sent));
    assert(hub_public_broker_receive_token(pair[1],sent.token,&received)==1 && received.op==HUB_BROKER_HELLO);
    hub_public_broker_dispose(&received);
    int namespace=open("/proc/self/ns/net",O_RDONLY|O_CLOEXEC);assert(namespace>=0);
    sent.op=HUB_BROKER_SOURCE;sent.slot=7;sent.generation=UINT64_C(0x0102030405060708);
    sent.length=4;memcpy(sent.payload,"test",4);sent.count=1;sent.descriptors[0]=namespace;
    unsigned before=descriptors();
    assert(!hub_public_broker_send(pair[0],&sent));
    assert(hub_public_broker_receive_token(pair[1],sent.token,&received)==1);
    assert(received.generation==sent.generation && received.slot==7 && received.length==4 && !memcmp(received.payload,"test",4));
    struct stat original,passed;assert(!fstat(namespace,&original) && !fstat(received.descriptors[0],&passed));
    assert(original.st_dev==passed.st_dev && original.st_ino==passed.st_ino);
    assert(fcntl(received.descriptors[0],F_GETFD)&FD_CLOEXEC);
    assert(descriptors()==before+1);hub_public_broker_dispose(&received);hub_public_broker_dispose(&received);
    assert(descriptors()==before && fcntl(namespace,F_GETFD)>=0);
    /* Capability mismatch must close even an otherwise valid received FD. */
    unsigned char wrong[16]={2};assert(!hub_public_broker_send(pair[0],&sent));
    assert(hub_public_broker_receive_token(pair[1],wrong,&received)<0 && errno==EACCES);
    assert(descriptors()==before);
    unsigned char header[HUB_BROKER_HEADER];assert(!hub_public_broker_encode(&sent,header));
    const int rights[4]={namespace,namespace,namespace,namespace};
    for (unsigned count=0;count<=4;++count) {
        /* Missing/extra/truncated ancillary data never leaks installed FDs. */
        raw(pair[0],header,sizeof header,rights,count);
        assert(hub_public_broker_receive(pair[1],&received)<0 && descriptors()==before);
    }
    /* Every malformed header is accompanied by a real kernel-transferred FD. */
    unsigned char bytes[HUB_BROKER_HEADER+4];memcpy(bytes,header,sizeof header);memcpy(bytes+sizeof header,"test",4);
    const unsigned offsets[]={0,4,6,7,8,12,40,44,63};
    for (unsigned i=0;i<sizeof offsets/sizeof *offsets;++i) {
        unsigned char bad[sizeof bytes];memcpy(bad,bytes,sizeof bad);bad[offsets[i]]^=0x40;
        raw(pair[0],bad,sizeof bad,rights,1);
        assert(hub_public_broker_receive(pair[1],&received)<0 && descriptors()==before);
    }
    for (size_t size=1;size<HUB_BROKER_HEADER;++size) {
        raw(pair[0],bytes,size,rights,1);
        assert(hub_public_broker_receive(pair[1],&received)<0 && descriptors()==before);
    }
    sent.op=HUB_BROKER_STATUS;sent.count=0;sent.error=EIO;sent.length=HUB_BROKER_PAYLOAD;
    memset(sent.payload,0xab,sizeof sent.payload);assert(!hub_public_broker_send(pair[0],&sent));
    assert(hub_public_broker_receive_token(pair[1],sent.token,&received)==1 && received.length==HUB_BROKER_PAYLOAD && received.error==EIO);
    hub_public_broker_dispose(&received);
    unsigned char large[HUB_BROKER_HEADER+HUB_BROKER_PAYLOAD+1]={0};memcpy(large,bytes,sizeof bytes);
    raw(pair[0],large,sizeof large,rights,1);
    assert(hub_public_broker_receive(pair[1],&received)<0 && descriptors()==before);
    int root=open("/",O_RDONLY|O_DIRECTORY|O_CLOEXEC);assert(root>=0);
    sent.op=HUB_BROKER_ALLOCATE;sent.error=0;sent.count=1;sent.descriptors[0]=root;sent.generation=99;sent.length=1;sent.payload[0]=1;
    assert(!hub_public_broker_send(pair[0],&sent));
    assert(hub_public_broker_receive_token(pair[1],sent.token,&received)==1 && received.op==HUB_BROKER_ALLOCATE);
    struct stat directory;assert(!fstat(received.descriptors[0],&directory) && S_ISDIR(directory.st_mode));
    hub_public_broker_dispose(&received);close(root);
    int stream[2];assert(!socketpair(AF_UNIX,SOCK_STREAM,0,stream));
    assert(hub_public_broker_send(stream[0],&sent)<0 && errno==EPROTOTYPE);
    close(stream[0]);close(stream[1]);close(pair[0]);
    assert(hub_public_broker_receive(pair[1],&received)<0 && errno==ECONNRESET);
    close(pair[1]);close(namespace);close(self);close(other);
    assert(descriptors()==baseline);
    puts("PASS: bounded broker packets, peer credentials/executable, per-launch capability, namespace FD transfer/CLOEXEC and malformed/truncated FD cleanup");
    return 0;
}

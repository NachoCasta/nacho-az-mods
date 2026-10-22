/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <dirent.h>
#include <spawn.h>
#include <sys/wait.h>
#include "public-broker-probe-child.h"
static unsigned descriptors(void) {
    DIR *d=opendir("/proc/self/fd");assert(d);unsigned n=0;struct dirent *e;
    while ((e=readdir(d))) if (strcmp(e->d_name,".") && strcmp(e->d_name,"..")) ++n;
    closedir(d);return n;
}
int main(int argc,char **argv) {
    if (argc>1 && !strcmp(argv[1],"--broker-probe")) return hub_public_broker_probe_child(argc,argv);
    assert(argc==1 || ((argc==3 || argc==5) && !strcmp(argv[1],"--helper")));
    if (argc==5) assert(!strcmp(argv[3],"--namespace"));
    const char *helper=argc>=3 ? argv[2]:"/proc/self/exe";
    unsigned baseline=descriptors();char directory[]="/tmp/azhub-broker.XXXXXX";assert(mkdtemp(directory));
    char path[108];snprintf(path,sizeof path,"%s/control.sock",directory);
    int listener=socket(AF_UNIX,SOCK_SEQPACKET|SOCK_NONBLOCK|SOCK_CLOEXEC,0);assert(listener>=0);
    struct sockaddr_un address={.sun_family=AF_UNIX};strcpy(address.sun_path,path);
    assert(!bind(listener,(struct sockaddr *)&address,offsetof(struct sockaddr_un,sun_path)+strlen(path)+1));
    assert(!chmod(path,0600) && !listen(listener,1));
    int executable=open(helper,O_PATH|O_CLOEXEC);assert(executable>=0);
    char parent[24];snprintf(parent,sizeof parent,"%ld",(long)getpid());
    char *child_argv[]={(char *)helper,"--broker-probe",path,"0102030405060708090a0b0c0d0e0f10",parent,NULL};
    char *environment[]={"PATH=/usr/sbin:/usr/bin:/sbin:/bin",NULL};pid_t child;
    assert(!posix_spawn(&child,helper,NULL,NULL,child_argv,environment));
    uint64_t limit=hub_public_broker_ms()+5000;int peer;
    while ((peer=accept4(listener,NULL,NULL,SOCK_NONBLOCK|SOCK_CLOEXEC))<0) {
        assert((errno==EAGAIN || errno==EINTR) && hub_public_broker_ms()<limit);
        struct pollfd wait={.fd=listener,.events=POLLIN};poll(&wait,1,10);
    }
    assert(!hub_public_broker_peer(peer,child,geteuid(),executable));
    unsigned char token[16];assert(!hub_public_broker_token_parse(child_argv[3],token));
    struct hub_public_broker_frame message={0};
    assert(hub_public_broker_await(peer,token,&message)==1 && message.op==HUB_BROKER_HELLO);
    int namespace=open(argc==5 ? argv[4]:"/proc/self/ns/net",O_RDONLY|O_CLOEXEC);assert(namespace>=0);struct stat original;
    assert(!fstat(namespace,&original));
    message.op=HUB_BROKER_SOURCE;message.slot=7;message.generation=99;message.length=1;
    message.payload[0]='N';message.count=1;message.descriptors[0]=namespace;
    assert(!hub_public_broker_send(peer,&message));
    /* Send borrows the parent's FD; do not dispose that borrowed frame. */
    message.count=0;
    assert(hub_public_broker_await(peer,token,&message)==1 && message.op==HUB_BROKER_STATUS);
    assert(message.slot==7 && message.generation==99 && message.length==8 &&
        hub_public_broker_get(message.payload,8)==original.st_ino);
    message.op=HUB_BROKER_STOP;message.slot=0;message.generation=0;message.length=0;
    assert(!hub_public_broker_send(peer,&message));
    assert(hub_public_broker_await(peer,token,&message)==1 && message.op==HUB_BROKER_STATUS && !message.length);
    int status;
    limit=hub_public_broker_ms()+5000;
    for (;;) {
        pid_t reaped=waitpid(child,&status,WNOHANG);
        if (reaped==child) break;
        assert((reaped==0 || errno==EINTR) && hub_public_broker_ms()<limit);
        struct timespec pause={0,1000000};nanosleep(&pause,NULL);
    }
    assert(WIFEXITED(status) && !WEXITSTATUS(status));
    close(namespace);close(peer);close(listener);close(executable);
    assert(!unlink(path) && !rmdir(directory));assert(descriptors()==baseline);
    puts("PASS: independently execd helper authenticates parent; HELLO, namespace FD acknowledgement and STOP over owned named socket; descriptor/path cleanup");
    return 0;
}

/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include "public-file-root.h"

static void path_join(char *out,size_t size,const char *root,const char *name) {
    int count=snprintf(out,size,"%s/%s",root,name); assert(count>0 && (size_t)count<size);
}
static void write_file(const char *path,const char *content) {
    int fd=open(path,O_CREAT|O_EXCL|O_WRONLY|O_CLOEXEC,0600); assert(fd>=0);
    assert(write(fd,content,strlen(content))==(ssize_t)strlen(content)); assert(!close(fd));
}
static int open_track(struct hub_public_root *root,uint64_t generation,struct hub_public_file_lease *lease) {
    return hub_public_file_open(root,generation,"Track.bin",9,lease);
}
struct pending_read { struct hub_public_root *root; pthread_barrier_t ready,finish; };
static void *read_worker(void *arg) {
    struct pending_read *work=arg;
    struct hub_public_file_lease lease=HUB_PUBLIC_FILE_LEASE_INIT;
    assert(!open_track(work->root,1,&lease));
    pthread_barrier_wait(&work->ready); pthread_barrier_wait(&work->finish);
    char bytes[32]={0}; assert(hub_public_file_read(&lease,bytes,sizeof bytes,0)==8);
    assert(!strcmp(bytes,"original")); hub_public_file_finish(&lease);
    return NULL;
}
int main(void) {
    char base[]="/tmp/azhub-public-root.XXXXXX"; assert(mkdtemp(base));
    char one[512],two[512],replacement[512],path[512],other[512];
    path_join(one,sizeof one,base,"one"); path_join(two,sizeof two,base,"two");
    path_join(replacement,sizeof replacement,base,"replacement");
    assert(!mkdir(one,0700) && !mkdir(two,0700) && !mkdir(replacement,0700));
    path_join(path,sizeof path,one,"Track.bin"); write_file(path,"original");
    path_join(path,sizeof path,two,"Track.bin"); write_file(path,"neighbor");
    path_join(path,sizeof path,replacement,"Track.bin"); write_file(path,"new data");
    path_join(other,sizeof other,base,"outside.bin"); write_file(other,"private");
    path_join(path,sizeof path,one,"escape"); assert(!symlink(other,path));
    path_join(path,sizeof path,one,"outdir"); assert(!symlink(base,path));
    path_join(path,sizeof path,one,"pipe"); assert(!mkfifo(path,0600));
    struct hub_public_root a,b; assert(!hub_public_root_init(&a) && !hub_public_root_init(&b));
    assert(!hub_public_root_attach(&a,one,1) && !hub_public_root_attach(&b,two,1));
    const char *bad[]={"../outside.bin","/etc/passwd","./Track.bin","outdir/outside.bin","escape","pipe","a//b","Track.bin/"};
    for (size_t i=0;i<sizeof bad/sizeof *bad;++i) {
        struct hub_public_file_lease lease=HUB_PUBLIC_FILE_LEASE_INIT;
        assert(hub_public_file_open(&a,1,bad[i],strlen(bad[i]),&lease)<0);
        assert(!lease.owner && lease.fd==-1);
    }
    struct hub_public_file_lease lease=HUB_PUBLIC_FILE_LEASE_INIT;
    assert(hub_public_file_open(&a,1,"Track\0.bin",10,&lease)<0);
    assert(!open_track(&a,1,&lease));
    char bytes[32]={0}; assert(hub_public_file_read(&lease,bytes,sizeof bytes,0)==8);
    assert(!strcmp(bytes,"original"));
    assert(hub_public_file_read(&lease,bytes,HUB_PUBLIC_READ_MAX+1,0)<0 && errno==EINVAL);
    assert(hub_public_file_read(&lease,bytes,sizeof bytes,-1)<0 && errno==EINVAL);
    assert((fcntl(lease.fd,F_GETFL)&O_ACCMODE)==O_RDONLY);
    hub_public_file_finish(&lease); hub_public_file_finish(&lease);

    struct pending_read work={.root=&a};
    assert(!pthread_barrier_init(&work.ready,NULL,2) && !pthread_barrier_init(&work.finish,NULL,2));
    pthread_t worker; assert(!pthread_create(&worker,NULL,read_worker,&work));
    pthread_barrier_wait(&work.ready);
    hub_public_root_withdraw(&a); assert(!hub_public_root_drained(&a));
    assert(open_track(&a,1,&lease)<0 && errno==ESTALE);
    assert(hub_public_root_attach(&a,replacement,2)<0 && errno==EBUSY);
    assert(!open_track(&b,1,&lease)); hub_public_file_finish(&lease);
    pthread_barrier_wait(&work.finish); assert(!pthread_join(worker,NULL));
    assert(hub_public_root_drained(&a));
    assert(!pthread_barrier_destroy(&work.ready) && !pthread_barrier_destroy(&work.finish));
    assert(!hub_public_root_attach(&a,replacement,2));
    assert(open_track(&a,1,&lease)<0 && errno==ESTALE);
    assert(!open_track(&a,2,&lease)); memset(bytes,0,sizeof bytes);
    assert(hub_public_file_read(&lease,bytes,sizeof bytes,0)==8 && !strcmp(bytes,"new data"));
    hub_public_file_finish(&lease);

    struct hub_public_file_lease many[HUB_PUBLIC_LEASES_MAX];
    for (unsigned i=0;i<HUB_PUBLIC_LEASES_MAX;++i) {
        many[i]=(struct hub_public_file_lease)HUB_PUBLIC_FILE_LEASE_INIT;
        assert(!open_track(&a,2,&many[i]));
    }
    assert(open_track(&a,2,&lease)<0 && errno==EAGAIN);
    hub_public_root_withdraw(&a);
    for (unsigned i=0;i<HUB_PUBLIC_LEASES_MAX;++i) hub_public_file_finish(&many[i]);
    assert(hub_public_root_drained(&a));
    assert(hub_public_root_attach(&a,replacement,1)<0 && errno==EINVAL);
    hub_public_root_withdraw(&b); assert(hub_public_root_drained(&b));
    assert(!hub_public_root_destroy(&a) && !hub_public_root_destroy(&b));
    const char *names[]={"Track.bin","escape","outdir","pipe"};
    for (unsigned i=0;i<4;++i) { path_join(path,sizeof path,one,names[i]); assert(!unlink(path)); }
    path_join(path,sizeof path,two,"Track.bin"); assert(!unlink(path));
    path_join(path,sizeof path,replacement,"Track.bin"); assert(!unlink(path));
    assert(!unlink(other)); assert(!rmdir(one) && !rmdir(two) && !rmdir(replacement) && !rmdir(base));
    puts("PASS: confined reads, non-regular refusal, in-flight withdrawal, neighbor isolation, replacement and capacity");
    return 0;
}

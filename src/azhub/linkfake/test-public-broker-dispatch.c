/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include "public-broker-dispatch.h"
#include "public-broker-lease.h"
#include "public-broker-session.h"
struct fixture { struct hub_public_source_manager *manager;unsigned prepared,released;int pending,deny;
    struct hub_public_broker_leases *leases;struct hub_public_network_lease lease[2];uint64_t now;unsigned removed;int allocation_ready; };
static int allocate(void *opaque,unsigned slot,const struct hub_public_broker_source *source,int root) {
    struct fixture *f=opaque;assert(slot<8 && source->identity && fcntl(root,F_GETFD)>=0);++f->prepared;return 0;
}
static int allocation_ready(void *opaque,unsigned slot) { (void)slot;return ((struct fixture *)opaque)->allocation_ready; }
static uint64_t lease_now(void *opaque) { return ((struct fixture *)opaque)->now; }
static int remove_address(void *opaque,unsigned slot,const struct hub_public_network_spec *spec) {
    struct fixture *f=opaque;assert(hub_public_source_manager_drained(f->manager,slot));
    assert(spec->address.s_addr==f->lease[slot].current.address.s_addr);
    if (f->pending) return 1;
    ++f->removed;return 0;
}
static int allowed(void *unused,const struct sockaddr_in *peer) { (void)unused;return peer->sin_addr.s_addr==htonl(INADDR_LOOPBACK); }
static int endpoint(void *opaque,unsigned slot,struct hub_public_source_binding *b) {
    (void)opaque;(void)slot;
    b->listener=socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0);assert(b->listener>=0);
    struct sockaddr_in a={.sin_family=AF_INET,.sin_addr={htonl(INADDR_LOOPBACK)}};
    assert(!bind(b->listener,(void *)&a,sizeof a) && !listen(b->listener,8));
    b->player=41+slot;b->allowed=allowed;return 0;
}
static void endpoint_close(void *opaque,unsigned slot,struct hub_public_source_binding *b) {
    struct fixture *f=opaque;assert(!f->manager->sources[slot].worker.started);assert(!close(b->listener));
}
static int prepare(void *opaque,unsigned slot,const struct hub_public_broker_source *s,int root,int ns) {
    struct fixture *f=opaque;(void)slot;assert(s->identity && fcntl(root,F_GETFD)>=0 && fcntl(ns,F_GETFD)>=0);
    if (f->deny) { errno=EPERM;return -1; }
    if (f->leases) {
        struct hub_public_network_spec spec={.namespace_fd=ns,.address=f->lease[slot].current.address,
            .mask=f->lease[slot].current.mask,.broadcast=f->lease[slot].current.broadcast};
        if (hub_public_broker_lease_prepare(f->leases,slot,&f->lease[slot],&spec)<0) return -1;
    }
    ++f->prepared;return 0;
}
static int release(void *opaque,unsigned slot) {
    struct fixture *f=opaque;assert(hub_public_source_manager_drained(f->manager,slot));
    if (f->leases) return hub_public_broker_lease_release(f->leases,slot);
    if (f->pending) return 1;
    ++f->released;return 0;
}
static void wait_phase(struct hub_public_broker_dispatch *d,unsigned slot,unsigned phase) {
    uint64_t end=hub_public_worker_milliseconds()+5000;
    while (d->generations.slots[slot].phase!=phase) {
        assert(hub_public_broker_dispatch_poll(d,slot)>=0);assert(hub_public_worker_milliseconds()<end);poll(NULL,0,1);
    }
}
int main(void) {
    char path[]="/tmp/azhub-dispatch-XXXXXX";assert(mkdtemp(path));
    char pioneer[256],rb[256],pdb[256];snprintf(pioneer,sizeof pioneer,"%s/PIONEER",path);
    snprintf(rb,sizeof rb,"%s/PIONEER/rekordbox",path);snprintf(pdb,sizeof pdb,"%s/PIONEER/rekordbox/export.pdb",path);
    assert(!mkdir(pioneer,0700) && !mkdir(rb,0700));unsigned char bytes[1024]={0};
    bytes[5]=2;bytes[8]=1;bytes[36]=1;bytes[40]=1;bytes[516]=1;
    int fd=open(pdb,O_CREAT|O_EXCL|O_WRONLY,0600);assert(fd>=0 && write(fd,bytes,sizeof bytes)==sizeof bytes);close(fd);
    struct stat st;assert(!stat(path,&st));
    struct hub_public_source_manager *m=calloc(1,sizeof *m);assert(m);
    struct fixture f={.manager=m};assert(!hub_public_source_manager_init(m,(struct hub_public_source_manager_ops){endpoint,endpoint_close},&f));
    struct hub_public_broker_dispatch d;assert(!hub_public_broker_dispatch_init(&d,m,(struct hub_public_broker_dispatch_ops){.prepare=prepare,.release=release},&f));
    struct hub_public_broker_source source={.identity=9,.device=st.st_dev,.inode=st.st_ino,.player=41,.mac={2,0,0,0,0,3},.interface="azpub0",.label="Hub USB 1"};
    strcpy(source.directory,path);
    struct hub_public_broker_frame frame={.op=HUB_BROKER_SOURCE,.generation=1,.count=1,.token={1}};
    frame.descriptors[0]=open("/proc/self/ns/net",O_RDONLY|O_CLOEXEC);assert(frame.descriptors[0]>=0);
    struct stat parent_before,parent_after;assert(!stat("/proc/self/ns/net",&parent_before));
    assert(!hub_public_broker_namespace_validate(frame.descriptors[0]));
    int wrong=open("/proc/self/ns/mnt",O_RDONLY|O_CLOEXEC);assert(wrong>=0);
    assert(hub_public_broker_namespace_validate(wrong)<0);close(wrong);
    wrong=open(path,O_RDONLY|O_DIRECTORY|O_CLOEXEC);assert(wrong>=0);
    assert(hub_public_broker_namespace_validate(wrong)<0 && errno==ENOTTY);close(wrong);
    assert(!stat("/proc/self/ns/net",&parent_after) && parent_before.st_ino==parent_after.st_ino);
    assert(!hub_public_broker_source_encode(&source,frame.payload,sizeof frame.payload,&frame.length));
    assert(!hub_public_broker_dispatch_command(&d,&frame));wait_phase(&d,0,HUB_BROKER_ACTIVE);
    assert(hub_public_broker_dispatch_command(&d,&frame)<0 && errno==ESTALE);
    frame.slot=1;assert(!hub_public_broker_dispatch_command(&d,&frame));wait_phase(&d,1,HUB_BROKER_ACTIVE);
    struct hub_public_broker_frame detach={.op=HUB_BROKER_DETACH,.generation=1,.token={1}};
    f.pending=1;assert(!hub_public_broker_dispatch_command(&d,&detach));
    for (unsigned i=0;i<30;++i) { assert(hub_public_broker_dispatch_poll(&d,0)>=0);poll(NULL,0,1); }
    assert(d.generations.slots[0].phase==HUB_BROKER_DRAINING && !f.released);
    assert(d.generations.slots[1].phase==HUB_BROKER_ACTIVE);
    frame.slot=0;frame.generation=2;assert(hub_public_broker_dispatch_command(&d,&frame)<0 && errno==EBUSY);
    f.pending=0;wait_phase(&d,0,HUB_BROKER_EMPTY);
    assert(!hub_public_broker_dispatch_command(&d,&frame));wait_phase(&d,0,HUB_BROKER_ACTIVE);
    assert(hub_public_broker_dispatch_command(&d,&detach)<0 && errno==ESTALE);
    frame.slot=2;frame.generation=1;f.deny=1;
    assert(hub_public_broker_dispatch_command(&d,&frame)<0 && errno==EPERM);
    wait_phase(&d,2,HUB_BROKER_EMPTY);assert(d.error[2]==EPERM);
    f.deny=0;assert(hub_public_broker_dispatch_command(&d,&frame)<0 && errno==ESTALE);
    frame.slot=0;
    hub_public_source_worker_stop(&m->sources[1].worker);
    wait_phase(&d,1,HUB_BROKER_EMPTY);assert(d.error[1]==EPIPE);
    struct hub_public_broker_frame stop={.op=HUB_BROKER_STOP,.token={1}};
    assert(!hub_public_broker_dispatch_command(&d,&stop));wait_phase(&d,0,HUB_BROKER_EMPTY);wait_phase(&d,1,HUB_BROKER_EMPTY);
    assert(f.prepared==3 && f.released==3);
    frame.generation=3;assert(hub_public_broker_dispatch_command(&d,&frame)<0 && errno==ESHUTDOWN);
    /* Coupled expiry: actual staging threads must drain before the removal
     * fixture is called; a longer-lived neighbor remains available. */
    struct hub_public_broker_leases leases={.now=lease_now,.remove=remove_address,.context=&f};
    f.leases=&leases;f.now=300;
    for (unsigned i=0;i<2;++i) {
        char line[160];int n=snprintf(line,sizeof line,"1\tbound\tazpub0\t192.0.2.%u\t255.255.255.0\t%u\t192.0.2.2\n",3+i,60+60*i);
        struct hub_public_dhcp_event event;assert(!hub_public_dhcp_event_parse(line,n,"azpub0",&event));
        assert(!hub_public_lease_offer(&f.lease[i],&event,100,200));
        struct hub_public_acd proof={.state=HUB_ACD_CLAIMING,.pending=HUB_ACD_CLAIM};
        memcpy(proof.address,&event.address.s_addr,4);
        assert(!hub_public_lease_configured(&f.lease[i],1,300,&proof));
        proof.state=HUB_ACD_ANNOUNCING;proof.pending=HUB_ACD_ANNOUNCE;proof.announcements=1;
        assert(!hub_public_lease_activate(&f.lease[i],1,300,&proof));
    }
    assert(!hub_public_broker_dispatch_init(&d,m,(struct hub_public_broker_dispatch_ops){.prepare=prepare,.release=release},&f));
    for (unsigned i=0;i<2;++i) {
        frame.slot=i;frame.generation=1;assert(!hub_public_broker_dispatch_command(&d,&frame));wait_phase(&d,i,HUB_BROKER_ACTIVE);
    }
    f.now=60100;f.pending=1;
    assert(hub_public_broker_lease_dispatch_poll(&leases,&d,0)>=0);
    uint64_t deadline=hub_public_worker_milliseconds()+5000;
    while (f.lease[0].state!=HUB_LEASE_REMOVING) {
        assert(hub_public_broker_lease_dispatch_poll(&leases,&d,0)>=0);
        assert(hub_public_worker_milliseconds()<deadline);poll(NULL,0,1);
    }
    assert(d.generations.slots[0].phase==HUB_BROKER_DRAINING && !f.removed);
    assert(hub_public_broker_lease_dispatch_poll(&leases,&d,1)>=0);
    assert(d.generations.slots[1].phase==HUB_BROKER_ACTIVE && f.lease[1].state==HUB_LEASE_READY);
    f.pending=0;wait_phase(&d,0,HUB_BROKER_EMPTY);assert(f.removed==1 && d.error[0]==EADDRNOTAVAIL);
    assert(!hub_public_broker_dispatch_command(&d,&stop));wait_phase(&d,1,HUB_BROKER_EMPTY);
    assert(f.removed==2 && f.lease[0].state==HUB_LEASE_IDLE && f.lease[1].state==HUB_LEASE_IDLE);
    f.leases=NULL;
    for (unsigned trial=0;trial<3;++trial) {
        assert(!hub_public_broker_dispatch_init(&d,m,(struct hub_public_broker_dispatch_ops){.prepare=prepare,.release=release},&f));
        int pair[2];assert(!socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_NONBLOCK|SOCK_CLOEXEC,0,pair));
        struct hub_public_broker_session session;unsigned char token[16]={1};
        assert(!hub_public_broker_session_init(&session,pair[1],getpid(),geteuid(),token,&d,NULL,NULL));close(pair[1]);
        frame.slot=0;frame.generation=1;frame.token[0]=trial==2 ? 2:1;
        assert(!hub_public_broker_send(pair[0],&frame));
        deadline=hub_public_worker_milliseconds()+5000;
        int saw_hello=0,saw_active=0,saw_stop=0,result=0;
        while (!result) {
            result=hub_public_broker_session_poll(&session);assert(result>=0);
            if (pair[0]>=0) {
                struct hub_public_broker_frame response;
                int got;while ((got=hub_public_broker_receive_token(pair[0],token,&response))==1) {
                    if (response.op==HUB_BROKER_HELLO) saw_hello=1;
                    if (response.op==HUB_BROKER_STATUS && response.length==1) {
                        if (response.payload[0]==HUB_BROKER_ACTIVE) saw_active=1;
                        if (response.payload[0]==4) saw_stop=1;
                    }
                    hub_public_broker_dispose(&response);
                }
                if (saw_active && !d.generations.stopping) {
                    if (trial==0) assert(!hub_public_broker_send(pair[0],&stop));
                    else { close(pair[0]);pair[0]=-1; }
                }
            }
            assert(hub_public_worker_milliseconds()<deadline);poll(NULL,0,1);
        }
        assert(session.fd==-1 && hub_public_source_manager_drained(m,0));
        if (trial==0) assert(saw_hello && saw_active && saw_stop);
        if (trial==1) assert(saw_active && session.lost);
        if (trial==2) assert(!saw_active && session.lost && !d.generations.slots[0].generation);
        if (pair[0]>=0) close(pair[0]);
    }
    /* A non-reading parent must not prevent cleanup after disconnect. */
    assert(!hub_public_broker_dispatch_init(&d,m,(struct hub_public_broker_dispatch_ops){.prepare=prepare,.release=release},&f));
    frame.token[0]=1;assert(!hub_public_broker_dispatch_command(&d,&frame));wait_phase(&d,0,HUB_BROKER_ACTIVE);
    int blocked[2];assert(!socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_NONBLOCK|SOCK_CLOEXEC,0,blocked));
    int small=4096;assert(!setsockopt(blocked[1],SOL_SOCKET,SO_SNDBUF,&small,sizeof small));
    struct hub_public_broker_frame filler={.op=HUB_BROKER_HELLO,.token={1}};
    unsigned queued=0;while (!hub_public_broker_send(blocked[1],&filler)) { ++queued;assert(queued<10000); }
    assert(queued && (errno==EAGAIN || errno==EWOULDBLOCK));
    struct hub_public_broker_session congested;unsigned char token[16]={1};
    assert(!hub_public_broker_session_init(&congested,blocked[1],getpid(),geteuid(),token,&d,NULL,NULL));close(blocked[1]);
    assert(!hub_public_broker_session_poll(&congested) && congested.pending && !congested.complete);
    f.pending=1;close(blocked[0]);deadline=hub_public_worker_milliseconds()+5000;
    while (!hub_public_source_manager_drained(m,0) || !congested.lost) {
        assert(!hub_public_broker_session_poll(&congested));assert(hub_public_worker_milliseconds()<deadline);poll(NULL,0,1);
    }
    assert(!congested.complete && d.generations.slots[0].phase==HUB_BROKER_DRAINING);
    f.pending=0;
    while (!hub_public_broker_session_poll(&congested)) { assert(hub_public_worker_milliseconds()<deadline);poll(NULL,0,1); }
    assert(congested.complete && congested.fd==-1);
    assert(!hub_public_broker_dispatch_init(&d,m,(struct hub_public_broker_dispatch_ops){.release=release,.allocate=allocate,.ready=allocation_ready},&f));
    int namespace_fd=frame.descriptors[0];frame.op=HUB_BROKER_ALLOCATE;frame.generation=1;
    frame.descriptors[0]=open(path,O_RDONLY|O_DIRECTORY|O_CLOEXEC);assert(frame.descriptors[0]>=0);
    assert(!hub_public_broker_dispatch_command(&d,&frame));close(frame.descriptors[0]);
    unsigned opened_before=m->sources[0].worker.started;
    for (unsigned i=0;i<20;++i) assert(!hub_public_broker_dispatch_poll(&d,0));
    assert(!opened_before && !m->sources[0].worker.started && d.pending_root[0]>=0);
    detach.generation=1;f.pending=1;assert(!hub_public_broker_dispatch_command(&d,&detach));
    assert(d.pending_root[0]==-1);assert(!hub_public_broker_dispatch_poll(&d,0));
    assert(d.generations.slots[0].phase==HUB_BROKER_DRAINING);f.pending=0;wait_phase(&d,0,HUB_BROKER_EMPTY);
    frame.generation=2;frame.descriptors[0]=open(path,O_RDONLY|O_DIRECTORY|O_CLOEXEC);assert(frame.descriptors[0]>=0);
    assert(!hub_public_broker_dispatch_command(&d,&frame));close(frame.descriptors[0]);
    char moved[256];snprintf(moved,sizeof moved,"%s-moved",path);assert(!rename(path,moved) && !mkdir(path,0700));
    f.allocation_ready=1;wait_phase(&d,0,HUB_BROKER_ACTIVE);assert(d.pending_root[0]==-1);
    f.allocation_ready=0;assert(hub_public_broker_dispatch_poll(&d,0)>=0);wait_phase(&d,0,HUB_BROKER_EMPTY);
    assert(d.error[0]==EADDRNOTAVAIL);assert(!rmdir(path) && !rename(moved,path));
    frame.descriptors[0]=namespace_fd;
    close(frame.descriptors[0]);free(m);assert(!unlink(pdb) && !rmdir(rb) && !rmdir(pioneer) && !rmdir(path));
    puts("PASS: SOURCE codec to verified root/manager/worker, replay rejection, asynchronous network retirement, neighbor isolation and STOP");
}

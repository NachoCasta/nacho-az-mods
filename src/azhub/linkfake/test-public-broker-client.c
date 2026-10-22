/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include "public-broker-client.h"
#include "public-broker-session.h"
static int no_endpoint(void *context,unsigned slot,struct hub_public_source_binding *binding) {
    (void)context;(void)slot;(void)binding;assert(0);return -1;
}
static void no_close(void *context,unsigned slot,struct hub_public_source_binding *binding) {
    (void)context;(void)slot;(void)binding;assert(0);
}
static int reject_allocate(void *context,unsigned slot,const struct hub_public_broker_source *source,int root) {
    (void)context;assert(slot==0 && source->identity && fcntl(root,F_GETFD)>=0);errno=ENODEV;return -1;
}
static int no_ready(void *context,unsigned slot) { (void)context;(void)slot;assert(0);return -1; }
static int no_release(void *context,unsigned slot) { (void)context;(void)slot;assert(0);return -1; }
static void status(struct hub_public_broker_client *c,unsigned slot,uint64_t generation,unsigned phase,unsigned error) {
    struct hub_public_broker_frame f={.op=HUB_BROKER_STATUS,.slot=slot,.generation=generation,.length=1,.error=error};
    memcpy(f.token,c->token,16);f.payload[0]=phase;assert(!hub_public_broker_client_status(c,&f));
}
int main(void) {
    int pair[2];assert(!socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_NONBLOCK|SOCK_CLOEXEC,0,pair));
    char path[]="/tmp/azhub-client-XXXXXX";assert(mkdtemp(path));
    int root=open(path,O_RDONLY|O_DIRECTORY|O_CLOEXEC);assert(root>=0);struct stat st;assert(!fstat(root,&st));
    unsigned char token[16]={1};struct hub_public_broker_client c;hub_public_broker_client_init(&c,token);
    struct hub_public_broker_source source={.identity=1,.device=st.st_dev,.inode=st.st_ino,.player=41,
        .mac={2,0,0,0,1,1},.interface="azpub0",.label="Hub USB 1"};strcpy(source.directory,path);
    assert(!hub_public_broker_client_desire(&c,0,&source,root));
    /* Fill the real seqpacket queue: blocked ALLOCATE must not consume a generation. */
    struct hub_public_broker_frame f={.op=HUB_BROKER_HELLO};memcpy(f.token,token,16);
    while (!hub_public_broker_send(pair[0],&f)) {}
    assert(errno==EAGAIN || errno==EWOULDBLOCK);
    assert(!hub_public_broker_client_send(&c,pair[0],0) && !c.slots[0].generation);
    while (hub_public_broker_receive_token(pair[1],token,&f)==1) hub_public_broker_dispose(&f);
    assert(hub_public_broker_client_send(&c,pair[0],0)==1);
    assert(hub_public_broker_receive_token(pair[1],token,&f)==1 && f.op==HUB_BROKER_ALLOCATE && f.generation==1);
    assert(f.count==1 && !fstat(f.descriptors[0],&st) && st.st_ino==source.inode);hub_public_broker_dispose(&f);
    status(&c,0,1,HUB_BROKER_ACTIVE,0);
    /* Neighbor remains live while root replacement withdraws the old generation. */
    assert(!hub_public_broker_client_desire(&c,1,&source,root));
    assert(hub_public_broker_client_send(&c,pair[0],1)==1);
    assert(hub_public_broker_receive_token(pair[1],token,&f)==1);hub_public_broker_dispose(&f);
    status(&c,1,1,HUB_BROKER_ACTIVE,0);
    source.identity=2;assert(!hub_public_broker_client_desire(&c,0,&source,root));
    assert(hub_public_broker_client_send(&c,pair[0],0)==1);
    assert(hub_public_broker_receive_token(pair[1],token,&f)==1 && f.op==HUB_BROKER_DETACH && f.generation==1);
    assert(!hub_public_broker_client_drained(&c,0) && !hub_public_broker_client_send(&c,pair[0],0));
    status(&c,0,1,HUB_BROKER_DRAINING,0);status(&c,0,1,HUB_BROKER_EMPTY,0);
    assert(hub_public_broker_client_send(&c,pair[0],0)==1);
    assert(hub_public_broker_receive_token(pair[1],token,&f)==1 && f.generation==2);hub_public_broker_dispose(&f);
    f=(struct hub_public_broker_frame){.op=HUB_BROKER_STATUS,.generation=1,.length=1};memcpy(f.token,token,16);
    assert(hub_public_broker_client_status(&c,&f)<0 && errno==ESTALE && !hub_public_broker_client_drained(&c,0));
    status(&c,0,2,HUB_BROKER_ACTIVE,0);
    /* Changed DHCP configuration cannot replace live endpoints. Reacquire the
     * same pinned root only after authenticated EMPTY for the old generation. */
    status(&c,0,2,HUB_BROKER_DRAINING,ENETRESET);
    assert(!hub_public_broker_client_send(&c,pair[0],0));
    status(&c,0,2,HUB_BROKER_EMPTY,ENETRESET);
    assert(hub_public_broker_client_send(&c,pair[0],0)==1);
    assert(hub_public_broker_receive_token(pair[1],token,&f)==1 && f.op==HUB_BROKER_ALLOCATE && f.generation==3);
    hub_public_broker_dispose(&f);
    status(&c,0,3,HUB_BROKER_EMPTY,ESTALE);
    assert(!hub_public_broker_client_send(&c,pair[0],0));
    status(&c,0,3,HUB_BROKER_EMPTY,EEXIST);
    assert(!hub_public_broker_client_send(&c,pair[0],0)); /* Do not reclaim an occupied player number. */
    source.identity=3;assert(!hub_public_broker_client_desire(&c,0,&source,root));
    assert(hub_public_broker_client_send(&c,pair[0],0)==1);
    assert(hub_public_broker_receive_token(pair[1],token,&f)==1 && f.generation==4);hub_public_broker_dispose(&f);
    status(&c,0,4,HUB_BROKER_DRAINING,EADDRINUSE);status(&c,0,4,HUB_BROKER_EMPTY,EADDRINUSE);
    /* Eject reservation cancels a queued conflict recovery. */
    assert(!hub_public_broker_client_desire(&c,0,NULL,-1));
    assert(!hub_public_broker_client_send(&c,pair[0],0) && hub_public_broker_client_drained(&c,0));
    /* Repeated EMPTY reporting a configuration reset cannot undo withdrawal. */
    status(&c,0,4,HUB_BROKER_EMPTY,ENETRESET);
    assert(!hub_public_broker_client_send(&c,pair[0],0) && !c.slots[0].wanted);
    assert(c.slots[1].phase==HUB_BROKER_ACTIVE);
    close(pair[1]);assert(!hub_public_broker_client_desire(&c,1,NULL,-1));
    assert(hub_public_broker_client_send(&c,pair[0],1)<0 && c.lost && !hub_public_broker_client_drained(&c,1));
    hub_public_broker_client_close(&c);close(pair[0]);
    /* Exercise the real authenticated session: a rejected ALLOCATE produces
     * an error response plus a repeated change notification for the same EMPTY. */
    assert(!socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_NONBLOCK|SOCK_CLOEXEC,0,pair));
    struct hub_public_source_manager *manager=calloc(1,sizeof *manager);assert(manager);
    assert(!hub_public_source_manager_init(manager,(struct hub_public_source_manager_ops){no_endpoint,no_close},NULL));
    struct hub_public_broker_dispatch dispatch;
    assert(!hub_public_broker_dispatch_init(&dispatch,manager,(struct hub_public_broker_dispatch_ops){
        .allocate=reject_allocate,.ready=no_ready,.release=no_release},NULL));
    struct hub_public_broker_session session;
    assert(!hub_public_broker_session_init(&session,pair[1],getpid(),geteuid(),token,&dispatch,NULL,NULL));close(pair[1]);
    hub_public_broker_client_init(&c,token);
    assert(!hub_public_broker_session_poll(&session));
    assert(hub_public_broker_receive_token(pair[0],token,&f)==1 && f.op==HUB_BROKER_HELLO);
    assert(!hub_public_broker_client_desire(&c,0,&source,root));
    assert(hub_public_broker_client_send(&c,pair[0],0)==1);
    unsigned replies=0;
    for (unsigned i=0;i<20;++i) {
        assert(!hub_public_broker_session_poll(&session));
        if (hub_public_broker_receive_token(pair[0],token,&f)==1) {
            assert(f.op==HUB_BROKER_STATUS && f.error==ENODEV);
            assert(!hub_public_broker_client_status(&c,&f));++replies;
        }
    }
    assert(replies>=2 && hub_public_broker_client_drained(&c,0));
    assert(!hub_public_broker_client_send(&c,pair[0],0)); /* No hard-error retry loop. */
    close(pair[0]);while (!hub_public_broker_session_poll(&session)) {}
    assert(session.complete && session.fd==-1);hub_public_broker_client_close(&c);free(manager);
    close(root);assert(!rmdir(path));
    puts("PASS: inventory IPC backpressure, pinned roots, replacement ordering, stale status rejection, eject wins over retry and peer loss is not retirement");
}

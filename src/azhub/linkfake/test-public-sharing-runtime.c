/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "azmod-mod.h"
static int stopped,alive;
static const char *missing_path(const char *id) { (void)id;return NULL; }
static int stop_child(const char *id,int pid) { (void)id;assert(pid==42);++stopped;return 0; }
static int child_alive(const char *id,int pid) { (void)id;assert(pid==42);return alive; }
static const az_core_v1 core={.assets_dir=missing_path,.data_dir=missing_path,
    .launch_stop=stop_child,.launch_alive=child_alive};
static const az_core_v1 *lf_core=&core;
#define lf_log(...) ((void)0)
static struct { char dir[256];int present,unmount_pending,eject_reserved; } fakes[8];
static int nfakes=5;
#include "public-sharing-runtime.h"
static void report(int fd,unsigned phase,unsigned error) {
    struct hub_public_broker_frame frame={.op=HUB_BROKER_STATUS,.slot=4,.generation=1,.length=1,.error=error};
    memcpy(frame.token,public_runtime.token,16);frame.payload[0]=phase;
    assert(!hub_public_broker_send(fd,&frame));
}
static void session(int pair[2],const char *root) {
    assert(!socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_NONBLOCK|SOCK_CLOEXEC,0,pair));
    public_runtime.peer=pair[0];public_runtime.child=42;
    public_runtime.token[0]=1;
    unsigned char parent[6]={0x52,0x54,0,0x12,0x34,0x56};
    assert(!hub_public_sharing_inventory_init(&public_runtime.inventory,public_runtime.token,parent));
    public_runtime.initialized=1;public_runtime.phase=PUBLIC_RUNTIME_ACTIVE;public_runtime_set_mode(1);public_runtime.stop_sent=public_runtime.stop_requested=0;
    strcpy(fakes[4].dir,root);fakes[4].present=1;fakes[4].eject_reserved=0;
}
int main(void) {
    /* Missing resources retry without aborting the local mod or inventing a session. */
    public_runtime_poll();assert(public_runtime.phase==PUBLIC_RUNTIME_IDLE && !public_runtime.retry_at);
    public_runtime_set_mode(0);public_runtime_poll();assert(!public_runtime.retry_at);
    public_runtime_set_mode(1);public_runtime_poll();assert(public_runtime.phase==PUBLIC_RUNTIME_IDLE && public_runtime.retry_at);
    char root[]="/tmp/azhub-runtime-XXXXXX";assert(mkdtemp(root));
    int pair[2];session(pair,root);public_runtime_poll();
    struct hub_public_broker_frame frame;
    assert(hub_public_broker_receive_token(pair[1],public_runtime.token,&frame)==1);
    assert(frame.op==HUB_BROKER_ALLOCATE && frame.slot==4 && frame.generation==1);
    hub_public_broker_dispose(&frame);
    report(pair[1],HUB_BROKER_ACTIVE,0);public_runtime_poll();
    assert(!public_runtime_drain(4));
    assert(hub_public_broker_receive_token(pair[1],public_runtime.token,&frame)==1 && frame.op==HUB_BROKER_DETACH);
    hub_public_broker_dispose(&frame);fakes[4].eject_reserved=1;
    assert(!public_runtime_drain(4));
    report(pair[1],HUB_BROKER_EMPTY,0);public_runtime_poll();assert(public_runtime_drain(4));
    /* Fill the transport: an Off request must remain latched if STOP blocks,
     * even when native On completes before the next writable opportunity. */
    frame=(struct hub_public_broker_frame){.op=HUB_BROKER_HELLO};memcpy(frame.token,public_runtime.token,16);
    unsigned queued=0;
    while (!hub_public_broker_send(pair[0],&frame)) assert(++queued<10000);
    assert(queued && (errno==EAGAIN || errno==EWOULDBLOCK));
    public_runtime_set_mode(0);public_runtime_poll();
    assert(public_runtime.stop_requested && !public_runtime.stop_sent);
    public_runtime_set_mode(1);
    while (hub_public_broker_receive_token(pair[1],public_runtime.token,&frame)==1) hub_public_broker_dispose(&frame);
    /* Off sends exactly one STOP; On during drain cannot republish early. */
    public_runtime_poll();
    assert(public_runtime.stop_sent && public_runtime.inventory.quiescing);
    assert(hub_public_broker_receive_token(pair[1],public_runtime.token,&frame)==1 && frame.op==HUB_BROKER_STOP);
    hub_public_broker_dispose(&frame);public_runtime_poll();
    assert(hub_public_broker_receive_token(pair[1],public_runtime.token,&frame)==0);
    public_runtime_set_mode(1);public_runtime_poll();
    assert(hub_public_broker_receive_token(pair[1],public_runtime.token,&frame)==0);
    /* Global completion waits for SDK process exit before closing IPC/restarting. */
    frame=(struct hub_public_broker_frame){.op=HUB_BROKER_STATUS,.length=1};
    memcpy(frame.token,public_runtime.token,16);frame.payload[0]=4;
    assert(!hub_public_broker_send(pair[1],&frame));public_runtime_poll();
    assert(public_runtime.phase==PUBLIC_RUNTIME_EXIT);alive=1;public_runtime_poll();
    assert(public_runtime.initialized && public_runtime.peer>=0);
    alive=0;public_runtime_poll();assert(public_runtime.phase==PUBLIC_RUNTIME_IDLE && !public_runtime.initialized);
    close(pair[1]);
    /* Broken IPC with an outstanding generation must never unlock native eject. */
    session(pair,root);public_runtime_poll();
    assert(hub_public_broker_receive_token(pair[1],public_runtime.token,&frame)==1);hub_public_broker_dispose(&frame);
    close(pair[1]);public_runtime_poll();
    assert(public_runtime.phase==PUBLIC_RUNTIME_FAULT && stopped==1 && !public_runtime_drain(4));
    assert(public_runtime_drain(0));public_runtime_cleanup();
    assert(!rmdir(root));
    puts("PASS: automatic all-slot inventory, exact retirement eject gate, SDK-exit restart ordering and broken-channel refusal");
}

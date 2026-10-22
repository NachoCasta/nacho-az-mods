/* SPDX-License-Identifier: GPL-3.0-only */
/* Authenticated nonblocking helper command channel. One controller polls it.
 * Stop/EOF/token failure drain services before terminal return. A poll fault
 * is not terminal: keep the session/context alive until resources are drained. */
#ifndef HUB_PUBLIC_BROKER_SESSION_H
#define HUB_PUBLIC_BROKER_SESSION_H
#include "public-broker-dispatch.h"
struct hub_public_broker_session {
    struct hub_public_broker_dispatch *dispatch;
    int (*poll_source)(void *,struct hub_public_broker_dispatch *,unsigned);
    void *context;
    int fd,pending,lost,complete;
    unsigned char token[16];
    struct hub_public_broker_frame reply;
    uint64_t reported_generation[8];
    unsigned reported_phase[8];int reported_error[8];
};
static int hub_public_broker_session_init(struct hub_public_broker_session *s,int fd,
        pid_t parent,uid_t uid,const unsigned char token[16],struct hub_public_broker_dispatch *dispatch,
        int (*poll_source)(void *,struct hub_public_broker_dispatch *,unsigned),void *context) {
    if (!s || !dispatch || !token || !memcmp(token,"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",16)) { errno=EINVAL;return -1; }
    if (hub_public_broker_peer(fd,parent,uid,-1)<0) return -1;
    int owned=fcntl(fd,F_DUPFD_CLOEXEC,0);if (owned<0) return -1;
    memset(s,0,sizeof *s);s->fd=owned;s->dispatch=dispatch;s->poll_source=poll_source;s->context=context;
    memcpy(s->token,token,16);s->reply.op=HUB_BROKER_HELLO;memcpy(s->reply.token,token,16);s->pending=1;return 0;
}
static void hub_public_broker_session_loss(struct hub_public_broker_session *s) {
    if (s->lost) return;
    s->lost=1;s->pending=0;close(s->fd);s->fd=-1;
    struct hub_public_broker_frame stop={.op=HUB_BROKER_STOP};memcpy(stop.token,s->token,16);
    hub_public_broker_dispatch_command(s->dispatch,&stop);
}
static void hub_public_broker_session_status(struct hub_public_broker_session *s,unsigned slot,
        uint64_t generation,unsigned phase,int error) {
    memset(&s->reply,0,sizeof s->reply);s->reply.op=HUB_BROKER_STATUS;s->reply.slot=slot;
    s->reply.generation=generation;s->reply.length=1;s->reply.payload[0]=(unsigned char)phase;
    s->reply.error=error>0 ? (unsigned)error:0;memcpy(s->reply.token,s->token,16);s->pending=1;
}
/* 0 ongoing, 1 fully drained/closed; -1 fault requiring continued ownership.
 * At most one input command and one queued response are handled per call.
 * Drain polling continues even when the parent stops reading responses. */
static int hub_public_broker_session_poll(struct hub_public_broker_session *s) {
    if (!s || !s->dispatch) { errno=EINVAL;return -1; }
    if (s->complete) return 1;
    int empty=1;
    for (unsigned i=0;i<8;++i) {
        int result=s->poll_source ? s->poll_source(s->context,s->dispatch,i):hub_public_broker_dispatch_poll(s->dispatch,i);
        if (result<0) return -1;
        if (s->dispatch->generations.slots[i].phase!=HUB_BROKER_EMPTY) empty=0;
    }
    if (s->lost) {
        if (empty) { s->complete=1;return 1; }return 0;
    }
    if (s->pending) {
        if (hub_public_broker_send(s->fd,&s->reply)<0) {
            if (errno==EAGAIN || errno==EWOULDBLOCK) return 0;
            hub_public_broker_session_loss(s);return 0;
        }
        s->pending=0;
        if (s->reply.op==HUB_BROKER_STATUS && s->reply.length==1 && s->reply.payload[0]==4) {
            close(s->fd);s->fd=-1;s->complete=1;return 1;
        }
    }
    for (unsigned i=0;i<8;++i) {
        struct hub_public_broker_generation_slot *g=s->dispatch->generations.slots+i;
        int error=s->dispatch->error[i];
        if (s->reported_generation[i]!=g->generation || s->reported_phase[i]!=g->phase || s->reported_error[i]!=error) {
            hub_public_broker_session_status(s,i,g->generation,g->phase,error);
            s->reported_generation[i]=g->generation;s->reported_phase[i]=g->phase;s->reported_error[i]=error;
            return 0;
        }
    }
    if (s->dispatch->generations.stopping && empty) {
        hub_public_broker_session_status(s,0,0,4,0);return 0;
    }
    struct hub_public_broker_frame command;
    int received=hub_public_broker_receive_token(s->fd,s->token,&command);
    if (received<0) { hub_public_broker_session_loss(s);return 0; }
    if (!received) return 0;
    int result=hub_public_broker_dispatch_command(s->dispatch,&command),error=errno;
    if (result<0) hub_public_broker_session_status(s,command.slot,command.generation,
        s->dispatch->generations.slots[command.slot].phase,error);
    hub_public_broker_dispose(&command);return 0;
}
#endif

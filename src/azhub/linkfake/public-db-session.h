/* SPDX-License-Identifier: GPL-3.0-only */
/* Single-worker nonblocking owned connection. The provider bounds session
 * count and peer admission. Callbacks must be bounded and never block on I/O.
 * Cancellation is atomic; join the worker before destroy. */
#ifndef HUB_PUBLIC_DB_SESSION_H
#define HUB_PUBLIC_DB_SESSION_H
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include "public-db-wire.h"
#define HUB_DB_HANDSHAKE_MS 5000U
#define HUB_DB_FRAME_MS 5000U
#define HUB_DB_IDLE_MS 30000U
#define HUB_DB_INPUT_MAX 131072U
#define HUB_DB_OUTPUT_INITIAL 4096U
/* begin consumes borrowed request fields synchronously. next returns one
 * borrowed response(1), bounded work without a response yet(2), completed
 * response sequence(0), or failure(-1). */
struct hub_db_callbacks {
    int (*begin)(void *,const struct hub_db_message *);
    int (*next)(void *,struct hub_db_message *);
    int (*alive)(void *);
};
struct hub_db_session {
    int fd,state,canceled,pending;
    unsigned provider,requester,extended_menu;
    uint64_t started,last_progress,frame_started;
    unsigned char *input,*output;
    size_t received,sending,sent,output_capacity;
    void *context;
    struct hub_db_callbacks callbacks;
};
static int hub_db_session_close(struct hub_db_session *session) {
    if (session->fd>=0) { shutdown(session->fd,SHUT_RDWR); close(session->fd); session->fd=-1; }
    session->state=3; session->received=session->sending=session->sent=0; return -1;
}
/* Ownership transfers even on failure; destroy is always required. */
static int hub_db_session_init(struct hub_db_session *session,int fd,unsigned provider,uint64_t now,
        struct hub_db_callbacks callbacks,void *context) {
    memset(session,0,sizeof *session); session->fd=fd; session->provider=provider;
    session->started=session->last_progress=now; session->callbacks=callbacks; session->context=context;
    if (fd<0 || !provider || provider>255 || !callbacks.begin || !callbacks.next || !callbacks.alive)
        return hub_db_session_close(session);
    int flags=fcntl(fd,F_GETFL);
    if (flags<0 || fcntl(fd,F_SETFL,flags|O_NONBLOCK)<0) return hub_db_session_close(session);
    session->input=malloc(HUB_DB_INPUT_MAX); session->output=malloc(HUB_DB_OUTPUT_INITIAL);
    session->output_capacity=HUB_DB_OUTPUT_INITIAL;
    if (!session->input || !session->output) return hub_db_session_close(session);
    return 0;
}
static void hub_db_session_cancel(struct hub_db_session *session) {
    __atomic_store_n(&session->canceled,1,__ATOMIC_RELEASE);
}
static void hub_db_session_destroy(struct hub_db_session *session) {
    hub_db_session_close(session); free(session->input); free(session->output);
    session->input=session->output=NULL;
}
static int hub_db_session_queue(struct hub_db_session *session,const struct hub_db_message *message) {
    size_t size;
    if (hub_db_message_size(message,&size)<0) return -1;
    if (size>session->output_capacity) {
        unsigned char *output=realloc(session->output,size);
        if (!output) return -1;
        session->output=output; session->output_capacity=size;
    }
    return hub_db_message_write(message,session->output,session->output_capacity,&session->sending);
}
/* Returns0 while live,-1 once closed. The caller supplies monotonic milliseconds
 * and schedules all live sessions fairly, including timeout checks without I/O. */
static int hub_db_session_step(struct hub_db_session *session,uint64_t now) {
    if (session->state==3) return -1;
    if (__atomic_load_n(&session->canceled,__ATOMIC_ACQUIRE) || !session->callbacks.alive(session->context) ||
        now<session->last_progress || now-session->last_progress>=HUB_DB_IDLE_MS ||
        (session->state<2 && now-session->started>=HUB_DB_HANDSHAKE_MS) ||
        (session->received && now-session->frame_started>=HUB_DB_FRAME_MS)) return hub_db_session_close(session);
    if (session->sending) {
        ssize_t result=send(session->fd,session->output+session->sent,session->sending-session->sent,MSG_NOSIGNAL);
        if (result<0 && errno!=EAGAIN && errno!=EWOULDBLOCK && errno!=EINTR) return hub_db_session_close(session);
        if (result>0) { session->sent+=(size_t)result; session->last_progress=now; }
        if (session->sent==session->sending) {
            session->sending=session->sent=0;
            /* Large analysis responses must not permanently enlarge idle
             * sessions. Shrinking failure is harmless and retried later. */
            if (session->output_capacity>16384) {
                unsigned char *output=realloc(session->output,HUB_DB_OUTPUT_INITIAL);
                if (output) { session->output=output; session->output_capacity=HUB_DB_OUTPUT_INITIAL; }
            }
        }
        return 0;
    }
    if (session->pending) {
        struct hub_db_message response; memset(&response,0,sizeof response);
        int result=session->callbacks.next(session->context,&response);
        if (result==2) return 0;
        if (result<0 || (result>0 && hub_db_session_queue(session,&response)<0))
            return hub_db_session_close(session);
        if (!result) session->pending=0;
        return 0;
    }
    /* Consume buffered messages before receiving more, preserving coalesced
     * successors and applying TCP backpressure during response production. */
    if (session->received) {
        size_t used=0; int result;
        if (!session->state) {
            struct hub_db_field greeting;
            result=hub_db_field_read(session->input,session->received,&used,&greeting);
            if (result==1) {
                if (greeting.kind!=HUB_DB_NUMBER || greeting.number!=1) return hub_db_session_close(session);
                session->output[0]=17; hub_db_put32(session->output+1,1); session->sending=5; session->state=1;
            }
        } else {
            struct hub_db_message request;
            result=hub_db_message_read(session->input,session->received,&request,&used);
            if (result==1) {
                if (session->state==1) {
                    if (request.type || (request.count!=1 && request.count!=2) || request.args[0].kind!=HUB_DB_NUMBER ||
                        !request.args[0].number || request.args[0].number>255 ||
                        (request.count==2 && (request.args[1].kind!=HUB_DB_NUMBER || request.args[1].number!=20)))
                        return hub_db_session_close(session);
                    session->extended_menu=request.count==2;
                    session->requester=request.args[0].number; session->state=2;
                    struct hub_db_message reply={.transaction=request.transaction,.type=0x4000,.count=2};
                    reply.args[0].kind=reply.args[1].kind=HUB_DB_NUMBER; reply.args[1].number=session->provider;
                    if (hub_db_session_queue(session,&reply)<0) return hub_db_session_close(session);
                } else {
                    if (!request.type || request.type==0x100) return hub_db_session_close(session);
                    if (session->callbacks.begin(session->context,&request)<0) return hub_db_session_close(session);
                    session->pending=1;
                }
            }
        }
        if (result<0) return hub_db_session_close(session);
        if (result==1) {
            session->received-=used; memmove(session->input,session->input+used,session->received);
            session->frame_started=now; session->last_progress=now; return 0;
        }
    }
    if (session->received==HUB_DB_INPUT_MAX) return hub_db_session_close(session);
    ssize_t result=recv(session->fd,session->input+session->received,HUB_DB_INPUT_MAX-session->received,0);
    if (!result) return hub_db_session_close(session);
    if (result<0) {
        if (errno!=EAGAIN && errno!=EWOULDBLOCK && errno!=EINTR) return hub_db_session_close(session);
        return 0;
    }
    if (!session->received) session->frame_started=now;
    session->received+=(size_t)result; session->last_progress=now; return 0;
}
#endif

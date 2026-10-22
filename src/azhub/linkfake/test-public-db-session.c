/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "public-db-session.h"
struct context { unsigned calls,remaining,value; uint32_t transaction; int live; };
static int alive(void *opaque) { return ((struct context *)opaque)->live; }
static int begin(void *opaque,const struct hub_db_message *request) {
    struct context *context=opaque; ++context->calls; context->remaining=2; context->transaction=request->transaction; return 0;
}
static int next(void *opaque,struct hub_db_message *reply) {
    struct context *context=opaque; if (!context->remaining) return 0;
    --context->remaining; reply->transaction=context->transaction; reply->type=0x4000; reply->count=1;
    reply->args[0]=(struct hub_db_field){.kind=HUB_DB_NUMBER,.number=context->value}; return 1;
}
static void open_session(struct hub_db_session *session,int *peer,struct context *context) {
    int pair[2]; assert(!socketpair(AF_UNIX,SOCK_STREAM,0,pair)); *peer=pair[1];
    struct hub_db_callbacks callbacks={begin,next,alive}; assert(!hub_db_session_init(session,pair[0],41,100,callbacks,context));
}
static size_t requests(unsigned char bytes[512]) {
    bytes[0]=17; hub_db_put32(bytes+1,1); size_t length=5,written;
    struct hub_db_message setup={.transaction=0xfffffffe,.type=0,.count=1};
    setup.args[0]=(struct hub_db_field){.kind=HUB_DB_NUMBER,.number=3};
    assert(!hub_db_message_write(&setup,bytes+length,512-length,&written)); length+=written;
    setup.transaction=7; setup.type=0x1004; setup.args[0].number=0x03010401;
    assert(!hub_db_message_write(&setup,bytes+length,512-length,&written)); return length+written;
}
int main(void) {
    for (unsigned variant=20;variant<=21;++variant) {
        struct context context={.live=1}; struct hub_db_session session; int peer;
        open_session(&session,&peer,&context);
        unsigned char bytes[128]={17,0,0,0,1}; size_t written;
        struct hub_db_message setup={.transaction=0xfffffffe,.type=0,.count=2};
        setup.args[0]=(struct hub_db_field){.kind=HUB_DB_NUMBER,.number=3};
        setup.args[1]=(struct hub_db_field){.kind=HUB_DB_NUMBER,.number=variant};
        assert(!hub_db_message_write(&setup,bytes+5,sizeof bytes-5,&written));
        assert(send(peer,bytes,written+5,0)==(ssize_t)(written+5));
        int result=0;
        for (unsigned i=0;i<10 && result==0;++i) result=hub_db_session_step(&session,101+i);
        if (variant==20) assert(!result && session.state==2 && session.extended_menu && session.requester==3);
        else assert(result<0 && !session.extended_menu);
        hub_db_session_destroy(&session); close(peer);
    }
    struct context a={.value=17,.live=1},b={.value=29,.live=1};
    struct hub_db_session first,second; int p,q;
    open_session(&first,&p,&a); open_session(&second,&q,&b);
    unsigned char data[512]; size_t length=requests(data);
    assert(send(p,data,1,0)==1); assert(!hub_db_session_step(&first,101));
    assert(send(q,data,length,0)==(ssize_t)length);
    for (unsigned i=0;i<20;++i) assert(!hub_db_session_step(&second,102+i));
    assert(b.calls==1 && a.calls==0 && second.requester==3);
    assert(send(p,data+1,length-1,0)==(ssize_t)length-1);
    for (unsigned i=0;i<20;++i) assert(!hub_db_session_step(&first,102+i));
    assert(a.calls==1);
    for (unsigned n=0;n<2;++n) {
        int fd=n?q:p; ssize_t got=recv(fd,data,sizeof data,MSG_DONTWAIT); assert(got>5);
        assert(data[0]==17 && hub_db_u32(data+1)==1);
        size_t offset=5,used; struct hub_db_message reply;
        assert(hub_db_message_read(data+offset,(size_t)got-offset,&reply,&used)==1);
        assert(reply.transaction==0xfffffffe && reply.args[1].number==41); offset+=used;
        for (unsigned i=0;i<2;++i) {
            assert(hub_db_message_read(data+offset,(size_t)got-offset,&reply,&used)==1);
            assert(reply.transaction==7 && reply.args[0].number==(n?29U:17U)); offset+=used;
        }
        assert(offset==(size_t)got);
    }
    /* A real three-band fixture is larger than64KiB. Force a small socket
     * send buffer and drain in tiny chunks to exercise partial writes. */
    const unsigned payload_size=119610; unsigned char *payload=malloc(payload_size);
    unsigned char *large=malloc(payload_size+128); assert(payload && large);
    for (unsigned i=0;i<payload_size;++i) payload[i]=(unsigned char)i;
    struct hub_db_message analysis={.transaction=19,.type=0x4f02,.count=2};
    analysis.args[0]=(struct hub_db_field){.kind=HUB_DB_NUMBER,.number=payload_size};
    analysis.args[1]=(struct hub_db_field){.kind=HUB_DB_BINARY,.length=payload_size,.data=payload};
    size_t expected; assert(!hub_db_message_size(&analysis,&expected));
    int small=1024; assert(!setsockopt(first.fd,SOL_SOCKET,SO_SNDBUF,&small,sizeof small));
    assert(!hub_db_session_queue(&first,&analysis) && first.output_capacity==expected);
    size_t got=0;
    for (unsigned i=0;i<10000 && got<expected;++i) {
        assert(!hub_db_session_step(&first,125));
        size_t available=expected-got; if (available>257) available=257;
        ssize_t bytes=recv(p,large+got,available,MSG_DONTWAIT);
        if (bytes>0) got+=(size_t)bytes; else assert(bytes<0 && (errno==EAGAIN || errno==EWOULDBLOCK));
    }
    assert(got==expected && first.output_capacity==HUB_DB_OUTPUT_INITIAL);
    struct hub_db_message decoded; size_t consumed;
    assert(hub_db_message_read(large,got,&decoded,&consumed)==1 && consumed==got);
    assert(decoded.args[1].length==payload_size && !memcmp(decoded.args[1].data,payload,payload_size));
    free(payload); free(large);
    hub_db_session_cancel(&first); assert(hub_db_session_step(&first,130)<0);
    assert(recv(p,data,sizeof data,0)==0 && !hub_db_session_step(&second,130));
    b.live=0; assert(hub_db_session_step(&second,131)<0);
    hub_db_session_destroy(&first); hub_db_session_destroy(&second); close(p); close(q);
    a.live=1; open_session(&first,&p,&a);
    data[0]=17; assert(send(p,data,1,0)==1); assert(!hub_db_session_step(&first,101));
    /* More bytes cannot extend the absolute handshake deadline. */
    assert(send(p,data,1,0)==1); assert(!hub_db_session_step(&first,4999));
    assert(hub_db_session_step(&first,5100)<0);
    hub_db_session_destroy(&first); close(p);
    open_session(&first,&p,&a); data[0]=99; assert(send(p,data,1,0)==1);
    assert(!hub_db_session_step(&first,101)); assert(hub_db_session_step(&first,102)<0);
    hub_db_session_destroy(&first); close(p);
    puts("PASS: nonblocking fragmented/coalesced handshake, independent clients, streamed responses, source withdrawal and absolute deadline");
}

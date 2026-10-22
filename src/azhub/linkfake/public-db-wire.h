/* SPDX-License-Identifier: GPL-3.0-only */
/* Bounded dbserver framing. Views borrow the input buffer; retain that buffer
 * until dispatch finishes. A session owns buffering, deadlines and sockets. */
#ifndef HUB_PUBLIC_DB_WIRE_H
#define HUB_PUBLIC_DB_WIRE_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#define HUB_DB_FIELD_MAX (4U*1024U*1024U)
#define HUB_DB_FRAME_MAX (HUB_DB_FIELD_MAX+4096U)
#define HUB_DB_ARGS_MAX 16U
enum hub_db_kind { HUB_DB_NUMBER=6,HUB_DB_STRING=2,HUB_DB_BINARY=3 };
struct hub_db_field { unsigned kind; uint32_t number,length; const unsigned char *data; };
struct hub_db_message { uint32_t transaction,type,count; struct hub_db_field args[HUB_DB_ARGS_MAX]; };
static uint32_t hub_db_u32(const unsigned char *p) {
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}
static void hub_db_put32(unsigned char *p,uint32_t value) {
    p[0]=(unsigned char)(value>>24); p[1]=(unsigned char)(value>>16);
    p[2]=(unsigned char)(value>>8); p[3]=(unsigned char)value;
}
static int hub_db_string_valid(const unsigned char *p,size_t size) {
    if (!p || size<2 || size%2 || p[size-2] || p[size-1]) return 0;
    for (size_t i=0;i+2<size;i+=2) {
        unsigned c=((unsigned)p[i]<<8)|p[i+1];
        if (!c) return 0;
        if (c>=0xd800 && c<=0xdbff) {
            if (i+4>=size) return 0;
            unsigned low=((unsigned)p[i+2]<<8)|p[i+3];
            if (low<0xdc00 || low>0xdfff) return 0;
            i+=2;
        } else if (c>=0xdc00 && c<=0xdfff) return 0;
    }
    return 1;
}
/* Returns1 for a complete field,0 for more bytes,-1 for malformed/oversized. */
static int hub_db_field_read(const unsigned char *data,size_t length,size_t *offset,struct hub_db_field *field) {
    size_t at=*offset;
    if (at>=length) return 0;
    unsigned tag=data[at],bytes=tag==15 ? 1 : tag==16 ? 2 : tag==17 ? 4 : 0;
    memset(field,0,sizeof *field);
    if (bytes) {
        if (length-at<bytes+1) return 0;
        field->kind=HUB_DB_NUMBER;
        for (unsigned i=0;i<bytes;++i) field->number=(field->number<<8)|data[at+1+i];
        *offset=at+bytes+1; return 1;
    }
    if (tag!=20 && tag!=38) return -1;
    if (length-at<5) return 0;
    uint32_t size=hub_db_u32(data+at+1);
    if (size>HUB_DB_FIELD_MAX/(tag==38 ? 2U : 1U)) return -1;
    if (tag==38) size*=2;
    if (at>HUB_DB_FRAME_MAX-5 || size>HUB_DB_FRAME_MAX-at-5) return -1;
    if (length-at-5<size) return 0;
    field->kind=tag==20 ? HUB_DB_BINARY : HUB_DB_STRING;
    field->length=size; field->data=data+at+5;
    if (tag==38 && !hub_db_string_valid(field->data,size)) return -1;
    *offset=at+5+size; return 1;
}
/* Parse one message, leaving any coalesced successor to the caller. On0/-1,
 * consumed is zero and no partially parsed output may be dispatched. */
static int hub_db_message_read(const unsigned char *data,size_t length,
        struct hub_db_message *message,size_t *consumed) {
    *consumed=0; memset(message,0,sizeof *message);
    struct hub_db_field header[5]; size_t offset=0;
    for (unsigned i=0;i<5;++i) {
        int result=hub_db_field_read(data,length,&offset,header+i);
        if (result!=1) return result;
        if (i<4 && header[i].kind!=HUB_DB_NUMBER) return -1;
    }
    if (header[0].number!=0x872349ae || header[2].number>65535 || header[3].number>HUB_DB_ARGS_MAX ||
        header[4].kind!=HUB_DB_BINARY) return -1;
    unsigned count=header[3].number,width=header[4].length;
    if (width<count || (width!=count && width!=12 && width!=16)) return -1;
    message->transaction=header[1].number; message->type=header[2].number; message->count=count;
    for (unsigned i=0;i<count;++i) {
        unsigned kind=header[4].data[i]; struct hub_db_field *field=message->args+i;
        if (kind!=2 && kind!=3 && kind!=4 && kind!=5 && kind!=6) return -1;
        if (kind==3 && i && message->args[i-1].kind==HUB_DB_NUMBER && !message->args[i-1].number)
            field->kind=HUB_DB_BINARY; /* Native empty binary fields have no wire bytes. */
        else {
            int result=hub_db_field_read(data,length,&offset,field);
            if (result!=1) return result;
        }
        if (field->kind!=(kind>=4 ? HUB_DB_NUMBER : kind)) return -1;
    }
    if (offset>HUB_DB_FRAME_MAX) return -1;
    *consumed=offset; return 1;
}
static int hub_db_message_size(const struct hub_db_message *message,size_t *written) {
    *written=0;
    if (message->count>HUB_DB_ARGS_MAX || message->type>65535) return -1;
    unsigned width=message->count>12 ? 16 : 12;
    size_t size=20+width;
    for (unsigned i=0;i<message->count;++i) {
        const struct hub_db_field *field=message->args+i;
        if (field->kind==HUB_DB_NUMBER) size+=5;
        else if (field->kind==HUB_DB_STRING || field->kind==HUB_DB_BINARY) {
            if (field->length>HUB_DB_FIELD_MAX || (field->length && !field->data)) return -1;
            if (field->kind==HUB_DB_STRING && !hub_db_string_valid(field->data,field->length)) return -1;
            if (field->kind==HUB_DB_BINARY && !field->length) {
                if (!i || message->args[i-1].kind!=HUB_DB_NUMBER || message->args[i-1].number) return -1;
            } else {
                if (field->kind==HUB_DB_BINARY && i && message->args[i-1].kind==HUB_DB_NUMBER && !message->args[i-1].number) return -1;
                size+=5+field->length;
            }
        } else return -1;
    }
    if (size>HUB_DB_FRAME_MAX) return -1;
    *written=size; return 0;
}
static int hub_db_message_write(const struct hub_db_message *message,unsigned char *data,
        size_t capacity,size_t *written) {
    size_t size; *written=0;
    if (hub_db_message_size(message,&size)<0 || size>capacity) return -1;
    unsigned width=message->count>12 ? 16 : 12;
    data[0]=17; hub_db_put32(data+1,0x872349ae); data[5]=17; hub_db_put32(data+6,message->transaction);
    data[10]=16; data[11]=(unsigned char)(message->type>>8); data[12]=(unsigned char)message->type;
    data[13]=15; data[14]=(unsigned char)message->count; data[15]=20; hub_db_put32(data+16,width);
    memset(data+20,0,width); size_t offset=20+width;
    for (unsigned i=0;i<message->count;++i) {
        const struct hub_db_field *field=message->args+i; data[20+i]=(unsigned char)field->kind;
        if (field->kind==HUB_DB_NUMBER) { data[offset++]=17; hub_db_put32(data+offset,field->number); offset+=4; }
        else if (field->length) {
            data[offset++]=field->kind==HUB_DB_STRING ? 38 : 20;
            hub_db_put32(data+offset,field->kind==HUB_DB_STRING ? field->length/2 : field->length); offset+=4;
            memcpy(data+offset,field->data,field->length); offset+=field->length;
        }
    }
    *written=offset; return 0;
}
#endif

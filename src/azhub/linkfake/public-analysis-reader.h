/* SPDX-License-Identifier: GPL-3.0-only */
/* Incremental ANLZ section reader. Caller supplies generation/fingerprint-checked
 * source I/O and publication/withdrawal policy. Payload retains its tag header. */
#ifndef HUB_PUBLIC_ANALYSIS_READER_H
#define HUB_PUBLIC_ANALYSIS_READER_H
#include <errno.h>
#include <stdlib.h>
#include "public-db-wire.h"
#define HUB_ANALYSIS_FILE_MAX (64U*1024U*1024U)
#define HUB_ANALYSIS_TAG_MAX (4U*1024U*1024U)
#define HUB_ANALYSIS_TAG_COUNT_MAX 1024U
typedef int (*hub_analysis_read_fn)(void *,uint64_t,void *,size_t);
struct hub_analysis_reader {
    hub_analysis_read_fn read;
    void *context;
    uint32_t size,offset,target,occurrence,tags,length,filled;
    unsigned char *data;
    int done,error;
};
static int hub_analysis_fail(struct hub_analysis_reader *reader,int error) {
    reader->error=error ? error : EIO; errno=reader->error; return -1;
}
static void hub_analysis_destroy(struct hub_analysis_reader *reader) {
    free(reader->data); memset(reader,0,sizeof *reader);
}
static int hub_analysis_init(struct hub_analysis_reader *reader,uint64_t size,hub_analysis_read_fn read,
        void *context,uint32_t tag,unsigned occurrence) {
    memset(reader,0,sizeof *reader); reader->read=read; reader->context=context;
    reader->target=tag; reader->occurrence=occurrence;
    if (!read || size<12 || size>HUB_ANALYSIS_FILE_MAX || occurrence>=HUB_ANALYSIS_TAG_COUNT_MAX)
        return hub_analysis_fail(reader,EINVAL);
    unsigned char header[12];
    if (read(context,0,header,sizeof header)<0) return hub_analysis_fail(reader,errno);
    reader->size=(uint32_t)size; reader->offset=hub_db_u32(header+4);
    if (memcmp(header,"PMAI",4) || hub_db_u32(header+8)!=size || reader->offset<12 || reader->offset>size)
        return hub_analysis_fail(reader,EINVAL);
    return 0;
}
/* At most one12-byte tag header or8192-byte payload chunk per step. Repeated
 * tags are selected by occurrence because memory/hot-cue tags share a fourcc. */
static int hub_analysis_step(struct hub_analysis_reader *reader) {
    if (reader->error) { errno=reader->error; return -1; }
    if (reader->done) return 1;
    if (reader->data) {
        size_t size=reader->length-reader->filled; if (size>8192) size=8192;
        if (reader->read(reader->context,(uint64_t)reader->offset+reader->filled,reader->data+reader->filled,size)<0)
            return hub_analysis_fail(reader,errno);
        reader->filled+=(uint32_t)size;
        if (reader->filled==reader->length) { reader->done=1; return 1; }
        return 0;
    }
    if (reader->offset==reader->size) return hub_analysis_fail(reader,ENOENT);
    if (reader->size-reader->offset<12 || reader->tags==HUB_ANALYSIS_TAG_COUNT_MAX)
        return hub_analysis_fail(reader,EINVAL);
    unsigned char header[12];
    if (reader->read(reader->context,reader->offset,header,sizeof header)<0) return hub_analysis_fail(reader,errno);
    ++reader->tags;
    uint32_t header_size=hub_db_u32(header+4),length=hub_db_u32(header+8);
    if (header_size<12 || length<header_size || length>reader->size-reader->offset)
        return hub_analysis_fail(reader,EINVAL);
    if (hub_db_u32(header)==reader->target && !reader->occurrence) {
        if (length>HUB_ANALYSIS_TAG_MAX) return hub_analysis_fail(reader,EFBIG);
        reader->data=malloc(length); if (!reader->data) return hub_analysis_fail(reader,ENOMEM);
        reader->length=length; reader->filled=12; memcpy(reader->data,header,12);
        if (length==12) { reader->done=1; return 1; }
    } else {
        if (hub_db_u32(header)==reader->target) --reader->occurrence;
        reader->offset+=length;
    }
    return 0;
}
static const unsigned char *hub_analysis_ready(const struct hub_analysis_reader *reader,size_t *length) {
    *length=0;
    if (!reader->done || reader->error) return NULL;
    *length=reader->length; return reader->data;
}
#endif

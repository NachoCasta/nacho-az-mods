/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "link-member-retention.h"
static unsigned allocations,releases;
static void initialize(unsigned char *record,unsigned member,unsigned serial,int private_ip) {
    memset(record,0,HUB_MEMBER_RECORD_SIZE);
    record[0]=1;record[1]=member;
    record[8]=private_ip?127:192;record[9]=private_ip?77:0;
    record[10]=private_ip?0:2;record[11]=member;
    memcpy(record+0x48,&serial,sizeof serial);
    for(unsigned offset=0x30;offset<=0x40;offset+=0x10) {
        unsigned *payload=malloc(sizeof *payload);assert(payload);
        *payload=serial;memcpy(record+offset,&payload,sizeof payload);allocations++;
    }
}
static void release_record(void *opaque) {
    unsigned char *record=opaque;
    unsigned serial;memcpy(&serial,record+0x48,sizeof serial);
    for(unsigned offset=0x30;offset<=0x40;offset+=0x10) {
        unsigned *payload;memcpy(&payload,record+offset,sizeof payload);
        assert(*payload==serial);free(payload);releases++;
    }
}
static void exercise(size_t count,unsigned rotation,uint32_t mask) {
    unsigned char table[HUB_MEMBER_RETENTION_MAX+1][HUB_MEMBER_RECORD_SIZE];
    unsigned char before[sizeof table];
    for(size_t i=0;i<count;i++) {
        unsigned kind=(i+rotation)%5;
        initialize(table[i],kind==0?7:kind==1?8:kind==2?7:kind==3?8:2,
            (unsigned)i+100,kind<2);
    }
    memcpy(before,table,count*HUB_MEMBER_RECORD_SIZE);
    struct hub_member_removed removed={.release_record=release_record};
    size_t kept=999,owned=0;
    for(size_t i=0;i<count;i++) owned+=hub_member_is_owned(table[i],mask);
    int result=hub_member_partition(table[0],count,mask,&removed,&kept);
    if(count>HUB_MEMBER_RETENTION_MAX || !owned) {
        assert(result==(count>HUB_MEMBER_RETENTION_MAX?-1:0));
        assert(kept==999 && removed.count==0 && !memcmp(before,table,count*HUB_MEMBER_RECORD_SIZE));
        for(size_t i=0;i<count;i++) release_record(table[i]);
    } else {
        assert(result==1 && kept==owned && removed.count==count-owned);
        size_t a=0,b=0;
        for(size_t i=0;i<count;i++) {
            unsigned char *old=before+i*HUB_MEMBER_RECORD_SIZE;
            unsigned char *actual=hub_member_is_owned(old,mask)?table[a++]:removed.records[b++];
            assert(!memcmp(old,actual,HUB_MEMBER_RECORD_SIZE));
        }
        hub_member_removed_release(&removed);
        hub_member_removed_release(&removed); /* Cleanup is idempotent. */
        for(size_t i=0;i<kept;i++) release_record(table[i]);
    }
    assert(allocations==releases);
}
int main(void) {
    uint32_t both=(UINT32_C(1)<<7)|(UINT32_C(1)<<8);
    for(size_t count=0;count<=HUB_MEMBER_RETENTION_MAX+1;count++)
        for(unsigned rotation=0;rotation<5;rotation++) {
            exercise(count,rotation,0);
            exercise(count,rotation,UINT32_C(1)<<7);
            exercise(count,rotation,UINT32_C(1)<<8);
            exercise(count,rotation,both);
        }
    unsigned char row[HUB_MEMBER_RECORD_SIZE]={0};row[0]=1;row[1]=7;
    row[8]=127;row[9]=77;row[11]=7;
    assert(hub_member_is_owned(row,both));
    row[0]=2;assert(!hub_member_is_owned(row,both));row[0]=1;
    row[11]=8;assert(!hub_member_is_owned(row,both));row[11]=7;
    row[10]=1;assert(!hub_member_is_owned(row,both));row[10]=0;
    row[1]=32;assert(!hub_member_is_owned(row,UINT32_MAX));
    struct hub_member_removed removed={.release_record=release_record};
    size_t kept=123;
    assert(hub_member_partition(removed.records[0],1,both,&removed,&kept)==-1);
    assert(hub_member_partition(NULL,1,both,&removed,&kept)==-1);
    assert(kept==123 && !removed.count && allocations==releases);
    puts("PASS: mixed member retention, exact private-address gates, stable order, ownership transfer, fallback and capacity/alias guards");
}

/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include "source-order.h"
struct row { uint32_t hub,number; uint64_t identity,string1,string2,other; };
_Static_assert(sizeof(struct row)==40,"EP147 row stride");
static int number(const unsigned char *bytes) {
    struct row r;memcpy(&r,bytes,sizeof r);return r.hub ? (int)r.number:0;
}
int main(void) {
    struct row rows[12]={0};
    static const int order[]={8,2,5,1,7,3,6,4};
    rows[0]=(struct row){0,1,100,101,102,103};
    rows[1]=(struct row){0,2,200,201,202,203};
    for (int i=0;i<8;i++) rows[i+2]=(struct row){1,order[i],order[i]+300,
        order[i]+400,order[i]+500,order[i]+600};
    rows[10]=(struct row){0,0,700,701,702,703};
    struct row native[3]={rows[0],rows[1],rows[10]};
    assert(hub_source_order_rows(rows,11,number));
    assert(!memcmp(&rows[0],&native[0],40) && !memcmp(&rows[1],&native[1],40));
    assert(!memcmp(&rows[10],&native[2],40));
    for (int i=0;i<8;i++) {
        struct row r=rows[i+2];assert(r.number==(unsigned)i+1);
        assert(r.identity==r.number+300 && r.string1==r.number+400 &&
               r.string2==r.number+500 && r.other==r.number+600);
    }
    assert(!hub_source_order_rows(rows,11,number));
    rows[11]=(struct row){1,1,0,0,0,0};
    struct row before[12];memcpy(before,rows,sizeof rows);
    assert(!hub_source_order_rows(rows,12,number));
    assert(!memcmp(before,rows,sizeof rows)); /* Fail closed on excess hubs. */
    assert(!hub_source_order_rows(rows,129,number));
    return 0;
}

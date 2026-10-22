/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HUB_SOURCE_ORDER_H
#define HUB_SOURCE_ORDER_H
#include <stddef.h>
#include <string.h>

/* Exact EP147 Source rows contain two retained String handles, not inline
 * self-relative strings. Move whole rows without acquiring/releasing handles.
 * Only hub positions participate; native and external rows keep their places. */
static int hub_source_order_rows(void *storage,size_t count,
                                int (*number)(const unsigned char *)) {
    if (!storage || !number || count>128) return 0;
    unsigned char *rows=storage,temporary[40];
    size_t positions[8],used=0;
    int numbers[8];
    for (size_t i=0;i<count;i++) {
        int n=number(rows+i*40);
        if (!n) continue;
        if (n<1 || n>8 || used==8) return 0;
        positions[used]=i; numbers[used++]=n;
    }
    int moved=0;
    for (size_t i=1;i<used;i++) {
        size_t j=i;
        while (j && numbers[j]<numbers[j-1]) {
            unsigned char *a=rows+positions[j-1]*40,*b=rows+positions[j]*40;
            memcpy(temporary,a,40); memcpy(a,b,40); memcpy(b,temporary,40);
            int n=numbers[j];numbers[j]=numbers[j-1];numbers[j-1]=n;
            j--;moved=1;
        }
    }
    return moved;
}
#endif

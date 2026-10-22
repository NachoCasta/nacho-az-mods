/* SPDX-License-Identifier: GPL-3.0-only */
/* Uniform0..1000ms from independent random16-bit samples. Invalid sentinel1001
 * makes readiness fail closed; never silently replace missing entropy by zero. */
#ifndef HUB_PUBLIC_ACD_JITTER_H
#define HUB_PUBLIC_ACD_JITTER_H
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
static unsigned hub_public_acd_jitter(int entropy) {
    for (unsigned attempt=0;attempt<8;++attempt) {
        uint16_t value;ssize_t got=read(entropy,&value,sizeof value);
        if (got<0 && errno==EINTR) continue;
        if (got!=(ssize_t)sizeof value) { if (got>=0) errno=EIO;return 1001; }
        if (value<65065) return value%1001;
    }
    errno=EAGAIN;return 1001;
}
#endif

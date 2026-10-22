/* SPDX-License-Identifier: GPL-3.0-only */
/* RFC3927 candidate selection only. A sampled address is not configured or
 * advertised until independent inventory exclusion and continuous ACD pass. */
#ifndef HUB_PUBLIC_LINK_LOCAL_H
#define HUB_PUBLIC_LINK_LOCAL_H
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
static int hub_public_link_local_sample(int entropy,struct in_addr *address) {
    if (!address) { errno=EINVAL;return -1; }
    for (unsigned attempt=0;attempt<8;++attempt) {
        uint16_t sample;ssize_t got=read(entropy,&sample,sizeof sample);
        if (got<0 && errno==EINTR) continue;
        if (got!=(ssize_t)sizeof sample) { if (got>=0) errno=EIO;return -1; }
        if (sample<65024) { address->s_addr=htonl(UINT32_C(0xa9fe0100)+sample);return 0; }
    }
    errno=EAGAIN;return -1;
}
#endif

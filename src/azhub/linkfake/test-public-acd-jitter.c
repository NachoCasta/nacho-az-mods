/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "public-acd-jitter.h"
int main(void) {
    int pipefd[2];assert(!pipe(pipefd));
    uint16_t samples[]={65065,65535,65064,0,1001};
    assert(write(pipefd[1],samples,sizeof samples)==sizeof samples);close(pipefd[1]);
    assert(hub_public_acd_jitter(pipefd[0])==1000);
    assert(!hub_public_acd_jitter(pipefd[0]) && !hub_public_acd_jitter(pipefd[0]));
    assert(hub_public_acd_jitter(pipefd[0])==1001 && errno==EIO);close(pipefd[0]);
    assert(hub_public_acd_jitter(-1)==1001 && errno==EBADF);
    puts("PASS: unbiased bounded ACD jitter, rejection sampling and missing-entropy failure sentinel");
}

/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include "public-parent-rx-probe.h"
static int acquired,released,fail;
static int acquire(const char *name) {assert(!strcmp(name,"eth0"));++acquired;if(fail){errno=EPERM;return -1;}return 42;}
static int release(int fd) {assert(fd==42);++released;return 0;}
int main(void) {
 struct hub_public_parent_rx_probe p={.fd=-1};
 hub_public_parent_rx_probe_poll(&p,1,0,"eth0",acquire,release);assert(!p.phase && !acquired);
 hub_public_parent_rx_probe_poll(&p,100,1,"eth0",acquire,release);assert(p.phase==1);
 hub_public_parent_rx_probe_poll(&p,10099,1,"eth0",acquire,release);assert(!acquired);
 hub_public_parent_rx_probe_poll(&p,10100,1,"eth0",acquire,release);assert(acquired==1 && p.fd==42);
 hub_public_parent_rx_probe_poll(&p,70099,1,"eth0",acquire,release);assert(!released);
 hub_public_parent_rx_probe_poll(&p,70100,1,"eth0",acquire,release);assert(released==1 && p.fd==-1);
 hub_public_parent_rx_probe_poll(&p,90000,1,"eth0",acquire,release);assert(acquired==1 && released==1);
 p=(struct hub_public_parent_rx_probe){.fd=-1};fail=1;
 hub_public_parent_rx_probe_poll(&p,1,1,"eth0",acquire,release);
 hub_public_parent_rx_probe_poll(&p,10001,1,"eth0",acquire,release);assert(p.phase==3 && p.fd==-1 && released==1);
 puts("PASS: diagnostic waits for readiness, preserves baseline, owns one bounded membership and does not retry failure");
}

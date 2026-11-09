/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <string.h>
#include <pthread.h>
typedef uint64_t (*fn8)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
static int hub_local_transport_only,tmpl_len;
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static int lf_runtime_ready(void) { return 1; }
#define lf_log(...) ((void)0)
static fn8 make_hook(uintptr_t a,fn8 h) { (void)a;return h; }
extern void local_transport_native_change(uintptr_t owner);
#define HUB_LOCAL_NETWORK_CHANGE(owner) local_transport_native_change(owner)
#include "local-transport-native.h"
void local_transport_unwind_run(uintptr_t owner) { hub_local_transport_kick(owner); }
int local_transport_unwind_active(void) { return hub_local_transport_starting; }

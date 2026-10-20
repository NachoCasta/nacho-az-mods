/* SPDX-License-Identifier: GPL-3.0-only */
/* Compile as C with -fexceptions; a C++ listener unwinds through the actual
 * native adapter. Only call-site provenance and string release are substituted. */
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef uint64_t (*fn8)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
static pthread_mutex_t probe_mode_cache_mutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t safe_source_mutex=PTHREAD_MUTEX_INITIALIZER;
static int probe_mode_transition_active=1,nfakes=1;
static uint32_t probe_mode_retained_mask;
static struct fake {int player,present,eject_reserved,unmount_pending;} fakes[]={{7,1,0,0}};
static unsigned freed;
static uintptr_t test_call_site=0x13f0770;
static int lf_runtime_ready(void){return 1;}
static void host_release(void *field) {void *value;memcpy(&value,field,sizeof value);free(value);freed++;}
#define lf_log(...) ((void)0)
#define LF_MEMBER_CALL_SITE() test_call_site
#define LF_MEMBER_STRING_RELEASE(value) host_release(value)
static fn8 make_hook(uintptr_t address,fn8 callback){(void)address;return callback;}
#include "link-member-continuity.h"
static unsigned char *owner;
static unsigned char table[3][HUB_MEMBER_RECORD_SIZE];
static uintptr_t listener,registry;
static fn8 listener_vtable[6];
extern uint64_t throwing_member_listener(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
void member_unwind_setup(void) {
    freed=0;probe_mode_retained_mask=0;
    owner=calloc(1,0x500);assert(owner);
    assert(!pthread_mutex_init((pthread_mutex_t *)(owner+0x3c8),NULL));
    memset(table,0,sizeof table);
    for(unsigned i=0;i<3;i++) {
        table[i][0]=1;table[i][1]=i==1?7:2+i;
        table[i][8]=i==1?127:192;table[i][9]=i==1?77:0;table[i][11]=table[i][1];
        for(unsigned offset=0x30;offset<=0x40;offset+=0x10) {
            void *payload=malloc(16);assert(payload);memcpy(table[i]+offset,&payload,sizeof payload);
        }
    }
    uintptr_t begin=(uintptr_t)table,end=begin+sizeof table;
    memcpy(owner+0x380,&begin,8);memcpy(owner+0x388,&end,8);memcpy(owner+0x390,&end,8);
    listener_vtable[5]=throwing_member_listener;
    listener=(uintptr_t)listener_vtable;registry=(uintptr_t)&listener;
    uintptr_t vector=(uintptr_t)&registry;memcpy(owner+0x3b0,&vector,8);
    int count=1;memcpy(owner+0x3c0,&count,4);
}
void member_unwind_dispatch(void) {probe_member_clear((uintptr_t)owner,1,0,0,0,0,0,0);}
void member_listener_assert_unlocked(void) {
    pthread_mutex_t *locks[]={&safe_source_mutex,&probe_mode_cache_mutex,
        (pthread_mutex_t *)(owner+0x3c8)};
    for(unsigned i=0;i<sizeof locks/sizeof locks[0];i++) {
        assert(!pthread_mutex_trylock(locks[i]));assert(!pthread_mutex_unlock(locks[i]));
    }
}
void member_unwind_verify(void) {
    uintptr_t end;memcpy(&end,owner+0x388,8);
    assert(end==(uintptr_t)table+HUB_MEMBER_RECORD_SIZE && table[0][1]==7);
    assert(freed==4 && probe_mode_retained_mask==(1u<<7));
    probe_member_release_record(table[0]);assert(freed==6);
    assert(!pthread_mutex_destroy((pthread_mutex_t *)(owner+0x3c8)));
    free(owner);
}

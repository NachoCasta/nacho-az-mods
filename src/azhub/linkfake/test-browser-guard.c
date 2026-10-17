/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint64_t (*fn8)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
struct fake { int player,slot,present; };
static struct fake fakes[1]={{3,7,1}};
static int nfakes=1;
static int media_sdid(int slot) { return slot==7 ? 0x22 : 2; }
#define lf_log(...) ((void)0)
static int deleted,updated,replied,complete,consume;
static void destroy(uint64_t event) { if(event) deleted++; }
#define HUB_DESTROY_RENEWAL(event) destroy(event)
#include "browser-guard.h"
static uint64_t handler[4],incoming[24],next[24],current[20],agent[8],event;
static uint64_t original_destroy(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) { return 0; }
static uint64_t original_update(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    updated++;
    if(consume) {
        uint64_t old=*(uint64_t *)(a+0x10);
        h_browser_event_destroy(old,0,0,0,0,0,0,0);
        *(uint64_t *)(a+0x10)=*(uint64_t *)b;
        *(uint64_t *)b=0;
    }
    return 42;
}
static uint64_t original_reply(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    replied++; if(complete) agent[6]=0; return 73;
}
static void setup(void) {
    memset(hub_renewals,0,sizeof hub_renewals);memset(handler,0,sizeof handler);
    memset(incoming,0,sizeof incoming);memset(next,0,sizeof next);
    memset(current,0,sizeof current);memset(agent,0,sizeof agent);
    incoming[0]=next[0]=0x3165bf0;handler[2]=(uintptr_t)current;
    current[5]=(uintptr_t)agent;
    agent[0]=0x3161140;agent[5]=0x1012200000202ULL;agent[6]=1;
    incoming[0x98/8]=next[0x98/8]=agent[5];event=(uintptr_t)incoming;
    deleted=updated=replied=complete=0;consume=1;fakes[0].present=1;
}
static uint64_t update(void) { return h_browser_update((uintptr_t)handler,(uintptr_t)&event,0,0,0,0,0,0); }
static uint64_t reply(void) { return h_browser_reply(0,0,0,0,0,0,0,0); }
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"check failed at line %d: %s\n",__LINE__,#x);return 1; } } while(0)
int main(void) {
    o_browser_update=original_update;o_browser_reply=original_reply;o_browser_event_destroy=original_destroy;
    setup();CHECK(update()==0 && !event && !updated && hub_renewals[0].event==(uintptr_t)incoming && handler[2]==(uintptr_t)current);
    CHECK(reply()==73 && replied==1 && !updated && hub_renewals[0].event);
    event=(uintptr_t)next;CHECK(update()==0 && !event && deleted==1 && hub_renewals[0].event==(uintptr_t)next);
    complete=1;CHECK(reply()==73 && updated==1 && !hub_renewals[0].event && handler[2]==(uintptr_t)next && deleted==1);
    setup();update();event=(uintptr_t)next;next[0]=0x3160000;CHECK(update()==42 && updated==1 && deleted==1 && !hub_renewals[0].event);
    setup();update();h_browser_event_destroy((uintptr_t)current,0,0,0,0,0,0,0);handler[2]=1;CHECK(reply()==73 && deleted==1 && !updated && !hub_renewals[0].event);
    setup();agent[5]=0x1010200000000ULL;CHECK(update()==42 && updated==1 && !hub_renewals[0].event);
    setup();next[0x98/8]=0x1012200000302ULL;event=(uintptr_t)next;CHECK(update()==42 && updated==1);
    setup();agent[6]=0x100000001ULL;CHECK(update()==42 && updated==1);
    setup();agent[0]=0x3160000;CHECK(update()==42 && updated==1);
    setup();fakes[0].present=0;CHECK(update()==42 && updated==1);
    setup();update();consume=0;complete=1;reply();CHECK(updated==1 && deleted==1 && !hub_renewals[0].event);
    setup();update();handler[2]=0;hub_flush_renewals();CHECK(deleted==1 && !updated && !hub_renewals[0].event);
    setup();update();current[5]=0;hub_flush_renewals();CHECK(deleted==1 && !updated && !hub_renewals[0].event);
    setup();for(int i=0;i<HUB_RENEWALS;i++) hub_renewals[i]=(struct hub_renewal){1,2,3,4};
    CHECK(update()==0 && event==(uintptr_t)incoming && !updated && !deleted);
    puts("browser renewal deferral passed (15 ownership/lifecycle checks)");return 0;
}

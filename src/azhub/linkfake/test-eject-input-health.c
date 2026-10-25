/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "eject-input-health.h"
static void stamp(unsigned char *frame) {
    unsigned crc=0xffff;
    for (unsigned i=0;i<96;i++) {
        crc^=frame[i];
        for (unsigned bit=0;bit<8;bit++) crc=(crc>>1)^((crc&1) ? 0x8408:0);
    }
    crc^=0xffff;frame[96]=crc;frame[97]=crc>>8;
}
static void *reader(void *argument) {
    struct hub_input_health *state=argument;
    for (unsigned n=1;n<=100000;n++) hub_input_health_note(state,n,n);
    return NULL;
}
int main(void) {
    struct hub_input_health state={0};
    assert(!hub_input_health_ready(&state));
    hub_input_health_note(&state,0,0); /* startup before the core loads the mod */
    assert(!state.fault);
    __atomic_store_n(&state.armed,1,__ATOMIC_RELEASE);
    for (unsigned idle=0;idle<100000;idle++) assert(hub_input_health_ready(&state));
    hub_input_health_note(&state,1,1);
    assert(hub_input_health_ready(&state));
    hub_input_health_note(&state,2,1); /* editing filter stopped */
    assert(!hub_input_health_ready(&state));
    state=(struct hub_input_health){.armed=1,.previous_raw=1,.previous_edit=1};
    hub_input_health_note(&state,1,2); /* raw filter stopped */
    assert(!hub_input_health_ready(&state));
    state=(struct hub_input_health){.armed=1,.previous_raw=1,.previous_edit=1};
    hub_input_health_note(&state,1,1); /* both stopped during a real input read */
    assert(!hub_input_health_ready(&state));
    assert(hub_input_health_fault(&state)==HUB_INPUT_FAULT_MASK);
    hub_input_health_note(&state,2,2); /* one good read is insufficient */
    assert(!hub_input_health_ready(&state));
    for (unsigned n=3;n<=8;n++) {
        hub_input_health_note(&state,n,n);
        assert(!hub_input_health_ready(&state));
    }
    hub_input_health_note(&state,9,9);
    assert(hub_input_health_ready(&state));
    hub_input_health_note(&state,9,9); /* isolated duplicate valid read */
    assert(!hub_input_health_ready(&state));
    for (unsigned n=10;n<1000010;n++) hub_input_health_note(&state,n,n);
    assert(hub_input_health_ready(&state));
    for (unsigned n=1000010;n<1000110;n++) hub_input_health_note(&state,1000009,n);
    assert(hub_input_health_fault(&state)==HUB_INPUT_RAW_MISSING);
    for (unsigned n=1000110;n<1000117;n++) hub_input_health_note(&state,n,n);
    assert(!hub_input_health_ready(&state));
    hub_input_health_note(&state,1000116,1000117); /* resets recovery */
    for (unsigned n=1000118;n<1000125;n++) {
        hub_input_health_note(&state,n,n);
        assert(!hub_input_health_ready(&state));
    }
    hub_input_health_note(&state,1000125,1000125);
    assert(hub_input_health_ready(&state));
    hub_input_health_note(&state,1000126,1000125);
    assert(hub_input_health_fault(&state)==HUB_INPUT_EDIT_MISSING);
    for (unsigned n=1000127;n<1000227;n++) hub_input_health_note(&state,n,1000125);
    assert(!hub_input_health_ready(&state));
    state=(struct hub_input_health){.armed=1,.previous_raw=UINT_MAX,.previous_edit=UINT_MAX};
    hub_input_health_note(&state,0,0);
    assert(hub_input_health_ready(&state));
    unsigned char frame[128]={0},original[128];
    stamp(frame);memcpy(original,frame,sizeof frame);
    assert(hub_panel_frame_valid(frame,98));
    assert(hub_panel_frame_valid(frame,128));
    assert(!hub_panel_frame_valid(frame,97));
    frame[34]^=1;assert(!hub_panel_frame_valid(frame,128));frame[34]^=1;
    assert(!memcmp(original,frame,sizeof frame));
    memset(frame,255,sizeof frame);assert(!hub_panel_frame_valid(frame,128));
    state=(struct hub_input_health){0};
    pthread_t thread;assert(!pthread_create(&thread,NULL,reader,&state));
    __atomic_store_n(&state.armed,1,__ATOMIC_RELEASE);
    for (unsigned n=0;n<100000;n++) assert(hub_input_health_ready(&state));
    assert(!pthread_join(thread,NULL));assert(!state.fault);
    puts("eject input health: idle/startup, fault reasons, bounded recovery, interrupted recovery, permanently stopped filters, million-read continuation, wrap, CRC and concurrent publication passed");
}

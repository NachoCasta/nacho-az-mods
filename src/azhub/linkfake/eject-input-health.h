/* SPDX-License-Identifier: GPL-3.0-only */
/* Panel readiness is checked per valid read, never by an idle timeout. */
#ifndef HUB_EJECT_INPUT_HEALTH_H
#define HUB_EJECT_INPUT_HEALTH_H
#include <stddef.h>
#include <stdint.h>
struct hub_input_health {
    unsigned armed, fault;
    unsigned previous_raw, previous_edit;
};
enum {
    HUB_INPUT_RAW_MISSING=1,
    HUB_INPUT_EDIT_MISSING=2,
    HUB_INPUT_FAULT_MASK=3,
    HUB_INPUT_RECOVERY_READS=8
};
static int hub_panel_frame_valid(const unsigned char *frame,size_t length) {
    if (length<98) return 0;
    uint16_t crc=0xffff;
    for (size_t i=0;i<96;i++) {
        crc^=frame[i];
        for (unsigned bit=0;bit<8;bit++) crc=(crc>>1)^((crc&1) ? 0x8408:0);
    }
    crc^=0xffff;
    return frame[96]==(unsigned char)crc && frame[97]==(unsigned char)(crc>>8);
}
/* The UI publishes armed and reads fault. Atomic snapshots also tolerate an
 * auxiliary reader opening the panel: an unsupported unfiltered read refuses
 * ownership conservatively rather than creating a data race. */
static void hub_input_health_note(struct hub_input_health *state,unsigned raw,unsigned edited) {
    unsigned previous_raw=__atomic_exchange_n(&state->previous_raw,raw,__ATOMIC_RELAXED);
    unsigned previous_edit=__atomic_exchange_n(&state->previous_edit,edited,__ATOMIC_RELAXED);
    if (!__atomic_load_n(&state->armed,__ATOMIC_ACQUIRE)) return;
    unsigned missing=(raw==previous_raw ? HUB_INPUT_RAW_MISSING:0) |
                     (edited==previous_edit ? HUB_INPUT_EDIT_MISSING:0);
    /* Keep the reason and recovery streak in one atomic word. A new failed
     * observation resets the streak; idle time cannot clear a fault. Only
     * consecutive reads with both callbacks advancing establish recovery.
     * A permanently disabled SDK filter therefore remains unavailable. */
    unsigned old=__atomic_load_n(&state->fault,__ATOMIC_ACQUIRE),next;
    do {
        next=0;
        if (missing) next=(old&HUB_INPUT_FAULT_MASK)|missing;
        else if (old&HUB_INPUT_FAULT_MASK) {
            unsigned healthy=(old>>2)+1;
            if (healthy<HUB_INPUT_RECOVERY_READS)
                next=(old&HUB_INPUT_FAULT_MASK)|(healthy<<2);
        }
    } while (!__atomic_compare_exchange_n(&state->fault,&old,next,0,
                                          __ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE));
}
static unsigned hub_input_health_fault(const struct hub_input_health *state) {
    return __atomic_load_n(&state->fault,__ATOMIC_ACQUIRE)&HUB_INPUT_FAULT_MASK;
}
static int hub_input_health_ready(const struct hub_input_health *state) {
    return __atomic_load_n(&state->armed,__ATOMIC_ACQUIRE) &&
        !hub_input_health_fault(state);
}
#endif

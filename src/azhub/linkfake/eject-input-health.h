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
    if (__atomic_load_n(&state->armed,__ATOMIC_ACQUIRE) &&
        (raw==previous_raw || edited==previous_edit))
        __atomic_store_n(&state->fault,1,__ATOMIC_RELEASE);
}
static int hub_input_health_ready(const struct hub_input_health *state) {
    return __atomic_load_n(&state->armed,__ATOMIC_ACQUIRE) &&
        !__atomic_load_n(&state->fault,__ATOMIC_ACQUIRE);
}
#endif

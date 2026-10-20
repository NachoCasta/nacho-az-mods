/* SPDX-License-Identifier: GPL-3.0-only */
/* Scoped admission of the mod's private members, included after linkfake.c.
 * Callers must first enforce transport provenance at the UDP receive boundary.
 * Exact EP147: connecting event 9 uses its parser's member record at +0x78;
 * record byte 0 is the compact number and bytes 8..11 are the IPv4 address.
 * The second network snapshot supplies the subnet mask for admission. Modify
 * that returned copy only; never change the manager's stored configuration.
 */
static fn8 probe_operate_original;
/* Native return storage is passed in x8, confirmed at 0x013df920. */
struct probe_network_copy { unsigned char bytes[0x50]; };
typedef struct probe_network_copy (*probe_network_fn)(uint64_t);
static probe_network_fn probe_network_original;
static __thread uint64_t probe_private_manager;
static __thread unsigned probe_network_reads;

static struct probe_network_copy probe_network_snapshot(uint64_t manager) {
    struct probe_network_copy result=probe_network_original(manager);
    if (manager==probe_private_manager && ++probe_network_reads==2) {
        /* IPv4 mask occupies snapshot +0x19..+0x1c. */
        memset(result.bytes+0x19,0,4);
    }
    return result;
}

static uint64_t probe_operate(uint64_t state, uint64_t event,
    uint64_t c, uint64_t d, uint64_t e, uint64_t f, uint64_t g, uint64_t h) {
    uint64_t previous_manager=probe_private_manager;
    unsigned previous_reads=probe_network_reads;
    probe_private_manager=0;
    probe_network_reads=0;
    if (lf_runtime_ready() && (event&255)==9) {
        uint64_t parser=((uint64_t *)state)[1];
        const unsigned char *record=*(const unsigned char **)(parser+0x78);
        if (record && (record[0]==7 || record[0]==8) &&
            record[8]==127 && record[9]==77 && record[10]==0 &&
            record[11]==record[0])
            probe_private_manager=((uint64_t *)state)[2];
    }
    uint64_t result=probe_operate_original(state,event,c,d,e,f,g,h);
    probe_private_manager=previous_manager;
    probe_network_reads=previous_reads;
    return result;
}

static void probe_private_admission_install(void) {
    probe_network_original=make_hook(0x13df908,probe_network_snapshot);
    probe_operate_original=make_hook(0x13f22b8,probe_operate);
}

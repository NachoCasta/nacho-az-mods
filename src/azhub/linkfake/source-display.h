/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HUB_SOURCE_DISPLAY_H
#define HUB_SOURCE_DISPLAY_H
/* Presentation-only port reservations preserve transport identities and routing.
 * Legacy/manual configurations retain the original identity-based labels.
 * Callers must establish private-source ownership before rendering a number. */
static unsigned char hub_source_port_numbers[8];
static int hub_source_identity_index(int player,int slot) {
    static const int players[]={7,7,8,8,7,8,7,8};
    static const int slots[]={3,7,3,7,8,8,2,2};
    for (unsigned i=0;i<sizeof players/sizeof players[0];i++)
        if (player==players[i] && slot==slots[i]) return (int)i;
    return -1;
}
/* Written only by source reconciliation; atomics publish to native UI readers.
 * Reservations survive removal so reconnect cannot steal a cached track label. */
static int hub_source_display_number(int player,int slot) {
    int index=hub_source_identity_index(player,slot);
    if (index<0) return 0;
    unsigned number=__atomic_load_n(&hub_source_port_numbers[index],__ATOMIC_ACQUIRE);
    return number ? (int)number:index+1;
}
static int hub_source_port_available(int player,int slot,unsigned number) {
    int index=hub_source_identity_index(player,slot);
    if (index<0 || number<1 || number>8) return 0;
    for (int i=0;i<8;i++) {
        unsigned current=__atomic_load_n(&hub_source_port_numbers[i],__ATOMIC_ACQUIRE);
        if ((i==index && current && current!=number) || (i!=index && current==number)) return 0;
    }
    return 1;
}
static int hub_source_port_reserve(int player,int slot,unsigned number) {
    if (!hub_source_port_available(player,slot,number)) return 0;
    __atomic_store_n(&hub_source_port_numbers[hub_source_identity_index(player,slot)],
                     (unsigned char)number,__ATOMIC_RELEASE);
    return 1;
}
#endif

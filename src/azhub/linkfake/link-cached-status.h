/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef LINK_CACHED_STATUS_H
#define LINK_CACHED_STATUS_H
/* Private playback classifier, included after safe-eject.h. The retained disconnect
 * ticket establishes origin; content ID captured when that proof was created
 * only rejects stale packet metadata. A pending or failed replacement updates
 * the load ledger but does not retire the cached track's proof. Native success
 * or actual unload retires it through safe_cached_forget_locked, including a
 * successful native replacement with the same content ID. The optional
 * committed load ledger also proves private playback after abrupt physical
 * disconnection, which creates no safe-eject ticket. Pending and failed loads
 * never replace that committed origin. This does not prove ordering of packets
 * serialized before retirement and sent afterward.
 */
static int probe_cached_status_private(const uint8_t *packet,size_t size) {
    if (!packet || size!=1152 || memcmp(packet,"Qspt1WmJOL",10) ||
        packet[10]!=0x0a || packet[0x21]<1 || packet[0x21]>4) return 0;
    int deck=packet[0x21]-1,known=0;
    uint32_t content=((uint32_t)packet[0x2c]<<24)|((uint32_t)packet[0x2d]<<16)|
        ((uint32_t)packet[0x2e]<<8)|packet[0x2f];
    uintptr_t owner=safe_native_interface(NULL,NULL);
    if (!content) return 0;
    pthread_mutex_lock(&safe_lock);
#ifdef PROBE_COMMITTED_STATUS_ORIGIN
    for (int i=0;i<16;i++) {
        const struct safe_load *load=&safe_loads[i];
        if (load->controller && load->deck==deck && load->committed_valid &&
            load->committed_source>=0 && load->committed_source<8 &&
            load->committed_epoch && load->committed_media &&
            load->committed_content_id==content &&
            load->committed_origin.generation && load->committed_origin.devnum &&
            load->committed_origin.usb[0]) known=1;
    }
#endif
    int index=safe_cached_decks[deck].ticket;
    if (owner && safe_cached_decks[deck].valid && safe_cached_decks[deck].epoch &&
        safe_cached_decks[deck].content_id==content && index>=0 && index<HUB_DETACH_SLOTS) {
        const struct hub_detach_ticket *ticket=&safe_detach_pool.slots[index];
        if (ticket->owner==owner && ticket->references && ticket->submitted &&
            ticket->connection && (ticket->phase==HUB_DETACH_PENDING || ticket->phase==HUB_DETACH_DONE)) {
            for (int i=0;i<16;i++) {
                const struct safe_load *load=&safe_loads[i];
                if (load->controller && load->controller==safe_cached_decks[deck].controller &&
                    load->deck==deck) known=1;
            }
        }
    }
    pthread_mutex_unlock(&safe_lock);
    return known;
}
#endif

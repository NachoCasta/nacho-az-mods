/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef AZHUB_LINK_STATUS_H
#define AZHUB_LINK_STATUS_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* The private member is a library endpoint, never a transport/sync master.
 * CDJ status prefix used by exact EP147: flags +0x89, handoff target +0x9f.
 * Clear playing, master, synced, on-air and BPM-sync flags while preserving
 * unrelated bits. A real deck's outgoing status must not use this helper.
 */
static int hub_status_make_idle(uint8_t *packet,size_t length) {
    if (!packet || length<0xcc || memcmp(packet,"Qspt1WmJOL",10) || packet[10]!=0x0a)
        return 0;
    packet[0x89]&=(uint8_t)~0x7au;
    packet[0x9f]=0xff;
    return 1;
}
/* Private member media state belongs to its own sources. Copying these fields
 * from physical decks alternates USB2 presence and causes native source removal.
 * Exact EP147 uses big-endian USB/SD state words and USB2/USB3 extension bytes.
 * Generate only local synthetic statuses here; native outgoing statuses retain
 * their original media states. Slots use their wire identifiers2,3,7,8.
 */
static int hub_status_own_media(uint8_t *packet,size_t length,unsigned slots) {
    if(!packet || length!=1152 || memcmp(packet,"Qspt1WmJOL",10) || packet[10]!=0x0a)return 0;
    memset(packet+0x6c,0,4);packet[0x6f]=(slots&(1u<<3))?0:4;
    memset(packet+0x70,0,4);packet[0x73]=(slots&(1u<<2))?0:4;
    packet[0x45e]=(slots&(1u<<7))?0:4;
    packet[0x45f]=(slots&(1u<<8))?0:4;
    return 1;
}
/* Exact EP147 1152-byte outgoing status: suppress only a known private media
 * reference. Transport roles, tempo, pitch, beat counters and deck identity
 * retain their native values. Call only for external destinations; local
 * synthetic clients still need the complete identity. Auxiliary track-location
 * references are handled independently below, regardless of the current deck.
 */
static int hub_status_hide_source(uint8_t *packet,size_t length,
                                  unsigned player,unsigned slot) {
    if (!packet || length!=1152 || memcmp(packet,"Qspt1WmJOL",10) ||
        packet[10]!=0x0a || !player || player>255 || !slot || slot>255 ||
        packet[0x28]!=player || packet[0x29]!=slot) return 0;
    memset(packet+0x28,0,3);
    memset(packet+0x2c,0,4);
    return 1;
}
/* Exact EP147 track-location packing: 0x1041330 emits a 36-byte location with
 * valid +0, MediaID kind +1, zero-based member +2 and content ID +8. The track
 * serializer 0x10d8840 writes ten auxiliary locations at payload +0x168+0x40*i;
 * 0x10d5ea8 writes another at +0x404. Wire payload begins at +0x24. Each row's
 * origin is independent of the packet sender and its current source tuple.
 * A private Link MediaID (kind 2, member 7/8) is never externally shareable.
 * Clearing the complete location uses the native empty-location representation
 * from 0x1041408; surrounding transport/cue data and native locations survive.
 */
static int hub_status_private_location(const uint8_t *location) {
    return location[0]==1 && location[1]==2 &&
        (location[2]==6 || location[2]==7);
}
static int hub_status_hide_private_locations(uint8_t *packet,size_t length) {
    if (!packet || length!=1152 || memcmp(packet,"Qspt1WmJOL",10) || packet[10]!=0x0a)
        return 0;
    int changed=0;
    for (unsigned i=0;i<11;i++) {
        size_t offset=i<10 ? 0x18c+0x40*i:0x428;
        if (hub_status_private_location(packet+offset)) {
            memset(packet+offset,0,36);changed++;
        }
    }
    return changed;
}
#endif

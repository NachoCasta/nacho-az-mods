/* SPDX-License-Identifier: GPL-3.0-only */
/* Stable per-controller/source LAA candidates. This is not collision proof:
 * admission must still check the native/local inventory and on-wire conflicts.
 * Player-number selection is deliberately a separate protocol policy. */
#ifndef HUB_PUBLIC_SOURCE_MAC_H
#define HUB_PUBLIC_SOURCE_MAC_H
#include <errno.h>
#include <stdint.h>
#include <string.h>
static int hub_public_source_mac(const unsigned char parent[6],unsigned slot,unsigned char out[6]) {
    if (!parent || !out || slot>=8 || (parent[0]&1)) { errno=EINVAL;return -1; }
    unsigned present=0;for (unsigned i=0;i<6;++i) present|=parent[i];
    if (!present) { errno=EINVAL;return -1; }
    /* Versioned domain separates these identities from other virtual devices.
     * The low three bits encode the source, so a controller's eight candidates
     * are distinct regardless of hash truncation. No paths or mount order enter. */
    static const unsigned char domain[]="usb-hub-support-public-mac-v1";
    uint64_t hash=UINT64_C(14695981039346656037);
    for (unsigned i=0;i<sizeof domain-1;++i) hash=(hash^domain[i])*UINT64_C(1099511628211);
    for (unsigned i=0;i<6;++i) hash=(hash^parent[i])*UINT64_C(1099511628211);
    unsigned char candidate[6];
    for (unsigned i=0;i<6;++i) candidate[i]=(unsigned char)(hash>>(8*i));
    candidate[0]=(candidate[0]&0xfc)|2;candidate[5]=(candidate[5]&0xf8)|slot;
    /* Apply the same prefix adjustment to the entire group if any candidate
     * would equal a locally administered parent; preserve source distinctness. */
    if (!memcmp(candidate,parent,5) && (candidate[5]&0xf8)==(parent[5]&0xf8)) candidate[0]^=4;
    memcpy(out,candidate,6);return 0;
}
#endif

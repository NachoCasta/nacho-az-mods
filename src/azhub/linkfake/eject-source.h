/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HUB_EJECT_SOURCE_H
#define HUB_EJECT_SOURCE_H
#include <stdint.h>

/* MediaID contains a complete outer member identity (bytes 0..3), then sdid
 * (byte 4). Byte 5 is the nested LocalMediaID member representation: native
 * browser requests and our server registry can encode it differently for the
 * same outer member/USB. Bytes 6+ contain track-location validity, not storage.
 * Preserve every outer-member bit and the USB/SD type+instance; never use a
 * content ID, display name or nested member representation as ownership. */
static uint64_t hub_eject_media_key(uint64_t identity) {
    return identity & UINT64_C(0x000000ffffffffff);
}
/* Recognize only the AZ's known AllInOne USB identities as unrelated local
 * media. An unrecognized loaded identity is not evidence of a different stick. */
static int hub_eject_known_native_usb(uint64_t identity) {
    uint64_t key=hub_eject_media_key(identity);
    unsigned slot=(unsigned)(key>>32);
    return (uint32_t)key==1 && (slot==2 || slot==0x22 || slot==0x42);
}
#endif

/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "eject-source.h"

int main(void) {
    /* Observed server, browser and LoadedOneTrack encodings for member 3 USB2. */
    assert(hub_eject_media_key(UINT64_C(0x0000012200000202)) == UINT64_C(0x2200000202));
    assert(hub_eject_media_key(UINT64_C(0x0000022200000202)) == UINT64_C(0x2200000202));
    assert(hub_eject_media_key(UINT64_C(0x0001022200000202)) == UINT64_C(0x2200000202));
    uint64_t usb2=hub_eject_media_key(UINT64_C(0x0000012200000202));
    assert(usb2 != hub_eject_media_key(UINT64_C(0x0000012200000302))); /* other player */
    assert(usb2 != hub_eject_media_key(UINT64_C(0x0000010200000202))); /* other USB */
    assert(usb2 != hub_eject_media_key(UINT64_C(0x0000012200000201))); /* other member kind */
    assert(usb2 != hub_eject_media_key(UINT64_C(0x0000012200010202))); /* other member bits */
    assert(usb2 != hub_eject_media_key(UINT64_C(0x0000010300000202))); /* SD */
    assert(usb2 != hub_eject_media_key(UINT64_C(0x0000012200000001))); /* native AllInOne USB */
    assert(hub_eject_known_native_usb(UINT64_C(0x0000010200000001)));
    assert(hub_eject_known_native_usb(UINT64_C(0x0000012200000001)));
    assert(hub_eject_known_native_usb(UINT64_C(0x0000014200000001)));
    assert(!hub_eject_known_native_usb(0));
    assert(!hub_eject_known_native_usb(UINT64_C(0x0000012200000202)));
    assert(!hub_eject_known_native_usb(UINT64_C(0x0000012200000101)));
    assert(!hub_eject_known_native_usb(UINT64_C(0x0000010600000001)));
    puts("Eject ownership: native media encodings agree; member, type and instance remain independent");
    return 0;
}

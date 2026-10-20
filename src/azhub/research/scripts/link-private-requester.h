/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef LINK_PRIVATE_REQUESTER_H
#define LINK_PRIVATE_REQUESTER_H
/* Emulator experiment: private member IDs and native DataStream requester keys
 * are distinct namespaces. Keep requester keys away from local/external decks
 * 1..4 and give each private member its own communicator. */
static unsigned probe_requester_for_player(unsigned player) {
    return player==7 ? 5 : player==8 ? 6 : 0;
}
static int probe_requester_reserved(unsigned requester) {
    return requester==5 || requester==6;
}
#endif

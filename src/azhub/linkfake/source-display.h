/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HUB_SOURCE_DISPLAY_H
#define HUB_SOURCE_DISPLAY_H
/* Presentation only: these pairs keep their transport identity and routing.
 * Discovery may sort differently after removal; the loader retains established
 * pairs and reuses only dormant sources, so connected sources retain numbers.
 * Callers must establish private-source ownership before rendering a number. */
static int hub_source_display_number(int player,int slot) {
    static const int players[]={7,7,8,8,7,8,7,8};
    static const int slots[]={3,7,3,7,8,8,2,2};
    for (unsigned i=0;i<sizeof players/sizeof players[0];i++)
        if (player==players[i] && slot==slots[i]) return (int)i+1;
    return 0;
}
#endif
